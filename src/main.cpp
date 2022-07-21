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
#include <stdio.h>
#include <string.h>

// important parameters for AFSK
#define TX_FREQUENCY 432.10
#define MORSE_SPEED 20
#define FREQ_DEVIATION 2.5F
#define TX_POWER 10

// not important to AFSK
#define BAUDRATE 4.8F
#define BANDWIDTH 125.0F
#define PREAMBLE 0

SX1276 radio = new Module(LoRa_cs, -1, LoRa_rst, -1);
AFSKClient audio(&radio, LoRa_dio2);
MorseClient morse(&audio);

void setup() {
  Serial.begin(9600);

  Serial.print(F("[SX1278] Initializing ... "));
  int state = radio.beginFSK(TX_FREQUENCY, BAUDRATE, FREQ_DEVIATION, BANDWIDTH,
                             TX_POWER, PREAMBLE, false);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }

  Serial.print(F("[Morse] Initializing ... "));
  state = morse.begin(TX_FREQUENCY, MORSE_SPEED);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }
}

void beep(int freq) {
  audio.tone(freq);
  delay(125);
  audio.tone(1);
  delay(125);
}

void loop() {
  radio.setOutputPower(17);
  Serial.print(F("[Morse] Sending Morse data ... "));

  morse.startSignal();
  morse.print("N9WXU Rocket Finder");

  delay(100);
  radio.setOutputPower(17);
  beep(800);
  radio.setOutputPower(12);
  beep(700);
  radio.setOutputPower(6);
  beep(600);
  radio.setOutputPower(3);
  beep(500);
  radio.setOutputPower(2);
  beep(400);
  delay(100);
  radio.setOutputPower(17);
  beep(800);
  radio.setOutputPower(12);
  beep(700);
  radio.setOutputPower(6);
  beep(600);
  radio.setOutputPower(3);
  beep(500);
  radio.setOutputPower(2);
  beep(400);

  radio.standby();
  delay(2000);
}
