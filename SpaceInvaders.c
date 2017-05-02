// SpaceInvaders.c
// Runs on LM4F120/TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the EE319K Lab 10

// Last Modified: 3/6/2015 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014

   "Embedded Systems: Introduction to Arm Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2014

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PE2/AIN1
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5



#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "ST7735.h"
#include "Random.h"
#include "TExaS.h"
#include "ADC.h"
#include "Timer0.h"
#include "Sound.h"
#include "Print.h"
#include "Timer1.h"
#include "DAC.h"




void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds

//Global

	uint32_t data;

struct Player{
	int8_t xpos; 
	int8_t velx;
	uint8_t lives;
	uint8_t alive; 
	uint16_t score; 
	uint8_t level;
	uint8_t clears;
};

typedef struct Player Player;

struct Multi{
	uint8_t xpos;
	uint8_t Lflag;


};
typedef struct Multi Multi;


struct Enemy{
	uint8_t xpos[14]; 
	uint8_t ypos[14];  
	int8_t dead; 
	uint8_t deadx[14];
	uint8_t deady[14];
	int8_t expflag;
	
	const uint16_t *picture; 
};
typedef struct Enemy Enemy; 

struct Bullet{
	uint8_t xpos;
	int16_t ypos;
	uint8_t Dflag;
	uint8_t Bflag;
	uint8_t EDflag;
	uint8_t EBflag;
	uint8_t expos;
	int16_t eypos;
};
typedef struct Bullet Bullet;

static Player player;
static Enemy enemy;
static Bullet bullet;
static Multi multi;
//static Bullet bullet;


void SysTick_Init(void){
	NVIC_ST_CTRL_R = 0; //Turn off while setting up
	NVIC_ST_RELOAD_R = 2000000; // 40 Hz
	NVIC_ST_CURRENT_R = 0; //clear current to reset 
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0x00FFFFFF) | 0x40000000; //Set priority to 2
	NVIC_ST_CTRL_R = 0x00000007; //Enable systick clock with interrupts
}

void PortF_Init(void){volatile uint32_t delay;
// Intialize PortF for hearbeat
	SYSCTL_RCGCGPIO_R |= 0x20;//F
	delay = 0x30;
	GPIO_PORTF_LOCK_R = 0x4C4F434B;
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF0
	GPIO_PORTF_AMSEL_R = 0;
	GPIO_PORTF_AFSEL_R = 0;
	GPIO_PORTF_DIR_R &=  ~0x11; //Input for PF1
	GPIO_PORTF_DIR_R |=  0x02; //Heartbeat for PF1
	GPIO_PORTF_DEN_R |= 0x13;
	GPIO_PORTF_PCTL_R &= ~0x000F0000; //  configure PF4 as GPIO
	GPIO_PORTF_PUR_R |= 0x11;
	
	GPIO_PORTF_IS_R &= ~0x11;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x11;    //     PF4 is not both edges
  GPIO_PORTF_IEV_R &= ~0x11;    //     PF4 falling edge event
  GPIO_PORTF_ICR_R = 0x11;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x11;      // (f) arm interrupt on PF4
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
}




