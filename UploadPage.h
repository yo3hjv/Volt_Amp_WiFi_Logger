// Upload/Logs page HTML generator.

#pragma once

#include <Arduino.h>

String buildUploadPageHtml(const String& msg, const String& fileTableWeb, const String& fileTableLogs);
