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
#ifdef ENABLE_WDT
#include <avr/wdt.h>
#endif
#include <avr/power.h>

// initialize the library with the numbers of the interface pins
//LiquidCrystal lcd(9, 10, 5, 4, 3, 2);
LiquidCrystal lcd(14,10, 2, 3, 4, 5);

#define PIN_KEY1 17
#define PIN_KEY2 16
#define LCD_BACKLIGH 6

#define LCD_SCEAN_MAIN 0
/*
    |### OFF 50/50 10 |
    |            SET  |
*/

/*  if HEATER_LVL chosen
    |HEATER LVL: --#        |
    | CHANGE        CANCEL  |
    if CANCEL goto LCD_SCEAN_SET_SELECT
*/
#define LCD_SCEAN_SET_HEATERLVL 3
#define LCD_SCEAN_SET_FANSPEED 4 
#define LCD_SCEAN_SET_POWER 5
#define LCD_SCEAN_SET_POWER_PERIOD_ON 6
#define LCD_SCEAN_SET_POWER_PERIOD_OFF 7

#define KEY1 1 
#define KEY2 2 
#define KEY12 3

byte scean ;
unsigned char heater_status;
unsigned char power_status;
unsigned short power_period_on;
unsigned short power_period_off;
unsigned short power_timer;

unsigned char comm_err;
byte blklight_count;
byte timer_issue_cmd; 

#ifdef ENABLE_WDT
ISR(WDT_vect){
  setup();
  loop();
}

void setup_wdt(){
  wdt_enable(WDTO_4S);      
  WDTCSR |= _BV(WDIE);  
}
#endif

void setup_24l01(){

    Mirf.spi = &MirfHardwareSpi;
    Mirf.init();
    Mirf.configRegister(RF_SETUP,0x27);//250Kbps
    Mirf.setRADDR((byte *)"clie1");
    Mirf.payload = PAYLOAD;
    Mirf.channel = 39;
    Mirf.config();
}

