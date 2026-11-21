// Text_DAC.ino
// Text als Vektorgrafik im X-Y-Betrieb (Strichschrift).
// ESP32 DevKit (ESP-WROOM-32), interne 8-Bit-DACs
//
//   D25 (GPIO25, DAC1) -> Fluke 199C Eingang A (vertikal,   Y)
//   D26 (GPIO26, DAC2) -> Fluke 199C Eingang B (horizontal, X)
//   GND                -> Masseklemme A und B
//
// Board in der Arduino IDE: "ESP32 Dev Module"

#include <Arduino.h>

// ---------------- Einstellungen ----------------
const char     TEXT[]       = "Hallo Joe"; // A-Z, a-z, 0-9, Leerzeichen und . - ! : / +
const uint32_t SAMPLE_US    = 100;   // Abtastintervall -> 10 kHz
const float    STEP         = 2.0f;  // Schrittweite je Abtastwert in DAC-Einheiten
const uint8_t  CORNER_DWELL = 2;     // zusaetzliche Abtastwerte je Eckpunkt
const float    MAX_SCALE    = 16.0f; // groesste Zeichengroesse (DAC-Einheiten je Rasterpunkt)
// -----------------------------------------------

const uint8_t PIN_DAC_Y = 25;
const uint8_t PIN_DAC_X = 26;

// ---------------- Strichschrift ----------------
// Raster je Zeichen: x = 0..4, y = 0..8
//   y = 2 Grundlinie, y = 6 Hoehe der Kleinbuchstaben, y = 8 Hoehe der Grossbuchstaben,
//   y = 0 Unterlaenge (g, j, p, q, y)
// Kodierung: je Punkt zwei Ziffern "xy", '|' = absetzen (Sprung zum naechsten Punkt)
const uint8_t GLYPH_W      = 4;
const uint8_t GLYPH_H      = 8;
const uint8_t CHAR_ADVANCE = 6;      // Zeichenbreite + Abstand

const char *const FONT_UPPER[26] = {
  "022842|1535",                        // A
  "02083847463505|3544433202",          // B
  "4738180703123243",                   // C
  "02082846442202",                     // D
  "42020848|0535",                      // E
  "020848|0535",                        // F
  "47381807031232434525",               // G
  "0208|4248|0545",                     // H
  "1232|2228|1838",                     // I
  "4843321203",                         // J
  "0208|480542",                        // K
  "080242",                             // L
  "0208254842",                         // M
  "02084248",                           // N
  "120307183847433212",                 // O
  "02083847463505",                     // P
  "120307183847433212|2442",            // Q
  "02083847463505|2542",                // R
  "473818070615354443321203",           // S
  "0848|2822",                          // T
  "080312324348",                       // U
  "082248",                             // V
  "0812253248",                         // W
  "0248|0842",                          // X
  "082548|2522",                        // Y
  "08480242",                           // Z
};

const char *const FONT_LOWER[26] = {
  "4642|4536160503123243",              // a
  "0802|0312324345361605",              // b
  "4536160503123243",                   // c
  "4842|4332120305163645",              // d
  "04444536160503123243",               // e
  "38281712|0636",                      // f
  "4332120305163645|4641301001",        // g
  "0802|0516364542",                    // h
  "2622|2828",                          // i
  "26211000|2828",                      // j
  "0802|460442",                        // k
  "2822",                               // l
  "0602|05162522|25364542",             // m
  "0602|0516364542",                    // n
  "120305163645433212",                 // o
  "0600|0516364543321203",              // p
  "4640|4536160503123243",              // q
  "0602|051636",                        // r
  "45361605143443321203",               // s
  "18132232|0636",                      // t
  "0603123243|4642",                    // u
  "062246",                             // v
  "0612243246",                         // w
  "0246|0642",                          // x
  "0622|46221000",                      // y
  "06460242",                           // z
};

const char *const FONT_DIGIT[10] = {
  "120307183847433212|0347",            // 0
  "162822|1232",                        // 1
  "07183847460242",                     // 2
  "07183847463525|354443321203",        // 3
  "32380444",                           // 4
  "480805354443321203",                 // 5
  "4738180703123243443505",             // 6
  "084822",                             // 7
  "15060718384746351504031232434435",   // 8
  "0312324347381807061545",             // 9
};

