/*
 * timer0.h
 *
 *  Created on: ٠٧‏/٠٤‏/٢٠٢٣
 *      Author: lap shop
 */

#ifndef TIMER0_H_
#define TIMER0_H_

/*******************************************************************************
 *                      Functions Prototypes                                   *
 *******************************************************************************/

/* Description:
* The function responsible for trigger the Timer0 with the PWM Mode.
* Setup the PWM mode with Non-Inverting.
* Setup the prescaler with F_CPU/8.
* Setup the compare value based on the required input duty cycle
* Setup the direction for OC0 as output pin through the GPIO driver.
* The generated PWM signal frequency will be 500Hz to control the DC
* Motor speed.
*/
void PWM_Timer0_Start(uint8 duty_cycle);

#endif /* PWM_H_ */