void update_lcd_status(){
  switch(scean){
      case LCD_SCEAN_MAIN: 
    /*
    |### OFF 50/50 10 |
    |            SET  |
    */

          lcd.setCursor(0,0);
          switch(heater_status){
              case HEATER_1_SET:
                  lcd.print("--#  ");
                  break;
              case HEATER_2_SET:
                  lcd.print("-##  ");
                  break;
              case HEATER_3_SET:
                  lcd.print("###  ");
                  break;
              case HEATER_0_SET:
                  lcd.print("---  ");
                  break;
                  default:
                  lcd.print("FALT");
          }

          lcd.setCursor(4,0);
          switch(power_status) {
            case POWER_ON:
                lcd.print(" ON ");
                break;
            case POWER_OFF:
                lcd.print("OFF ");
                break;
            default:
                lcd.print("ERR ");
          }

          lcd.setCursor(8,0);
	  if (power_period_on>100)
		  lcd.print("EE");
	  else
		  lcd.print(power_period_on);

	  lcd.print("/");

	  if (power_period_off>100)
		  lcd.print("EE");
	  else
		  lcd.print(power_period_off);

	  lcd.print(" ");

	  if (power_timer>100)
		  lcd.print("EE");
	  else
		  lcd.print(power_timer);
    lcd.print(" ");
    
          lcd.setCursor(0,1);
          lcd.print("            SET ");
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
          lcd.print(" N:POWER    CHG ");
          break;

    case LCD_SCEAN_SET_POWER:
          lcd.setCursor(0,0);
          lcd.print("POWER:");
          lcd.setCursor(8,0);

          switch(power_status) {
            case POWER_ON:
                lcd.print("      ON");
                break;
            case POWER_OFF:
                lcd.print("     OFF");
                break;
            default:
                lcd.print("     ERR");
          }

          lcd.setCursor(0,1);
          lcd.print(" N:TM_ON    CHG ");

          break;

    case LCD_SCEAN_SET_POWER_PERIOD_ON:
          lcd.setCursor(0,0);
          lcd.print("PERIOD ON: ");
          lcd.setCursor(14,0);

          if (power_period_on>100)
              lcd.print("EE");
          else
              lcd.print(power_period_on);

          lcd.setCursor(0,1);
          lcd.print(" N:TM_OF    CHG ");
          break;

    case LCD_SCEAN_SET_POWER_PERIOD_OFF:
          lcd.setCursor(0,0);
          lcd.print("PERIOD OFF: ");
          lcd.setCursor(14,0);

          if (power_period_off>100)
              lcd.print("EE");
          else
              lcd.print(power_period_off);

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
        Serial.flush();
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

    if(data[0] == CMD_GET_HEATER ||
    data[0] == CMD_GET_POWER ||
    data[0] == CMD_GET_POWER_PERIOD_ON ||
    data[0] == CMD_GET_POWER_PERIOD_OFF ||
    data[0] == CMD_GET_POWER_TIMER)
       data[1]=data_ret[1];

    Mirf.flushRx();
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

	case CMD_SET_POWER:
	case CMD_GET_POWER:
            ret = dev24l01_send_cmd(data);
	    if (ret == CMD_STATUS_OK)
	        power_status = data[1];
	    break;

	case CMD_GET_POWER_PERIOD_ON:
	case CMD_SET_POWER_PERIOD_ON:
            ret = dev24l01_send_cmd(data);
	    if (ret == CMD_STATUS_OK)
	        power_period_on = data[1];
	    break;

	case CMD_GET_POWER_PERIOD_OFF:
	case CMD_SET_POWER_PERIOD_OFF:
            ret = dev24l01_send_cmd(data);
	    if (ret == CMD_STATUS_OK)
	        power_period_off = data[1];
	    break;

	case CMD_GET_POWER_TIMER:
            ret = dev24l01_send_cmd(data);
	    if (ret == CMD_STATUS_OK)
	        power_timer= data[1];
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
    power_status = 0xff;
    heater_status = 0xff;
    power_period_on = 0xffff;
    power_period_off = 0xffff;
    power_timer = 0xffff;

    setup_1602();
    Serial.println("Setup 24L01");
    setup_24l01();
    Serial.println("Setup Keyboard");
    setup_keyboard();
    heater_status = 0;
    comm_err = 1;
    timer_issue_cmd=255;
    blklight_count = 0;
#ifdef ENABLE_WDT
    setup_wdt();
#endif
}

void reset(){
    power_status = 0xff;
    heater_status = 0xff;
    power_period_on = 0xffff;
    power_period_off = 0xffff;
    power_timer = 0xffff;

    Serial.println("Setup 24L01");
    setup_24l01();
    Serial.println("Setup Keyboard");
    setup_keyboard();
    heater_status = 0;
    comm_err = 1;
    timer_issue_cmd=255;
    blklight_count = 0;
#ifdef ENABLE_WDT
        setup_wdt();
#endif
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
                    scean=LCD_SCEAN_SET_HEATERLVL;
                    break;
                default:
                    break;
            }

            break;

        case LCD_SCEAN_SET_HEATERLVL:
            switch(key){
                case KEY1:
                    scean=LCD_SCEAN_SET_POWER;
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

        case LCD_SCEAN_SET_POWER:
            switch(key){
                case KEY1:
                    scean=LCD_SCEAN_SET_POWER_PERIOD_ON;
                    break;
                case KEY2:
                    scean=LCD_SCEAN_SET_POWER;
                    power_status = !power_status;
                    command_processing(CMD_SET_POWER,power_status,0);
                    break;
            }
            break;

        case LCD_SCEAN_SET_POWER_PERIOD_ON:
            switch(key){
                case KEY1:
                    scean=LCD_SCEAN_SET_POWER_PERIOD_OFF;
                    break;
                case KEY2:
                    scean=LCD_SCEAN_SET_POWER_PERIOD_ON;
                    power_period_on = power_period_on/5;
                    power_period_on++;
                    power_period_on *= 5;
                    if (power_period_on > 50)
                        power_period_on = 5;
                    command_processing(CMD_SET_POWER_PERIOD_ON,power_period_on,0);
                    break;
            }
            break;

        case LCD_SCEAN_SET_POWER_PERIOD_OFF:
            switch(key){
                case KEY1:
                    scean=LCD_SCEAN_MAIN;
                    break;
                case KEY2:
                    scean=LCD_SCEAN_SET_POWER_PERIOD_OFF;
                    power_period_off = power_period_off/5;
                    power_period_off++;
                    power_period_off *= 5;
                    if (power_period_off > 50)
                        power_period_off = 5;

                    command_processing(CMD_SET_POWER_PERIOD_OFF,power_period_off,0);
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
#ifdef ENABLE_WDT
    wdt_reset();
#endif

    if(timer_issue_cmd > 15){
        timer_issue_cmd = 0;
        command_processing(CMD_GET_HEATER,0,0);
        command_processing(CMD_GET_POWER,0,0);
        command_processing(CMD_GET_POWER_PERIOD_ON,0,0);
        command_processing(CMD_GET_POWER_PERIOD_OFF,0,0);
        command_processing(CMD_GET_POWER_TIMER,0,0);
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
            delay(1000);
            error_count=0;
        }
        return;
    }
    blklight_count ++;

    timer_issue_cmd ++;
    delay(100);
}
