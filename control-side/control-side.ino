/*
   LiquidCrystal Library - Hello World

   Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
   library works with all LCD displays that are compatible with the
   Hitachi HD44780 driver. There are many of them out there, and you
   can usually tell them by the 16-pin interface.

   This sketch prints "Hello World!" to the LCD
   and shows the time.

   The circuit:
 * LCD RS pin to digital pin 9
 * LCD Enable pin to digital pin 10
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe

 This example code is in the public domain.

http://www.arduino.cc/en/Tutorial/LiquidCrystal
 */

#include <LiquidCrystal.h>
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include "freashair.h"
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(9, 10, 5, 4, 3, 2);

#define KEY_SELECT 14
#define KEY_COMFIRM 15

unsigned char heater_status;
unsigned char fan_status;
unsigned char comm_err;
void setup_24l01(){
    Mirf.spi = &MirfHardwareSpi;
    Mirf.init();
    Mirf.setRADDR((byte *)"clie1");
    Mirf.payload = PAYLOAD;
    Mirf.config();
}

void update_lcd_machine_status(){
    // Print a message to the LCD.
    static byte error_count;
    lcd.setCursor(0,0);
    if(comm_err){
        error_count++;
        lcd.print("-  Link Error -");
        if(error_count > 5){
        delay(100);
        setup_24l01();
        delay(100);
        error_count=0;
        }
        return;
    }
    switch(heater_status){
        case HEATER_1_SET:
            lcd.print("HEAT:--#");
            break;
        case HEATER_2_SET:
            lcd.print("HEAT:-##");
            break;
        case HEATER_3_SET:
            lcd.print("HEAT:###");
            break;
        default:
            lcd.print("HEAT:---");
            break;
    }
    lcd.setCursor(8,0);
    if(fan_status == FAN_SPEED_HIGH)
        lcd.print("FAN:HIGH");
    else
        lcd.print("FAN:LOW ");
}

void setup_1602() {
    // set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
    update_lcd_machine_status();
}

byte dev24l01_send_cmd(byte *data){
    byte data_ret[4];
    byte checksum;
    comm_err = 0;
    Mirf.setTADDR((byte *)"serv1");
    Mirf.send(data);
    unsigned long time = millis();
   // Serial.print("time =");
  //  Serial.println(time);
 //   Serial.println("sending");
    while(Mirf.isSending()){
    }
    delay(10);
  //  Serial.println("Waitforready");
   // Serial.flush();
    while(!Mirf.dataReady()){
        // if(millis()>1000)  Serial.println(millis());
        //Serial.flush();
        if ( ( millis() - time ) > 1000 ) {
            Serial.println("Timeout on response from server!");
            comm_err = 1;
            return CMD_STATUS_ERR_TIMEOUT;
        }
        delay(50);
    }
   // Serial.println("get respond");

    Mirf.getData(data_ret);
    checksum = data_ret[0]+data_ret[1]+data_ret[2];
    if((data[0] != data_ret[0]) || (checksum!=data_ret[3]))
    {
        comm_err = 1;
        Serial.println("Error in get command status.");
        return CMD_STATUS_ERR;
    }
    if(data[0]==CMD_GET_HEATER || data[0] == CMD_GET_FANSPEED)
       data[1]=data_ret[1];
    return data_ret[2];
} 

byte command_processing(byte cmd, byte arg, byte sts){
    byte data[4];
    byte ret=0;
    data[0]=cmd;
    data[1]=arg;
    data[2]=sts;
    data[3]=data[0]+data[1]+data[2];

    Serial.print("CMD = ");
    Serial.println(cmd);
    switch(cmd){
        case CMD_SET_HEATER:
        case CMD_GET_HEATER:
            ret = dev24l01_send_cmd(data);
            if(ret == CMD_STATUS_OK){
                heater_status = data[1];
            }
            break;
        case CMD_GET_FANSPEED:
        case CMD_SET_FANSPEED:
         ret = dev24l01_send_cmd(data);
            if(ret == CMD_STATUS_OK)
                fan_status = data[1];
            break;
        default:
            Serial.println("invalid command");
            ret = CMD_STATUS_ERR;
            break;
    }
    return ret;
}

void setup_keyboard(){
    pinMode(KEY_SELECT,INPUT_PULLUP);
    pinMode(KEY_SELECT,INPUT_PULLUP);
}


void setup(){
    Serial.begin(9600);
    Serial.println("Setup LCD"); 
    fan_status = FAN_SPEED_LOW;
    heater_status = HEATER_0_SET;

    setup_1602();
    Serial.println("Setup 24L01");
    setup_24l01();
    Serial.println("Setup Keyboard");
    setup_keyboard();
    heater_status = 0;
    fan_status=0;
    comm_err = 1;
}
/* Show 
    |FAN_SPEED  HEATER Level|
    |  SET                  |
    if 5 seconds no action on KEY, return to this scean
*/
#define LCD_SCEAN_MAIN 0
/* if SET is pressed , then <HEATEER LVL> blinking 
    show
    | HEATER LVL  FAN SPD   |
    | OK            NEXT    |
    if cancel go to LCD_SCEAN_MAIN
*/
#define LCD_SCEAN_SET_SELECT_HEATLVL 1
#define LCD_SCEAN_SET_SELECT_FANSPD 2 
/*  if HEATER_LVL chosen
    |HEATER LVL: --#        |
    | CHANGE        CANCEL  |
    if CANCEL goto LCD_SCEAN_SET_SELECT
*/
#define LCD_SCEAN_SET_HEATERLVL 3
#define LCD_SCEAN_SET_FANSPEED 4 