const char *glyphFor(char c) {
  if (c >= 'A' && c <= 'Z') return FONT_UPPER[c - 'A'];
  if (c >= 'a' && c <= 'z') return FONT_LOWER[c - 'a'];
  if (c >= '0' && c <= '9') return FONT_DIGIT[c - '0'];
  switch (c) {
    case '.': return "2222";
    case '-': return "0545";
    case '!': return "2824|2222";
    case ':': return "2525|2323";
    case '/': return "0248";
    case '+': return "0545|2327";
    default:  return nullptr;           // Leerzeichen und unbekannte Zeichen
  }
}

// ---------------- Pfad ----------------
struct Vertex {
  uint8_t x, y;
  bool    draw;   // true: Linie vom vorherigen Punkt hierher, false: Sprung
};

const size_t MAX_VERTS = 800;
Vertex verts[MAX_VERTS];
size_t vertCount = 0;

void buildPath() {
  const size_t n = strlen(TEXT);
  vertCount = 0;

  if (n > 0) {
    const float wUnits = (float)n * CHAR_ADVANCE - (CHAR_ADVANCE - GLYPH_W);
    float scale = 235.0f / wUnits;                         // nutzbarer Bereich 10..245
    if (scale > 235.0f / GLYPH_H) scale = 235.0f / GLYPH_H;
    if (scale > MAX_SCALE)        scale = MAX_SCALE;

    const float x0 = 127.5f - wUnits * scale / 2.0f;
    const float y0 = 127.5f - GLYPH_H * scale / 2.0f;

    for (size_t i = 0; i < n; i++) {
      const char *g = glyphFor(TEXT[i]);
      if (g == nullptr) continue;

      bool penUp = true;
      for (const char *p = g; p[0] != '\0'; ) {
        if (p[0] == '|') { penUp = true; p++; continue; }
        if (p[1] == '\0') break;
        const float gx = (float)(p[0] - '0');
        const float gy = (float)(p[1] - '0');
        p += 2;
        if (vertCount < MAX_VERTS) {
          verts[vertCount].x    = (uint8_t)lroundf(x0 + ((float)i * CHAR_ADVANCE + gx) * scale);
          verts[vertCount].y    = (uint8_t)lroundf(y0 + gy * scale);
          verts[vertCount].draw = !penUp;
          vertCount++;
        }
        penUp = false;
      }
    }
  }

  if (vertCount == 0) {                 // leerer Text: Punkt in der Bildmitte
    verts[0].x = 128; verts[0].y = 128; verts[0].draw = false;
    vertCount = 1;
  }
}

// ---------------- Ausgabe ----------------
size_t   seg     = 0;      // aktuelle Strecke: verts[seg] -> verts[(seg + 1) % vertCount]
int      stepIdx = 0;
int      stepCnt = 1;
float    posX = 0, posY = 0, dX = 0, dY = 0;
uint8_t  dwell   = 0;
uint32_t nextSample   = 0;
uint32_t frameSamples = 0;
bool     frameReported = false;

void startSegment() {
  const Vertex &p0 = verts[seg];
  const Vertex &p1 = verts[(seg + 1) % vertCount];
  const float lx = (float)p1.x - (float)p0.x;
  const float ly = (float)p1.y - (float)p0.y;

  if (p1.draw) {                        // Linie: in kleinen Schritten abfahren
    stepCnt = (int)lroundf(sqrtf(lx * lx + ly * ly) / STEP);
    if (stepCnt < 1) stepCnt = 1;
  } else {                              // Sprung: ein einziger Schritt
    stepCnt = 1;
  }

  dX = lx / stepCnt;
  dY = ly / stepCnt;
  posX = p0.x;
  posY = p0.y;
  stepIdx = 0;
  dwell = CORNER_DWELL;
}

void setup() {
  Serial.begin(115200);
  buildPath();
  startSegment();
  nextSample = micros();
}

void loop() {
  // auf den naechsten Abtastzeitpunkt warten
  while ((int32_t)(micros() - nextSample) < 0) { }
  nextSample += SAMPLE_US;

  dacWrite(PIN_DAC_Y, (uint8_t)lroundf(posY));
  dacWrite(PIN_DAC_X, (uint8_t)lroundf(posX));
  frameSamples++;

  if (dwell > 0) {                      // am Eckpunkt kurz verweilen
    dwell--;
    return;
  }

  posX += dX;
  posY += dY;
  if (++stepIdx >= stepCnt) {
    seg = (seg + 1) % vertCount;
    if (seg == 0) {                     // Bild komplett
      if (!frameReported) {
        Serial.printf("Bildzeit: %.1f ms\n", frameSamples * SAMPLE_US / 1000.0f);
        frameReported = true;
      }
      frameSamples = 0;
    }
    startSegment();
  }
}
