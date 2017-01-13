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

unsigned char header_status;

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

void setup_heater(){
	pinMode(HEATER_PIN0,  OUTPUT);
	pinMode(HEATER_PIN1,  OUTPUT);
	pinMode(HEATER_PIN2,  OUTPUT);
	delay(10);
  digitalWrite(HEATER_PIN0, HIGH);
  digitalWrite(HEATER_PIN1, HIGH);
  digitalWrite(HEATER_PIN2, HIGH);

	header_status = 0;
}

void setup(){
	Serial.begin(9600);
	Serial.println("Heater management");
	Serial.println("Setup heater.");
	setup_heater();
	Serial.println("Setup 24l01.");
	setup_24l01();
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

	/*
	 * If a packet has been recived.
	 *
	 * isSending also restores listening mode when it 
	 * transitions from true to false.
	 */

	if(!Mirf.isSending() && Mirf.dataReady()){
		/*
		 * Get load the packet into the buffer.
		 */
		Mirf.getData((byte*)data);
		checksum = data[0]+data[1]+data[2];
		Serial.println("Get data: ");
		Serial.println(data[0]);
		Serial.println(data[1]);
		Serial.println(data[2]);
		Serial.println(data[3]);


		if(checksum != data[3]){
			Serial.println("Invalid command data");
			return;
		}

		switch(data[0]){
			case CMD_SET_HEATER:
				switch(data[1]){
					case HEATER_1_SET:
						digitalWrite(HEATER_PIN0, HIGH);
						digitalWrite(HEATER_PIN1, HIGH);
						digitalWrite(HEATER_PIN2, LOW);
						header_status=1;
						break;
					case HEATER_2_SET:
						digitalWrite(HEATER_PIN0, LOW);
						digitalWrite(HEATER_PIN1, LOW);
						digitalWrite(HEATER_PIN2, HIGH);
						header_status=2;
            break;
					case HEATER_3_SET:
						digitalWrite(HEATER_PIN0, LOW);
						digitalWrite(HEATER_PIN1, LOW);
						digitalWrite(HEATER_PIN2, LOW);
						header_status=3;
						break;
					case HEATER_0_SET:
					default:
						digitalWrite(HEATER_PIN0, HIGH);
						digitalWrite(HEATER_PIN1, HIGH);
						digitalWrite(HEATER_PIN2, HIGH);
						header_status=0;
						break;
				}
				data[2]=CMD_STATUS_OK;
				break;
			case CMD_GET_HEATER:
				data[1]=header_status;
				data[2]=CMD_STATUS_OK;
				break;
			default:
				data[2]=CMD_STATUS_INVALID;
				break;
		}

		data[3]=data[0]+data[1]+data[2];
		/*
		 * Set the send address.
		 */ 
		Mirf.setTADDR((byte *)"clie1");
		/*
		 * Send the data back to the client.
		 */
		Mirf.send((byte*)data);
	}
}
