#ifndef CGIWIFI_H
#define CGIWIFI_H

#include "httpd.h"
#define CONFIG_SECTOR 0x2C 
#define CONFIG_ADDRESS 0x2C000
typedef struct {
   uint8_t country[4];
   uint8_t title[18];
   uint8_t body[34];
   uint8_t workingmode[4];
   int firstconfig;
} config;

int cgiWiFiScan(HttpdConnData *connData);

int tplWlan(HttpdConnData *connData, char *token, void **arg);
int tplPb(HttpdConnData *connData, char *token, void **arg);
int cgiWiFi(HttpdConnData *connData);
int cgiWiFiConnect(HttpdConnData *connData);

int cgiWiFiSetMode(HttpdConnData *connData);
int cgiWiFiConnStatus(HttpdConnData *connData);

#endif