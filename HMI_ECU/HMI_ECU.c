/*
 * HMI_ECU.c
 *
 *  Created on: ١٨‏/٠٥‏/٢٠٢٣
 *      Author: Salma Mahmoud
 */

#include "std_types.h"
#include "keypad.h"
#include "common_macros.h"
#include "uart.h"
#include "timer1.h"
#include "lcd.h"
#include<util/delay.h>




#define MC_READY 0x01     // when MC1 is ready to receive from MC2
#define STORE_EEPROM 0x02 // when MC1 send password to MC2 to store in eeprom
#define OPEN 0x03         // rotate the motor CW
#define CLOSE 0x04        // rotate the motor ACW
#define WAIT 0xF5         // stop the motor
#define BUZZER_ON 0xF6    // turn on the buzzer
#define BUZZER_OFF 0xF7   // turn off the buzzer

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

void enterpass(uint8 *pass)
{
	uint8 key_num;

	for (int i = 0; i < 5; i++) {
		key_num = KEYPAD_getPressedKey();
		if ((key_num >= 0) && (key_num <= 9)) {
			pass[i] = key_num;
			LCD_displayCharacter('*'); /* display the pressed keypad switch */
		}
		_delay_ms(500);
	}
}

void sendpass(uint8 *pass)
{

	for (int i = 0; i < 5; i++) {

		UART_sendByte(pass[i]);
		_delay_ms(500);
	}


}


void newpass(uint8 matched_flag1,uint8 *pass1,uint8 *pass2)
{
	uint8 step1=1;
	while(step1)
	{
		LCD_clearScreen();
		LCD_displayString("Plz enter pass:");
		LCD_moveCursor(1, 0);
		enterpass(pass1);
		if(KEYPAD_getPressedKey() == '=')
		{
			LCD_clearScreen();
			UART_sendByte(STORE_EEPROM); /*       mc1 send pass to mc2 to store in eeprom         */
			sendpass(pass1);
		}
		LCD_clearScreen();
		LCD_displayString("Reenter password:");
		LCD_moveCursor(1, 0);
		enterpass(pass2);

		if (KEYPAD_getPressedKey() == '=') {
			LCD_clearScreen();
			for (int i = 0; i < 5; i++) {
				if (pass1[i] == pass2[i]) {
					matched_flag1 = 0;
				} else {
					matched_flag1 = 1;
					break;
				}
			}
			if (matched_flag1 == 0) {
				LCD_displayString("Saved");
				_delay_ms(500);
				LCD_clearScreen();
				step1=0;
			} else {
				LCD_clearScreen();
				LCD_displayString("Wrong password");
				_delay_ms(500);
				LCD_clearScreen();
			}

		}

	}

}

void BUZZER()
{
	UART_sendByte(BUZZER_ON); /* a command to make the CONTROL_ECU ready for using the buzzer */
	LCD_clearScreen();
	LCD_displayString("ERROR!!!!");
	g_tick = 0; /* begin the counts again to get the needed 60 seconds (1 minute) */
	while (g_tick <= 60)
		;		/* wait until the 60 seconds more are over */
	g_tick = 0; /* make the counter zero again as it finished */
	LCD_clearScreen();
	UART_sendByte(BUZZER_OFF);
}


int main()
{
	uint8 password1[5];
	uint8 password2[5];
	uint8 eeprom_password[5];
	uint8 old_password[5];     /*old password entered by the user*/
	uint8 matched_flag1=0;
	uint8 matched_flag2=0;
	uint8 error_flag=0;
	uint8 correct_flag=0;

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

	LCD_init();							  /* Initialize the LCD */

	newpass(matched_flag1,password1,password2);
	matched_flag1=0;

	/* receive password from eeprom interfaced with MC2  */
	UART_sendByte(MC_READY);
	for (int i = 0; i < 5; i++) {
		eeprom_password[i] = UART_recieveByte();
		_delay_ms(100);
	}

	while(1)
	{

		/* Display the main options on the screen */

		LCD_displayStringRowColumn(0, 0, "+ : Open Door");
		LCD_displayStringRowColumn(0, 13, "   ");
		LCD_displayStringRowColumn(1, 0, "- : Change Pass");
		LCD_displayStringRowColumn(1, 15, "  ");

		if (KEYPAD_getPressedKey() == '+')
		{
			while ((error_flag < 3) && (correct_flag == 0))
			{
				LCD_clearScreen();
				LCD_displayString("enter password :");
				LCD_moveCursor(1, 0);
				enterpass(old_password);
				if ((KEYPAD_getPressedKey()) == '=') {
					for (int i = 0; i < 5; i++) {
						if (old_password[i] == eeprom_password[i]) {
							matched_flag2 = 0;
						} else {
							matched_flag2 = 1;
							break;
						}
					}
					if (matched_flag2 == 0)
					{

						LCD_clearScreen();
						LCD_displayStringRowColumn(0, 0, "Door is");
						LCD_displayStringRowColumn(1, 0, "Unlocking");
						UART_sendByte(OPEN);     /* a command to make the CONTROL_ECU ready for using the door(motor) */

						g_tick = 0; /* begin the counts again to get the needed 15 seconds */

						while (g_tick <= 15)
							; /* wait until the 15 seconds are over */
						LCD_clearScreen();
						LCD_displayString("WARNING..."); /* warning to warn the user that the door will close in 3 seconds */
						UART_sendByte(WAIT);

						while (g_tick <= 18)
							; /* wait until the 3 seconds more are over */

						LCD_clearScreen();
						LCD_displayStringRowColumn(0, 0, "Door is");
						LCD_displayStringRowColumn(1, 0, "Locking");
						UART_sendByte(CLOSE);
						while (g_tick <= 33)
							; /* wait until the 15 seconds more are over */

						g_tick = 0; /* make the counter zero again as it finished */
						LCD_clearScreen();
						correct_flag = 1;
					}
					else{
						LCD_clearScreen();
						LCD_displayString("wrong password");
						_delay_ms(400);
						error_flag++;
						if (error_flag == 3){
							BUZZER();
						}
					}
				}
			}
			matched_flag2 = 0;
			error_flag = 0;
			correct_flag = 0;
		}

		else if (KEYPAD_getPressedKey() == '-')
		{
			while ((error_flag < 3) && (correct_flag == 0))          /* we can enter wrong password 3 times only */
			{
				LCD_clearScreen();
				LCD_displayString("enter old password:");
				LCD_moveCursor(1, 0);
				enterpass(old_password);
				if ((KEYPAD_getPressedKey()) == '=') {
					for (int i = 0; i < 5; i++) {
						if (old_password[i] == eeprom_password[i]) {
							matched_flag2 = 0;
						} else {
							matched_flag2 = 1;
							break;
						}
					}
					if (matched_flag2 == 0) {
						LCD_clearScreen();
						LCD_displayString("correct password");
						_delay_ms(400);
						LCD_clearScreen();
						newpass(matched_flag1, password1,password2);
						matched_flag1 = 0;
						UART_sendByte(MC_READY);
						for (int i = 0; i < 5; i++) {
							eeprom_password[i] = UART_recieveByte();
						}
						correct_flag = 1;
					}
					else{
						LCD_clearScreen();
						LCD_displayString("wrong password");
						_delay_ms(400);
						error_flag++;
						if (error_flag == 3){
							BUZZER();
						}
					}
				}
				;
			}
			matched_flag2 = 0;
							error_flag = 0;
							correct_flag = 0;
		}/* end of else if */
	}
}
















