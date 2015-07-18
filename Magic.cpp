#include "Magic.h"

void initializeAccelerometer();
void color_update_hsl(int led);
void color_show(int led);

/* LEDのセットアップ */
int NumOfLED = 2;                     // フルカラーLEDの個数
int PinOfLED = 2;                     // LEDが繋がってるpin
Adafruit_NeoPixel LED = Adafruit_NeoPixel(NumOfLED, PinOfLED, NEO_RGB + NEO_KHZ800);
/* 加速度センサのセットアップ */
ADXL345 adxl;

uint32_t led_colors[2] = { 0x000000, 0x000000 };
bool blink_color[2] = { false };

double led_hue[2] = { -1.0, -1.0 };
double led_brightness[2] = { 0.5, 0.5 };
double led_saturation[2] = { 1.0, 1.0 };

bool isActive = false;
bool isFreeFall = false;
ActionStatus action_s = Nothing;
ColorStatus led_cs[2] = { Normal, Normal };
LEDStatus led_s[2] = {Off, Off};
LEDStatus led_ls[2] = {Off, Off}; // last status
Timer t1, t1_c, t2, t2_c;

const int REPEAT_CYCLE_MS = 33; // [ms]

double constraint(double value, double min, double max) {
  if (value < min) value = min;
  if (max < value) value = max;
  return value;
}

unsigned long cycle_time = REPEAT_CYCLE_MS; // 毎回のループに掛かる時間

void setup() {
  // ピン初期化
  pinMode(2,OUTPUT);
  digitalWrite(2,LOW);

  /* シリアル初期化 */
  Serial.begin(9600);
  // while (!Serial )
  // {
  //   ; // wait for serial port to connect. Needed for Leonardo only
  // }

  /* LED初期化 */
  LED.begin();
  LED.setBrightness(50);
  LED.setPixelColor(0, 0, 0, 0); // 0番目のLED, r, g, b
  LED.setPixelColor(1, 0, 0, 0); // 1番目のLED, r, g, b
  LED.show();

  /* 加速度センサ初期化 */
  initializeAccelerometer();

  // led1_s = LEDStatus::On;
  // led2_s = LEDStatus::Off;

  // Serial.println("setup done");
}

/* 加速度センサ初期化 */
void initializeAccelerometer() {

  adxl.powerOn();

  /* 動作したかを監視する軸の設定 (1 == on; 0 == off) */
  //各軸の判定の論理和
  adxl.setActivityX(true);
  adxl.setActivityY(true);
  adxl.setActivityZ(true);

  // 動作してないを監視する軸の設定
  //各軸の判定の論理積
  adxl.setInactivityX(true);
  adxl.setInactivityY(true);
  adxl.setInactivityZ(true);

  // タップされたことを検視する軸の設定
  adxl.setTapDetectionOnX(false);
  adxl.setTapDetectionOnY(false);
  adxl.setTapDetectionOnZ(true);

  /* 動作のしきい値を設定 (0-255) */
  adxl.setActivityThreshold(25);    //値:*62.5[mg]
  adxl.setInactivityThreshold(30); // 75 //値:*62.5[mg]
  adxl.setTimeInactivity(1);       // 10 //非動作の判定までに要する時間//値:*5[ms]

  // タップ, ダブルタップを検出するしきい値の設定 (0-255)
  // タップ検出
  adxl.setTapThreshold(80);         // 値:*62.5[mg]
  adxl.setTapDuration(100);          // 値:*0.625[ms]
  // ダブルタップ検出
  adxl.setDoubleTapLatency(10);     // 値:*1.25[ms]
  adxl.setDoubleTapWindow(200);     // 値:*1.25[ms]

  // 自由落下を検出するしきい値の設定 (0-255)
  // adxl.setFreeFallThreshold(0x09);  // 値:*62.5[mg] (推奨: 0x05 - 0x09)
  // adxl.setFreeFallDuration(0x0A);   // 値:*5[ms] (推奨: 0x14 - 0.46)
  adxl.setFreeFallThreshold(8);  // 値:*62.5[mg] (推奨: 0x05 - 0x09)
  adxl.setFreeFallDuration(0);   // 値:*5[ms] (推奨: 0x14 - 0.46)

  // 割り込みに使うピンの設定 (pin1 を使う, pin2はリセットができないという不具合がある)
  adxl.setInterruptMapping(ADXL345_INT_SINGLE_TAP_BIT,   ADXL345_INT1_PIN);
  adxl.setInterruptMapping(ADXL345_INT_DOUBLE_TAP_BIT,   ADXL345_INT1_PIN);
  adxl.setInterruptMapping(ADXL345_INT_FREE_FALL_BIT,    ADXL345_INT1_PIN);
  adxl.setInterruptMapping(ADXL345_INT_ACTIVITY_BIT,     ADXL345_INT1_PIN);
  adxl.setInterruptMapping(ADXL345_INT_INACTIVITY_BIT,   ADXL345_INT1_PIN);

  // 割込みレジスタの設定
  adxl.setInterrupt(ADXL345_INT_SINGLE_TAP_BIT, true);
  adxl.setInterrupt(ADXL345_INT_DOUBLE_TAP_BIT, true);
  adxl.setInterrupt(ADXL345_INT_FREE_FALL_BIT,  true);
  adxl.setInterrupt(ADXL345_INT_ACTIVITY_BIT,   true);
  adxl.setInterrupt(ADXL345_INT_INACTIVITY_BIT, true);
}

