// WiFi web routes.
//
// Provides a simple HTML configuration page at /wifi and endpoints for:
// - requesting an asynchronous scan (/wifi_scan)
// - switching back to AP (/wifi_ap)
// - saving credentials and switching to STA (/wifi_save)

#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#include "Globals.h"
#include "WifiMgr.h"
#include "WifiPage.h"

static String escapeHtml(const String& s) {
  String out;
  out.reserve(s.length());
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '\"') out += "&quot;";
    else out += c;
  }
  return out;
}

void registerWiFiWebRoutes(AsyncWebServer& server) {
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest* req) {
    String msg = "";
    if (req->hasParam("msg")) msg = req->getParam("msg")->value();

    String refreshNote = "";
    if (g_wifiSsidOptionsHtml.length() == 0) {
      refreshNote = " (no scan results yet - press Scan, wait ~5s, then reload)";
    }

    String html = buildWifiPageHtml(msg + refreshNote, g_wifiSsidOptionsHtml, g_staSsid);
    req->send(200, "text/html", html);
  });

  server.on("/wifi_scan", HTTP_GET, [](AsyncWebServerRequest* req) {
    g_wifiScanRequested = true;
    req->redirect("/wifi?msg=Scanning...");
  });

  server.on("/wifi_ap", HTTP_GET, [](AsyncWebServerRequest* req) {
    startApMode();
    // Keep STA enabled to allow scanning even while in AP
    WiFi.mode(WIFI_AP_STA);
    g_wifiScanRequested = true;
    req->redirect("/wifi?msg=Switched%20to%20AP");
  });

  server.on("/wifi_save", HTTP_POST, [](AsyncWebServerRequest* req) {
    String ssid = "";
    if (req->hasParam("ssid_manual", true)) {
      ssid = req->getParam("ssid_manual", true)->value();
      ssid.trim();
    }
    if (ssid.length() == 0) {
      if (!req->hasParam("ssid", true)) {
        req->redirect("/wifi?msg=Error:%20missing%20ssid");
        return;
      }
      ssid = req->getParam("ssid", true)->value();
      ssid.trim();
    }

    if (ssid.length() == 0) {
      req->redirect("/wifi?msg=Error:%20missing%20ssid");
      return;
    }

    String pass = "";
    if (req->hasParam("pass", true)) pass = req->getParam("pass", true)->value();

    String action = "save_switch";
    if (req->hasParam("action", true)) action = req->getParam("action", true)->value();

    g_staSsid = ssid;
    g_staPass = pass;
    g_prefs.putString("sta_ssid", g_staSsid);
    g_prefs.putString("sta_pass", g_staPass);

    if (action == "save_only") {
      req->redirect("/wifi?msg=Saved");
      return;
    }

    startStaMode(g_staSsid.c_str(), g_staPass.c_str());
    req->redirect("/wifi?msg=Switching%20to%20STA...");
  });
}
