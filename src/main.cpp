#include <esp_system.h>
#include <esp_log.h>
#include <Arduino.h>
#include <optional>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include <time.h>
#include <RTClib.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <DHTesp.h>
#include "maputils.h"
#include "ReadEspNow.h"
#include "nvsDebugData.h"
#include "EventLogger.h"
#include "mappings.h"
#include "NTCSensor.h"
#include "VEDirectSerialReader.h"
#include "VEDirectParseMessage.h"
#include "VEDirectDecoder.h"
#include "configuration.h"
// #include "dev_configuration.h"

WebServer localWebServer(80);

InfluxDBClient influxClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_DATA_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
InfluxDBClient influxLogClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_LOG_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

EventLogger eventLog(influxLogClient, SD_DET, logFileName);

DHTesp dhtSensorRoom;
NTCSensor ntcSensor1(NTC_POWER_PIN, NTC1_READ_PIN);
NTCSensor ntcSensor2(NTC_POWER_PIN, NTC2_READ_PIN);

VEDirectSerialReader victronInverter(Serial1);
VEDirectSerialReader victronMppt(Serial2);

VEDirectParseMessage inverterData;
VEDirectParseMessage mpptData;

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

powerSwitch inverterPowerState = ON;
powerSwitch xmasLightState = OFF;

/*
According to VE.Direct documentation, these are the maximum sizes:
  Field label: 9 bytes
  Field value: 33 bytes
  Message size: 22 fields (like 2 * 22 + like 10 other non-VE fields makes that like 60)

So, instead of using String, and std::map<String, String>,
could use char[] and a std::array<Field, 60> where Field is:
struct Field {
  char label[20];
  char value[33];
}

*/

const char *getResetReason(esp_reset_reason_t);

void initWiFi();
bool collectSensorData();
void sendHouseToInflux();
Point greenhouseToInflux(GreenhouseSensorData data);
Point sysStatsToInflux();
Point houseStatsToInflux();
Point veToInflux(String pointName, VEDirectParseMessage parsedMessage, std::map<String, int> conversionFactors, std::map<String, String> displayNames, CodeMap codes);
void controlInverterByVoltage();
void controlLight();

void setup()
{
  initNvs();
  String lastEventBeforeReboot = String(readDataFromNvs("lastState"));
  esp_reset_reason_t resetReason = esp_reset_reason();

  Serial.begin(115200);
  Serial1.begin(19200, SERIAL_8N1, RXD1, TXD1);
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);

  Serial.println("====================");
  Serial.println(" System is starting ");
  Serial.println("====================");

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);

  // Make sure relay positions match the corresponding power switch
  digitalWrite(RELAY1, (inverterPowerState == ON) ? LOW : HIGH); // NC
  digitalWrite(RELAY2, (xmasLightState == ON) ? HIGH : LOW);     // NO
  digitalWrite(RELAY3, LOW);
  digitalWrite(RELAY4, LOW);

  greenhouseData.begin();

  initWiFi();

  timeSync(TIME_ZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

  // OTA
  localWebServer.on("/", []()
                    { localWebServer.send(200, "text/plain", "Tere tulemast Eesti saatkonda!"); });
  ElegantOTA.setAuth(otaUsername, otaPassword);
  ElegantOTA.begin(&localWebServer);
  localWebServer.begin();
  eventLog.log("Webbserver startad", EventLogger::LogLevel::INFO);

  if (!influxClient.validateConnection())
    eventLog.log(String("Kunde inte ansluta till Influx DB på " + influxClient.getServerUrl() + "\nFelmeddelande:\n" + influxClient.getLastErrorMessage() + "\n"), EventLogger::LogLevel::ERROR);

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
  {
    WiFi.reconnect();
    eventLog.log("Återansluter till WiFi", EventLogger::LogLevel::INFO);
    delay(500);
  }

  localWebServer.handleClient();
  ElegantOTA.loop();

  if (victronInverter.update())
    Serial.println("Tog emot ny data från inverter");

  if (victronMppt.update())
    Serial.println("Tog emot ny data från MPPT");

  std::vector<Point> influxPoints;

  // Send data at regular intervals
  if (millis() >= lastUpdate + (UPDATE_INTERVAL * 1000))
  {
    Serial.println("\nSamlar ihop och skickar data till Influx");
    storeDataToNvs("lastState", "Send data triggered");

    controlInverterByVoltage(); // Turn off fridge if power is low
    controlLight();

    inverterData.stringToMap(victronInverter.getMessage());
    mpptData.stringToMap(victronMppt.getMessage());

    // Victron VE.Direct units
    if (!inverterData.getIntMap().empty())
      influxPoints.emplace_back(veToInflux("Inverter", inverterData, inverterConversions, inverterDisplayNames, inverterCodes));
    const auto &mpptDataMap = mpptData.getIntMap();
    if (!mpptDataMap.empty())
      influxPoints.emplace_back(veToInflux("MPPT", mpptData, mpptConversions, mpptDisplayNames, mpptCodes));

    influxPoints.emplace_back(sysStatsToInflux());
    influxPoints.emplace_back(houseStatsToInflux());

    lastUpdate = millis();
  }

  // Fetch greenhouse data if it has been recieved over ESP-NOW
  if (greenhouseData.hasNewData())
  {
    Serial.println("Tog emot data från växthuset");
    storeDataToNvs("lastState", "Recieved data from greenhouse");

    auto data = greenhouseData.getData();
    greenhouseData.clearNewDataFlag();
    influxPoints.emplace_back(greenhouseToInflux(data));
  }

  // Send all gathered data to InfluxDB
  for (auto &thisPoint : influxPoints)
  {
    Serial.println("Skickar till Influx: ");
    const bool influxDbResponse = influxClient.writePoint(thisPoint); // Send data point to InfluxDB
    Serial.println(thisPoint.toLineProtocol());

    if (!influxDbResponse)
    {
      Serial.print("Misslyckades med att skicka: ");
      Serial.println(thisPoint.toLineProtocol());
      eventLog.log(influxClient.getLastErrorMessage(), EventLogger::LogLevel::ERROR);
      continue;
    }
  }

  yield();
}