#define KEY1 0
#define KEY2 1
#define KEY12 3

byte scean ;
byte scan_key(){
    return 0;
}

void handling_key(byte key){
    switch(scean){
    /*
        |FAN_SPEED  HEATER Level|
        |  SET                  |
    */
    case LCD_SCEAN_MAIN:
        switch(key){
            case KEY1:
                /*
                   | HEATER LVL  FAN SPD   |
                   |    NEXT          OK   |
                 */
                scean=LCD_SCEAN_SET_SELECT_HEATLVL;
                break;
            case KEY2:
                              break;
            default:
                break;
        }
        break;
    case LCD_SCEAN_SET_SELECT_HEATLVL:
        switch(key){
            case KEY1:
                scean=LCD_SCEAN_SET_SELECT_FANSPD;
                break;
            case KEY2:
                /*
                   |HEATER LVL: --#        |
                   | CHANGE        CANCEL  |
                 */
                scean=LCD_SCEAN_SET_HEATERLVL;
                break;
            default:
                break;
        }
        break;
    case LCD_SCEAN_SET_SELECT_FANSPD:
        switch(key){
            case KEY1:
                scean=LCD_SCEAN_SET_SELECT_HEATLVL;
                break;
            case KEY2:
                /*
                   |FAN SPEED:   LOW       |
                   | CHANGE        CANCEL  |
                 */
                scean=LCD_SCEAN_SET_FANSPEED;
                break;
            default:
                break;
        }
        break;
    case LCD_SCEAN_SET_HEATERLVL:
        switch(key){
            case KEY1:
                scean=LCD_SCEAN_SET_HEATERLVL;
                heater_status ++;
                heater_status &= 3;
                break;
            case KEY2:
                scean=LCD_SCEAN_SET_SELECT_HEATLVL;
                break;
            default:
                break;
        }
        break;
    case LCD_SCEAN_SET_FANSPEED:
        switch(key){
            case KEY1:
                scean=LCD_SCEAN_SET_FANSPEED;
                if(fan_status == FAN_SPEED_HIGH)
                      fan_status = FAN_SPEED_LOW;
                else
                      fan_status = FAN_SPEED_HIGH;
                break;
            case KEY2:
                scean=LCD_SCEAN_SET_SELECT_FANSPD;
                break;
            default:
                break;
        }
        break;
    default:
        break;
    }
}

void loop() {
    byte char0,char1;
    static byte cmd,arg,sts;
    byte ret;
    byte key;
    command_processing(CMD_GET_HEATER,0,0);
  //  command_processing(CMD_GET_FANSPEED,0,0);
    update_lcd_machine_status();
    
    /*
     * Check Key status
     */
    if(key=scan_key()){
    /* Any key has been pressed */
    handling_key(key);
    }

#if 0
    /*
     * Clear Input bufer
     */
    while(Serial.available()>0)
        Serial.read();

    Serial.println("Please Input cmd/arg/sts");

    while(Serial.available()<2);
    char0=Serial.read();
    if(char0>='0' || char0<='9')
        char0 -= '0';
    else
        char0 -= 'a';
    char1=Serial.read();
    if(char1>='0' || char1<='9')
        char1 -= '0';
    else
        char1 -= 'a';
    Serial.println(char0);
    Serial.println(char1);
    cmd=(char0<<4|char1);

    while(Serial.available()<2);
    char0=Serial.read();
    if(char0>='0' || char0<='9')
        char0 -= '0';
    else
        char0 -= 'a';
    char1=Serial.read();  
    if(char1>='0' || char1<='9')
        char1 -= '0';
    else
        char1 -= 'a';
    arg=(char0<<4|char1);
    while(Serial.available()<2);
    char0=Serial.read();
    if(char0>='0' || char0<='9')
        char0 -= '0';
    else
        char0 -= 'a';
    char1=Serial.read();
    if(char1>='0' || char1<='9')
        char1 -= '0';
    else
        char1 -= 'a';
    sts=(char0<<4|char1);

    Serial.print("CMD ARG STS = ");

    Serial.print(cmd);
    Serial.print(" ");
    Serial.print(arg);
    Serial.print(" ");
    Serial.print(sts);
    Serial.println(" ");
#else
  static byte count=100 ;
    cmd = 0x80;
    count++;
  
    #if 0
    arg++;
    if(arg>3)
        arg=0;
    #else
        arg=2;
        #endif
    sts=0;
  
#endif 
if(count>2)
    {
   // ret = command_processing(cmd,arg,sts);
    count=0;
      }
      Serial.print( count);
    Serial.print("  Return: ");
    Serial.println(ret );
    update_lcd_machine_status();
    delay(1500);
}