// *************************** Images ***************************
// enemy ship that starts at the top of the screen (arms/mouth closed)
// width=16 x height=10
const unsigned short SmallEnemy10pointA[] = {
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000,
 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

// enemy ship that starts at the top of the screen (arms/mouth open)
// width=16 x height=10
/*const unsigned short SmallEnemy10pointB[] = {
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
 0x0000, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000,
 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
*/

// enemy ship that starts in the middle of the screen (arms together)
// width=16 x height=10
const unsigned short SmallEnemy20pointA[] = {
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

// enemy ship that starts in the middle of the screen (arms apart)
// width=16 x height=10




// enemy ship that starts at the bottom of the screen (arms down)
// width=16 x height=10
const unsigned short SmallEnemy30pointA[] = {
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

// enemy ship that starts at the bottom of the screen (arms up)
// width=16 x height=10


//Lazer to fire
//Width = 1. Height = 10
const unsigned short Lazer[] = {
 0x0000,0x0000,0x0000,0xED00, 0xED00, 0xED00, 0xED00
};
const unsigned short ELazer[] = {
 0x20FD, 0x20FD, 0x20FD, 0x20FD,0x0000, 0x0000, 0x0000
};




//ramesh = 73x58

//15x15
const unsigned short Player0[] = {
 0x0000, 0x0000, 0x0000, 0x3186, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2945, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0xDA40, 0x0000, 0x3186, 0x0000, 0x0000, 0x0000, 0x2104, 0x0000, 0xD240, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x528A, 0xE260, 0x6143, 0x6143, 0x0000, 0x0000, 0x0000, 0x7163, 0x69C5, 0xDA40, 0x632C, 0x0000, 0x0000, 0x0000, 0x0000, 0xFAA0,
 0x99A0, 0x61E6, 0x2840, 0x0000, 0x0000, 0x0000, 0x2820, 0x6A07, 0x8960, 0xF327, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFBCB,
 0x6A07, 0x99A0, 0x0000, 0x39C7, 0x0000, 0x8960, 0x6A28, 0xDA40, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x39C7, 0x8160,
 0x7C10, 0x18C3, 0xAD75, 0x10A2, 0xFC0C, 0x8180, 0x39C7, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7BEF, 0xA534,
 0x18C3, 0x5AEB, 0x28A1, 0xA514, 0x8410, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x8430, 0xB5B6, 0x0000,
 0xFAC0, 0xBA00, 0xB5B6, 0x3186, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4208, 0x0000, 0x39E7,
 0x0000, 0x4A69, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xDC0E, 0x0000, 0x9180, 0x0000,
 0xECD1, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xCC91, 0x0000, 0x9A25, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2124, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x528A, 0x0000, 0x39C7, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x31A6, 0x0000, 0x31A6, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000,
};







//All explosions are 16x16 to cover enemies or player?
const unsigned short Explosion0[] = {
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0003, 0x0004, 0x0000, 0x0800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0001, 0x13B4, 0x0339, 0x12DC, 0x000B, 0x00AB, 0x006B, 0x0003, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x08A6, 0x092C, 0x01AF, 0x1CBF, 0x03DB, 0x073F, 0x16DF, 0x03BD, 0x0008, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x1000, 0x0049, 0x0315, 0x05BD, 0x077F, 0x07FF, 0x0FFF, 0x07BF, 0x065F, 0x02BB, 0x086D, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x022D, 0x17DF, 0x0F9F, 0x17FF, 0x07FF, 0x07FF, 0x0FFF, 0x077F, 0x05DF, 0x0131, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0020, 0x0000, 0x0271, 0x06BF, 0x07FF, 0x07FF, 0x0FFF, 0x0FFF, 0x07FF, 0x07BF, 0x171F, 0x139A, 0x0003, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0001, 0x1E3E, 0x0FFF, 0x07FF, 0x07FF, 0x1FFF, 0x57FF, 0x4FDF, 0x07FF, 0x07DF, 0x067F, 0x10B0, 0x0000, 0x0000, 0x0000,
 0x0800, 0x01E8, 0x1EBF, 0x07DF, 0x07FF, 0x0FFF, 0x67FF, 0x0FFF, 0x77FF, 0x0FBF, 0x079F, 0x0D9F, 0x0070, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0024, 0x0335, 0x0F5F, 0x07FF, 0x0FDF, 0x1FBF, 0x1FBF, 0x07DF, 0x0FDF, 0x179F, 0x157F, 0x002B, 0x0823, 0x0000, 0x0000,
 0x0000, 0x214C, 0x006D, 0x15BD, 0x0FDF, 0x07FF, 0x07FF, 0x07FF, 0x0FBF, 0x0FFF, 0x071F, 0x00D0, 0x000B, 0x1046, 0x0000, 0x0000,
 0x0001, 0x09D0, 0x1172, 0x02B3, 0x0F7F, 0x179F, 0x079F, 0x077F, 0x077F, 0x0F1F, 0x0B1A, 0x0050, 0x0010, 0x0007, 0x0000, 0x0001,
 0x0800, 0x098E, 0x0956, 0x0067, 0x0049, 0x0299, 0x04DC, 0x03FC, 0x037C, 0x00EE, 0x0008, 0x0174, 0x0073, 0x0975, 0x0002, 0x0020,
 0x0800, 0x0000, 0x0003, 0x0000, 0x0040, 0x0004, 0x00A9, 0x137D, 0x1235, 0x0069, 0x0007, 0x08F3, 0x082D, 0x0008, 0x0001, 0x0002,
 0x0001, 0x0020, 0x0020, 0x0000, 0x0000, 0x0800, 0x0000, 0x0007, 0x0069, 0x110D, 0x08AE, 0x0971, 0x0002, 0x0020, 0x0020, 0x0022,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x0800, 0x0040, 0x0000, 0x0001, 0x0000, 0x0800, 0x0000, 0x0000
};




const unsigned short Blank16[] = {
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};








void GameWinPlayer1(void){
	
	NVIC_ST_CTRL_R = 0;
	ST7735_DrawBitmap(multi.xpos, 10 ,Explosion0, 16,16); // Explosion
	Timer1A_Set(&Sound_Explosion);
	while(TIMER1_CTL_R == 1){} // wait until sound over
	
	
	
	ST7735_FillScreen(0x0000);            // set screen to black
  ST7735_SetCursor(1, 1);
  ST7735_OutString("Congratulations!");
  ST7735_SetCursor(1, 2);
  ST7735_OutString("You Have Won!");
  ST7735_SetCursor(1, 3);
  ST7735_OutString("Player 1!");
  ST7735_SetCursor(2, 4);
	while(1){};
}

void GameWinPlayer2(void){
	NVIC_ST_CTRL_R = 0;
	ST7735_DrawBitmap(player.xpos, 159 ,Explosion0, 16,16); // Explosion
	Timer1A_Set(&Sound_PExplode);
	while(TIMER1_CTL_R == 1){} // wait until sound over
		
	ST7735_FillScreen(0x0000);            // set screen to black
  ST7735_SetCursor(1, 1);
  ST7735_OutString("Congratulations!");
  ST7735_SetCursor(1, 2);
  ST7735_OutString("You Have Won!");
  ST7735_SetCursor(1, 3);
  ST7735_OutString("Player2!");
  ST7735_SetCursor(2, 4);
	while(1){};



}
void GameOver(void){
  ST7735_FillScreen(0x0000);            // set screen to black
  ST7735_SetCursor(1, 1);
  ST7735_OutString("GAME OVER");
  ST7735_SetCursor(1, 2);
  ST7735_OutString("Nice try,");
  ST7735_SetCursor(1, 3);
  ST7735_OutString("Earthling!");
  ST7735_SetCursor(1, 4);
	ST7735_OutString("Your Final Score: ");
	ST7735_SetCursor(1, 5);
	ST7735_OutUDec(player.score);

	while(1){};
	}

void CheckCollison(void){
	uint8_t i=0;
	
	if(player.level==1)
	for(i=0;i<3;i++){
		if((bullet.xpos>(enemy.xpos[i])) & (bullet.xpos < (enemy.xpos[i]+16)) & (bullet.ypos < enemy.ypos[i]) & (enemy.xpos[i] != 0)){
			ST7735_DrawBitmap(enemy.xpos[i],enemy.ypos[i],Explosion0, 16,16); // Explosion
			if(TIMER1_CTL_R == 1){
				enemy.expflag = 1;
			}
			else{
				Timer1A_Set(&(Sound_Explosion)); //set sound as explosion
			}
			player.score += 10;
			bullet.Bflag = 0;
			enemy.deadx[i] = enemy.xpos[i]; //save location of dead enemy
			enemy.deady[i] = enemy.ypos[i];
			enemy.xpos[i]= 0; //clear alive positions
			enemy.ypos[i]= 0;
			TIMER0_CTL_R |= 0x00000001;    // 10) enable TIMER0A
		}
	}
	
	
	
	if(player.level==2)
	for(i=0;i<7;i++){
		if((bullet.xpos>(enemy.xpos[i])) & (bullet.xpos < (enemy.xpos[i]+16)) & (bullet.ypos < enemy.ypos[i]) & (enemy.xpos[i] != 0)){
			ST7735_DrawBitmap(enemy.xpos[i],enemy.ypos[i],Explosion0, 16,16); // Explosion
			if(TIMER1_CTL_R == 1){
				enemy.expflag = 1;
			}
			else{
			Timer1A_Set(&(Sound_Explosion)); //set sound as explosion
			}
			
			if(i<3){player.score += 10;}//update score lvl 1
			if(i>2){player.score += 20;}//update score lvl 2
			bullet.Bflag = 0;
			enemy.deadx[i] = enemy.xpos[i]; //save location of dead enemy
			enemy.deady[i] = enemy.ypos[i];
			enemy.xpos[i]= 0; //clear alive positions
			enemy.ypos[i]= 0;
			TIMER0_CTL_R |= 0x00000001;    // 10) enable TIMER0A
		}
	}
	if(player.level==3){
	//Will Check for collison with any of the enemies on screen (checks all 20 possible positions)
	for(i=0;i<14;i++){
		if((bullet.xpos>(enemy.xpos[i])) & (bullet.xpos < (enemy.xpos[i]+16)) & (bullet.ypos < enemy.ypos[i]) & (enemy.xpos[i] != 0)){
			ST7735_DrawBitmap(enemy.xpos[i],enemy.ypos[i],Explosion0, 16,16); // Explosion
			if(TIMER1_CTL_R == 1){
				enemy.expflag = 1;
			}
			else{
			Timer1A_Set(&(Sound_Explosion)); //set sound as explosion
			}
			if(i<3){player.score += 10;}//update score lvl 1
			if(i>2 & i<7){player.score += 20;}//update score lvl 2
			if(i>6){player.score += 30;}//update score lvl 3
			bullet.Bflag = 0;
			enemy.deadx[i] = enemy.xpos[i]; //save location of dead enemy
			enemy.deady[i] = enemy.ypos[i];
			enemy.xpos[i]= 0; //clear alive positions
			enemy.ypos[i]= 0;
			TIMER0_CTL_R |= 0x00000001;    // 10) enable TIMER0A
		}
	}
}
	
	if(player.level==4){
		if((bullet.xpos>(multi.xpos)) & (bullet.xpos < (multi.xpos+16)) & (bullet.ypos < 10) & (bullet.ypos > 0)){
			GameWinPlayer1();
		}	
	}
	
}

void ClearExplosion(void){
	uint8_t i=0;
	
		if(player.level == 1){
			for(i=0;i<3;i++){
				if(enemy.deadx[i] != 0){
					ST7735_DrawBitmap(enemy.deadx[i],enemy.deady[i],Blank16, 16,16); // Clear Explosion
					enemy.deadx[i] = 0;
					enemy.deady[i] = 0;
					enemy.dead = 0; //clear flag
				}
			}
		}
		if(player.level == 2){
			for(i=0;i<7;i++){
				if(enemy.deadx[i] != 0){
					ST7735_DrawBitmap(enemy.deadx[i],enemy.deady[i],Blank16, 16,16); // Clear Explosion
					enemy.deadx[i] = 0;
					enemy.deady[i] = 0;
					enemy.dead = 0; //clear flag
				}
			}
		}
		
		if(player.level == 3){
			for(i=0;i<14;i++){
				if(enemy.deadx[i] != 0){
					ST7735_DrawBitmap(enemy.deadx[i],enemy.deady[i],Blank16, 16,16); // Clear Explosion
					enemy.deadx[i] = 0;
					enemy.deady[i] = 0;
					enemy.dead = 0; //clear flag
				}
			}
		}
		
		player.clears++; //track number of clears
		
}

void DrawLazer(void){
		bullet.xpos = player.xpos+8; //save bullet x coord initial
		bullet.ypos = 150; //save bullet y coord initial
		ST7735_DrawBitmap(bullet.xpos,bullet.ypos,Lazer, 1,7); // Player Laser
		GPIO_PORTF_DATA_R ^= 0x02;
		bullet.Bflag = 1; //Set flag to signal moving the lazer
	}

void DrawELazer(void){
		uint8_t index;
		bullet.EDflag = 0; //Unset flag
		
		index = (Random())%3; //Random between 0-2
		bullet.expos = enemy.xpos[index]+8;
		bullet.eypos = enemy.ypos[index]+8;
		
		if(bullet.expos!=8){
			ST7735_DrawBitmap(bullet.expos,bullet.eypos,ELazer, 1,7); // Enemy laser
			bullet.EBflag = 1;
			Timer1A_Set(&Sound_EShoot); //set pointer for sound
		}
	}


void SetExplosion(void){
	enemy.dead = 1;
}

void GameWon(void){uint32_t delay = 40000000;
	ST7735_FillScreen(0x0000);            // set screen to black
  ST7735_SetCursor(1, 1);
  ST7735_OutString("Congratulations!");
  ST7735_SetCursor(1, 2);
  ST7735_OutString("You Have Won!");
  ST7735_SetCursor(1, 3);
  ST7735_OutString("Earthling!");
  ST7735_SetCursor(2, 4);
	
	while(delay>0){
		delay--;
	}
	player.level++;
}

void MoveLazer(void){
		CheckCollison();
		if(bullet.ypos<=-1){
			bullet.Bflag=0;
		}
		else{
			ST7735_DrawBitmap(bullet.xpos,bullet.ypos,Lazer, 1,7); // Lazer 1
		}
	}


void MultiDraw(void){
		bullet.expos = multi.xpos+8; //save bullet x coord initial
		bullet.eypos = 11; //save bullet y coord initial
		ST7735_DrawBitmap(bullet.expos,bullet.eypos,ELazer, 1,7); // Player Laser
		//GPIO_PORTF_DATA_R ^= 0x02;
		bullet.EBflag = 1; //Set flag to signal moving the enemy lazer
		bullet.EDflag = 0;
}





	
	
void playerInit(uint8_t level){
	uint8_t i =0; 
	player.alive = 1;  //is alive!
	player.level = level;  //set level
	player.xpos = 35;
		
	//LEVEL 1
	if(level == 1){
		
		

		
		
		enemy.picture = &SmallEnemy10pointA[0];
		enemy.xpos[0] = 17;
		enemy.ypos[0] = 45;
		ST7735_DrawBitmap(enemy.xpos[0], enemy.ypos[0], enemy.picture, 16,10);
		
		for(i=1;i<3;i++){
			enemy.ypos[i] = 45;
			}
		
		for(i=1;i<3;i++){
			enemy.xpos[i] = enemy.xpos[i-1] + 34;
			ST7735_DrawBitmap(enemy.xpos[i], enemy.ypos[i], enemy.picture, 16,10);
		}
		
	}
	
	//LEVEL 2
	if(level == 2){
		
	for(i=0;i<7;i++){
		enemy.deadx[i]=0;
		enemy.deady[i]=0;
	}
	
	enemy.dead=0;
	uint32_t delay=30000000;
	player.xpos = 35;
	
	ST7735_FillScreen(0x0000);            // set screen to black
	ST7735_SetCursor(64, 80);
	ST7735_OutString("Level 2");
	
		while(delay!=0){delay--;}
		ST7735_FillScreen(0x0000);            // set screen to black
		
		
		enemy.picture = &SmallEnemy10pointA[0];
		enemy.xpos[0] = 17;
		enemy.ypos[0] = 45; 
		ST7735_DrawBitmap(enemy.xpos[0], enemy.ypos[0], enemy.picture, 16,10);
		for(int i = 1; i<3; i++){
			enemy.xpos[i] = enemy.xpos[i-1] + 34;  
			enemy.ypos[i] = 45; 
			ST7735_DrawBitmap(enemy.xpos[i], enemy.ypos[i], enemy.picture, 16,10);
		}
		
	//	enemy.xpos[3] = 2;
		enemy.picture = &SmallEnemy20pointA[0];
		enemy.xpos[3] = 2;
		enemy.ypos[3] = 30;
		ST7735_DrawBitmap(enemy.xpos[3], enemy.ypos[3], enemy.picture, 16,10);
		for(i = 4; i<7; i++){
			enemy.xpos[i] = enemy.xpos[i-1] + 34;  
			enemy.ypos[i] = 30;
			ST7735_DrawBitmap(enemy.xpos[i], enemy.ypos[i], enemy.picture, 16,10);
		}
		NVIC_ST_CTRL_R = 0x007; //Turn on
	
	}
	//LEVEL 3 
	 if(level == 3){
		 for(i=0;i<14;i++){
			enemy.deadx[i]=0;
			enemy.deady[i]=0;
		 }
		uint32_t delay=30000000;
		player.xpos = 35;	
		ST7735_FillScreen(0x0000);            // set screen to black
		ST7735_SetCursor(64, 80);
		ST7735_OutString("Level 3");
		while(delay!=0){delay--;}
		ST7735_FillScreen(0x0000);            // set screen to black
		
		enemy.picture = &SmallEnemy10pointA[0];
		enemy.xpos[0] = 17;
		enemy.ypos[0] = 45;
		ST7735_DrawBitmap(enemy.xpos[0], enemy.ypos[0], enemy.picture, 16,10);
		
		for(i=1;i<3;i++){
			enemy.ypos[i] = 45;
			}
		
		for(i=1;i<3;i++){
			enemy.xpos[i] = enemy.xpos[i-1] + 34;
			ST7735_DrawBitmap(enemy.xpos[i], enemy.ypos[i], enemy.picture, 16,10);
		}

  
		enemy.picture = &SmallEnemy20pointA[0];
		enemy.xpos[3] = 2;
		enemy.ypos[3] = 30;
		ST7735_DrawBitmap(enemy.xpos[3], enemy.ypos[3], enemy.picture, 16,10);
		for(i = 4; i<7; i++){
			enemy.xpos[i] = enemy.xpos[i-1] + 34;  
			enemy.ypos[i] = 30;
			ST7735_DrawBitmap(enemy.xpos[i], enemy.ypos[i], enemy.picture, 16,10);
		}

		enemy.picture = &SmallEnemy30pointA[0]; 
		enemy.xpos[7] = 2;
		enemy.ypos[7] = 15;
		ST7735_DrawBitmap(enemy.xpos[7], enemy.ypos[7], enemy.picture, 16,10);
		for(i = 8; i<14; i++){
			enemy.xpos[i] = enemy.xpos[i-1] + 17;  
			enemy.ypos[i] = 15;
			ST7735_DrawBitmap(enemy.xpos[i], enemy.ypos[i], enemy.picture, 16,10);
		}
	}
	NVIC_ST_CTRL_R = 0x07; //Turn off while setting up
}
 
	




void SysTick_Handler(void){
	data = ADC_In();
	if(((data>=2048)&(player.xpos>=110))|((data<=2048)&(player.xpos<=0))){player.velx=0;}
	else{
		if((data>2457)&(data<3071)){player.velx = 1;} //Slow right
		if((data <= 1638)&(data>1023)){player.velx= -1;} //Slow left
		if(data<1023){player.velx = -2;} //Fast left
		if(data>3071){player.velx=2;} //Fast Right
		if((data <= 2457)&(data>1638)){player.velx= 0;} //Middle 20% = no movement
	}
	player.xpos += player.velx; //Slide Location Update
	//Update bullet position if B1flag is on
	if(bullet.Bflag==1){
		bullet.ypos-=3;  //move bullet
	}
	
	if(bullet.EBflag==1){
		if(player.level == 4){
			bullet.eypos += 2;
		}
		else{
			bullet.eypos ++;
		}
		if(player.level == 3){
			bullet.eypos ++;
		}
	}
	
	if(player.level == 4){
			if(multi.Lflag==1){
				if(multi.xpos==1){
					multi.Lflag=0;
				}
				else{
				multi.xpos--;
				}
			}
		if(multi.Lflag==0){
			if(multi.xpos==110){
				multi.Lflag=1;
			}
			else{
				multi.xpos++;
			}
		}
	}
}
void ECheckCollision(void){
	uint32_t delay = 10000000;
	if((bullet.expos>(player.xpos)) & (bullet.expos < (player.xpos+16)) & ((bullet.eypos > 149)) & (bullet.eypos < 159)){
		ST7735_DrawBitmap(player.xpos,159,Explosion0, 16,16); // Explosion
		Timer1A_Set(&Sound_PExplode);
		while(delay>0){
			delay--;
		}
		if(player.level==4){
			GameWinPlayer2();
		}
		else{
			GameOver();
		}
	}
}

void MoveELazer(void){
	
	ECheckCollision();
	if(bullet.eypos > 168){
		bullet.EBflag=0;
	}
	else{
		ST7735_DrawBitmap(bullet.expos,bullet.eypos,ELazer, 1,7); // Lazer 1
	}

}



int main(void){
  TExaS_Init();  // set system clock to 80 MHz
	ST7735_InitR(INITR_REDTAB);
	ADC_Init();
	SysTick_Init();
	ST7735_FillScreen(0x0000);            // set screen to black
	PortF_Init();
	Timer1_Init(10000);
	DAC_Init();
	playerInit(1);
	Timer0_Init(&(SetExplosion),40000000);
	player.xpos =35; //initial position 
	EnableInterrupts();           // (i) Enable global Interrupt flag (I)
	Random_Init(ADC_In());//seed with random number from slide pot

	while(1){
		
		ST7735_DrawBitmap(player.xpos, 159, Player0, 15,15); // player ship middle bottom
		
		if(bullet.EBflag == 0){bullet.EDflag = 1;}
		if(bullet.EDflag==1){DrawELazer();}
		if(bullet.EBflag == 1){MoveELazer();}
		
		
		//Check if we need to draw a bullet
		if(bullet.Dflag==1){
			DrawLazer();
			bullet.Dflag=0;
		}
		//Draw bullet 1 as long as flag is on
		if(bullet.Bflag == 1){
			MoveLazer();
		}
		if(enemy.dead == 1){
			ClearExplosion();
		}
			if(player.clears == 3){
				NVIC_ST_CTRL_R = 0; //Turn off while setting up
				bullet.EBflag = 0;
				player.clears ++;
				playerInit(2);
			}

			if(player.clears == 11){
				NVIC_ST_CTRL_R = 0; //Turn off while setting up
				bullet.EBflag = 0;
				player.clears++;
				playerInit(3);
			} 
			
			if(player.clears == 26){
				GameWon();
		}
			
		if(enemy.expflag == 1){
			Timer1A_Set(&Sound_Explosion);
			enemy.expflag = 0;
		}
	
	if(player.level==4){
		ST7735_FillScreen(0x0000);            // set screen to black
		while(1){
			ST7735_DrawBitmap(player.xpos, 159, Player0, 15,15); // player ship middle bottom
			ST7735_DrawBitmap(multi.xpos,10,SmallEnemy10pointA, 16,10); // player ship middle bottom
			//Check if we need to draw a bullet
			if(bullet.Dflag==1){
				DrawLazer();
				bullet.Dflag=0;
			}
			//Draw bullet 1 as long as flag is on
			if(bullet.Bflag == 1){
				MoveLazer();
			}
			
			if(bullet.EDflag==1){
				MultiDraw();
			}
			if(bullet.EBflag==1){
				MoveELazer();
			}
			}
		}
	}
}


void GPIOPortF_Handler(void){
	
	if(GPIO_PORTF_RIS_R&0x10){
  GPIO_PORTF_ICR_R = 0x10;      // acknowledge flag4
	if(bullet.Bflag!=1){
		if(TIMER1_CTL_R == 0){			//only if timer is unused
			bullet.Dflag =1;
			Timer1A_Set(&(Sound_Shoot)); //set sound as shoot
		}
	}
}
	
	if(GPIO_PORTF_RIS_R&0x01){
		GPIO_PORTF_ICR_R = 0x01;      // acknowledge flag0
		if(bullet.EBflag!=1){
			bullet.EDflag = 1;
			Timer1A_Set(&(Sound_EShoot));
		}
	}
}