void initWiFi() // Connect to WiFi
{
  storeDataToNvs("lastState", "initWiFi");
  eventLog.log(String("Ansluter till WiFi " + String(ssid)), EventLogger::LogLevel::INFO);
  eventLog.log(String("MAC-adress: " + WiFi.macAddress()), EventLogger::LogLevel::INFO);

  WiFi.begin(ssid, password, 6);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);

    Serial.print(".");
  }
  IPAddress myIp = WiFi.localIP();
  const String myIpString = String(myIp[0]) + "." +
                            String(myIp[1]) + "." +
                            String(myIp[2]) + "." +
                            String(myIp[3]);

  IPAddress gwIp = WiFi.gatewayIP();
  const String gwIpString = String(gwIp[0]) + "." +
                            String(gwIp[1]) + "." +
                            String(gwIp[2]) + "." +
                            String(gwIp[3]);

  Serial.println();
  eventLog.log("Ansluten till WiFi " + String(ssid), EventLogger::LogLevel::INFO);
  Serial.flush();
  Serial.println("\t\t\t\t\tKanal    \t" + WiFi.channel());
  Serial.println("\t\t\t\t\tIP-adress\t" + myIpString);
  Serial.println("\t\t\t\t\tGateway  \t" + gwIpString);

  Point netStat("Network");
  netStat.addField("Channel", WiFi.channel());
  netStat.addField("Hostname", WiFi.getHostname());
  netStat.addField("IP address", myIpString);
  netStat.addField("Gateway", gwIpString);
  netStat.addField("MAC address", WiFi.macAddress());
  netStat.addField("Transmitter MAC", transmitterMAC);

  influxClient.writePoint(netStat);
}

Point greenhouseToInflux(GreenhouseSensorData data)
{
  storeDataToNvs("lastState", "greenhouseToInflux");

  Point dataPoint("Greenhouse");
  dataPoint.addField("indoorTemp", (data.indoorTemp / 10.0));
  dataPoint.addField("indoorHumidity", (data.indoorHumidity / 10.0));
  dataPoint.addField("outdoorTemp", (data.outdoorTemp / 10.0));
  dataPoint.addField("soilTemp1", (data.soilTemp1 / 10.0));
  dataPoint.addField("soilTemp2", (data.soilTemp2 / 10.0));
  dataPoint.addField("soilMoisture1", data.soilMoisture1);
  dataPoint.addField("soilMoisture2", data.soilMoisture2);
  dataPoint.addField("batteryVoltage", (data.batteryVoltage / 1000.0));

  dataPoint.addField("climateSensorStatus", data.status.ClimateSensorStatus);
  dataPoint.addField("soilSensor1Status", data.status.SoilSensor1Status);
  dataPoint.addField("soilSensor2Status", data.status.SoilSensor2Status);
  dataPoint.addField("oneWireDeviceCount", data.status.OneWireDeviceCount);

  return dataPoint;
}

Point sysStatsToInflux()
{
  storeDataToNvs("lastState", "sysStatsToInflux");

  Point sysStats("ESP");
  time(&now);
  sysStats.addField("Timestamp", now);
  sysStats.addField("Uptime", millis());
  sysStats.addField("Mem_free", ESP.getFreeHeap());
  sysStats.addField("PSRAM_free", ESP.getFreePsram());
  sysStats.addField("PSRAM_lowest", ESP.getMinFreePsram());

  return sysStats;
}

