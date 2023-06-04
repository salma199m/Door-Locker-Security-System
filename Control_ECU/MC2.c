/*
 * MC2.c
 *
 *  Created on: ١٧‏/٠٥‏/٢٠٢٣
 *      Author: Salma Mahmoud
 */


#include "buzzer.h"
#include "dc_motor.h"
#include "external_eeprom.h"
#include "uart.h"
#include "timer1.h"
#include <avr/io.h>
#include "buzzer.h"
#include "twi.h"
#include<util/delay.h>



#define MC_READY 0x01     // when MC1 is ready to receive from MC2
#define STORE_EEPROM 0x02 // when MC1 send password to MC2 to store in eeprom
#define OPEN 0x03         // rotate the motor CW
#define CLOSE 0x04        // rotate the motor ACW
#define WAIT 0xF5         // stop the motor
#define BUZZER_ON 0xF6    // turn on the buzzer
#define BUZZER_OFF 0xF7   // turn off the buzzer

/*******************************************************************************
 *                           Global Variables                                  *
 *******************************************************************************/

/* global variable contain the ticks count of the timer */
unsigned char g_tick = 0;


/* Description:
 * counts the ticks of the timer
 * It is the callback function
 */

void TIMER1_ticks()
{
	g_tick++;
}



int main()
{
	uint8 data; /*data from eeprom*/
	uint8 command; /*command sent by MC1 to MC2 in the UART*/
	uint8 recieved_password[5];/*the received password from MC1*/
	uint8 pass_to_send[5]; /*password loaded from eeprom to send to MC1*/

	DcMotor_Init();
	Buzzer_init();

	/* choose the configuration of UART:
	 * 8-bit data
	 * parity check is disabled
	 * stop bit is one bit
	 * baud rate is 9600
	 */

	UART_ConfigType config1 = {EIGHT, DISABLED, ONE_BIT, BR7};
	UART_init(&config1); /* Initialize UART with the required configurations */

	/* choose the configuration of TIMER1:
	 * initial value is 0
	 * compare value is 999
	 * prescaler is 1024
	 * the mode is compare mode
	 */
	Timer1_ConfigType config2 = {0, 999, F_CPU_1024, CTC};
	Timer1_init(&config2); /* Initialize TIMER1 with the required configurations */



	Timer1_setCallBack(TIMER1_ticks); /* set the TIMER1_ticks to be the callback function */

	TWI_ConfigType TWI_config = { SLAVE1, NORMAL_MODE };
	TWI_init(&TWI_config);

	while(1)
	{
		command = UART_recieveByte(); /* receive command from MC1 through the UART*/

		if (command == STORE_EEPROM) {
			for (int i = 0; i < 5; i++) {
				recieved_password[i] = UART_recieveByte(); /* take the password from MC1  */
			}
			for (int i = 0; i < 5; i++) {
				EEPROM_writeByte(0x0311 + i, recieved_password[i]);/* store this password in eeprom starting from address 0x0311 */
				_delay_ms(10);
			}
			_delay_ms(100);
			for (int i = 0; i < 5; i++) {
				EEPROM_readByte(0x0311 + i, &data); /* read the password from eeprom */
				_delay_ms(10);
				pass_to_send[i] = data; /* store it in array to send it to MC1 */
			}
			while (UART_recieveByte() != MC_READY) { /* wait until MC1 is ready to receive password */
			};
			for (int i = 0; i < 5; i++) {
				UART_sendByte(pass_to_send[i]);
				_delay_ms(100);
			}
		}
		else if (command == BUZZER_ON) { /* if received command from MC1 to turn on the buzzer */
			Buzzer_on();
			while (UART_recieveByte() != BUZZER_OFF) { /* wait until MC1 send a command to turn off buzzer */
			};
			Buzzer_off();
		}
		else if (command == OPEN) { /* if received from MC1 to open the door */
			DcMotor_Rotate(CW, 100); /* rotate the motor CW */
			g_tick = 0;
			while (g_tick <= 15)
				;
		}
		else if (command == WAIT) { /* leave the door open  */
			DcMotor_Rotate(STOP, 0); /* stop the motor */
			while (g_tick <= 18)
				;
		}
		else if (command == CLOSE) { /* if received from MC1 to close the door */
			DcMotor_Rotate(A_CW, 100); /* rotate the motor ACW */
						while (g_tick <= 33)
							;

						g_tick = 0;

			DcMotor_Rotate(STOP, 0); /* Stop the motor to not let it always run  */
		}
	}

}













