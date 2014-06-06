/*	Partner 1 Name & E-mail: Jesse R. Garcia jgarc***@email
 *	Partner 2 Name & E-mail: N/A
 *	Lab Section: 21
 *	Assignment: Lab # 10 Exercise # 1
 *	Exercise Description: Custom Lab Project with a focus on hardware components.
 *						Essentially I've made a "hard of hearing" sound activated light. More in actual lab write up.
 *						The following statement is false with the exception of the following functions:
 *												LCD_ClearCell(unsigned char location)
 *												LCD_DisplayChar(unsigned char location, unsigned char data)
 *												LCD_DefineChar was found online at
 *														http://winavr.scienceprog.com/example-avr-projects/simplified-avr-lcd-routines.html
 *												and adapted to our predefined functions.
 *												NOTE: I tried adding the source code from here to my citations folder, but it wouldn't upload.
 *														For reference, I used the file "lcd_lib.c" from the zipper file at the bottom of the page.
 *						 Otherwise, this software was provided to us during lab.
 *	
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "io.h"
#include <avr/pgmspace.h>

#define SET_BIT(p,i) ((p) |= (1 << (i)))
#define CLR_BIT(p,i) ((p) &= ~(1 << (i)))
#define GET_BIT(p,i) ((p) & (1 << (i)))
          
/*-------------------------------------------------------------------------*/

#define DATA_BUS PORTC		// port connected to pins 7-14 of LCD display
#define CONTROL_BUS PORTD	// port connected to pins 4 and 6 of LCD disp.
#define RS 6			// pin number of uC connected to pin 4 of LCD disp.
#define E 7			// pin number of uC connected to pin 6 of LCD disp.

/*-------------------------------------------------------------------------*/

void LCD_ClearScreen(void) {
   LCD_WriteCommand(0x01);
}

void LCD_init(void) {

    //wait for 100 ms.
	delay_ms(100);
	LCD_WriteCommand(0x38);
	LCD_WriteCommand(0x06);
	LCD_WriteCommand(0x0f);
	LCD_WriteCommand(0x01);
	delay_ms(10);						 
}

void LCD_WriteCommand (unsigned char Command) {
   CLR_BIT(CONTROL_BUS,RS);
   DATA_BUS = Command;
   SET_BIT(CONTROL_BUS,E);
   asm("nop");
   CLR_BIT(CONTROL_BUS,E);
   delay_ms(2); // ClearScreen requires 1.52ms to execute
}

void LCD_WriteData(unsigned char Data) {
   SET_BIT(CONTROL_BUS,RS);
   DATA_BUS = Data;
   SET_BIT(CONTROL_BUS,E);
   asm("nop");
   CLR_BIT(CONTROL_BUS,E);
   delay_ms(1);
}

void LCD_DisplayString( unsigned char column, const unsigned char* string) {
   //LCD_ClearScreen();
   unsigned char c = column;
   while(*string) {
      LCD_Cursor(c++);
      LCD_WriteData(*string++);
   }
}

void LCD_Cursor(unsigned char column) {
   if ( column < 17 ) { // 16x1 LCD: column < 9
						// 16x2 LCD: column < 17
      LCD_WriteCommand(0x80 + column - 1);
   } else {
      LCD_WriteCommand(0xB8 + column - 9);	// 16x1 LCD: column - 1
											// 16x2 LCD: column - 9
   }
}

void delay_ms(int miliSec) //for 8 Mhz crystal

{
    int i,j;
    for(i=0;i<miliSec;i++)
    for(j=0;j<775;j++)
  {
   asm("nop");
  }
}

void LCD_DefineChar(const uint8_t *pc,uint8_t char_code){
	uint8_t a, pcc;
	uint16_t i;
	a=(char_code<<3)|0x40;
	for (i=0; i<8; i++){
		pcc=pgm_read_byte(&pc[i]);
		LCD_WriteCommand(a++);
		LCD_WriteData(pcc);
	}
}

// Basic Function
// Assigns passed in value of location to LCD_Cursor and writes to that position the character passed in on data perameter
void LCD_DisplayChar(unsigned char location, unsigned char data)
{
	LCD_Cursor(location);
	LCD_WriteData(data);
}

// Basic Function
// Assigns passed in value of location to LCD_Cursor and writes to it the empty character from program memory
void LCD_ClearCell(unsigned char location)
{
	LCD_Cursor(location);
	LCD_WriteData(0b11111110);
}
