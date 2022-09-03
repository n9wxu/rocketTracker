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

#include <TimeLib.h>
#include <TinyGPSPlus.h>
#include <stdio.h>
#include <string.h>

#include <APRS-Decoder.h>

#define VBATPIN A7

// radio parameters
#define TX_FREQUENCY 443.775F
#define BANDWIDTH 125.0F
#define SPREADING_FACTOR 9U
#define CODE_RATE 7
#define SYNC_WORD 0x12
#define PREAMBLE 8
#define TX_POWER 10

#define UPDATE_RATE_MS 5000

// this combination should be a rocket
#define SYMBOL "O"
#define OVERLAY "/"

SX1276 radio = new Module(LoRa_cs, LoRa_dio0, LoRa_rst, LoRa_dio1);
TinyGPSPlus gps;

char *s_min_nn(uint32_t min_nnnnn, int high_precision);
String create_lat_aprs(RawDegrees lat);
String create_long_aprs(RawDegrees lng);
String create_lat_aprs_dao(RawDegrees lat);
String create_long_aprs_dao(RawDegrees lng);
String create_dao_aprs(RawDegrees lat, RawDegrees lng);
String createDateString(time_t t);
String createTimeString(time_t t);
String padding(unsigned int number, unsigned int width);
float batteryVoltage();

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
    digitalWrite(LED_BUILTIN, HIGH);
    APRSMessage msg;
    String lat;
    String lng;
    String alt = "";

    msg.setSource(CALLSIGN);
    msg.setDestination("APLT00");
    lat = create_lat_aprs(gps.location.rawLat());
    lng = create_long_aprs(gps.location.rawLng());

    int alt_int = max(-99999, min(999999, (int)gps.altitude.feet()));
    if (alt_int < 0) {
      alt = "/A=-" + padding(alt_int * -1, 5);
    } else {
      alt = "/A=" + padding(alt_int, 6);
    }

    String course_and_speed = "";
    int speed_int = max(0, min(999, (int)gps.speed.knots()));
    String speed = padding(speed_int, 3);
    int course_int = max(0, min(360, (int)gps.course.deg()));
    /* course in between 1..360 due to aprs spec */
    if (course_int == 0) {
      course_int = 360;
    }
    String course = padding(course_int, 3);
    course_and_speed = course + "/" + speed;

    String aprsmsg;
    aprsmsg = "!" + lat + OVERLAY + lng + SYMBOL + course_and_speed + alt;

    aprsmsg += " -  _Bat.: " + String(batteryVoltage(), 2) +
               "V - Cur.: " + "0" + " mA ";

    msg.getAPRSBody()->setData(aprsmsg);
    // msg.getBody()->setData(aprsmsg);
    const char header[] = {'<', 0xFF, 0x01, 0};
    String data = String(header) + String(msg.encode());

    Serial.println(data);
    radio.transmit(data);

    digitalWrite(LED_BUILTIN, LOW);
  }

  if (gps.satellites.isValid()) {
    Serial.println("Sat Count : " + String(gps.satellites.value()));
  }
}

String create_lat_aprs(RawDegrees lat) {
  char str[20];
  char n_s = 'N';
  if (lat.negative) {
    n_s = 'S';
  }
  // we like sprintf's float up-rounding.
  // but sprintf % may round to 60.00 -> 5360.00 (53° 60min is a wrong
  // notation
  // ;)
  sprintf(str, "%02d%s%c", lat.deg, s_min_nn(lat.billionths, 0), n_s);
  String lat_str(str);
  return lat_str;
}

String create_lat_aprs_dao(RawDegrees lat) {
  // round to 4 digits and cut the last 2
  char str[20];
  char n_s = 'N';
  if (lat.negative) {
    n_s = 'S';
  }
  // we need sprintf's float up-rounding. Must be the same principle as in
  // aprs_dao(). We cut off the string to two decimals afterwards. but sprintf
  // % may round to 60.0000 -> 5360.0000 (53° 60min is a wrong notation ;)
  sprintf(str, "%02d%s%c", lat.deg,
          s_min_nn(lat.billionths, 1 /* high precision */), n_s);
  String lat_str(str);
  return lat_str;
}

String create_long_aprs(RawDegrees lng) {
  char str[20];
  char e_w = 'E';
  if (lng.negative) {
    e_w = 'W';
  }
  sprintf(str, "%03d%s%c", lng.deg, s_min_nn(lng.billionths, 0), e_w);
  String lng_str(str);
  return lng_str;
}

String create_long_aprs_dao(RawDegrees lng) {
  // round to 4 digits and cut the last 2
  char str[20];
  char e_w = 'E';
  if (lng.negative) {
    e_w = 'W';
  }
  sprintf(str, "%03d%s%c", lng.deg,
          s_min_nn(lng.billionths, 1 /* high precision */), e_w);
  String lng_str(str);
  return lng_str;
}

String create_dao_aprs(RawDegrees lat, RawDegrees lng) {
  // !DAO! extension, use Base91 format for best precision
  // /1.1 : scale from 0-99 to 0-90 for base91, int(... + 0.5): round to
  // nearest integer https://metacpan.org/dist/Ham-APRS-FAP/source/FAP.pm
  // http://www.aprs.org/aprs12/datum.txt
  //

  char str[10];
  sprintf(str, "!w%s", s_min_nn(lat.billionths, 2));
  sprintf(str + 3, "%s!", s_min_nn(lng.billionths, 2));
  String dao_str(str);
  return dao_str;
}

String createDateString(time_t t) {
  return String(padding(day(t), 2) + "." + padding(month(t), 2) + "." +
                padding(year(t), 4));
}

String createTimeString(time_t t) {
  return String(padding(hour(t), 2) + ":" + padding(minute(t), 2) + ":" +
                padding(second(t), 2));
}

String padding(unsigned int number, unsigned int width) {
  String result;
  String num(number);
  if (num.length() > width) {
    width = num.length();
  }
  for (unsigned int i = 0; i < width - num.length(); i++) {
    result.concat('0');
  }
  result.concat(num);
  return result;
}

char *s_min_nn(uint32_t min_nnnnn, int high_precision) {
  /* min_nnnnn: RawDegrees billionths is uint32_t by definition and is n'telth
   * degree (-> *= 6 -> nn.mmmmmm minutes) high_precision: 0: round at decimal
   * position 2. 1: round at decimal position 4. 2: return decimal position
   * 3-4 as base91 encoded char
   */

  static char buf[6];
  min_nnnnn = min_nnnnn * 0.006;

  if (high_precision) {
    if ((min_nnnnn % 10) >= 5 && min_nnnnn < 6000000 - 5) {
      // round up. Avoid overflow (59.999999 should never become 60.0 or more)
      min_nnnnn = min_nnnnn + 5;
    }
  } else {
    if ((min_nnnnn % 1000) >= 500 && min_nnnnn < (6000000 - 500)) {
      // round up. Avoid overflow (59.9999 should never become 60.0 or more)
      min_nnnnn = min_nnnnn + 500;
    }
  }

  if (high_precision < 2)
    sprintf(buf, "%02u.%02u", (unsigned int)((min_nnnnn / 100000) % 100),
            (unsigned int)((min_nnnnn / 1000) % 100));
  else
    sprintf(buf, "%c", (char)((min_nnnnn % 1000) / 11) + 33);
  // Like to verify? type in python for i.e. RawDegrees billions 566688333: i
  // = 566688333; "%c" % (int(((i*.0006+0.5) % 100)/1.1) +33)
  return buf;
}

// the battery voltage is only suitable if a resistor divider is added
float batteryVoltage() {
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  return measuredvbat;
}
