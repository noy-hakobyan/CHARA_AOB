#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include <Arduino.h>
#include <SD.h>
#include "config.h"

// Global network settings (will be set by readNetworkConfig)
IPAddress g_deviceIP;
IPAddress g_deviceGateway;
IPAddress g_deviceDNS;
IPAddress g_deviceSubnet;
uint16_t g_port;

/* Parse IP address string (e.g., "192.168.2.149" → IPAddress) */
static inline bool parseIP(const char *str, IPAddress &addr) {
  if (!str) return false;
  
  int parts[4] = {0, 0, 0, 0};
  int count = sscanf(str, "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]);
  
  if (count != 4) return false;
  if (parts[0] < 0 || parts[0] > 255 || parts[1] < 0 || parts[1] > 255 ||
      parts[2] < 0 || parts[2] > 255 || parts[3] < 0 || parts[3] > 255) {
    return false;
  }
  
  addr = IPAddress((uint8_t)parts[0], (uint8_t)parts[1], (uint8_t)parts[2], (uint8_t)parts[3]);
  return true;
}

/* Parse key=value line (e.g., "IP=192.168.2.149") */
static inline bool parseLine(const char *line, const char *key, char *value, size_t maxLen) {
  if (!line || !key || !value) return false;
  
  // Skip whitespace
  while (*line && isspace(*line)) line++;
  
  // Check if line starts with key
  size_t keyLen = strlen(key);
  if (strncasecmp(line, key, keyLen) != 0) return false;
  
  line += keyLen;
  
  // Expect '='
  while (*line && isspace(*line)) line++;
  if (*line != '=') return false;
  line++;
  
  // Skip whitespace after '='
  while (*line && isspace(*line)) line++;
  
  // Copy value, trim trailing whitespace
  size_t i = 0;
  while (*line && !isspace(*line) && i < maxLen - 1) {
    value[i++] = *line++;
  }
  value[i] = '\0';
  
  return i > 0;
}

/* Read network.txt from SD card and populate globals */
static inline void readNetworkConfig(EthernetClient &client) {
  // Initialize with defaults
  g_deviceIP = deviceIP();
  g_deviceGateway = deviceGateway();
  g_deviceDNS = deviceDNS();
  g_deviceSubnet = deviceSubnet();
  g_port = PORT_NUM;
  
  // Try to read network.txt
  if (!SD.exists("network.txt")) {
    Serial.println("WARN: network.txt not found on SD card, using defaults");
    return;
  }
  
  File f = SD.open("network.txt", FILE_READ);
  if (!f) {
    Serial.println("ERROR: Failed to open network.txt");
    return;
  }
  
  char line[100];
  int lineLen = 0;
  bool readSuccess = true;
  int successCount = 0;
  
  while (f.available() > 0 && lineLen < sizeof(line) - 1) {
    char c = f.read();
    
    if (c == '\n' || c == '\r') {
      if (lineLen > 0) {
        line[lineLen] = '\0';
        
        char value[50] = {0};
        IPAddress temp;
        uint32_t portVal = 0;
        
        if (parseLine(line, "IP", value, sizeof(value)) && parseIP(value, temp)) {
          g_deviceIP = temp;
          successCount++;
        } else if (parseLine(line, "GATEWAY", value, sizeof(value)) && parseIP(value, temp)) {
          g_deviceGateway = temp;
          successCount++;
        } else if (parseLine(line, "DNS", value, sizeof(value)) && parseIP(value, temp)) {
          g_deviceDNS = temp;
          successCount++;
        } else if (parseLine(line, "SUBNET", value, sizeof(value)) && parseIP(value, temp)) {
          g_deviceSubnet = temp;
          successCount++;
        } else if (parseLine(line, "PORT", value, sizeof(value))) {
          portVal = atol(value);
          if (portVal > 0 && portVal < 65536) {
            g_port = (uint16_t)portVal;
            successCount++;
          }
        }
      }
      lineLen = 0;
    } else if (c != '\r') {
      line[lineLen++] = c;
    }
  }
  
  f.close();
  
  // Print results
  if (successCount >= 4) {
    Serial.println("Network config loaded from network.txt");
    Serial.print("  IP: "); Serial.println(g_deviceIP);
    Serial.print("  Gateway: "); Serial.println(g_deviceGateway);
    Serial.print("  Subnet: "); Serial.println(g_deviceSubnet);
    Serial.print("  DNS: "); Serial.println(g_deviceDNS);
    Serial.print("  Port: "); Serial.println(g_port);
  } else {
    Serial.println("ERROR: network.txt read failed or incomplete, using defaults");
    g_deviceIP = deviceIP();
    g_deviceGateway = deviceGateway();
    g_deviceDNS = deviceDNS();
    g_deviceSubnet = deviceSubnet();
    g_port = PORT_NUM;
  }
}

#endif // NETWORK_CONFIG_H
