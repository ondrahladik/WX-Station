#ifndef WEB_H
#define WEB_H

#include <Arduino.h>
#include <WebServer.h>

extern WebServer server;
extern bool loadConfig();
extern bool saveConfig();
extern void startCaptivePortal();

void setupWeb();       
void handleRoot();   
void handleSave();    

#endif