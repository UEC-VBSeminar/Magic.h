#ifndef Magic_h
#define Magic_h

#include "arduino.h"
#include <Wire.h>                     //I2C用
#include <stdbool.h>
#include "Adafruit_NeoPixel.h"        //フルカラーLED用//別途DL必要
#include "ADXL345.h"                  //加速度センサ用
#include "Timer.h"

enum ActionStatus { Nothing, Tap, DoubleTap, FreeFall };
enum ColorStatus { Normal, Rainbow };
enum LEDStatus { Off, On, Blink, FadeIn, FadeOut };

void repeat();

bool active();
bool freefall();
bool tap();
bool doubletap();

// led の色相を変更する
// 色相: 0.0-1.0
void color(int led, double hue);

// led の明るさを変更する
// 明るさ: 0.0-1.0
void brightness(int led, double brightness);

// led の彩度を変更する
// 彩度 0.0-1.0
void saturation(int led, double saturation);

// led をレインボーに表示する。
// speed は 100
void rainbow(int led, double period);

// led の色をランダムに点灯する
void randomcolor(int led);

// デバッグ用 (led の現在標示中の色に関する値を返す)
double color(int led);
double brightness(int led);
double saturation(int led);

// led を period 秒間隔で点滅させる
void blink(int led, double period);

void fadein(int led, double period);
void fadeout(int led, double period);

void on(int led);
void off(int led);

#endif
