// Storage manager.
//
// Responsibilities:
// - Initialize SPIFFS at boot.
// - Set global flags indicating storage availability.

#include <SPIFFS.h>
#include "Globals.h"
#include "StorageMgr.h"

void initStorage() {
  if (SPIFFS.begin(true)) {
    g_spiffsOk = true;
  } else {
    g_spiffsOk = false;
  }
}
