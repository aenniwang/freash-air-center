#ifndef _FRESHAIR_H_
#define _FRESHAIR_H_

#define PAYLOAD 4

#define POWER_PIN 3
#define HEATER_PIN0 4
#define HEATER_PIN1 5
#define HEATER_PIN2 6

#define CMD_SET_HEATER 0x80
#define CMD_GET_HEATER 0x81
#define CMD_SET_POWER_PERIOD 0x82
#define CMD_GET_POWER_PERIOD 0x83
#define CMD_SET_POWER 0x84
#define CMD_GET_POWER 0x85

#define CMD_STATUS_INVALID 0x5F
#define CMD_STATUS_ERR_TIMEOUT 0xfe
#define CMD_STATUS_OK 0x5a
#define CMD_STATUS_ERR 0xf0

/*Header level*/
#define HEATER_0_SET 0
#define HEATER_1_SET 1
#define HEATER_2_SET 2
#define HEATER_3_SET 3

/* Fan speed */
#define POWER_ON 0
#define POWER_OFF 1

#endif // _FRESHAIR_H_
