// Vektor_DAC.ino
// Vektorgrafik im X-Y-Betrieb: der ESP32 faehrt eine Punktliste ab.
// ESP32 DevKit (ESP-WROOM-32), interne 8-Bit-DACs
//
//   D25 (GPIO25, DAC1) -> Fluke 199C Eingang A (vertikal,   Y)
//   D26 (GPIO26, DAC2) -> Fluke 199C Eingang B (horizontal, X)
//   GND                -> Masseklemme A und B
//
// Board in der Arduino IDE: "ESP32 Dev Module"

#include <Arduino.h>

// ---------------- Einstellungen ----------------
const uint32_t SAMPLE_US    = 100;   // Abtastintervall -> 10 kHz
const float    STEP         = 3.0f;  // Schrittweite je Abtastwert in DAC-Einheiten
                                     // kleiner = dichtere Linie, aber laengere Bildzeit
const uint8_t  CORNER_DWELL = 2;     // zusaetzliche Abtastwerte je Eckpunkt
// -----------------------------------------------

const uint8_t PIN_DAC_Y = 25;
const uint8_t PIN_DAC_X = 26;

struct Point { uint8_t x, y; };      // 0..255, Ursprung links unten

// "Haus vom Nikolaus": alle 8 Linien in einem Zug, ohne Absetzen.
// Nach dem letzten Punkt geht es zurueck zum ersten (Bodenlinie doppelt).
const Point PATH[] = {
  { 60,  20},   // unten links
  { 60, 156},   // oben links
  {128, 236},   // Dachspitze
  {196, 156},   // oben rechts
  { 60, 156},   // oben links
  {196,  20},   // unten rechts
  {196, 156},   // oben rechts
  { 60,  20},   // unten links
  {196,  20},   // unten rechts
};
const size_t PATH_LEN = sizeof(PATH) / sizeof(PATH[0]);

size_t   seg     = 0;      // aktuelle Linie: PATH[seg] -> PATH[(seg + 1) % PATH_LEN]
int      stepIdx = 0;
int      stepCnt = 1;
float    posX = 0, posY = 0, dX = 0, dY = 0;
uint8_t  dwell   = 0;
uint32_t nextSample = 0;

void startSegment() {
  const Point &p0 = PATH[seg];
  const Point &p1 = PATH[(seg + 1) % PATH_LEN];
  float lx  = (float)p1.x - (float)p0.x;
  float ly  = (float)p1.y - (float)p0.y;
  float len = sqrtf(lx * lx + ly * ly);

  stepCnt = (int)lroundf(len / STEP);
  if (stepCnt < 1) stepCnt = 1;

  dX = lx / stepCnt;
  dY = ly / stepCnt;
  posX = p0.x;
  posY = p0.y;
  stepIdx = 0;
  dwell = CORNER_DWELL;
}

void setup() {
  startSegment();
  nextSample = micros();
}

void loop() {
  // auf den naechsten Abtastzeitpunkt warten
  while ((int32_t)(micros() - nextSample) < 0) { }
  nextSample += SAMPLE_US;

  dacWrite(PIN_DAC_Y, (uint8_t)lroundf(posY));
  dacWrite(PIN_DAC_X, (uint8_t)lroundf(posX));

  if (dwell > 0) {          // am Eckpunkt kurz verweilen
    dwell--;
    return;
  }

  posX += dX;
  posY += dY;
  if (++stepIdx >= stepCnt) {
    seg = (seg + 1) % PATH_LEN;
    startSegment();
  }
}
