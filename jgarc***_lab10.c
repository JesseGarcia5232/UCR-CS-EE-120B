/*
 * jgarc***_lab10.c
 *
 * Created: 6/3/2014 7:56:59 AM
 *  Author: Jesse R. Garcia
 */ 
/*	Partner 1 Name & E-mail: Jesse R. Garcia jgarc***@email
 *	Partner 2 Name & E-mail: N/A
 *	Lab Section: 21
 *	Assignment: Lab # 10 Exercise # 1
 *	Exercise Description: Custom Lab Project with a focus on hardware components
 *						Essentially I've made a "deaf" sound activated light. More in actual lab write up.
 *						The following statement holds true unless explicitly stated otherwise. 
 *						eg: Functions found on the internet.
 *	
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */



#include <avr/io.h>
#include <avr/pgmspace.h>
#include "io.c"
#include "timer.h"

// Speaker custom character 
const uint8_t Speaker[] PROGMEM = {
	0b00010,
	0b00110,
	0b11110,
	0b11110,
	0b11110,
	0b11110,
	0b00110,
	0b00010
};

//Sound waves to be displayed emanating from the speaker CC
const uint8_t Sound_Waves[] PROGMEM = {
	0b00010,
	0b01001,
	0b00101,
	0b10101,
	0b10101,
	0b00101,
	0b01001,
	0b00010,
};

// Creates my custom characters in the memory of the LCD screen
void LCD_DeclareCC()
{
	LCD_DefineChar(Speaker,0);
	LCD_DefineChar(Sound_Waves,1);
}

// Shift register function given to us in lab. Just modified its PORT selection
void transmit_data(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTB = 0x08;
		// set SER = next bit of data to be sent.
		PORTB |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTB |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTB |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTB = 0x00;
}
void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	//        in Free Running Mode, a new conversion will trigger whenever
	//        the previous conversion completes.
}

typedef struct task
{
	int state; // Current state of the task
	unsigned long period; // Rate at which the task should tick
	unsigned long elapsedTime; // Time since task's previous tick
	int (*TickFct)(int); // Function to call for task's tick
} task;

task tasks[3];

const unsigned char tasksNum = 3;			    // Number of tasks used in the system
const unsigned long POWER_Period = 50;		// Period of my POWER task to get button input
const unsigned long Sense_Period = 25;		// Period of my Sense task to pick up sound. 
                                          // Needed to be low to actually pick up motion in the membrane.
const unsigned long Display_Period = 100;	// Period of my Display task that outputs to the LCD screen and LED matrix
const unsigned long PeriodGCD = 25;			  // GCD of all task periods
   
int TickFct_POWER(int state);	
int TickFct_Sense(int state);
int TickFct_Display(int state);

