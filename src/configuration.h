#pragma once
#include <HTTPClient.h>

#define debugModeEnabled false

const char dataFileName[] = "/datalog2.csv";
const char logFileName[] = "/system.log";

const char *ssid = "Lithuanian Consulate";
const char *password ="LJ&pp&UgheR4hpS4Qr2*P2QMTBvELbDnQ7bHfUVyd7%ayKzdKcz%gd";

const char *NTP_SERVER1 = "ntp1.sp.se";
const char *NTP_SERVER2 = "ntp2.sp.se";
const char *NTP_SERVER3 = "pool.ntp.org";
const char *TIME_ZONE = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";

// IPAddress for DNS servers
IPAddress primaryDNS(8, 8, 8, 8);   // Google primary DNS
IPAddress secondaryDNS(8, 8, 4, 4); // Google secondary DNS

//#define INFLUXDB_URL "http://treehouse.diskstation.me:8086" // On public internet 
#define INFLUXDB_URL "http://10.8.0.1:8086" // On VPN 
#define INFLUXDB_TOKEN "GZtnEBhSI-2xoMq3oCVpMZiMPGU-Zye_nhkptYh4XDMGmLtOkoVUoc0MZ3fZE23c7K2GsPhrwY4aLwXNUO1GbA=="
#define INFLUXDB_ORG "ef4fc5e4b4404a3a"
#define INFLUXDB_DATA_BUCKET "Embassy"
#define INFLUXDB_LOG_BUCKET "Embassy_logs"

// MAC address for transmitting ESP32 (greenhouse)
uint8_t transmitterMAC[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};


const int UPDATE_INTERVAL = 5; // How often to send data, in seconds
const int INV_OFF_VOLTAGE = 11000; // mV
const int INV_ON_VOLTAGE = 13000;  // mV
const int INV_RETRY_PERIOD = 1800; // seconds

// Define pins
#define SD_DET 16
#define RXD1 26
#define TXD1 27
#define RXD2 17
#define TXD2 25
#define DHT_PIN 33
#define NTC1_READ_PIN A7 // Pin, to which the voltage divider is connected
#define NTC2_READ_PIN A6 // Pin, to which the voltage divider is connected
#define NTC_POWER_PIN 32 // 3.3V for the voltage divider
#define RELAY1 5
#define RELAY2 23
#define RELAY3 19
#define RELAY4 18

