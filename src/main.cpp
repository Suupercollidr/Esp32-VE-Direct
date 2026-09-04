#include <esp_system.h>
#include <esp_log.h>
#include <esp_now.h>
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
#include <AsyncMqttClient.h>
#include <DHTesp.h>
#include "maputils.h"
#include "debounce.h"
#include "ReadEspNow.h"
#include "nvsDebugData.h"
#include "EventLogger.h"
#include "mappings.h"
#include "mappingsMqtt.h"
#include "espNowTypdef.h"
#include "NTCSensor.h"
#include "VEDirectSerialReader.h"
#include "VEDirectParseMessage.h"
#include "VEDirectDecoder.h"
#include "configuration.h"
// #include "dev_configuration.h"

WebServer localWebServer(80);

InfluxDBClient influxClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_DATA_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
InfluxDBClient influxLogClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_LOG_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

Debounce influxVerifyTimer(60000);
uint8_t influxFailedAttempts = 0;

AsyncMqttClient mqttClient;
MqttTopics topics;
Debounce MQTTReconnect(10000);

EventLogger eventLog(influxLogClient, -1, logFileName, hostname);

DHTesp dhtSensorRoom;
NTCSensor ntcSensor1(NTC_POWER_PIN, NTC1_READ_PIN, 10000, 25, 3435, 10000);
NTCSensor ntcSensor2(NTC_POWER_PIN, NTC2_READ_PIN);

VEDirectSerialReader victronInverter(Serial1);
VEDirectSerialReader victronMppt(Serial2);

VEDirectParseMessage inverterData;
VEDirectParseMessage mpptData;

ESPNowReceiver greenhouseData;

Debounce WiFiConnectTimeout(30000); // Used for both intial connect and reconnect period
Debounce NTPSyncInterval(NTP_SYNC_INTERVAL * 3600000);
Debounce influxSendInterval(UPDATE_INTERVAL * 1000);
Debounce controlFridgeInterval(CTRL_FRIDGE_INTERVAL * 60 * 1000);

tm timeinfo;
time_t now;

InverterAction whatToDoWithInverter = InverterAction::NO_CHANGE;

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
void reconnectMqtt();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
bool collectSensorData();
void sendHouseToInflux();
Point greenhouseToInflux(GreenhouseSensorData data);
Point sysStatsToInflux();
Point houseStatsToInflux();
Point veToInflux(String pointName,
                 VEDirectParseMessage parsedMessage,
                 std::map<String, int> conversionFactors,
                 std::map<String, String> displayNames,
                 CodeMap codes);
void veToMqtt(VEDirectParseMessage parsedMessage,
              std::map<String, int> conversionFactors,
              const std::map<String, String> mqttTopics);
void greenhouseToMqtt(GreenhouseSensorData data);
void publishMqtt(const String &topic,
                 const String &payload,
                 bool retain = false);
InverterAction shouldInverterBeOn();
void sendInverterCommandViaEspNow(InverterAction action);

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

  greenhouseData.begin();

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);

  if (esp_now_init() != ESP_OK)
    eventLog.log("ESP-NOW: Fel vid initializering");

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, controlUnitMacAdress, 6);
  peerInfo.channel = 0; // 0 = använd aktuell wifi-kanal
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
    eventLog.log("ESP-NOW: Kunde inte lägga till styrenheten som peer", EventLogger::LogLevel::WARNING);

  while (!WiFi.isConnected())
  {
    Serial.print("Connecting to WiFi " + String(ssid));
    Serial.print(" 🛜 ");
    delay(100);
    if (millis() > (wifiTimeout * 1000))
    {
      storeDataToNvs("lastState", "Failed to connect");
      delay(200);
      ESP.restart();
    }
  }
  eventLog.log("Ansluten till WiFi " + String(ssid), EventLogger::LogLevel::INFO);

  timeSync(TIME_ZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

  Point netStat("Network");
  netStat.addTag("hostname", WiFi.getHostname());
  netStat.addTag("device", WiFi.getHostname());
  netStat.addField("Channel", WiFi.channel());
  netStat.addField("IP address", WiFi.localIP().toString());
  netStat.addField("Gateway", WiFi.gatewayIP().toString());
  netStat.addField("MAC address", WiFi.macAddress());
  eventLog.writePoint(netStat);

  // OTA
  localWebServer.on("/", []()
                    { String websiteContents = "Tere tulemast Eesti saatkonda!\nJaotis: " + String(hostname);
    localWebServer.send(200, "text/plain", websiteContents); });
  ElegantOTA.setAuth(otaUsername, otaPassword);
  ElegantOTA.begin(&localWebServer);
  localWebServer.begin();
  eventLog.log("Webbserver startad", EventLogger::LogLevel::INFO);

  if (!influxClient.validateConnection())
    eventLog.log(String("Kunde inte ansluta till Influx DB på " + influxClient.getServerUrl() + "\nFelmeddelande:\n" + influxClient.getLastErrorMessage() + "\n"), EventLogger::LogLevel::ERROR);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.setCredentials(MQTT_USER, MQTT_PASS);
  mqttClient.setServer(MQTT_HOST, 1883);
  mqttClient.setWill(topics.esp32_status_topic, 1, true, "offline");
  mqttClient.connect();

  dhtSensorRoom.setup(DHT_PIN, DHTesp::DHT11);

  eventLog.sendPendingPoints();

  float setupTime = millis() / 1000.0f;

  eventLog.log(String("Systemet startat. Uppstarten tog " + String(setupTime) + " s."), EventLogger::LogLevel::INFO);
  eventLog.log(String("Senaste återställning: " + String(getResetReason(resetReason)) + " (" + String(resetReason) + ")"), EventLogger::LogLevel::INFO);
  eventLog.log(String("Senaste åtgärd: " + lastEventBeforeReboot), EventLogger::LogLevel::INFO);

  storeDataToNvs("lastState", "Setup end");
}

