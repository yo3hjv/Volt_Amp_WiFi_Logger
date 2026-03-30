// HTML generation for the WiFi configuration page.
//
// This is a small self-contained HTML page served by the firmware at /wifi.
// It allows the user to scan for SSIDs, save STA credentials, and switch
// between AP and STA modes.

#include "WifiPage.h"

String buildWifiPageHtml(const String& msg, const String& ssidOptionsHtml, const String& savedSsid) {
  String html;

  html += "<!DOCTYPE html>\n";
  html += "<html>\n";
  html += "<head>\n";
  html += "  <meta name='viewport' content='width=device-width, initial-scale=1'>\n";
  html += "  <title>LabSupplyLog WiFi</title>\n";
  html += "  <style>\n";
  html += "    body { font-family: Arial, sans-serif; margin: 0; padding: 20px; color: #333; }\n";
  html += "    .container { max-width: 700px; margin: 0 auto; }\n";
  html += "    .section { margin-bottom: 20px; padding: 15px; border: 1px solid #ddd; border-radius: 4px; }\n";
  html += "    .btn { display: inline-block; padding: 8px 12px; background-color: #3498db; color: white; text-decoration: none; border-radius: 4px; border: none; cursor: pointer; }\n";
  html += "    .btn:hover { background-color: #2980b9; }\n";
  html += "    .message { margin-bottom: 20px; padding: 10px; border-radius: 4px; }\n";
  html += "    .message-ok { background-color: #d4edda; border: 1px solid #c3e6cb; color: #155724; }\n";
  html += "    .message-err { background-color: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; }\n";
  html += "    label { display:block; margin-top: 10px; }\n";
  html += "    input, select { width: 100%; padding: 8px; margin-top: 6px; box-sizing: border-box; }\n";
  html += "  </style>\n";
  html += "</head>\n";

  html += "<body>\n";
  html += "<div class='container'>\n";
  html += "<h2>WiFi Setup (AP -> STA)</h2>\n";

  if (msg.length() > 0) {
    String cls = "message-ok";
    if (msg.indexOf("Error") >= 0 || msg.indexOf("failed") >= 0) cls = "message-err";
    html += "<div class='message " + cls + "'>" + msg + "</div>\n";
  }

  html += "<div class='section'>\n";
  html += "<form method='POST' action='/wifi_save'>\n";
  html += "<label>SSID</label>\n";
  html += "<select name='ssid'>\n";
  if (savedSsid.length() > 0) {
    html += "<option value='" + savedSsid + "' selected>" + savedSsid + " (saved)</option>\n";
  }
  html += ssidOptionsHtml;
  html += "</select>\n";

  html += "<label>SSID (manual)</label>\n";
  html += "<input type='text' name='ssid_manual' placeholder='Type SSID here (optional)'>\n";

  html += "<label>Password</label>\n";
  html += "<input type='password' name='pass' placeholder='WiFi password'>\n";

  html += "<label>Action</label>\n";
  html += "<select name='action'>\n";
  html += "  <option value='save_switch' selected>Save and switch to STA</option>\n";
  html += "  <option value='save_only'>Save only</option>\n";
  html += "</select>\n";

  html += "<div style='margin-top: 15px;'>\n";
  html += "<input type='submit' value='Apply' class='btn' style='width: 140px;'>\n";
  html += " <a class='btn' href='/wifi_scan' style='background-color:#6c757d;'>Scan</a>\n";
  html += " <a class='btn' href='/wifi_ap' style='background-color:#e67e22;'>Switch to AP</a>\n";
  html += "</div>\n";
  html += "</form>\n";
  html += "</div>\n";

  html += "<div class='section'>\n";
  html += "<p>After switching to STA, access the device via mDNS: <b>http://powersupply.local/</b> (or by its DHCP IP).</p>\n";
  html += "<p><a class='btn' href='/'>Back</a></p>\n";
  html += "</div>\n";

  html += "</div>\n";

  html += "<div style='margin-top: 22px; text-align:center; color:#777; font-size:12px;'>Author: Adrian YO3HJV - March 2026</div>\n";

  html += "</body>\n";
  html += "</html>\n";

  return html;
}