Point houseStatsToInflux()
{
  storeDataToNvs("lastState", "houseStatsToInflux");

  Point houseStats("Huvudbyggnad");
  if (auto temperature = ntcSensor1.temperature(); temperature.has_value())
    houseStats.addField("Temp_kyl", temperature.value());

  if (auto temperature = ntcSensor2.temperature(); temperature.has_value())
    houseStats.addField("Temp_frys", temperature.value());

  houseStats.addField("Rumstemp_1", dhtSensorRoom.getTemperature());
  houseStats.addField("Luftfukt_1", dhtSensorRoom.getHumidity());

  houseStats.addField("Inverter", inverterPowerState);
  houseStats.addField("Julbelysning", xmasLightState);

  return houseStats;
}

Point veToInflux(String pointName, VEDirectParseMessage parsedMessage, std::map<String, int> conversionFactors, std::map<String, String> displayNames, CodeMap codes)
{
  storeDataToNvs("lastState", "veToInflux");

  Point newPoint(pointName);
  const auto &intData = parsedMessage.getIntMap();
  VEDirectDecoder decoder(displayNames, codes);
  std::map<String, String> decodedMessages = decoder.VEDirectCodeMapToHumanReadable(intData);

  if (intData.empty())
    eventLog.log("Försökte skapa Point \"" + pointName + "\" från en tom Map", EventLogger::LogLevel::WARNING);

  for (auto const &[key, val] : intData)
  {
    if (conversionFactors.count(key))
    {
      float floatVal = static_cast<float>(val) / conversionFactors.at(key);
      newPoint.addField(key, floatVal);
      continue;
    }
    newPoint.addField(key, val);
  }

  for (auto const &[key, val] : decodedMessages)
    newPoint.addField(key, val);

  return newPoint;
}

void controlInverterByVoltage()
{
  const auto &intData = mpptData.getIntMap(); // Battery voltage in mV. Using MPPT voltage, since Inv. voltage = 0 when off

  auto it = intData.find("V");
  if (it == intData.end())
  {
    eventLog.log("Hittade ingen batterispänning från MPPT", EventLogger::LogLevel::WARNING);
    return;
  }
  const int voltage = it->second;

  // If state changed more recent than retry period, do nothing
  if (millis() - lastInverterPowerChange < (INV_RETRY_PERIOD * 1000))
    return;

  // Do nothing on coco-bananas values (<1 V or >20 V)
  if (voltage < 1000 || voltage > 20000)
  {
    String messageText = "Orealistikt spänningsvärde (" + String(voltage) + " mV), ändrar inte status på inverter";
    eventLog.log(messageText, EventLogger::LogLevel::WARNING);
    return;
  }

  // If battery voltage is lower than off voltage, turn inverter off
  if (inverterPowerState == ON && voltage < INV_OFF_VOLTAGE)
  {
    digitalWrite(RELAY1, HIGH); // Relay is NC, so triggering it will turn off the inverter
    lastInverterPowerChange = millis();
    inverterPowerState = OFF;
    eventLog.log("Inverter stängdes av, låg batterispänning", EventLogger::LogLevel::INFO);
    return;
  }

  // If battery voltage is higher than on voltage, turn inverter on
  if (inverterPowerState == OFF && voltage > INV_ON_VOLTAGE)
  {
    digitalWrite(RELAY1, LOW); // Relay is NC, so releasing it will turn on the inverter
    lastInverterPowerChange = millis();
    inverterPowerState = ON;
    eventLog.log("Inverter slogs på, tillräcklig batterispänning ", EventLogger::LogLevel::INFO);
    return;
  }
}

/**
 * @brief Turns on light (by triggering a relay) when the following conditions are met:
 *        - It is christmas time (between December 1 and January 13)
 *        - It is daytime (between 08:00 and 20:00)
 *        - It is dark (the panel voltage of the photovoltaic panel is < 5000 mV)
 */
void controlLight()
{
  const auto &intData = mpptData.getIntMap(); // Contains panel voltage (VPV) in mV

  auto it = intData.find("VPV");
  if (it == intData.end())
  {
    eventLog.log("Hittade ingen panelspänning från MPPT", EventLogger::LogLevel::WARNING);
    return;
  }

  const int panelVoltage = it->second;

  time(&now);
  struct tm *timeinfo = localtime(&now);

  const bool isXmas = (timeinfo->tm_mon == 11 && timeinfo->tm_mday >= 1) || (timeinfo->tm_mon == 0 && timeinfo->tm_mday <= 13);
  const bool isDark = (panelVoltage < 5000);
  const bool isDay = (timeinfo->tm_hour >= 8) && (timeinfo->tm_hour < 20);

  xmasLightState = (isXmas && isDark && isDay) ? ON : OFF;

  switch (xmasLightState)
  {
  case ON:
    digitalWrite(RELAY2, HIGH); // Relay is NO
    break;

  case OFF:
    digitalWrite(RELAY2, LOW); // Relay is NO
    break;

  default:
    break;
  }
}
