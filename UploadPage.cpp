// HTML generation for the SPIFFS Manager pages.
//
// The same generator is used for:
// - /upload (web assets + logs)
// - /logs (logs only)

#include "UploadPage.h"

String buildUploadPageHtml(const String& msg, const String& fileTableWeb, const String& fileTableLogs) {
  String html;

  html += "<!DOCTYPE html>\n";
  html += "<html>\n";
  html += "<head>\n";
  html += "  <meta name='viewport' content='width=device-width, initial-scale=1'>\n";
  html += "  <title>LabSupplyLog Upload</title>\n";
  html += "  <style>\n";
  html += "    body { font-family: Arial, sans-serif; margin: 0; padding: 20px; color: #333; }\n";
  html += "    h1, h2 { margin-top: 0; }\n";
  html += "    .container { max-width: 900px; margin: 0 auto; }\n";
  html += "    .section { margin-bottom: 20px; padding: 15px; border: 1px solid #ddd; border-radius: 4px; }\n";
  html += "    .section-upload { background-color: #e8f4f8; }\n";
  html += "    .section-web { background-color: #e8f0ff; }\n";
  html += "    .section-logs { background-color: #f8f4e8; }\n";
  html += "    .btn { display: inline-block; padding: 8px 12px; background-color: #3498db; color: white; text-decoration: none; border-radius: 4px; border: none; cursor: pointer; }\n";
  html += "    .btn:hover { background-color: #2980b9; }\n";
  html += "    .btn-danger { background-color: #e74c3c; }\n";
  html += "    .btn-danger:hover { background-color: #c0392b; }\n";
  html += "    table { width: 100%; border-collapse: collapse; }\n";
  html += "    th, td { text-align: left; padding: 8px; border: 1px solid #ddd; }\n";
  html += "    th { background-color: #f2f2f2; }\n";
  html += "    .message { margin-bottom: 20px; padding: 10px; border-radius: 4px; }\n";
  html += "    .message-ok { background-color: #d4edda; border: 1px solid #c3e6cb; color: #155724; }\n";
  html += "    .message-err { background-color: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; }\n";
  html += "  </style>\n";
  html += "</head>\n";

  html += "<body>\n";
  html += "<div class='container'>\n";
  html += "<h1>LabSupplyLog - SPIFFS Manager</h1>\n";

  if (msg.length() > 0) {
    String cls = "message-ok";
    if (msg.indexOf("Error") >= 0 || msg.indexOf("failed") >= 0) cls = "message-err";
    html += "<div class='message " + cls + "'>" + msg + "</div>\n";
  }

  if (fileTableWeb.length() > 0) {
    html += "<div class='section section-upload'>\n";
    html += "<h2>Upload</h2>\n";
    html += "<p>Allowed extensions: html, css, js, png, ico, svg, csv</p>\n";
    html += "<form method='POST' action='/upload' enctype='multipart/form-data'>\n";
    html += "  <input type='file' name='upload'>\n";
    html += "  <input type='submit' value='Upload' class='btn' style='width: 120px;'>\n";
    html += "</form>\n";
    html += "</div>\n";

    html += "<div class='section section-web'>\n";
    html += "<h2>Web Files</h2>\n";
    html += fileTableWeb;
    html += "</div>\n";
  }

  html += "<div class='section section-logs'>\n";
  html += "<h2>Log Files</h2>\n";
  html += "<form method='POST' action='/api/logs/delete_all' onsubmit=\"return confirm('Delete ALL logs?');\">\n";
  html += "  <input type='submit' value='Delete All Logs' class='btn btn-danger' style='width: 160px;'>\n";
  html += "</form>\n";
  html += fileTableLogs;
  html += "</div>\n";

  html += "</div>\n";

  html += "<div style='margin-top: 22px; text-align:center; color:#777; font-size:12px;'>Author: Adrian YO3HJV - March 2026</div>\n";

  html += "</body>\n";
  html += "</html>\n";

  return html;
}
