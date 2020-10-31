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
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include "freashair.h"

byte heater_status = 0;
byte power = 0;
static unsigned short idle;
static unsigned short power_timer;
static unsigned short power_period_on;
static unsigned short power_period_off;

// maximum minutes for power period
#define MAX_POWER_PERIOD 50

void setup_24l01(){

	/*
	 * Set the SPI Driver.
	 */

	Mirf.spi = &MirfHardwareSpi;
	/*
	 * Setup pins / SPI.
	 */

	Mirf.init();

	/*
	 * Configure reciving address.
	 */

	Mirf.setRADDR((byte *)"serv1");
	Mirf.configRegister(RF_SETUP,0x27);//250Kbps
	Mirf.channel = 39;
	/*
	 * Set the payload length to sizeof(unsigned long) the
	 * return type of millis().
	 *
	 * NB: payload on client and server must be the same.
	 */

	Mirf.payload = PAYLOAD;

	/*
	 * Write channel and payload config then power up reciver.
	 */

	Mirf.config();
}

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

void setup_heater(){
	digitalWrite(HEATER_PIN0, LOW);
	digitalWrite(HEATER_PIN1, LOW);
	digitalWrite(HEATER_PIN2, HIGH);

	delay(10);

	pinMode(HEATER_PIN0,  OUTPUT);
	pinMode(HEATER_PIN1,  OUTPUT);
	pinMode(HEATER_PIN2,  OUTPUT);
	heater_status = HEATER_1_SET;

	power_period_on = MAX_POWER_PERIOD;
	power_period_on = power_period_on * 60 *20;
	power_period_on = power_period_on;

	power_period_off = MAX_POWER_PERIOD;
	power_period_off = power_period_off * 60 *20;
	power_period_off = power_period_off;

	power = POWER_ON;
	pinMode(POWER_PIN, OUTPUT);

	/* POWER PIN HIGH = 'power on' */
	digitalWrite(POWER_PIN, HIGH);
	power_timer = power_period_on;
}

void setup(){
	Serial.begin(9600);
	Serial.println("Heater management");
	Serial.println("Setup heater.");
	setup_heater();
	Serial.println("Setup 24l01.");

	setup_24l01();
}

void power_off()
{
	power_timer = power_period_off;
	power = POWER_OFF;
	digitalWrite(POWER_PIN, LOW);
  set_heater( heater_status, 0);
}

void power_on()
{
	power_timer = power_period_on;
	power = POWER_ON;
	digitalWrite(POWER_PIN, HIGH);
  heater_status=set_heater(0, heater_status);
}

void loop(){
	/*
	 * A buffer to store the data.
	 */
	/*
	 * data[0]=command
	 * data[1]=data0
	 * data[2]=data1 | status
	 * data[3]=checksum
	 */
	unsigned char data[4];
	unsigned char checksum;

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
    _set_heater( 0);

	Serial.print(" -> ");
	Serial.print(power ? "OFF" : "ON");
	Serial.print("   HEAT: ");
	Serial.println(heater_status);

	if(!Mirf.isSending() && Mirf.dataReady()){
		delay(50);
    Serial.println("--");
		static unsigned int count;
		count++;
		Mirf.getData((byte*)data);
		checksum = data[0]+data[1]+data[2];
		Serial.print("Get data ");    
		Serial.print("[");   
		Serial.print(count);
		Serial.print("]: ");
		Serial.print(data[0]);
		Serial.print(" - ");
		Serial.print(data[1]);
		Serial.print(" - ");
		Serial.print(data[2]);
		Serial.print(" - ");
		Serial.println(data[3]);
		Mirf.flushRx();
		if(checksum != data[3]){
			Serial.println("Invalid command data");
			return;
		}
   Serial.println("2");
		switch(data[0]){
			case CMD_SET_HEATER:
				switch(data[1]){
					case HEATER_1_SET:
						if (power == POWER_ON)
              heater_status=set_heater(heater_status, 1);
						break;
            
					case HEATER_2_SET:
						if (power == POWER_ON)
              heater_status=set_heater(heater_status, 2);
						break;
            
					case HEATER_3_SET:
						if (power == POWER_ON)
              heater_status=set_heater(heater_status, 3);
						break;
					case HEATER_0_SET:
					default:
						if (power == POWER_ON)
	            heater_status=set_heater(heater_status, 0);
						break;
				}
       
				data[2]=CMD_STATUS_OK;
				break;

			case CMD_GET_HEATER:
				data[1]=heater_status;
				data[2]=CMD_STATUS_OK;
				break;

			case CMD_SET_POWER_PERIOD_ON:
				if (data[1] >  MAX_POWER_PERIOD)
					data[1] = MAX_POWER_PERIOD;
				power_period_on = (short)data[1] * 20 * 60;
				data[2]=CMD_STATUS_OK;
				power_on();
				break;

			case CMD_GET_POWER_PERIOD_ON:
				data[1]= power_period_on / 60 / 20;
				data[2]=CMD_STATUS_OK;
				break;

			case CMD_SET_POWER_PERIOD_OFF:
				if (data[1] >  MAX_POWER_PERIOD)
					data[1] = MAX_POWER_PERIOD;
				power_period_off = (short)data[1] * 20 * 60;
				data[2]=CMD_STATUS_OK;
				power_on();
				break;

			case CMD_GET_POWER_PERIOD_OFF:
				data[1]= power_period_off / 60 / 20;
				data[2]=CMD_STATUS_OK;
				break;

			case CMD_GET_POWER_TIMER:
				data[1]= power_timer/ 60 / 20;
				data[2]=CMD_STATUS_OK;
				break;
	
			case CMD_SET_POWER:
				if (data[1] == POWER_ON) {
					power_on();
				} else {
					power_off();
				}
				data[2]=CMD_STATUS_OK;
				break;

			case CMD_GET_POWER:
				data[1] = power;
				data[2]=CMD_STATUS_OK;
				break;
			default:
				data[2]=CMD_STATUS_INVALID;
				break;
		}
   Serial.println("3");
   
   data[3]=data[0]+data[1]+data[2];
		/*
		 * Set the send address.
		 */ 
		Mirf.setTADDR((byte *)"clie1");
		/*
		 * Send the data back to the client.
		 */
		Mirf.send((byte*)data);
		idle=0; 
		Serial.println("4");

		while(Mirf.isSending()){}
      Serial.println("5");

		delay(10);
	} else {
		delay(50);
		idle++;
		if(idle>1200){
			Serial.println("Reset Link");
			setup_24l01();
			idle=0;
		}
	}
}
