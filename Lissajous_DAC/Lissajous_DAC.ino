// Lissajous_DAC.ino
// ESP32 DevKit (ESP-WROOM-32), interne 8-Bit-DACs
//
//   D25 (GPIO25, DAC1) -> Fluke 199C Eingang A (vertikal,   Y)
//   D26 (GPIO26, DAC2) -> Fluke 199C Eingang B (horizontal, X)
//   GND                -> Masseklemme A und B
//
// Board in der Arduino IDE: "ESP32 Dev Module"

#include <Arduino.h>

// ---------------- Einstellungen ----------------
const float FREQ_A_HZ    = 300.2f;  // Frequenz Kanal A
const float FREQ_B_HZ    = 200.0f;  // Frequenz Kanal B
const float PHASE_B_DEG  = 0.0f;   // Phasenverschiebung B gegen A (90 = Kreis)
const uint8_t AMPLITUDE  = 120;     // 0..127, Aussteuerung um die Mitte (127)
// -----------------------------------------------

const uint8_t  PIN_DAC_A   = 25;
const uint8_t  PIN_DAC_B   = 26;
const uint32_t SAMPLE_US   = 100;                    // Abtastintervall -> 10 kHz
const float    SAMPLE_RATE = 1000000.0f / SAMPLE_US;

uint8_t  sineTable[256];
uint32_t phaseA = 0, phaseB = 0;   // 32-Bit-Phasenakkumulatoren (DDS)
uint32_t stepA  = 0, stepB  = 0;
uint32_t nextSample = 0;

void setup() {
  for (int i = 0; i < 256; i++) {
    sineTable[i] = (uint8_t)lroundf(127.5f + AMPLITUDE * sinf(2.0f * PI * i / 256.0f));
  }

  stepA  = (uint32_t)((double)FREQ_A_HZ / SAMPLE_RATE * 4294967296.0);
  stepB  = (uint32_t)((double)FREQ_B_HZ / SAMPLE_RATE * 4294967296.0);
  phaseB = (uint32_t)((double)PHASE_B_DEG / 360.0 * 4294967296.0);

  nextSample = micros();
}

void loop() {
  // auf den naechsten Abtastzeitpunkt warten
  while ((int32_t)(micros() - nextSample) < 0) { }
  nextSample += SAMPLE_US;

  dacWrite(PIN_DAC_A, sineTable[phaseA >> 24]);
  dacWrite(PIN_DAC_B, sineTable[phaseB >> 24]);

  phaseA += stepA;
  phaseB += stepB;
}
