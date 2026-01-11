#include <esp_system.h>
#include <esp_log.h>
#include <Arduino.h>
#include <optional>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <RTClib.h>
// #include <ESP32Ping.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <DHTesp.h>
#include "maputils.h"
#include "ReadEspNow.h"
#include "nvsDebugData.h"
#include "EventLogger.h"
#include "mappings.h"
#include "NTCSensor.h"
#include "VEDirectData.h"
#include "VEDirectDecoder.h"
// #include "configuration.h"
#include "dev_configuration.h"

InfluxDBClient influxClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_DATA_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
InfluxDBClient influxLogClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_LOG_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

EventLogger eventLog(influxLogClient, SD_DET, logFileName);

DHTesp dhtSensorRoom;
NTCSensor ntcSensor1(NTC_POWER_PIN, NTC1_READ_PIN);
NTCSensor ntcSensor2(NTC_POWER_PIN, NTC2_READ_PIN);

VEDirectSerial victronInverter(Serial1, "INV");
VEDirectSerial victronMppt(Serial2, "MPPT");

ESPNowReceiver greenhouseData;

uint32_t lastUpdate = millis();
uint32_t lastInverterPowerChange = millis();

tm timeinfo;
time_t now;

enum powerSwitch
{
  OFF,
  ON
};

powerSwitch inverterPowerState;

std::map<String, int> numSensorData;    // Numerical data
std::map<String, String> humSensorData; // Human-readable data

const char *getResetReason(esp_reset_reason_t);

void initWiFi();
bool collectSensorData();
void sendHouseToInflux();
void sendGreenhouseToInflux(GreenhouseSensorData data);
void controlInverterByVoltage();

void setup()
{
  initNvs();
  String lastEventBeforeReboot = String(readDataFromNvs("lastState"));
  esp_reset_reason_t resetReason = esp_reset_reason();

  Serial.begin(9600);
  Serial1.begin(19200, SERIAL_8N1, RXD1, TXD1);
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);

  Serial.println("====================");
  Serial.println(" System is starting ");
  Serial.println("====================");

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);

  greenhouseData.begin();
  initWiFi();
  timeSync(TIME_ZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
  if (!influxClient.validateConnection())
    eventLog.log(String("Kunde inte ansluta till Influx DB på " + influxClient.getServerUrl() + "\nFelmeddelande: " + influxClient.getLastErrorMessage()), EventLogger::LogLevel::ERROR);

  dhtSensorRoom.setup(DHT_PIN, DHTesp::DHT11);

  float setupTime = millis() / 1000;

  eventLog.log(String("Systemet startat. Uppstarten tog " + String(setupTime) + " s."), EventLogger::LogLevel::INFO);
  eventLog.log(String("Senaste återställning: " + String(getResetReason(resetReason)) + " (" + String(resetReason) + ")"), EventLogger::LogLevel::INFO);
  eventLog.log(String("Senaste åtgärd: " + lastEventBeforeReboot), EventLogger::LogLevel::INFO);

  storeDataToNvs("lastState", "Setup end");
}

void loop()
{
  storeDataToNvs("lastState", "Loop");

  if (WiFi.status() != WL_CONNECTED)
    WiFi.reconnect();

  // Fetch VE.Direct data at regular intervals
  if (millis() >= lastUpdate + (UPDATE_INTERVAL * 1000))
  {
    collectSensorData();
    controlInverterByVoltage(); // Turn off fridge if power is low
    sendHouseToInflux();
    lastUpdate = millis();
  }

  // Fetch greenhouse data if it has been recieved over ESP-NOW
  if (greenhouseData.hasNewData())
  {
    Serial.println("Received data from greenhouse");
    auto data = greenhouseData.getData();
    greenhouseData.clearNewDataFlag();
    sendGreenhouseToInflux(data);
  }

  yield();
  delay(10);
}

void initWiFi() // Connect to WiFi
{
  storeDataToNvs("lastState", "initWiFi");
  eventLog.log(String("Connecting to WiFi " + String(ssid)), EventLogger::LogLevel::INFO);
  eventLog.log(String("MAC-adress: " + WiFi.macAddress()), EventLogger::LogLevel::INFO);

  WiFi.begin(ssid, password, 6);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);

    Serial.print(".");
  }
  Serial.println();
  eventLog.log(String("Connected to WiFi " + String(ssid) + " (channel: " + WiFi.channel() + ")"), EventLogger::LogLevel::INFO);
}

void sendHouseToInflux()
{
  storeDataToNvs("lastState", "sendHouseToInflux");
  Point dataPoint("SolarPower");

  for (auto const &entry : numSensorData) // Go through all values in numSensorData
  {
    String label = entry.first;
    int value = entry.second;

    if (!mapLabelDisplaynameUnit.count(label) > 0)
      continue; // Only send values if they exist in 'mapLabelDisplaynameUnit'

    auto conversionFactor = mapLabelDisplaynameUnit.at(label).conversionFactor;

    if (conversionFactor == 1) // Exactly 1 means no conversion, send int
      dataPoint.addField(label, value);

    if (conversionFactor > 1) // More than 1, divide by conversion factor to get reasonable unit (for example, convert mV to V)
      dataPoint.addField(label, static_cast<double>(value) / conversionFactor);

    if (conversionFactor == 0) //  0: Status or code
    {
      dataPoint.addField(label, value); // Send original value as int (no float for error codes)
    }
  }

  for (auto const &entry : humSensorData)
    dataPoint.addField(entry.first, entry.second);

  const bool influxDbResponse = influxClient.writePoint(dataPoint); // Send data point to InfluxDB
  Serial.print("Sending solar data to InfluxDB");
  if (!influxDbResponse)
  {
    Serial.println(" failed.");
    eventLog.log(influxClient.getLastErrorMessage(), EventLogger::LogLevel::ERROR);
    return;
  }
  Serial.println(" successful.");
}