void loop() {
  unsigned long begin = millis();

  action_s = Nothing;

  byte interrupts = adxl.getInterruptSource();

  // 動作してない?
  if(adxl.triggered(interrupts, ADXL345_INACTIVITY) ) {
    isActive = false;
  }

  // 動作した?
  if(adxl.triggered(interrupts, ADXL345_ACTIVITY) ) {
    isActive = true;
  }

  // タップ
  if(adxl.triggered(interrupts, ADXL345_SINGLE_TAP) ) {
    action_s = Tap;
  }

  // ダブルタップ
  if(adxl.triggered(interrupts, ADXL345_DOUBLE_TAP) ) {
    action_s = DoubleTap;
  }

  // 自由落下
  if(adxl.triggered(interrupts, ADXL345_FREE_FALL)) {
    // action_s = FreeFall;
    isFreeFall = true;
  } else {
    isFreeFall = false;
  }

  t1.update();
  t2.update();
  t1_c.update();
  t2_c.update();

  repeat();

  delay(REPEAT_CYCLE_MS);
  cycle_time = millis() - begin;

  led_ls[0] = led_s[0];
  led_ls[1] = led_s[1];
}

// http://www.peko-step.com/tool/hslrgb.html#ppick3
void color_update_hsl(int led) {
  double s = led_saturation[led-1];
  double l = led_brightness[led-1];
  double h = led_hue[led-1] * 360; // 0 - 360

  double max, min;
  if (l < 0.5) {
    max = 255 * l * (s + 1);
    min = 255 * l * (s - 1);
  } else {
    max = 255 * (l + (1 - l) * s);
    min = 255 * (l - (1 - l) * s);
  }

  uint8_t r, g, b;

  if (h < 60) {
    r = max;
    g = (h / 60) * (max - min) + min;
    b = min;
  } else if (h < 120) {
    r = ((120 - h) / 60) * (max - min) + min;
    g = max;
    b = min;
  } else if (h < 180) {
    r = min;
    g = max;
    b = ((h - 120) / 60) * (max - min) + min;
  } else if (h < 240) {
    r = min;
    g = ((240 - h) / 60) * (max - min) + min;
    b = max;
  } else if (h < 300) {
    r = ((h - 240) / 60) * (max - min) + min;
    g = min;
    b = max;
  } else {
    r = max;
    g = min;
    b = ((360-h) / 60) * (max - min) + min;
  }

  uint32_t rgb = Adafruit_NeoPixel::Color(r, g, b);
  // Serial.println(rgb, HEX);
  led_colors[led-1] = rgb;

}

void color_show(int led) {
  // Serial.println(led_colors[led-1], HEX);
  LED.setPixelColor(led-1, led_colors[led-1]);
  LED.show();
}

// Action
////////////////////////////////////////////////////////

bool active() {
  return isActive;
}

bool freefall() {
  // return action_s == FreeFall;
  return isFreeFall;
}

bool tap() {
  return action_s == Tap;
}

bool doubletap() {
  return action_s == DoubleTap;
}

// LED Color (HSL)
////////////////////////////////////////////////////////

double color(int led) {
  return led_hue[led-1];
}

double brightness(int led) {
  return led_brightness[led-1];
}

double saturation(int led) {
  return led_saturation[led-1];
}

void color_(int led, double hue) {
  if (hue < 0) hue = 1 - fmod(-hue, 1); //(-hue % 1);
  if (1 < hue) hue = fmod(hue, 1);

  if (led_hue[led-1] != hue) {
    led_hue[led-1] = hue;
    color_update_hsl(led);
    // color_show(led);
  }
}

void color(int led, double hue) {
  if (led == 1) {
    t1_c.stop(0);
  }

  if (led == 2) {
    t2_c.stop(0);
  }

  color_(led, hue);

  led_cs[led-1] = Normal;
}

void brightness(int led, double brightness) {
  brightness = constraint(brightness, 0, 1);

  if (led_brightness[led-1] != brightness) {
    led_brightness[led-1] = brightness;
    color_update_hsl(led);
  }
}

void saturation(int led, double saturation) {
  saturation = constraint(saturation, 0, 1);

  if (led_saturation[led-1] != saturation) {
    led_saturation[led-1] = saturation;
    color_update_hsl(led);
  }
}

void randomcolor(int led) {
  color(led, (double)(random(0, 100))/100.0);
  color_show(led);
}

// rainbow
////////////////////////////////////////////////////////

double rainbow_period[2] = { 0.0, 0.0 };

