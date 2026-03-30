// INA219 manager.
//
// Responsibilities:
// - Initialize the INA219 over I2C.
// - Periodically sample bus voltage and current.
// - Maintain a simple IIR average for smoother UI values.
// - Apply calibration parameters (gain/offset) to produce calibrated V/A.
// - Integrate energy in mWh while logging is active.

#include <Wire.h>
#include <Adafruit_INA219.h>

#include "Globals.h"
#include "InaMgr.h"

static Adafruit_INA219 s_ina219;
static const uint32_t SAMPLE_MS = 50;

void initIna() {
  Wire.begin();
  g_inaOk = s_ina219.begin();
  g_lastInaSampleMs = millis();

  g_busV_avg = 0.0f;
  g_currentA_avg = 0.0f;
  g_lastEnergyMs = millis();
}

static void updateEnergy() {
  uint32_t now = millis();
  uint32_t dtMs = now - g_lastEnergyMs;
  if (dtMs == 0) return;
  g_lastEnergyMs = now;

  float dt_s = (float)dtMs / 1000.0f;
  if (g_loggingActive) {
    g_energy_mWh += (double)g_power_mW * (double)dt_s / 3600.0;
  }
}

void tickIna() {
  if (!g_inaOk) return;

  uint32_t now = millis();
  if (now - g_lastInaSampleMs < SAMPLE_MS) {
    updateEnergy();
    return;
  }
  g_lastInaSampleMs = now;

  float busV = s_ina219.getBusVoltage_V();
  float shuntmV = s_ina219.getShuntVoltage_mV();
  float currentmA = s_ina219.getCurrent_mA();

  g_busV_raw = busV;
  g_shuntmV_raw = shuntmV;
  g_currentA_raw = currentmA / 1000.0f;

  // Simple IIR average for UI smoothness
  const float alpha = 0.2f;
  g_busV_avg = (1.0f - alpha) * g_busV_avg + alpha * g_busV_raw;
  g_currentA_avg = (1.0f - alpha) * g_currentA_avg + alpha * g_currentA_raw;

  g_voltageV = g_busV_avg * g_vGain + g_vOffset_V;
  g_currentA = g_currentA_avg * g_iGain + g_iOffset_A;

  g_power_mW = g_voltageV * g_currentA * 1000.0f;

  updateEnergy();
}
