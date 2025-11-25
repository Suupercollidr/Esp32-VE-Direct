#include <esp_system.h>
#include <esp_log.h>
#include <Arduino.h>
#include <optional>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <FS.h>
#include <SD.h>
#include <RTClib.h>
#include <ESP32Ping.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "configuration.h"
// #include "dev_configuration.h"
#include "mappings.h"
#include "logger.h"
#include "DHTSensor.h"
#include "NTCSensor.h"

InfluxDBClient influxClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_DATA_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
InfluxDBClient influxLogClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_LOG_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point sensor("solar_status");

DHTSensor dhtSensor(DHT_PIN);
NTCSensor ntcSensor1(NTC_POWER_PIN, NTC1_READ_PIN);
NTCSensor ntcSensor2(NTC_POWER_PIN, NTC2_READ_PIN);

/*
// Define stuff for the NTC
const int nominal_resistance = 10000; // Nominal resistance at 25⁰C
const int nominal_temperature = 25;   // temperature for nominal resistance (almost always 25⁰ C)
const int samplingrate = 5;           // Number of samples
const int beta = 3950;                // The beta coefficient or the B value of the thermistor (usually 3000-4000) check the datasheet for the accurate value.
const int Rref = 9860;                // Value of  resistor used for the voltage divider (measured)
const float measuredOffset = 5.82;    // Measured offset compared to DHT11 sensor
*/

uint32_t lastUpdate = millis();
// uint32_t lastTempReading = millis();
uint32_t lastInverterPowerChange = millis();

tm timeinfo;
time_t now;

bool sdInserted;
int inverterOn = 1;

struct greenhouseSensorData
{
  int16_t outdoorTemp;
  int16_t indoorTemp;
  int16_t soilTemp1;
  int16_t soilTemp2;
  uint16_t indoorHumidity;
  uint16_t soilMoisture1;
  uint16_t soilMoisture2;
  uint16_t batteryVoltage;
  uint8_t checksum;
};

std::map<String, int> dataMap; // Map to store data from all sources

void logEvent(const String, const String);
bool logEventToFile(const char *, const String, const String);
bool logEventToInfluxDB(const char *, const String, const String);
uint8_t calculateChecksum(const greenhouseSensorData &);
const char *getResetReason(esp_reset_reason_t);
void storeDataToNvs(const char *, const char *);
String readDataFromNvs(const char *);
bool populateDataMap();
void initSd();
void initWiFi();
bool checkInfluxDbConnection();
void sendToInfluxDB();
void appendDataToFile(const String);
String veDirectGetFromSerial(HardwareSerial &serial);
String veDirectTrimMessage(String);
void veDirectGetData(HardwareSerial &serial, const String &prefix);
bool veDirectCalcChecksum(String);
void veDirectStoreMessage(String, String);
int getNtcTemp(int READ_PIN);
void getTempHumid();
greenhouseSensorData getGreenhouseData();
String convertMessageCode(String label, int code);
// std::vector<int> findCombination(const std::vector<int> &series, int code);
void controlInverterByVoltage();

void setup()
{
  nvs_flash_init();
  String lastEventBeforeReboot = readDataFromNvs("lastState");

  Serial.begin(9600);
  Serial1.begin(19200, SERIAL_8N1, RXD1, TXD1);
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);

  Serial.println("====================");
  Serial.println(" System is starting ");
  Serial.println("====================");

  // pinMode(NTC_POWER_PIN, OUTPUT);
  // pinMode(NTC1_READ_PIN, INPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(SD_DET, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SD_DET), initSd, CHANGE);

  //  initSd();
  initWiFi();
  timeSync(TIME_ZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
  checkInfluxDbConnection();
  //  dht.setup(DHT_PIN, DHTesp::DHT11);

  esp_reset_reason_t resetReason = esp_reset_reason();

  float setupTime = millis() / 1000;
  logEvent(String("Systemet startat. Uppstarten tog " + String(setupTime) + " s."), "INFO");
  logEvent(String("Senaste återställning: " + String(getResetReason(resetReason)) + " (" + String(resetReason) + ")"), "INFO");
  logEvent(String("Senaste åtgärd: " + lastEventBeforeReboot), "INFO");

  storeDataToNvs("lastState", "Setup end");
}

