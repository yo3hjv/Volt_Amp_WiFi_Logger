// WiFi manager public API.
//
// The WiFi manager controls AP/STA mode switching, asynchronous scanning,
// and basic connection timeout handling.

#pragma once

void initWiFi();
void tickWiFi();

bool startApMode();
bool startStaMode(const char* ssid, const char* pass);
