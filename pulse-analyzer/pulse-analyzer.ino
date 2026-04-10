#include <SPI.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

const byte GDO0_PIN = 2;                 // CC1101 GDO0 -> Arduino D2
const uint16_t MAX_PULSES = 180;         // keep small for Uno SRAM
const uint16_t MIN_PULSE_US = 200;        // ignore very short noise
const uint32_t FRAME_GAP_US = 12000;      // gap that marks end of frame

volatile int16_t pulses[MAX_PULSES];     // + = HIGH pulse, - = LOW pulse
volatile uint16_t pulseCount = 0;
volatile uint32_t lastEdgeUs = 0;
volatile bool started = false;
volatile bool frameReady = false;

void onEdge() {
  uint32_t now = micros();
  uint8_t levelNow = digitalRead(GDO0_PIN);

  if (!started) {
    started = true;
    lastEdgeUs = now;
    return;
  }

  uint32_t dt = now - lastEdgeUs;
  lastEdgeUs = now;

  if (frameReady) return;
  if (dt < MIN_PULSE_US) return;

  if (dt > FRAME_GAP_US) {
    if (pulseCount > 0) frameReady = true;
    return;
  }

  if (pulseCount < MAX_PULSES) {
    uint16_t clipped = (dt > 32767UL) ? 32767 : (uint16_t)dt;

    // levelNow is the NEW level after the edge,
    // so the level that just ended is the opposite.
    pulses[pulseCount++] = levelNow ? -(int16_t)clipped : (int16_t)clipped;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(GDO0_PIN, INPUT);

  if (!ELECHOUSE_cc1101.getCC1101()) {
    Serial.println(F("CC1101 not found"));
    while (1) {}
  }

  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setCCMode(0);         // raw/direct style
  ELECHOUSE_cc1101.setModulation(2);     // 2 = ASK/OOK
  ELECHOUSE_cc1101.setMHZ(433.92);
  ELECHOUSE_cc1101.setRxBW(325);

  // Raw async data on GDO0
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_IOCFG0, 0x0D);
  ELECHOUSE_cc1101.SpiWriteReg(CC1101_PKTCTRL0, 0x32);

  ELECHOUSE_cc1101.SetRx();
  attachInterrupt(digitalPinToInterrupt(GDO0_PIN), onEdge, CHANGE);

  Serial.println(F("Ready"));
  Serial.println(F("Press and hold remote button"));
  Serial.println(F("Format: H 320  /  L 980"));
  Serial.println();
}

void dumpFrame() {
  detachInterrupt(digitalPinToInterrupt(GDO0_PIN));

  uint16_t n = pulseCount;

  Serial.println(F("=== FRAME ==="));
  Serial.print(F("RSSI: "));
  Serial.println(ELECHOUSE_cc1101.getRssi());

  for (uint16_t i = 0; i < n; i++) {
    int16_t v = pulses[i];
    if (v >= 0) {
      Serial.print(F("H "));
      Serial.println(v);
    } else {
      Serial.print(F("L "));
      Serial.println(-v);
    }
  }

  Serial.println();

  pulseCount = 0;
  frameReady = false;
  started = false;

  ELECHOUSE_cc1101.SetRx();
  attachInterrupt(digitalPinToInterrupt(GDO0_PIN), onEdge, CHANGE);
}

void loop() {
  noInterrupts();
  bool ready = frameReady;
  uint16_t n = pulseCount;
  uint32_t last = lastEdgeUs;
  interrupts();

  if (!ready && n > 0 && (micros() - last) > FRAME_GAP_US) {
    noInterrupts();
    frameReady = true;
    interrupts();
    ready = true;
  }

  if (ready) {
    dumpFrame();
  }
}