void loop()
{
  storeDataToNvs("lastState", "Loop");

  if (millis() >= lastUpdate + (UPDATE_INTERVAL * 1000))
  {
    populateDataMap();
    // appendDataToFile(dataFileName);  // Skip writing to file for now, see if that helps uptimes
    controlInverterByVoltage();
    sendToInfluxDB();
    lastUpdate = millis();
  }
  yield();
  delay(10);
}

void initSd() // Initialize SD card
{
  storeDataToNvs("lastState", "initSd");
  sdInserted = (digitalRead(SD_DET) == LOW); // True if SD card is inserted

  if (!sdInserted)
  {
    logEvent("SD card removed", "INFO");
    return;
  }

  logEvent("SD card inserted", "INFO");

  if (!SD.begin())
  {
    logEvent("Could not mount SD card", "ERROR");
    return;
  }

  logEvent("SD card mounted", "INFO");
  String cardSizeMessage = "SD card size: " + String(SD.cardSize() / (1024 * 1024)) + "MB";
  logEvent(cardSizeMessage, "INFO");
}

void initWiFi() // Connect to WiFi
{
  storeDataToNvs("lastState", "initWiFi");
  logEvent(String("Connecting to WiFi " + String(ssid)), "INFO");
  WiFi.begin(ssid, password, 6);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    yield();
    Serial.print(".");
  }
  Serial.println();
  logEvent(String("Connected to WiFi " + String(ssid)), "INFO");
}

void logEvent(const String messageText, const String messageLevel = "ERROR")
{
  storeDataToNvs("lastState", "logEvent");

  time(&now); // Get current time
  char logTimeStamp[30];
  strftime(logTimeStamp, 30, "%Y-%m-%d\t%H:%M:%S", localtime(&now));

  bool writtenToFile = false;
  bool sentToInfluxDB = false;

  //  writtenToFile = logEventToFile(logTimeStamp, messageText, messageLevel);  // Skip writing logs to file

  if ((WiFi.status() == WL_CONNECTED)) // Send event to InfluxDB, but only if connected
  {
    sentToInfluxDB = logEventToInfluxDB(logTimeStamp, messageText, messageLevel);
  }

  Serial.print(String(logTimeStamp) + "\t" + messageLevel + "\t" + messageText);
  //  Serial.print(writtenToFile ? " (written to logfile, " : " (not written to logfile, ");
  Serial.println(sentToInfluxDB ? " sent to Influx DB)" : " not sent to Influx DB)");
}

bool logEventToFile(const char *logTimeStamp, const String messageText, const String messageLevel = "ERROR")
{
  storeDataToNvs("lastState", "logEventToFile");
  bool saveStatus;
  File logFile = SD.open(logFileName, FILE_WRITE);
  logFile.seek(logFile.size()); // Set file pointer to end of file
  String message = String(logTimeStamp) + "\t" + messageLevel + "\t" + messageText;
  if (logFile)
  {
    logFile.println(message);
    logFile.flush();
    logFile.close();
    return true;
  }
  return false;
}

bool logEventToInfluxDB(const char *logTimeStamp, const String messageText, const String messageLevel = "ERROR")
{
  storeDataToNvs("lastState", "logEventToInfluxDB");
  // Create data point for InfluxDB
  Point logPoint("EventLog");
  logPoint.addTag("level", messageLevel);
  logPoint.addField("message", messageText);
  logPoint.addField("timestamp", logTimeStamp);

  // Send data point to InfluxDB
  const bool influxDbResponse = influxLogClient.writePoint(logPoint); // Send data point to InfluxDB
  if (influxDbResponse)
  {
    return true;
  }
  Serial.println(influxLogClient.getLastErrorMessage());
  return false;
}

uint8_t calculateChecksum(const greenhouseSensorData &data)
{
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&data);
  uint8_t sum = 0;

  // Exclude the checksum field itself from calculation
  for (size_t i = 0; i < sizeof(greenhouseSensorData) - sizeof(data.checksum); ++i)
  {
    sum ^= bytes[i];
  }
  return sum;
}

