/**
 * An Mirf example which copies back the data it recives.
 *
 * Pins:
 * Hardware SPI:
 * MISO -> 12
 * MOSI -> 11
 * SCK -> 13
 *
 * Configurable:
 * CE -> 8
 * CSN -> 7
 *
 */

#include <SPI.h>
#include "freashair.h"

byte heater_status = 0;
byte power = 0;
static unsigned short idle;
static unsigned short power_timer;
static unsigned short power_period_on;
static unsigned short power_period_off;
 static   unsigned char p_timer = 40;

// maximum minutes for power period
#define MAX_POWER_PERIOD 50

void _set_heater(int num)
{
  switch(num) {
    default:
     case 0:
      digitalWrite(HEATER_PIN0, HIGH);
      digitalWrite(HEATER_PIN1, HIGH);
      digitalWrite(HEATER_PIN2, HIGH);
      break;
      case 1:
      digitalWrite(HEATER_PIN0, LOW);
      digitalWrite(HEATER_PIN1, HIGH);
      digitalWrite(HEATER_PIN2, HIGH);
      break;
      case 2:
      digitalWrite(HEATER_PIN0, LOW);
      digitalWrite(HEATER_PIN1, LOW);
      digitalWrite(HEATER_PIN2, HIGH);
      break;
      case 3:
      digitalWrite(HEATER_PIN0, LOW);
      digitalWrite(HEATER_PIN1, LOW);
      digitalWrite(HEATER_PIN2, LOW);
      break;
  }
}

int set_heater(int cur, int next)
{
  int i = 0;
  int step = 1;

  if (cur == 0)
    _set_heater(cur);

  if (cur == next)
  return cur;

  step = 1;
  if (cur>next)
    step = -1;

  i = cur;
  while(cur!=next) {
    cur += step;
    _set_heater(cur);
    delay(500);
  }
  return cur;
}

void power_off()
{
  power_timer = power_period_off;
  power = POWER_OFF;
  digitalWrite(POWER_PIN, LOW);
  delay(50);
  set_heater( heater_status, 0);
}

void power_on()
{
  heater_status = 0;
  power_timer = power_period_on;
  power = POWER_ON;
  digitalWrite(POWER_PIN, HIGH);
  delay(50);
  set_heater(0, heater_status);
}

void setup_heater(){
	delay(10);
  
	pinMode(HEATER_PIN0,  OUTPUT);
	pinMode(HEATER_PIN1,  OUTPUT);
	pinMode(HEATER_PIN2,  OUTPUT);
	heater_status = HEATER_0_SET;

	power_period_on = MAX_POWER_PERIOD;
	power_period_on = power_period_on * 60 * 20;
	power_period_on = power_period_on;

	power_period_off = MAX_POWER_PERIOD;
	power_period_off = power_period_off * 60 *20;
	power_period_off = power_period_off;

	power = POWER_ON;
	pinMode(POWER_PIN, OUTPUT);
  delay(2000);
  power_on();
}

void setup(){
	Serial.begin(9600);
	Serial.println("Heater management");
	Serial.println("Setup heater.");
	setup_heater();
	Serial.println("Setup 24l01.");
}

void loop(){
    power_timer--;

    if (power == POWER_ON) {
        /* Power is on, then check if it's time to power off */
        if (power_timer == 0) {
            power_off();
            Serial.println("Power changed from ON -> OFF");
        }
    } else {
        /* Power is OFF */
        if (power_timer == 0) {
            power_on();
            Serial.println("Power changed from OFF -> ON");
        }
    }

    /* Make sure all heater is off when power is off */
    if (power == POWER_OFF)
        _set_heater(0);

    p_timer --;
    if (p_timer == 0){
        Serial.print(" -> ");
        Serial.print(power ? "OFF" : "ON");
        Serial.print("   HEAT: ");
        Serial.println(heater_status);
        p_timer=40;
    }

    delay(50);
}
