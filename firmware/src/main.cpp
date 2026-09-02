/*
 * UNI PRO Modbus Bridge
 * ---------------------
 * Transparentes Modbus-TCP-zu-RTU-Gateway fuer Euroster UNI2 PRO / UNI3 PRO.
 *
 * Zielhardware: Waveshare ESP32-S3-RS485-CAN (Hutschienengehaeuse)
 *   RS485-Transceiver: SP3485EN, MANUELLE Richtungssteuerung
 *   Pinbelegung (laut Waveshare-Schaltplan, feldbestaetigt am UNI3 PRO):
 *     GPIO17 -> DI  (UART TX zur RS485-Sendeseite)
 *     GPIO18 <- RO  (UART RX von der RS485-Empfangsseite)
 *     GPIO21 -> DE/!RE (Richtungsumschaltung, HIGH = senden)
 *
 * Architektur (bewusst generisch gehalten):
 *   Die Firmware kennt KEINE UNI-PRO-Register. Sie nimmt Standard-Modbus-TCP
 *   entgegen (MBAP), leitet die Anfrage als Modbus-RTU-Frame auf den RS485-Bus
 *   weiter und gibt die RTU-Antwort als Modbus-TCP zurueck. Die gesamte
 *   Geraetelogik (Register, Skalierung, Entitaeten) liegt in der
 *   Home-Assistant-Integration "UNI PRO Modbus" (HACS) und kann dort zentral
 *   aktualisiert werden, ohne Kundengeraete neu zu flashen.
 *
 * Serielle Parameter zum Regler (fix laut Euroster-Modbus-Handbuch 14/05/2026):
 *   9600 Baud, 8 Datenbits, keine Paritaet, 1 Stoppbit
 *   Slave-ID Werkseinstellung: 1
 *   Antwort-Timeout des Reglers: mindestens 500 ms einplanen
 *
 * WLAN-Einrichtung beim Kunden:
 *   Beim ersten Start (oder wenn das gespeicherte WLAN nicht erreichbar ist)
 *   oeffnet die Bridge einen Einrichtungs-Hotspot "UNI-PRO-BRIDGE"
 *   (Passwort: unipro-setup). Nach Verbindung oeffnet sich das
 *   Konfigurationsportal (sonst: http://192.168.4.1), dort WLAN auswaehlen
 *   und Zugangsdaten eingeben. Danach ist die Bridge im Kundennetz unter
 *   "uni-pro-bridge.local", Port 502 erreichbar.
 *
 * WLAN-Zugangsdaten loeschen (Werksreset der Bridge):
 *   BOOT-Taste (GPIO0) im laufenden Betrieb ca. 5 Sekunden gedrueckt halten.
 *
 * Lizenz: MIT (siehe LICENSE im Integrations-Repo)
 * Version: 1.0.0
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>      // tzapu/WiFiManager
#include <ESPmDNS.h>
#include "ModbusClientRTU.h"  // eModbus
#include "ModbusBridgeWiFi.h" // eModbus

// ---------------------------------------------------------------------------
// Konfiguration
// ---------------------------------------------------------------------------
static constexpr int8_t   PIN_RS485_TX   = 17;   // -> DI (SP3485EN)
static constexpr int8_t   PIN_RS485_RX   = 18;   // <- RO (SP3485EN)
static constexpr int8_t   PIN_RS485_DE   = 21;   // -> DE/!RE (HIGH = senden)
static constexpr int8_t   PIN_BOOT_BTN   = 0;    // BOOT-Taste (LOW = gedrueckt)

static constexpr uint32_t RS485_BAUD     = 9600; // UNI PRO: fix 9600 8N1
static constexpr uint16_t MODBUS_TCP_PORT = 502;
static constexpr uint8_t  UNI_SLAVE_ID   = 1;    // Werkseinstellung UNI PRO
static constexpr uint32_t RTU_TIMEOUT_MS = 2000; // Handbuch fordert >= 500 ms

static const char* MDNS_HOSTNAME = "uni-pro-bridge"; // -> uni-pro-bridge.local
static const char* AP_NAME       = "UNI-PRO-BRIDGE";
static const char* AP_PASSWORD   = "unipro-setup";

// ---------------------------------------------------------------------------
// Globale Objekte
// ---------------------------------------------------------------------------
// ModbusClientRTU uebernimmt mit dem RTS-Pin die DE/!RE-Umschaltung selbst.
ModbusClientRTU   rtuClient(PIN_RS485_DE);
ModbusBridgeWiFi  tcpBridge;

// ---------------------------------------------------------------------------
// Hilfsfunktionen
// ---------------------------------------------------------------------------
static void startWifiPortalIfNeeded() {
  WiFiManager wm;
  wm.setHostname(MDNS_HOSTNAME);
  // Portal nach 5 Minuten schliessen und neu versuchen, damit die Bridge
  // nach einem Stromausfall + spaeter zurueckkehrendem WLAN von selbst
  // wieder online kommt und nicht dauerhaft im Hotspot haengen bleibt.
  wm.setConfigPortalTimeout(300);
  wm.setConnectTimeout(20);

  if (!wm.autoConnect(AP_NAME, AP_PASSWORD)) {
    Serial.println(F("[WLAN] Keine Verbindung, Neustart..."));
    delay(1000);
    ESP.restart();
  }
}

static void eraseWifiAndRestart() {
  Serial.println(F("[WLAN] Zugangsdaten werden geloescht (Werksreset Bridge)"));
  WiFiManager wm;
  wm.resetSettings();
  delay(500);
  ESP.restart();
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("=== UNI PRO Modbus Bridge v1.0.0 ==="));

  pinMode(PIN_BOOT_BTN, INPUT_PULLUP);

  // --- WLAN (WiFiManager: Hotspot beim Erststart, sonst gespeichertes Netz)
  startWifiPortalIfNeeded();
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  Serial.print(F("[WLAN] Verbunden, IP: "));
  Serial.println(WiFi.localIP());

  // --- mDNS: Bridge im Netz als uni-pro-bridge.local auffindbar
  if (MDNS.begin(MDNS_HOSTNAME)) {
    MDNS.addService("modbus", "tcp", MODBUS_TCP_PORT);
    Serial.printf("[mDNS] %s.local aktiv\n", MDNS_HOSTNAME);
  } else {
    Serial.println(F("[mDNS] Start fehlgeschlagen (nicht kritisch)"));
  }

  // --- RS485 / Modbus RTU (Master zum UNI PRO)
  RTUutils::prepareHardwareSerial(Serial1);
  Serial1.begin(RS485_BAUD, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
  rtuClient.setTimeout(RTU_TIMEOUT_MS);
  rtuClient.begin(Serial1);

  // --- Modbus-TCP-Server + Bruecke auf RTU
  // Anfragen an Unit-ID 1 (TCP) werden 1:1 an Slave-ID 1 (RTU) weitergereicht,
  // alle Funktionscodes erlaubt (03/04/06/16 nutzt die Integration).
  tcpBridge.attachServer(UNI_SLAVE_ID, UNI_SLAVE_ID, ANY_FUNCTION_CODE, &rtuClient);
  tcpBridge.start(MODBUS_TCP_PORT, 2 /*max. Clients*/, 20000 /*TCP-Idle-Timeout ms*/);

  Serial.printf("[Modbus] TCP-Bruecke auf Port %u aktiv, RTU %lu Baud 8N1, Slave-ID %u\n",
                MODBUS_TCP_PORT, (unsigned long)RS485_BAUD, UNI_SLAVE_ID);
  Serial.println(F("[Hinweis] A/B zum Regler GEKREUZT anschliessen (siehe Anleitung)."));
}

// ---------------------------------------------------------------------------
// Loop: WLAN-Watchdog + BOOT-Taste (WLAN-Reset)
// ---------------------------------------------------------------------------
void loop() {
  // WLAN-Watchdog: nach 60 s ohne Verbindung Neustart (autoConnect greift dann
  // erneut; ist das Netz weiter weg, oeffnet sich wieder der Hotspot).
  static uint32_t wifiLostSince = 0;
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiLostSince == 0) {
      wifiLostSince = millis();
      Serial.println(F("[WLAN] Verbindung verloren, warte auf Reconnect..."));
    } else if (millis() - wifiLostSince > 60000UL) {
      Serial.println(F("[WLAN] 60 s offline, Neustart"));
      ESP.restart();
    }
  } else {
    wifiLostSince = 0;
  }

  // BOOT-Taste 5 s halten -> WLAN-Zugangsdaten loeschen
  static uint32_t btnDownSince = 0;
  if (digitalRead(PIN_BOOT_BTN) == LOW) {
    if (btnDownSince == 0) {
      btnDownSince = millis();
    } else if (millis() - btnDownSince > 5000UL) {
      eraseWifiAndRestart();
    }
  } else {
    btnDownSince = 0;
  }

  delay(100);
}
