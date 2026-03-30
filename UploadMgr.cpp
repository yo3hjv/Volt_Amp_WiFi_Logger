// SPIFFS upload / file management routes.
//
// Responsibilities:
// - Provide a web UI for managing SPIFFS content.
// - /upload: upload and manage web assets (html/css/js/images).
// - /logs: list and manage log CSV files.
// - Download and delete routes for files stored in SPIFFS root.
//
// Note: this module intentionally filters log files out of the Web Files section.

#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

#include "UploadMgr.h"
#include "UploadPage.h"

static File s_uploadFile;
static String s_uploadPath;

static bool isAllowedExt(const String& filename) {
  String lower = filename;
  lower.toLowerCase();
  return lower.endsWith(".html") || lower.endsWith(".css") || lower.endsWith(".js") || lower.endsWith(".png") ||
         lower.endsWith(".ico") || lower.endsWith(".svg") || lower.endsWith(".csv");
}

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static bool isTimestampLogName(const String& name) {
  // /DDMMYY_HHMMSS.csv or /DDMMYY_HHMMSS_1.csv
  String n = name;
  if (!n.startsWith("/")) n = "/" + n;
  if (!n.endsWith(".csv")) return false;
  if (n.length() < 1 + 6 + 1 + 6 + 4) return false;

  for (int i = 1; i <= 6; i++) {
    if (!isDigit(n[i])) return false;
  }
  if (n[7] != '_') return false;
  for (int i = 8; i <= 13; i++) {
    if (!isDigit(n[i])) return false;
  }

  int dot = n.lastIndexOf('.');
  if (dot < 0) return false;
  if (dot == 14) return true;
  if (n[14] != '_') return false;
  for (int i = 15; i < dot; i++) {
    if (!isDigit(n[i])) return false;
  }
  return true;
}

static bool isLogName(const String& path) {
  String p = path;
  if (!p.startsWith("/")) p = "/" + p;
  if (p.startsWith("/log_") && p.endsWith(".csv")) return true;
  return isTimestampLogName(p);
}

static String htmlEscape(const String& s) {
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

static String buildFileTable(bool logs) {
  String html;
  html += "<table>\n";
  html += "<tr><th>File</th><th>Size</th><th>Actions</th></tr>\n";

  File root = SPIFFS.open("/");
  File f = root.openNextFile();
  while (f) {
    File cur = f;
    f = root.openNextFile();

    String name = String(cur.name());
    size_t size = cur.size();
    cur.close();

    bool isLog = isLogName(name);
    if (logs != isLog) continue;

    if (!logs) {
      if (!isAllowedExt(name)) continue;
      if (isLog) continue;
    }

    String nameNoSlash = name;
    if (nameNoSlash.startsWith("/")) nameNoSlash = nameNoSlash.substring(1);

    html += "<tr>";
    html += "<td>" + htmlEscape(nameNoSlash) + "</td>";
    html += "<td>" + String(size) + " bytes</td>";
    html += "<td>";
    html += "<a class='btn' href='/download?file=" + htmlEscape(name) + "'>Download</a> ";
    html += "<a class='btn btn-danger' href='/delete?file=" + htmlEscape(name) + "' onclick=\"return confirm('Delete " + htmlEscape(nameNoSlash) + "?');\">Delete</a>";
    html += "</td>";
    html += "</tr>\n";
  }

  html += "</table>\n";
  return html;
}

void registerUploadRoutes(AsyncWebServer& server) {
  server.on("/logs", HTTP_GET, [](AsyncWebServerRequest* req) {
    String msg = "";
    if (req->hasParam("msg")) {
      msg = req->getParam("msg")->value();
    }
    String html = buildUploadPageHtml(msg, "", buildFileTable(true));
    req->send(200, "text/html", html);
  });

  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest* req) {
    String msg = "";
    if (req->hasParam("msg")) {
      msg = req->getParam("msg")->value();
    }

    String html = buildUploadPageHtml(msg, buildFileTable(false), buildFileTable(true));
    req->send(200, "text/html", html);
  });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (!req->hasParam("file")) {
      req->send(400, "text/plain", "missing file");
      return;
    }
    String file = req->getParam("file")->value();
    if (!file.startsWith("/")) file = "/" + file;

    if (!SPIFFS.exists(file)) {
      req->send(404, "text/plain", "not found");
      return;
    }

    String contentType = "application/octet-stream";
    if (file.endsWith(".html")) contentType = "text/html";
    else if (file.endsWith(".css")) contentType = "text/css";
    else if (file.endsWith(".js")) contentType = "application/javascript";
    else if (file.endsWith(".png")) contentType = "image/png";
    else if (file.endsWith(".ico")) contentType = "image/x-icon";
    else if (file.endsWith(".svg")) contentType = "image/svg+xml";
    else if (file.endsWith(".csv")) contentType = "text/csv";

    req->send(SPIFFS, file, contentType, true);
  });

  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (!req->hasParam("file")) {
      req->send(400, "text/plain", "missing file");
      return;
    }
    String file = req->getParam("file")->value();
    if (!file.startsWith("/")) file = "/" + file;

    if (!SPIFFS.exists(file)) {
      req->redirect("/upload?msg=Error:%20not%20found");
      return;
    }

    if (SPIFFS.remove(file)) {
      req->redirect("/upload?msg=Deleted");
    } else {
      req->redirect("/upload?msg=Error:%20delete%20failed");
    }
  });

  server.on(
      "/upload", HTTP_POST,
      [](AsyncWebServerRequest* req) {
        req->redirect("/upload?msg=Upload%20done");
      },
      [](AsyncWebServerRequest* req, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
        (void)req;
        if (index == 0) {
          String fn = filename;
          if (!fn.startsWith("/")) fn = "/" + fn;

          if (!isAllowedExt(fn)) {
            s_uploadPath = "";
            return;
          }

          s_uploadPath = fn;
          if (SPIFFS.exists(s_uploadPath)) {
            SPIFFS.remove(s_uploadPath);
          }
          s_uploadFile = SPIFFS.open(s_uploadPath, "w");
        }

        if (s_uploadFile) {
          s_uploadFile.write(data, len);
        }

        if (final) {
          if (s_uploadFile) {
            s_uploadFile.close();
          }
          s_uploadPath = "";
        }
      });
}

void initUploadRoutes() {
  // routes are registered by WebMgr via registerUploadRoutes
}
