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
//LiquidCrystal lcd(9, 10, 5, 4, 3, 2);
LiquidCrystal lcd(14,10, 2, 3, 4, 5);

#define PIN_KEY1 17
#define PIN_KEY2 16
#define LCD_BACKLIGH 6

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

#define KEY1 1 
#define KEY2 2 
#define KEY12 3

byte scean ;
unsigned char heater_status;
unsigned char fan_status;
unsigned char comm_err;
byte blklight_count;
byte timer_issue_cmd; 

void setup_24l01(){
    Mirf.spi = &MirfHardwareSpi;
    Mirf.init();
    Mirf.configRegister(RF_SETUP,0x27);//250Kbps
    Mirf.setRADDR((byte *)"clie1");
    Mirf.payload = PAYLOAD;
    Mirf.channel = 39;
    Mirf.config();
}

void update_lcd_machine_status(){
    // Print a message to the LCD.
    lcd.setCursor(0,0);

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

void update_lcd_status(){
  switch(scean){
      case LCD_SCEAN_MAIN: 
          lcd.setCursor(0,0);
          switch(heater_status){
              case HEATER_1_SET:
                  lcd.print("   --#  ");
                  break;
              case HEATER_2_SET:
                  lcd.print("   -##  ");
                  break;
              case HEATER_3_SET:
                  lcd.print("   ###  ");
                  break;
              default:
                  lcd.print("   ---  ");
                  break;
          }
          lcd.setCursor(8,0);
          if(fan_status == FAN_SPEED_HIGH)
              lcd.print("  HIGH  ");
          else
              lcd.print("  LOW   ");

          lcd.setCursor(0,1);
          lcd.print("            SET ");
          break;

      case LCD_SCEAN_SET_SELECT_HEATLVL:
          lcd.setCursor(0,0);
          lcd.print(" set heater lvl ");

          lcd.setCursor(0,1);
          lcd.print(" NEXT        OK ");
          break;

        case LCD_SCEAN_SET_SELECT_FANSPD:
          lcd.setCursor(0,0);
          lcd.print(" set fan speed  ");

          lcd.setCursor(0,1);
          lcd.print(" NEXT        OK ");
          break;

    case LCD_SCEAN_SET_HEATERLVL:
          lcd.setCursor(0,0);
          lcd.print("HEAT LVL: ");
          lcd.setCursor(10,0);
          switch(heater_status){
              case HEATER_1_SET:
                  lcd.print("   --#  ");
                  break;
              case HEATER_2_SET:
                  lcd.print("   -##  ");
                  break;
              case HEATER_3_SET:
                  lcd.print("   ###  ");
                  break;
              default:
                  lcd.print("   ---  ");
                  break;
          }
          lcd.setCursor(0,1);
          lcd.print(" MAIN       CHG ");
          break;

    case LCD_SCEAN_SET_FANSPEED:
          lcd.setCursor(0,0);
          lcd.print("FAN SPD: ");
          lcd.setCursor(9,0);
          if(fan_status == FAN_SPEED_HIGH)
              lcd.print("  HIGH  ");
          else
              lcd.print("  LOW   ");

          lcd.setCursor(0,1);
          lcd.print(" MAIN       CHG ");
          break;
    default:
           break;
  }

  if(blklight_count>100){
      blklight_count =0;
      digitalWrite(LCD_BACKLIGH,LOW);
      scean = LCD_SCEAN_MAIN;
  }
}


void setup_1602() {
    // set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
    pinMode(LCD_BACKLIGH,OUTPUT);
    digitalWrite(LCD_BACKLIGH,HIGH);
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
    pinMode(PIN_KEY1,INPUT_PULLUP);
    pinMode(PIN_KEY2,INPUT_PULLUP);
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
    timer_issue_cmd=255;
    blklight_count = 0;
}

void reset(){
    fan_status = FAN_SPEED_LOW;
    heater_status = HEATER_0_SET;
    Serial.println("Setup 24L01");
    setup_24l01();
    Serial.println("Setup Keyboard");
    setup_keyboard();
    heater_status = 0;
    fan_status=0;
    comm_err = 1;
    timer_issue_cmd=255;
    blklight_count = 0;
}
/* Show 
    |FAN_SPEED  HEATER Level|
    |  SET                  |
    if 5 seconds no action on KEY, return to this scean
*/
byte scan_key(){
    byte key=0;
//#define KEY_FROM_UART 1
#ifdef KEY_FROM_UART
    while(Serial.available()>0)
        Serial.read();

    Serial.print("Key (1-KEY1, 2-KEY2): ");

    while(Serial.available()<=0);
    key=Serial.read();
    if(key>='0' && key<='2')
        key-= '0';
    else
        key= 0;

    if(key==KEY1)
        Serial.println("KEY1");
    else if(key==KEY2)
        Serial.println("KEY2");
    else
        Serial.println("UNKnown KEY");
    return key;
#else
    if(digitalRead(PIN_KEY2)==LOW){
        delay(10);
        if(digitalRead(PIN_KEY2)==LOW){
            delay(10);
            if(digitalRead(PIN_KEY2)==LOW){
                key=KEY2;
            }
        }
    }
    if(digitalRead(PIN_KEY1)==LOW){
        delay(10);
        if(digitalRead(PIN_KEY1)==LOW){
            delay(10);
            if(digitalRead(PIN_KEY1)==LOW){
                key=KEY1;
            }
        }
    }
  //  delay(100);
    //Serial.print("KEY");
    //Serial.println(key);
    return key;
#endif
}

void handling_key(byte key){
    switch(scean){
    /*
        |FAN_SPEED  HEATER Level|
        |                SET    |
    */
    case LCD_SCEAN_MAIN:
        switch(key){
            case KEY1:
                break;

            case KEY2:
                /*
                   | HEATER LVL  FAN SPD   |
                   |    NEXT          OK   |
                 */
                scean=LCD_SCEAN_SET_SELECT_HEATLVL;
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
                scean=LCD_SCEAN_SET_FANSPEED;
                break;
            default:
                break;
        }
        break;

    case LCD_SCEAN_SET_HEATERLVL:
        switch(key){
            case KEY1:
                scean=LCD_SCEAN_MAIN;
                break;
            case KEY2:
                scean=LCD_SCEAN_SET_HEATERLVL;
                heater_status ++;
                heater_status &= 3;
                command_processing(CMD_SET_HEATER,heater_status,0);
                break;
            default:
                break;
        }
        break;
    case LCD_SCEAN_SET_FANSPEED:
        switch(key){
            case KEY1:
                scean=LCD_SCEAN_MAIN;
                break;
            case KEY2:
                scean=LCD_SCEAN_SET_FANSPEED;
                if(fan_status == FAN_SPEED_HIGH)
                      fan_status = FAN_SPEED_LOW;
                else
                      fan_status = FAN_SPEED_HIGH;

                command_processing(CMD_SET_FANSPEED,fan_status,0);
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
    static  byte error_count;
    byte key;

    if(timer_issue_cmd > 15){
        timer_issue_cmd = 0;
        command_processing(CMD_GET_HEATER,0,0);
        command_processing(CMD_GET_FANSPEED,0,0);
    }

    /*
     * Check Key status
     */
     key=scan_key();
    if(key){
        /* Any key has been pressed */
        handling_key(key);
        digitalWrite(LCD_BACKLIGH, HIGH);
        blklight_count= 0;
        timer_issue_cmd=255;
    }

    update_lcd_status();

    if(comm_err){
        error_count++;
        lcd.setCursor(0,0);
        lcd.print("-  Link Error  -");
        delay(1500);
        if(error_count > 5){
            lcd.setCursor(0,1);
            lcd.print(" Reseting Board ");
            reset();
            delay(2000);
            error_count=0;
        }
        return;
    }
    blklight_count ++;
    
    timer_issue_cmd ++;
    delay(100);
}
