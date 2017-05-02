// dac.c
// This software configures DAC output
// Runs on LM4F120 or TM4C123
// Program written by: put your names here
// Date Created: 3/6/17 
// Last Modified: 3/6/17 
// Lab number: 6
// Hardware connections
// TO STUDENTS "REMOVE THIS LINE AND SPECIFY YOUR HARDWARE********

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "DAC.h"
// Code files contain the actual implemenation for public functions
// this file also contains an private functions and private data

// **************DAC_Init*********************
// Initialize 4-bit DAC, called once 
// Input: none
// Output: none


void DAC_Init(void){volatile uint32_t delay;
	SYSCTL_RCGCGPIO_R |= 0x02; //Activate clock for Port B
	delay = 0x30;
	GPIO_PORTB_AMSEL_R &= ~0x0F; //Turn off analog function
	GPIO_PORTB_DIR_R |= 0x0F; //Bits 0-3 outputs
	GPIO_PORTB_AFSEL_R = 0; //Turn off alt function
	GPIO_PORTB_DEN_R |= 0x0F; //Enable bits 0-3
}

// **************DAC_Out*********************
// output to DAC
// Input: 4-bit data, 0 to 15 
// Output: none
void DAC_Out(uint8_t data){
	
	GPIO_PORTB_DATA_R = data;
	
}

