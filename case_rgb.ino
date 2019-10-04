#include "Arduino.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2

using namespace std;

const int LED_R = 5; // digital output pin for LED red channel
const int LED_G = 3; // digital output pin for LED green channel
const int LED_B = 6; // digital output pin for LED blue channel

int rgb[3];  // 0-255 RGB values for the LEDs

void getRGB(int hue, int sat, int val, int colors[3]);

const uint32_t T_COLD = 2000; // 20C for lower bound
const int HUE_COLD = 180; // cyan-ish hue
const int SAT_COLD = 255; // maximum color

const uint32_t T_HOT = 4000; // 40C for upper bound
const int HUE_HOT = 360; // red hue
const int SAT_HOT = 255; // maximum color

OneWire oneWire(ONE_WIRE_BUS); // initialize 1-wire bus

DallasTemperature sensors(&oneWire); // initialize DS18B20 on 1-wire bus

void setup() {
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  calibrationSequence();

  Serial.begin(9600);
  sensors.begin();
}

void calibrationSequence() {
  /*  do and RGB calibration sequence at startup so we can
      visually see that the LED strip is working correctly
  */
  analogWrite(LED_R, 255);
  analogWrite(LED_G, 0);
  analogWrite(LED_B, 0);
  delay(1000);
  analogWrite(LED_R, 0);
  analogWrite(LED_G, 255);
  analogWrite(LED_B, 0);
  delay(1000);
  analogWrite(LED_R, 0);
  analogWrite(LED_G, 0);
  analogWrite(LED_B, 255);
  delay(1000);
}

void loop() {
  sensors.requestTemperatures();
  uint32_t celcius = (uint32_t) (100 * sensors.getTempCByIndex(0));  // will be temperature multiplied by 100

  /*  constrain temperature to upper & lower bounds so we don't output
      the wrong color if the temp is outside the upper/lower bounds */
  uint32_t celcius_c = constrain(celcius, T_COLD, T_HOT);

  // linear map hue and saturation values based on the temperature
  uint32_t hue = map(celcius_c, T_COLD, T_HOT, HUE_COLD, HUE_HOT);
  uint32_t sat = map(celcius_c, T_COLD, T_HOT, SAT_COLD, SAT_HOT);

  // convert HSV to RGB
  // We always want value = 255 for maximum brightness
  getRGB(hue, sat, 255, rgb);

  // write the RGB values to the LED strip
  led(rgb);

  // Debug serial output
  Serial.print("T: "); Serial.print(celcius);
  Serial.print("\tH: "); Serial.print(hue);
  Serial.print("\tS: "); Serial.println(sat);
  delay(1000);

  if (celcius > T_HOT) {
    led(0, 0, 0); // flash LEDs if we've exceeded the upper temperature limit
    delay(500);
  }
}

void led(int r, int g, int b) {
  /// Convert RGB to PWM for LED strip
  analogWrite(LED_R, r);
  analogWrite(LED_G, g);
  analogWrite(LED_B, b);
}

void led(int rgb[3]) {
  /// Convert RGB to PWM for LED strip
  analogWrite(LED_R, rgb[0]);
  analogWrite(LED_G, rgb[1]);
  analogWrite(LED_B, rgb[2]);
}

void getRGB(int hue, int sat, int val, int colors[3]) {
  /// Convert HSV to RGB
  // https://www.kasperkamperman.com/blog/arduino/arduino-programming-hsb-to-rgb/
  /* convert hue, saturation and brightness ( HSB/HSV ) to RGB
     The dim_curve is used only on brightness/value and on saturation (inverted).
     This looks the most natural.
  */

  const byte dim_curve[] = { // exponential brightness curve to linearize fade-in
      0,   1,   1,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,
      3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,   4,   4,   4,
      4,   4,   4,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   6,   6,   6,
      6,   6,   6,   6,   6,   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,   8,
      8,   8,   9,   9,   9,   9,   9,   9,   10,  10,  10,  10,  10,  11,  11,  11,
      11,  11,  12,  12,  12,  12,  12,  13,  13,  13,  13,  14,  14,  14,  14,  15,
      15,  15,  16,  16,  16,  16,  17,  17,  17,  18,  18,  18,  19,  19,  19,  20,
      20,  20,  21,  21,  22,  22,  22,  23,  23,  24,  24,  25,  25,  25,  26,  26,
      27,  27,  28,  28,  29,  29,  30,  30,  31,  32,  32,  33,  33,  34,  35,  35,
      36,  36,  37,  38,  38,  39,  40,  40,  41,  42,  43,  43,  44,  45,  46,  47,
      48,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,
      63,  64,  65,  66,  68,  69,  70,  71,  73,  74,  75,  76,  78,  79,  81,  82,
      83,  85,  86,  88,  90,  91,  93,  94,  96,  98,  99,  101, 103, 105, 107, 109,
      110, 112, 114, 116, 118, 121, 123, 125, 127, 129, 132, 134, 136, 139, 141, 144,
      146, 149, 151, 154, 157, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 190,
      193, 196, 200, 203, 207, 211, 214, 218, 222, 226, 230, 234, 238, 242, 248, 255,
  };

  val = dim_curve[val];
  sat = 255-dim_curve[255-sat];

  int r;
  int g;
  int b;
  int base;

  if (sat == 0) { // Acromatic color (gray). Hue doesn't mind.
    colors[0]=val;
    colors[1]=val;
    colors[2]=val;
  } else  {

    base = ((255 - sat) * val)>>8;

    switch(hue/60) {
    case 0:
        r = val;
        g = (((val-base)*hue)/60)+base;
        b = base;
    break;

    case 1:
        r = (((val-base)*(60-(hue%60)))/60)+base;
        g = val;
        b = base;
    break;

    case 2:
        r = base;
        g = val;
        b = (((val-base)*(hue%60))/60)+base;
    break;

    case 3:
        r = base;
        g = (((val-base)*(60-(hue%60)))/60)+base;
        b = val;
    break;

    case 4:
        r = (((val-base)*(hue%60))/60)+base;
        g = base;
        b = val;
    break;

    case 5:
        r = val;
        g = base;
        b = (((val-base)*(60-(hue%60)))/60)+base;
    break;
    }

    colors[0]=r;
    colors[1]=g;
    colors[2]=b;
  }
}