void sendGreenhouseToInflux(GreenhouseSensorData data)
{
  storeDataToNvs("lastState", "sendGreenhouseToInflux");

  Point dataPoint("Greenhouse");
  dataPoint.addField("indoorTemp", data.indoorTemp);
  dataPoint.addField("indoorHumidity", data.indoorHumidity);
  dataPoint.addField("outdoorTemp", data.outdoorTemp);
  dataPoint.addField("soilTemp1", data.soilTemp1);
  dataPoint.addField("soilTemp2", data.soilTemp2);
  dataPoint.addField("soilMoisture1", data.soilMoisture1);
  dataPoint.addField("soilMoisture2", data.soilMoisture2);
  dataPoint.addField("batteryVoltage", data.batteryVoltage);

  dataPoint.addField("climateSensorStatus", data.status.ClimateSensorStatus);
  dataPoint.addField("soilSensor1Status", data.status.SoilSensor1Status);
  dataPoint.addField("soilSensor2Status", data.status.SoilSensor2Status);
  dataPoint.addField("oneWireDeviceCount", data.status.OneWireDeviceCount);

  const bool influxDbResponse = influxClient.writePoint(dataPoint); // Send data point to InfluxDB
  Serial.print("Sending greenhouse data to InfluxDB");
  if (!influxDbResponse)
  {
    Serial.println(" failed.");
    eventLog.log(influxClient.getLastErrorMessage(), EventLogger::LogLevel::ERROR);
    return;
  }
  Serial.println(" successful.");
}

bool collectSensorData()
{
  storeDataToNvs("lastState", "collectSensorData");
  numSensorData.clear();
  humSensorData.clear();

  // Current time
  time(&now);
  char strTimestamp[20];

  numSensorData["TIMESTAMP"] = now;
  humSensorData["Time"] = strftime(strTimestamp, sizeof(strTimestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

  // ESP statistics and performance
  numSensorData["ESP_UPTIME"] = millis();
  numSensorData["ESP_MEM_FREE"] = ESP.getFreeHeap();
  numSensorData["ESP_MEM_LOWEST"] = ESP.getMinFreeHeap();
  numSensorData["ESP_PSRAM_FREE"] = ESP.getFreePsram();
  numSensorData["ESP_PSRAM_LOWEST"] = ESP.getFreePsram();

  // States
  numSensorData["CTRL_INV_ON"] = static_cast<int>(inverterPowerState);

  // Sensor data
  if (victronInverter.update())
  {
    std::map<String, int> intData = victronInverter.getData();
    mergeMaps(intData, numSensorData);
  }
  if (victronMppt.update())
  {
    std::map<String, int> intData = victronMppt.getData();
    mergeMaps(intData, numSensorData);
  }

  // Get human-readable messages for any staus codes that exist in LableCodeMappings
  VEDirectDecoder messageDecoder(mapLabelDisplaynameUnit, mapLabelCodeText);
  std::map<String, String> decodedMessages = messageDecoder.VEDirectCodeMapToHumanReadable(numSensorData);
  mergeMaps(decodedMessages, humSensorData);

  // NTC sensors
  if (auto temperature = ntcSensor1.temperature(); temperature.has_value())
    numSensorData["ENV_REFRIG_TEMP"] = *temperature;

  if (auto temperature = ntcSensor2.temperature(); temperature.has_value())
    numSensorData["ENV_FREEZER_TEMP"] = *temperature;

  numSensorData["ENV_ROOM_TEMP"] = dhtSensorRoom.getTemperature();
  numSensorData["ENV_ROOM_HUMID"] = dhtSensorRoom.getHumidity();

  return true;
}

void controlInverterByVoltage()
{
  const int voltage = numSensorData["VE_MPPT_V"]; // Battery voltage in mV. Using MPPT voltage, since Inv. voltage = 0 when off

  // If state changed more recent than retry period, do nothing
  if (millis() - lastInverterPowerChange < (INV_RETRY_PERIOD * 1000))
    return;

  // Do nothing on coco-bananas values
  if (voltage < 1000 || voltage > 20000)
  {
    String messageText = "Orealistikt spänningsvärde (" + String(voltage) + " mV), ändrar inte status på inverter";
    eventLog.log(messageText, EventLogger::LogLevel::WARNING);
    return;
  }

  // If battery voltage is lower than off voltage, turn inverter off
  if (inverterPowerState == ON && voltage < INV_OFF_VOLTAGE)
  {
    digitalWrite(RELAY1, HIGH); // Relay is NC
    lastInverterPowerChange = millis();
    inverterPowerState = OFF;
    eventLog.log("Inverter stängdes av, låg batterispänning", EventLogger::LogLevel::INFO);
    return;
  }

  // If battery voltage is higher than on voltage, turn inverter on
  if (inverterPowerState == OFF && voltage > INV_ON_VOLTAGE)
  {
    digitalWrite(RELAY1, LOW); // Relay is NC
    lastInverterPowerChange = millis();
    inverterPowerState = ON;
    eventLog.log("Inverter slogs på, tillräcklig batterispänning ", EventLogger::LogLevel::INFO);
    return;
  }
}