const char *getResetReason(esp_reset_reason_t reason)
{
  storeDataToNvs("lastState", "getResetReason");
  switch (reason)
  {
  case ESP_RST_UNKNOWN:
    return "Unknown";
  case ESP_RST_POWERON:
    return "Power on";
  case ESP_RST_EXT:
    return "External reset";
  case ESP_RST_SW:
    return "Software reset";
  case ESP_RST_PANIC:
    return "Software panic";
  case ESP_RST_INT_WDT:
    return "Interrupt watchdog";
  case ESP_RST_TASK_WDT:
    return "Task watchdog";
  case ESP_RST_WDT:
    return "Other watchdogs";
  case ESP_RST_DEEPSLEEP:
    return "Deep sleep";
  case ESP_RST_BROWNOUT:
    return "Brownout";
  case ESP_RST_SDIO:
    return "SDIO";
  default:
    return "Not a valid reset reason";
  }
}

void storeDataToNvs(const char *key, const char *value)
{
  if (!debugModeEnabled)
    return;
  nvs_handle_t stateLoggingHandle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &stateLoggingHandle);
  if (err == ESP_OK)
  {
    nvs_set_str(stateLoggingHandle, key, value);
    nvs_commit(stateLoggingHandle);
    nvs_close(stateLoggingHandle);
  }
}

String readDataFromNvs(const char *key)
{
  nvs_handle_t stateLoggingHandle;
  char value[100];
  size_t required_size;
  esp_err_t err = nvs_open("storage", NVS_READONLY, &stateLoggingHandle);
  if (err == ESP_OK)
  {
    nvs_get_str(stateLoggingHandle, key, NULL, &required_size);
    nvs_get_str(stateLoggingHandle, key, value, &required_size);
    nvs_close(stateLoggingHandle);
  }
  return String(value);
}

bool checkInfluxDbConnection()
{
  storeDataToNvs("lastState", "checkInfluxDbConnection");
  if (influxClient.validateConnection())
  {
    logEvent(String("Ansluten till Influx DB: " + influxClient.getServerUrl()), "INFO");
    return true;
  }
  else
  {
    logEvent(String("Kunde inte ansluta till Influx DB: " + influxClient.getLastErrorMessage()));
    return false;
  }
}

void sendToInfluxDB()
{
  storeDataToNvs("lastState", "sendToInfluxDB");
  Point dataPoint("SolarPower");

  for (auto const &entry : dataMap) // Go through all values in dataMap
  {
    String label = entry.first;
    float value = entry.second;

    if (labelMapping.count(label) > 0) // Only send values if they exist in 'labelMapping'
    {
      String name = labelMapping.at(label).displayName;
      int conversionFactor = labelMapping.at(label).conversionFactor;
      int intValue = round(value);
      String humanReadableMsg = "";
      switch (conversionFactor) // Check if we need to convert values
      {
      case 0:
        //  0: Status. Send both int and human-readable message
        humanReadableMsg = convertMessageCode(label, value);
        dataPoint.addField(name, humanReadableMsg); // Add human-readable message as a separate field
        dataPoint.addField(label, intValue);        // Send original value as int (no float for error codes)
        break;

      default:
        // >0: Divide by conversion factor to get reasonable unit (for example, convert mV to V)
        value /= conversionFactor; // Convert integerized value to sensible float value (like mV to V)
        dataPoint.addField(label, value);
        break;
      }
    }
    yield();
  }

  const bool influxDbResponse = influxClient.writePoint(dataPoint); // Send data point to InfluxDB
  Serial.print("Sending to InfluxDB");
  if (!influxDbResponse)
  {
    Serial.println(" failed.");
    logEvent(influxClient.getLastErrorMessage());
    return;
  }
  Serial.println(" sucessful.");
}

void appendDataToFile(const String fileName)
{
  storeDataToNvs("lastState", "appendDataToFile");
  if (!SD.exists("/") || !sdInserted)
  {
    logEvent("Can't write data to file because SD card is not available");
    return;
  }
  bool headers = false;
  if (!SD.exists(fileName)) // If new file, we must add headers
  {
    headers = true;
    logEvent("Creating new datafile", "INFO");
  }
  File dataFile = SD.open(fileName, FILE_WRITE);
  if (!dataFile)
  {
    logEvent("Couldn't open data file");
  }

  if (headers) // If new file, add headers
  {
    for (const auto &entry : labelMapping)
    { // Loop through list of labels and add the display name to the file
      dataFile.print(entry.second.displayName);
      dataFile.print(";");
      yield();
    }
    dataFile.println(); // Add linebreak after headers
  }

  dataFile.seek(dataFile.size()); // Set file pointer to end of file
  for (const auto &entry : labelMapping)
  {
    if (dataMap.count(entry.first) > 0) // Only print value if exist in dataMap (prevents creating empty value)
    {
      dataFile.print(dataMap[entry.first]);
    }
    dataFile.print(";");
    yield();
  }
  dataFile.println(); // Add linebreak after complete row
  Serial.println("Wrote one line of data to file.");
  dataFile.close();
  delay(10); // Just give the file time to be saved before we get up to any other shenannigans
}