void loop()
{
  storeDataToNvs("lastState", "Loop");

  if (!WiFi.isConnected())
  {
    WiFi.reconnect();
    if (millis() > (wifiTimeout * 1000))
    {
      storeDataToNvs("lastState", "Failed to connect");
      delay(200);
      ESP.restart();
    }
  }

  if (!mqttClient.connected())
    reconnectMqtt();

  localWebServer.handleClient();
  ElegantOTA.loop();

  if (victronInverter.update())
    Serial.println("Tog emot ny data från inverter");

  if (victronMppt.update())
  {
    whatToDoWithInverter = shouldInverterBeOn();
    Serial.println("Tog emot ny data från MPPT");
  }

  if (controlFridgeInterval.ready() && whatToDoWithInverter != InverterAction::NO_CHANGE)
  {
    sendInverterCommandViaEspNow(whatToDoWithInverter);
    whatToDoWithInverter = InverterAction::NO_CHANGE;
  }
  std::vector<Point> influxPoints;

  // Send data at regular intervals
  if (influxSendInterval.ready())
  {
    Serial.println("\nSamlar ihop och skickar data till Influx");
    storeDataToNvs("lastState", "Send data triggered");

    inverterData.stringToMap(victronInverter.getMessage());
    mpptData.stringToMap(victronMppt.getMessage());

    // Victron VE.Direct units
    if (!inverterData.getIntMap().empty())
    {
      influxPoints.emplace_back(veToInflux("Inverter", inverterData, inverterConversions, inverterDisplayNames, inverterCodes));
      veToMqtt(inverterData, inverterConversions, inverterMqttMappings);
    }
    const auto &mpptDataMap = mpptData.getIntMap();
    if (!mpptDataMap.empty())
    {
      influxPoints.emplace_back(veToInflux("MPPT", mpptData, mpptConversions, mpptDisplayNames, mpptCodes));
      veToMqtt(mpptData, mpptConversions, mpptMqttMappings);
    }

    influxPoints.emplace_back(sysStatsToInflux());
    influxPoints.emplace_back(houseStatsToInflux());
  }

  // Fetch greenhouse data if it has been recieved over ESP-NOW
  if (greenhouseData.hasNewData())
  {
    Serial.println("Tog emot data från växthuset");
    storeDataToNvs("lastState", "Recieved data from greenhouse");

    auto data = greenhouseData.getData();
    greenhouseData.clearNewDataFlag();
    influxPoints.emplace_back(greenhouseToInflux(data));
    greenhouseToMqtt(data);
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

  if (NTPSyncInterval.ready())
    timeSync(TIME_ZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

  if (influxVerifyTimer.ready())
  {
    influxClient.validateConnection() ? influxFailedAttempts = 0 : influxFailedAttempts++;
    if (influxFailedAttempts > 5)
    {
      ESP.restart();
    }
  }

  yield();
}

void reconnectMqtt()
{
  if (!MQTTReconnect.ready())
    return;

  if (!WiFi.isConnected()) // Need WiFi to connect to MQTT broker
    return;

  eventLog.log("Försöker återansluta till MQTT...", EventLogger::LogLevel::INFO);
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
  mqttClient.publish(topics.esp32_status_topic, 1, true, "online");
  uint16_t packetId = mqttClient.subscribe(topics.esp32_restart_topic, 1);

  eventLog.log("MQTT: Ansluten till broker", EventLogger::LogLevel::INFO);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  String message = "Frånkopplad från MQTT-broker p.g.a.: ";
  message += static_cast<int>(reason);
  eventLog.log(message, EventLogger::LogLevel::WARNING);

  if (reason != AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED)
    reconnectMqtt();
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties,
                   size_t len, size_t index, size_t total)
{
  if (strcmp(topic, topics.esp32_restart_topic) != 0)
    return;

  String message;
  for (size_t i = 0; i < len; i++)
    message += (char)payload[i];

  if (message == "RESTART")
  {
    eventLog.log("Startar om på begäran från MQTT", EventLogger::LogLevel::INFO);
    delay(1000);
    ESP.restart();
  }
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

void greenhouseToMqtt(GreenhouseSensorData data)
{
  publishMqtt(String(topics.greenhouseIndoorTemp), String((data.indoorTemp / 10.0)));
  publishMqtt(String(topics.greenhouseOutdoorTemp), String((data.outdoorTemp / 10.0)));
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
  {
    houseStats.addField("Temp_kyl_float", temperature.value());
    publishMqtt(topics.refrigeratorTemp, String(temperature.value()));
  }
  if (auto temperature = ntcSensor2.temperature(); temperature.has_value())
  {
    houseStats.addField("Temp_frys_float", temperature.value());
  }
  houseStats.addField("Rumstemp_1", dhtSensorRoom.getTemperature());
  publishMqtt(topics.roomTemp, String(dhtSensorRoom.getTemperature()));
  houseStats.addField("Luftfukt_1", dhtSensorRoom.getHumidity());

  return houseStats;
}

Point veToInflux(String pointName, VEDirectParseMessage parsedMessage, std::map<String, int> conversionFactors, std::map<String, String> displayNames, CodeMap codes)
{
  String action = "veToInflux: " + pointName;
  storeDataToNvs("lastState", action.c_str());

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

void veToMqtt(VEDirectParseMessage parsedMessage,
              std::map<String, int> conversionFactors,
              const std::map<String, String> mqttTopics)
{
  const auto &intData = parsedMessage.getIntMap();
  for (auto const &[key, val] : intData)
  {
    if (!mqttTopics.count(key))
      continue;

    float floatVal = conversionFactors.count(key)
                         ? static_cast<float>(val) / conversionFactors.at(key)
                         : static_cast<float>(val);

    publishMqtt(mqttTopics.at(key), String(floatVal, 2));
  }
}

void publishMqtt(const String &topic, const String &payload, bool retain)
{
  if (!mqttClient.connected())
    return;
  mqttClient.publish(topic.c_str(), 0, retain, payload.c_str());
}

InverterAction shouldInverterBeOn()
{
  const auto &mpptIntData = mpptData.getIntMap();
  bool hasBatteryVoltage = mpptIntData.count("V");
  bool hasPanelVoltage = mpptIntData.count("VPV");

  if (!hasBatteryVoltage)
    return InverterAction::NO_CHANGE;

  int batteryVoltage = mpptIntData.at("V");
  int panelVoltage = hasPanelVoltage ? mpptIntData.at("VPV") : -1;

  if (batteryVoltage > INV_ON_VOLTAGE) // Battery voltage OK, turn on
    return InverterAction::TURN_ON;

  if (batteryVoltage < INV_OFF_VOLTAGE) // Battery voltage very low, turn off
    return InverterAction::TURN_OFF;

  if (batteryVoltage > 12000 && panelVoltage > 30000) // Sun is up, so probably OK to turn on at lower voltage
    return InverterAction::TURN_ON;

  return InverterAction::NO_CHANGE;
}

void sendInverterCommandViaEspNow(InverterAction action)
{
  storeDataToNvs("lastState", "sendInverterCommandViaEspNow");

  InverterMessage message;
  message.action = action;

  esp_err_t result = esp_now_send(controlUnitMacAdress, (uint8_t *)&message, sizeof(message));
  if (result != ESP_OK)
    eventLog.log("ESP-NOW: Misslyckades med att skicka data till styrenheten", EventLogger::LogLevel::WARNING);
}