int main()
{
	DDRA = 0x00;	// Readies PORTA to be used for input
	PORTA = 0xFF;
	
	DDRB = 0xFF;	// Readies PORTD to be used for output
	PORTB = 0x00;
	
	DDRC = 0xFF;	// Readies PORTC to be used for output
	PORTC = 0x00;
	
	DDRD = 0xFF;	// Readies PORTD to be used for output
	PORTD = 0x00;
	
	unsigned char i = 0;
	tasks[i].state = -1;
	tasks[i].period = POWER_Period;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_POWER;
	i++;
	tasks[i].state = -1;
	tasks[i].period = Sense_Period;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_Sense;
	i++;
	tasks[i].state = -1;
	tasks[i].period = Display_Period;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_Display;
	
	LCD_init();
	LCD_DeclareCC();											   // Creates my custom characters in the memory of the LCD screen
	ADC_init();
	TimerSet(PeriodGCD);
	TimerOn();
	while(1)
	{
		i = 0;
		for(i = 0; i < tasksNum; i++)								        // RTOS-esque code.
		{															                      // When the a tasks elapsed time is greater than 
		                                                    //or equal to its period, 
			if(tasks[i].elapsedTime >= tasks[i].period)				// that tasks tick function gets called. 
			                                                  //Elapsed time is set to zero, then
			{														                      // is incremented by the GCD of all task periods
				tasks[i].state = tasks[i].TickFct(tasks[i].state);
				tasks[i].elapsedTime = 0;
			}
			tasks[i].elapsedTime += PeriodGCD;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}

	return 0;
}

unsigned char PWR_ON = 0;			// Indicates if the system is powered on or off.
unsigned char display_ready = 0;	// Bit flag indicating the display is ready which starts Sense SM.
unsigned char sound_detected = 0;	// Bit flag indicating if sound is detected, starts output sequence.
unsigned char acknowledged = 0;		// Bit flag indicating that Display SM has received 'sound_detected' from Sense SM.  
unsigned char ready_new = 0;		// Bit flag indicating Sense SM is ready to detected sound.
// array of my outputs to the rows of the LED matrix
const unsigned char light_stages [] = {0x00, 0x03, 0x0C, 0x30, 0xC0, 0xFF}; 

enum POWER_States {POWER_Off, POWER_On};
int TickFct_POWER(int state)
{
	static unsigned char press;	  // Storage of the button press on AVR PORTA[7]
	press = ~PINA & 0x80;		  // Our buttons are pull up resistors
	
	switch(state)
	{
		case -1:
			state = POWER_Off;
			break;
		case POWER_Off:
			if(press)
			{
				state = POWER_On;
				PWR_ON = ~PWR_ON;
			}
			else
			{
				state = POWER_Off;
			}
			break;
		case POWER_On:
			if(!press)
			{
				state = POWER_Off;
			}
			else
			{
				state = POWER_On;
			}
			break;
		default:
			state = -1;
			break;
	}
	
	switch(state)
	{
		case POWER_Off:
			break;
		case POWER_On:
			break;
		default:
			break;
	}
	return state;
}

enum Display_States {Display_WaitPWR, Display_WaitSense, Display_SoundDetected, Display_LightON, Display_LightOFF, Display_WaitRdy};
int TickFct_Display(int state)
{
	static unsigned char location = 0; // Current location to be used in the light_stages array
	static unsigned char light_on = 0; // Current lighted state of the LED matrix. 0 means 'off', ~0 means 'on'
	switch (state)
	{
		case -1:
			state = Display_WaitPWR;
			break;
		case Display_WaitPWR:
			if(!PWR_ON)
			{
				state = Display_WaitPWR;
				LCD_ClearScreen();
				light_on = 0;
				acknowledged = 0;
				location = 0;
				transmit_data(light_stages[location]);
			}
			else if(PWR_ON)
			{
				state = Display_WaitSense;
				LCD_ClearScreen();
				LCD_DisplayChar(1,0b00001000); // Displays my speaker custom character
				LCD_DisplayChar(2,0b01011000); // Displays an 'X' indicating no sound is currently detected
				LCD_DisplayString(4, "No Sound");
				LCD_DisplayString(17, "Lights Off");
				location = 0;
				transmit_data(light_stages[location]);
				display_ready = 1;
			}
			break;
		case Display_WaitSense:
			if(!PWR_ON)
			{
				state = Display_WaitPWR;
				LCD_ClearScreen();
				light_on = 0;
				acknowledged = 0;
				location = 0;
				transmit_data(light_stages[location]);
			}
			else if(!sound_detected && PWR_ON)
			{
				state = Display_WaitSense;
			}
			else if(sound_detected && PWR_ON)
			{
				state = Display_SoundDetected;
				display_ready = 0;
				acknowledged = 1;
				light_on = ~light_on;
			}
			break;
		case Display_SoundDetected:
			if(!PWR_ON)
			{
				state = Display_WaitPWR;
				LCD_ClearScreen();
				light_on = 0;
				acknowledged = 0;
				location = 0;
				transmit_data(light_stages[location]);
			}
			else if(light_on && PWR_ON)
			{
				state = Display_LightON;
				LCD_DisplayString(17, "Lights On");
				LCD_ClearCell(26);
				location = 1;
			}
			else if(!light_on && PWR_ON)
			{
				state = Display_LightOFF;
				LCD_DisplayString(17, "Lights Off");
				location = 5;
			}
			break;
		case Display_LightON:
			if(!PWR_ON)
			{
				state = Display_WaitPWR;
				LCD_ClearScreen();
				light_on = 0;
				acknowledged = 0;
				location = 0;
				transmit_data(light_stages[location]);
			}
			if(location < 6 && PWR_ON)
			{
				state = Display_LightON;
			}
			else if(location >= 6 && PWR_ON)
			{
				state = Display_WaitRdy;
			}
			break;
		case Display_LightOFF:
			if(!PWR_ON)
			{
				state = Display_WaitPWR;
				LCD_ClearScreen();
				light_on = 0;
				acknowledged = 0;
				location = 0;
				transmit_data(light_stages[location]);
			}
			if(location > 0 && PWR_ON)
			{
				state = Display_LightOFF;
			}
			else if(location <= 0 && PWR_ON)
			{
				state = Display_WaitRdy;
			}
			break;
		case Display_WaitRdy:
			if(!PWR_ON)
			{
				state = Display_WaitPWR;
				LCD_ClearScreen();
				light_on = 0;
				acknowledged = 0;
				location = 0;
				transmit_data(light_stages[location]);
			}
			if(!ready_new && PWR_ON)
			{
				state = Display_WaitRdy;
			}
			else if(ready_new && PWR_ON)
			{
				state = Display_WaitSense;
				acknowledged = 0;
				LCD_DisplayChar(2, 0b01011000); // Displays an 'X' next to the speaker, indicating no sound is detected
				LCD_DisplayString(4, "No Sound");
				display_ready = 1;
			}
			break;
		default:
			state = -1;
			break;
	}
	
	switch(state)
	{
		case Display_WaitPWR:
			break;
		case Display_WaitSense:
			break;
		case Display_SoundDetected:
			acknowledged = 1;
			LCD_DisplayChar(2, 0b00001001); // Displays "sound waves" from speaker character
			LCD_DisplayString(4, "Detected");
			break;
		case Display_LightON:
			transmit_data(light_stages[location]);
			location++;
			break;
		case Display_LightOFF:
			location--;
			transmit_data(light_stages[location]);
			break;
		case Display_WaitRdy:
			break;
		default:
			break;
	}
	return state;
}
enum Sense_States {Sense_WaitDisplayRDY, Sense_GetSound, Sense_WaitAck, Sense_WaitRdyRecieved};
int TickFct_Sense(int state)
{
	static unsigned short normal_point = 743; // sound level observed in quiet
	static unsigned short sound = 0;          // Storage of detected sound level from ADC
	switch(state)
	{
		case -1:
			state = Sense_WaitDisplayRDY;
			break;
		case Sense_WaitDisplayRDY:
			if(!display_ready)
			{
				state = Sense_WaitDisplayRDY;
			}
			else 
			{
				state = Sense_GetSound;
				ready_new = 0;
			}
			break;
		case Sense_GetSound:
			if(!PWR_ON)
			{
				state = Sense_WaitDisplayRDY;
			}
			else if(!sound_detected && PWR_ON)
			{
				state = Sense_GetSound;
			}
			else if(sound_detected && PWR_ON)
			{
				state = Sense_WaitAck;
			}
			break;
		case Sense_WaitAck:
			if(!PWR_ON)
			{
				state = Sense_WaitDisplayRDY;
			}
			else if(!acknowledged && PWR_ON)
			{
				state = Sense_WaitAck;
			}
			else if(acknowledged && PWR_ON)
			{
				state = Sense_WaitDisplayRDY;
				ready_new = 1;
			}
			break;
			
		default:
			state = -1;
			break;
	}
	
	switch(state)
	{
		case Sense_WaitDisplayRDY:
			sound_detected = 0; 
			break;
		case Sense_GetSound:
			sound = ADC;
			if(sound <= normal_point)
			{
				sound_detected = 0;
			}
			else if(sound > normal_point)
			{
				sound_detected = 1;
			}
			break;
		case Sense_WaitAck:
			break;
	}
	return state;
}	