bool populateDataMap()
{
  storeDataToNvs("lastState", "populateDataMap");
  dataMap.clear(); // Reset data map

  time(&now); // Get current time
  char currentDate[11];
  char currentTime[9];
  strftime(currentDate, 11, "%Y-%m-%d", localtime(&now));
  strftime(currentTime, 9, "%H:%M:%S", localtime(&now));
  struct tm *timeinfo = localtime(&now);
  int currentHour = timeinfo->tm_hour;
  dataMap["TIMESTAMP"] = now;

  dataMap["ESP_UPTIME"] = millis();
  dataMap["ESP_MEM_FREE"] = ESP.getFreeHeap();
  dataMap["ESP_MEM_LOWEST"] = ESP.getMinFreeHeap();
  dataMap["ESP_PSRAM_FREE"] = ESP.getFreePsram();
  dataMap["ESP_PSRAM_LOWEST"] = ESP.getFreePsram();
  dataMap["CTRL_INV_ON"] = inverterOn;

  veDirectGetData(Serial1, "INV"); // Get message from Phoenix inverter

  veDirectGetData(Serial2, "MPPT"); // Get message from SmartSolar MPPT

  getTempHumid(); // Get temperature and humidity from DHT and NTC sensors

  return true;
}

void veDirectGetData(HardwareSerial &serial, const String &prefix = "")
{
  storeDataToNvs("lastState", "veDirectGetData");
  String message;
  message = veDirectGetFromSerial(serial); // Get what's in the serial buffer
  message = veDirectTrimMessage(message);  // Make sure message contains exactly one message block
  if (!veDirectCalcChecksum(message))      // Verify checksum
  {
    String eventLogMessage = String("Checksum didn't match\r\nMessage block: \r\n" + message);
    logEvent(eventLogMessage);
    return;
  }
  veDirectStoreMessage(message, prefix); // Put values into the 'dataMap' variable
}

String veDirectGetFromSerial(HardwareSerial &serial)
{
  storeDataToNvs("lastState", "veDirectStoreMessage");
  String message;
  while (serial.available() > 0)
  {
    char c = serial.read();
    message += c;
    yield();
  }
  return message;
}

String veDirectTrimMessage(String message)
{
  storeDataToNvs("lastState", "veDirectTrimMessage");
  int startOfLastBlock = message.lastIndexOf("\r\nPID");                                   // The latest message is the last occurance of 'PID' in the string
  int endOfLastBlock = message.lastIndexOf("Checksum") + 10;                               // 10 = number of chars in 'Checksum' + tab + field value
  if (startOfLastBlock == -1 || endOfLastBlock == -1 || startOfLastBlock > endOfLastBlock) // If we can't find 'PID' or 'Checksum', or if message ends before it begins, something went wrong
  {
    String eventLogMessage = String("Serial block missing either \"PID\" or \"Checksum\"\r\nMessage block: \r\n" + message);
    logEvent(eventLogMessage);
    return "";
  }
  message = message.substring(startOfLastBlock, endOfLastBlock);
  yield();
  return message;
}

bool veDirectCalcChecksum(String message)
{
  storeDataToNvs("lastState", "veDirectCalcChecksum");
  int checksum = 0;
  for (char c : message)
  {
    checksum = (checksum + int(c)) & 256;
  }
  if (checksum != 0)
  {
    return false;
  }
  return true;
}

