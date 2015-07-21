#ifndef Magic_h
#define Magic_h

#include "arduino.h"
#include <Wire.h>                     //I2C�p
#include <stdbool.h>
#include "Adafruit_NeoPixel.h"        //�t���J���[LED�p//�ʓrDL�K�v
#include "ADXL345.h"                  //�����x�Z���T�p
#include "Timer.h"

enum ActionStatus { Nothing, Tap, DoubleTap, FreeFall };
enum ColorStatus { Normal, Rainbow };
enum LEDStatus { Off, On, Blink, FadeIn, FadeOut };

void repeat();

bool active();
bool freefall();
bool tap();
bool doubletap();

// led �̐F����ύX����
// �F��: 0.0-1.0
void color(int led, double hue);

// led �̖��邳��ύX����
// ���邳: 0.0-1.0
void brightness(int led, double brightness);

// led �̍ʓx��ύX����
// �ʓx 0.0-1.0
void saturation(int led, double saturation);

// led �����C���{�[�ɕ\������B
// speed �� 100
void rainbow(int led, double period);

// led �̐F�������_���ɓ_������
void randomcolor(int led);

// �f�o�b�O�p (led �̌��ݕW�����̐F�Ɋւ���l��Ԃ�)
double color(int led);
double brightness(int led);
double saturation(int led);

// led �� period �b�Ԋu�œ_�ł�����
void blink(int led, double period);

void fadein(int led, double period);
void fadeout(int led, double period);

void on(int led);
void off(int led);

#endif
