#include "Arduino.h"

using namespace std;

const int LED_R = 5; // digital output pin for LED red channel
const int LED_G = 3; // digital output pin for LED green channel
const int LED_B = 6; // digital output pin for LED blue channel
const int PIR_IN = 4; // digital input pin for PIR sensor

int rgb[3];  // 0-255 RGB values for the LEDs

int TICK_INTERVAL = 100; // time to wait between loops milliseconds
int FADE_TIME = 5000; // time to fade in / out in milliseconds
int MIN_ON_TIME = 10000; // minimum time to spend in on state milliseconds

uint8_t MAX_BRIGHTNESS = 255;

int brightness = 0; // led brightness
uint8_t hue = 0; // led hue

long previousTime;
long offTime;
long pirOnTime; // time when pir sensor goes from off to on
long pirOffTime; // time when pir sensor goes from on to off
bool pir; // state of the pir sensor

enum stateEnum { off, rising, on, falling };

stateEnum state = off;

void getRGB(int hue, int sat, int val, int colors[3]);

void setup() {
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(PIR_IN, INPUT);

  calibrationSequence();

  Serial.begin(9600);
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
  int dt = millis() - previousTime;

  // check previous pir state against current state
  if (!pir && digitalRead(PIR_IN)) { pirOnTime = millis(); }
  if (pir && !digitalRead(PIR_IN)) { pirOffTime = millis(); }

  // update current pir state
  pir = digitalRead(PIR_IN);

  // OFF -> RISING
  if ( state == off && pir ) { state = rising; }

  // RISING -> ON
  if ( state == rising && brightness == MAX_BRIGHTNESS) { state = on; }

  // ON -> FALLING
  if ( state == on && !pir && millis() - offTime > MIN_ON_TIME ) { state = falling; }

  // FALLING -> OFF
  if ( state == falling && brightness == 0 ) {
    state = off;
    hue = random(0, 360);
  }

  // FALLING -> RISING
  if ( state == falling && pir ) { state = rising; }

  switch (state) {
    case rising:
      brightness += MAX_BRIGHTNESS
                    * dt
                    / FADE_TIME;
      brightness = min(MAX_BRIGHTNESS, brightness); // clamp brigtness at 255
      break;
    case falling:
      brightness -= MAX_BRIGHTNESS
                    * dt
                    / FADE_TIME ;
      brightness = max(0, brightness); // clamp brigtness at 0
      break;
  }

  // convert HSV to RGB
  // We always want value = 255 for maximum brightness
  getRGB(hue, 255, 255, rgb);

  // write the RGB values to the LED strip
  led(rgb);

  // Debug serial output
  Serial.print("B: "); Serial.print(brightness);
  Serial.print("\tPIR: "); Serial.println(pir);
  delay(TICK_INTERVAL);
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