void veDirectStoreMessage(String message, String prefix)
{
  storeDataToNvs("lastState", "veDirectStoreMessage");
  String fieldLabel;
  String fieldValue;
  bool isFieldValue = false; // Decides if we read field label or field value
  for (char c : message)
  {
    switch (c)
    {
    case 0x0D:               // Carriage return means end of field. Finalize and save to dataMap.
      if (!prefix.isEmpty()) // If a prefix was sent to this function, add it to the field label
      {
        fieldLabel = "VE_" + prefix + "_" + fieldLabel;
      }
      dataMap[fieldLabel] = fieldValue.toInt(); // Save what we've got
      fieldLabel.clear();                       // Reset variables
      fieldValue.clear();
      isFieldValue = false; // Next char will be label
      break;

    case 0x0A: // Line feed. Ignore, because we finalized the field after carriage return
      break;

    case 0x09: // Tab, means end of label, start of value
      isFieldValue = true;
      break;

    default:            // Actual data
      if (isFieldValue) // Add character to value or label
      {
        fieldValue += c;
      }
      else
      {
        fieldLabel += c;
      }
      break;
    }
    yield();
  }
}

void getTempHumid() // Collect temperature and humidity and save to 'dataMap'
{
  storeDataToNvs("lastState", "getTempHumid");

  // DHT Sensor
  if (auto temp = dhtSensor.temperature(); temp.has_value())
    dataMap["ENV_ROOM_TEMP"] = *temp;

  if (auto humidity = dhtSensor.humidity(); humidity.has_value())
    dataMap["ENV_ROOM_HUMID"] = *humidity;

  // NTC sensors
  if (auto temperature = ntcSensor1.temperature(); temperature.has_value())
    dataMap["ENV_REFRIG_TEMP"] = *temperature;

  if (auto temperature = ntcSensor2.temperature(); temperature.has_value())
    dataMap["ENV_REFRIG_TEMP"] = *temperature;

  Serial.println("Read the following sensor values: ");
  Serial.printf("Room humidity:            %d %RH", dataMap["ENV_ROOM_HUMID"]);
  Serial.printf("Room temperature:         %d °C", dataMap["ENV_ROOM_TEMP"]);
  Serial.printf("Refrigurator temperature: %d °C", dataMap["ENV_REFRIG_TEMP"]);
  Serial.printf("Freezer temperature:      %d °C", dataMap["ENV_FREEZER_TEMP"]);
}


void controlInverterByVoltage()
{
  const int voltage = dataMap["VE_MPPT_V"]; // Battery voltage in mV. Using MPPT voltage, since Inv. voltage = 0 when off

  // If state changed more recent than retry period, do nothing
  if (millis() - lastInverterPowerChange < (INV_RETRY_PERIOD * 1000))
  {
    return;
  }

  // Do nothing on coco-bananas values
  if (voltage < 1000 || voltage > 20000)
  {
    String messageText = "Orealistikt spänningsvärde (" + String(voltage) + " V), ändrar inte status på inverter";
    logEvent(messageText, "WARNING");
    return;
  }

  // If battery voltage is lower then off voltage, turn inverter off
  if (voltage < INV_OFF_VOLTAGE)
  {
    digitalWrite(RELAY1, HIGH); // Relay is NC
    lastInverterPowerChange = millis();
    inverterOn = 0;
    logEvent("Inverter stängdes av, låg batterispänning", "INFO");
    return;
  }

  // If battery voltage is higher than on voltage, turn inverter on
  if (voltage > INV_ON_VOLTAGE)
  {
    digitalWrite(RELAY1, LOW); // Relay is NC
    lastInverterPowerChange = millis();
    inverterOn = 1;
    logEvent("Inverter slogs på, tillräcklig batterispänning ", "INFO");
    return;
  }
}

