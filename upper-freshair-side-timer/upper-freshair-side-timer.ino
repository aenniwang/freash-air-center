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
static unsigned short power_period;

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

void setup_heater(){
    digitalWrite(HEATER_PIN0, LOW);
    digitalWrite(HEATER_PIN1, LOW);
    digitalWrite(HEATER_PIN2, HIGH);

    digitalWrite(POWER_PIN, LOW);

    delay(10);
    
    pinMode(HEATER_PIN0,  OUTPUT);
    pinMode(HEATER_PIN1,  OUTPUT);
    pinMode(HEATER_PIN2,  OUTPUT);
    pinMode(POWER_PIN, OUTPUT);
    heater_status = HEATER_2_SET;
    power = POWER_ON;

    power_timer = MAX_POWER_PERIOD;
    power_timer = power_timer * 60 *20;
    power_period = power_timer;
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

    power_timer--;

    if (power == POWER_OFF) {
        digitalWrite(HEATER_PIN0, HIGH);
        digitalWrite(HEATER_PIN1, HIGH);
        digitalWrite(HEATER_PIN2, HIGH);
        heater_status=0;
    }

    if (!power_timer) {
        Serial.print("Power changed from ");
        Serial.print(power ? "OFF" : "ON");
        power_timer = power_period;
        if (power == POWER_ON) {
            power = POWER_OFF;
            digitalWrite(POWER_PIN, HIGH);
        } else {
            power =POWER_ON;
            digitalWrite(POWER_PIN, LOW);
        }
        
        Serial.print(" -> ");
        Serial.println(power ? "OFF" : "ON");
    }

    if(!Mirf.isSending() && Mirf.dataReady()){
        delay(50);
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

        switch(data[0]){
        case CMD_SET_HEATER:
            switch(data[1]){
            case HEATER_1_SET:
                digitalWrite(HEATER_PIN0, HIGH);
                digitalWrite(HEATER_PIN1, HIGH);
                digitalWrite(HEATER_PIN2, LOW);
                heater_status=1;
                break;
            case HEATER_2_SET:
                digitalWrite(HEATER_PIN0, LOW);
                digitalWrite(HEATER_PIN1, LOW);
                digitalWrite(HEATER_PIN2, HIGH);
                heater_status=2;
                break;
            case HEATER_3_SET:
                digitalWrite(HEATER_PIN0, LOW);
                digitalWrite(HEATER_PIN1, LOW);
                digitalWrite(HEATER_PIN2, LOW);
                heater_status=3;
                break;
            case HEATER_0_SET:
            default:
                digitalWrite(HEATER_PIN0, HIGH);
                digitalWrite(HEATER_PIN1, HIGH);
                digitalWrite(HEATER_PIN2, HIGH);
                heater_status=0;
                break;
            }
            data[2]=CMD_STATUS_OK;
            break;
        case CMD_GET_HEATER:
            data[1]=heater_status;
            data[2]=CMD_STATUS_OK;
            break;
        case CMD_SET_POWER_PERIOD:
            if (data[1] >  MAX_POWER_PERIOD)
                data[1] = MAX_POWER_PERIOD;

            power_period = (short)data[1] * 20 * 60;
            data[2]=CMD_STATUS_OK;
            break;
        case CMD_GET_POWER_PERIOD:
            data[1]= power_period / 60 / 20;
            data[2]=CMD_STATUS_OK;
            break;
        case CMD_SET_POWER:
            if (data[1] = POWER_ON) {
                digitalWrite(POWER_PIN, LOW);
                power = POWER_ON;
            } else {
                digitalWrite(POWER_PIN, HIGH);
                power = POWER_OFF;                
            }
            break;
        case CMD_GET_POWER:
            data[1] = power;
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
        idle=0;
    } else {
        delay(50);
        idle++;
        if(idle>12000){
            Serial.println("Reset Link");
            setup_24l01();
            idle=0;
        }
    }
}