void doRainbow(int led) {
  double diff = rainbow_period[led-1] / (1000.0 / cycle_time);
  color_(led, led_hue[led-1] + diff);
}

void doRainbow1() { doRainbow(1); }

void doRainbow2() { doRainbow(2); }

void rainbow(int led, double period) {
  if (led_cs[led-1] != Rainbow) {
    rainbow_period[led-1] = period;
    if (led == 1) {
      t1_c.stop(0);
      t1_c.every(REPEAT_CYCLE_MS, doRainbow1);
    }

    if (led == 2) {
      t2_c.stop(0);
      t2_c.every(REPEAT_CYCLE_MS, doRainbow2);
    }

    led_cs[led-1] = Rainbow;
  }
}

// Blink
////////////////////////////////////////////////////////

void doBlink(int led) {
  uint32_t new_color;

  if (blink_color[led-1]) {
    new_color = 0x000000;
  } else {
    new_color = led_colors[led-1];
  }

  blink_color[led-1] = !blink_color[led-1];

  LED.setPixelColor(led-1, new_color);
  LED.show();
}

void doBlink1() { doBlink(1); }

void doBlink2() { doBlink(2); }

void blink(int led, double period) {
  if (led_s[led-1] != Blink) {
    if (led == 1) {
      t1.stop(0);
      t1.every(period * 1000, doBlink1);
    }

    if (led == 2) {
      t2.stop(0);
      t2.every(period * 1000, doBlink2);
    }

    led_s[led-1] = Blink;
  }
}

// FadeIn
////////////////////////////////////////////////////////

double fadein_period[2] = { 0.0, 0.0 };
double fadein_diff[2] = { 0.0, 0.0 };

void doFadeIn(int led) {
  double b = brightness(led);
  b += fadein_diff[led-1];
  Serial.print("fadein"); Serial.print(b); Serial.print(" "); Serial.println(fadein_diff[led-1]) / ((1000.0 * fadein_period[led-1]) / cycle_time);
  if (0.5 <= b) {
    b = 0.5;
  }

  brightness(led, b);
  color_show(led);
}

void doFadeIn1() { doFadeIn(1); }

void doFadeIn2() { doFadeIn(2); }

void fadein(int led, double period) {
  if (led_s[led-1] != FadeIn) {
    if (led_ls[led-1] != FadeIn || fadein_period[led-1] != period) {
      fadein_period[led-1] = period;
      fadein_diff[led-1] = (0.5 - brightness(led)) / ((1000.0 * fadein_period[led-1]) / cycle_time);
    }

    if (led == 1) {
      t1.stop(0);
      t1.every(REPEAT_CYCLE_MS, doFadeIn1);
    }

    if (led == 2) {
      t2.stop(0);
      t2.every(REPEAT_CYCLE_MS, doFadeIn2);
    }

    led_s[led-1] = FadeIn;
  }
}

// FadeOut
////////////////////////////////////////////////////////

double fadeout_period[2] = { 0.0, 0.0 };
double fadeout_diff[2] = { 0.0, 0.0 };

void doFadeOut(int led) {
  double b = brightness(led);
  b -= fadeout_diff[led-1];
  Serial.print("fadeout"); Serial.print(b); Serial.print(" "); Serial.println(fadeout_diff[led-1] / ((1000.0 * fadeout_period[led-1]) / cycle_time));

  if (b < 0) {
    b = 0;
  }

  brightness(led, b);
  color_show(led);
}

void doFadeOut1() { doFadeOut(1); }

void doFadeOut2() { doFadeOut(2); }

void fadeout(int led, double period) {
  if (led_s[led-1] != FadeOut) {
    if (led_ls[led-1] != FadeOut || fadeout_period[led-1] != period) {
      fadeout_period[led-1] = period;
      fadeout_diff[led-1] = brightness(led) / ((1000.0 * fadeout_period[led-1]) / cycle_time);
    }

    if (led == 1) {
      t1.stop(0);
      t1.every(REPEAT_CYCLE_MS, doFadeOut1);
    }

    if (led == 2) {
      t2.stop(0);
      t2.every(REPEAT_CYCLE_MS, doFadeOut2);
    }

    led_s[led-1] = FadeOut;
  }
}

// On/Off
////////////////////////////////////////////////////////

void doOn(int led) {
  color_show(led);
}

void doOn1() { doOn(1); }

void doOn2() { doOn(2); }

void on(int led) {
  if (led_s[led-1] != On) {
    if (led == 1) {
      t1.stop(0);
      t1.every(10, doOn1);
    }

    if (led == 2) {
      t2.stop(0);
      t2.every(10, doOn2);
    }

    // color_show(led);
    led_s[led-1] = On;
  }
}

void off(int led) {
  if (led_s[led-1] != Off) {
    if (led == 1) {
      t1.stop(0);
      LED.setPixelColor(led-1, 0x000000);
    }

    if (led == 2) {
      t2.stop(0);
      LED.setPixelColor(led-1, 0x000000);
    }

    LED.show();
    led_s[led-1] = Off;
  }
}