String convertMessageCode(String label, int ReceivedCode) // Recieves a label representing message type and a code representing one or more messages
{
  storeDataToNvs("lastState", "convertMessageCode");
  try
  {
    const std::map<const int, const String> &CodeToMessageMappings = LabelCodeMapping.at(label); // Get the code-to-message mapping for this 'label'
    String ConcatenatedMessage;

    if (CodeToMessageMappings.count(ReceivedCode) > 0) // If the message code is an exact match, there is only one message and we can return that
    {
      ConcatenatedMessage = CodeToMessageMappings.at(ReceivedCode);
      return ConcatenatedMessage;
    }

    // Workaround that ignores the hassle of looking for combinations (because it always crashes)
    String messageForCobinedMessage = "No match for " + String(ReceivedCode) + ". Might be a combination message.";
    return (messageForCobinedMessage);

    /*
        // If no exact match, extract individual messages using maths

        // Gather all codes for this label
        std::vector<int> AvailableCodes;
        for (const auto &entry : CodeToMessageMappings)
        {
          AvailableCodes.push_back(entry.first);
        }

        // Get a vector of all codes that are summed to make up 'RecievedCode'
        std::vector<int> CodesInMessage = findCombination(AvailableCodes, ReceivedCode);

        if (CodesInMessage.empty())
        {
          throw std::logic_error("Code recieved from VE.Direct doesn't exactly match message table");
        }

        for (int ThisCode : CodesInMessage)
        {
          if (ConcatenatedMessage != "") // If this is not the first message, add comma and space before
          {
            ConcatenatedMessage += ", ";
          }
          ConcatenatedMessage += CodeToMessageMappings.at(ThisCode);
          yield();
        }
        return ConcatenatedMessage;*/
  }
  catch (const std::out_of_range &e) // There is no entry for this label in 'LabelCodeMapping'
  {
    String ErrorMessage = "Hittade inga meddelanden för " + label;
    logEvent(ErrorMessage);
    logEvent(e.what());
    return ErrorMessage;
  }
  catch (const std::runtime_error &e) // There is an entry, but it is empty
  {
    String ErrorMessage = "Hittade inget meddelanden för kod " + String(ReceivedCode) + " för " + label;
    logEvent(ErrorMessage);
    logEvent(e.what());
    return ErrorMessage;
  }
  catch (const std::logic_error &e) // Extracted a code that doesn't have an entry
  {
    String ErrorMessage = "Felaktig meddelandekod " + String(ReceivedCode) + " för " + label;
    logEvent(ErrorMessage);
    logEvent(e.what());
    return ErrorMessage;
  }
}

/*
// Function to find the combination of numbers from the series that sum up to the given code
std::vector<int> findCombination(const std::vector<int> &series, int code)
{
  storeDataToNvs("lastState", "findCombination");
  int32_t startTime = millis();
  int timeOut = 2 * 1000;

  try
  {
    // Get the size of the series
    int seriesSize = series.size();

    // Initialize a 2D array to store the dynamic programming table
    std::vector<std::vector<bool>> dp(seriesSize + 1, std::vector<bool>(code + 1, false));

    storeDataToNvs("lastState", "findCombination, about to init first column in table");

    // Initialize the first column of the table
    for (int i = 0; i <= seriesSize; i++)
    {
      dp[i][0] = true;
      // Timeout guard
      yield();
      if (millis() - startTime > timeOut)
      {
        throw std::runtime_error("Timeout");
      }
    }

    storeDataToNvs("lastState", "findCombination, about to fill dynamic programming table");

    // Fill the dynamic programming table
    for (int i = 1; i <= seriesSize; i++)
    {
      for (int j = 1; j <= code; j++)
      {
        // If the current element is greater than the sum, it cannot be included
        if (series[i - 1] > j)
        {
          dp[i][j] = dp[i - 1][j];
        }
        // Otherwise, consider including or excluding the current element
        else
        {
          dp[i][j] = dp[i - 1][j] || dp[i - 1][j - series[i - 1]];
        }

        // Timeout guard
        yield();
        if (millis() - startTime > timeOut)
        {
          throw std::runtime_error("Timeout");
        }
      }
    }

    // Check if there is a valid combination for the given code
    if (!dp[seriesSize][code])
    {
      // No valid combination found, return an empty vector
      return std::vector<int>();
    }

    // Reconstruct the combination from the dynamic programming table
    std::vector<int> combination;
    int i = seriesSize;
    int j = code;
    while (i > 0 && j > 0)
    {
      if (dp[i][j] && !dp[i - 1][j])
      {
        // Include the current element in the combination
        combination.push_back(series[i - 1]);
        j -= series[i - 1];
      }
      i--;
      // Timeout guard
      yield();
      if (millis() - startTime > timeOut)
      {
        throw std::runtime_error("Timeout");
      }
    }

    return combination;
  }
  catch (const std::runtime_error)
  {
    String errorMessage = "Timeout when trying to find combination of error messages. Returns empty vector. ";
    logEvent(errorMessage, "WARNING");
    return std::vector<int>();
  }
  catch (const std::exception)
  {
    String errorMessage = "Unhandled error when trying to find combination of error messages. Returns empty vector. ";
    logEvent(errorMessage, "WARNING");
    return std::vector<int>();

  }
}
*/