/*
 * RS485 RAW WIRE TEST v3 -- no eModbus, no WiFi, no bridge logic at all.
 *
 * Use this right after switching to the OTHER A/B terminal pair on the
 * UNI3 PRO controller, to see whether a real response now comes back.
 */
#include <Arduino.h>

#define RS485_TX_PIN 17
#define RS485_RX_PIN 18
#define RS485_EN_PIN 21

const uint32_t RTU_BAUD = 9600;
const uint32_t RTU_CONFIG = SERIAL_8N1;
const uint8_t SLAVE_ID = 1;
const uint32_t LISTEN_WINDOW_MS = 500;
const uint32_t REQUEST_INTERVAL_MS = 3000;

uint16_t modbusCRC16(const uint8_t *buf, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buf[pos];
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) { crc >>= 1; crc ^= 0xA001; }
      else { crc >>= 1; }
    }
  }
  return crc;
}

void printHex(const uint8_t *buf, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (buf[i] < 0x10) Serial.print('0');
    Serial.print(buf[i], HEX);
    Serial.print(' ');
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println("=== RS485 RAW WIRE TEST v3 (confirmed-good pins/polarity) ===");
  Serial.printf("Pins: TX=%d RX=%d EN=%d | Baud=%lu 8N1 | Slave ID=%u\n",
                 RS485_TX_PIN, RS485_RX_PIN, RS485_EN_PIN, (unsigned long)RTU_BAUD, SLAVE_ID);
  Serial.println();

  pinMode(RS485_EN_PIN, OUTPUT);
  digitalWrite(RS485_EN_PIN, LOW);

  Serial1.begin(RTU_BAUD, RTU_CONFIG, RS485_RX_PIN, RS485_TX_PIN);
  delay(100);
}

void loop() {
  static uint32_t lastRequest = 0;
  if (millis() - lastRequest < REQUEST_INTERVAL_MS) return;
  lastRequest = millis();

  uint8_t frame[8];
  frame[0] = SLAVE_ID;
  frame[1] = 0x04;
  frame[2] = 0x00;
  frame[3] = 0x00;
  frame[4] = 0x00;
  frame[5] = 0x01;
  uint16_t crc = modbusCRC16(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = (crc >> 8) & 0xFF;

  while (Serial1.available()) Serial1.read();

  Serial.print("[");
  Serial.print(millis());
  Serial.print(" ms] Sending: ");
  printHex(frame, sizeof(frame));
  Serial.println();

  digitalWrite(RS485_EN_PIN, HIGH);
  delayMicroseconds(50);
  Serial1.write(frame, sizeof(frame));
  Serial1.flush();
  delayMicroseconds(200);
  digitalWrite(RS485_EN_PIN, LOW);

  uint32_t listenStart = millis();
  size_t totalBytes = 0;
  bool printedAny = false;
  while (millis() - listenStart < LISTEN_WINDOW_MS) {
    if (Serial1.available()) {
      uint8_t b = Serial1.read();
      if (!printedAny) { Serial.print("  <-- RECEIVED: "); printedAny = true; }
      if (b < 0x10) Serial.print('0');
      Serial.print(b, HEX);
      Serial.print(' ');
      totalBytes++;
    }
  }

  if (totalBytes == 0) {
    Serial.println("  <-- NOTHING received in 500ms window.");
  } else {
    Serial.println();
    Serial.printf("  *** %u BYTE(S) RECEIVED! ***\n", (unsigned)totalBytes);
  }
  Serial.println();
}
