/*
   RadioLib Morse Transmit AFSK Example

   This example sends Morse code message using
   SX1278's FSK modem. The data is modulated
   as AFSK.

   Other modules that can be used for Morse Code
   with AFSK modulation:
    - SX127x/RFM9x
    - RF69
    - SX1231
    - CC1101
    - Si443x/RFM2x

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

#include <RadioLib.h>

#include <TinyGPSPlus.h>
#include <stdio.h>
#include <string.h>

#include <APRS-Decoder.h>

// important parameters for AFSK
#define TX_FREQUENCY 443.775F
#define BANDWIDTH 125.0F
#define SPREADING_FACTOR 9U
#define CODE_RATE 7
#define SYNC_WORD 0x12
#define PREAMBLE 8
#define TX_POWER 10

#define UPDATE_RATE_MS 1000

SX1276 radio = new Module(LoRa_cs, LoRa_dio0, LoRa_rst, LoRa_dio1);
TinyGPSPlus gps;

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  Serial1.begin(9600);
  Serial.begin(9600);
  Serial.print(F("[RADIO] Begin ... "));
  int state = radio.begin(TX_FREQUENCY, BANDWIDTH, SPREADING_FACTOR, CODE_RATE,
                          SYNC_WORD, TX_POWER, PREAMBLE, 0);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.println(F("[SX1278] Unable to begin"));
    Serial.println(state);
  } else {
  }
}

void loop() {
  String data;
  bool sendUpdate = false;
  static uint32_t previousUpdate = 0;
  uint32_t now = millis();

  if (now - previousUpdate > UPDATE_RATE_MS) {
    sendUpdate = true;
    previousUpdate = now;
  }

  while (Serial1.available()) {
    gps.encode(Serial1.read());
  }

  if (gps.location.isUpdated() && sendUpdate) {

    //    APRSMessage msg;
    //    msg.setSource(CALLSIGN);

    digitalWrite(LED_BUILTIN, HIGH);
    String location = String(gps.location.lat()) + String(gps.location.lng());
    Serial.println("location");
    radio.transmit(String(CALLSIGN) + " " + location);
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (gps.satellites.isValid()) {
    Serial.println("Sat Count : " + String(gps.satellites.value()));
  }
}
