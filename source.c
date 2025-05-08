#include <p18f452.h>
#include "definitions.h"
#pragma config WDT = OFF

#pragma interrupt interruptFunction
void interruptFunction(void)
{
	if(INTCONbits.INT0IF)
	{
		INTCONbits.INT0IF = 0;
		if(!choose)					
		{
			PIE1bits.SSPIE = 0;			// DISABLE SPI INTERRUPT
			SLAVE_SELECT = 1;			// DISABLE COMMUNICATION WITH SLAVE
			INTCON3bits.INT1IE = 1;		// ENABLE INT0 INTERRUPT
			INTCON3bits.INT1IF = 0;		
			choose = 0x01;				// STATE CHANGES
			clearHome();				// CLEAR LCD ENTIRELY
			displayParameter();			// DISPLAY THE PARAMETERS INTO LCD
		}
		else
		{
			changePosition = ~changePosition;		// EACH TIME INT0 IS TRIGGERED BY A BUTTON PUSH THE changePosition VALUE IS COMPLEMENTED
			changeArrowPosition();					// BASED ON THE changePosition VALUE, THE ARROW IS EITHER POINTING TO FIRST CHOICE OR SECOND CHOICE
		}
	}
	else if(INTCON3bits.INT1IF)						// THIS BLOCK IS USED TO VALIDATE USE CHOICE 
	{
		INTCON3bits.INT1IF = 0;					
		INTCON3bits.INT1IE = 0;						// DISABLE THE INT1 (THIS IS ACTIVATED ONLY WHEN THE INT0 IS TRIGERREED)
		PIE1bits.SSPIE = 1;							// ACTIVATE SPI INTERRUPT
		clearHome();								// CLEAR HOME 
		choose = 0;									// RESET THE CHOOSE VARIABLE
		SLAVE_SELECT = 0;							// PULL THE SLAVE SELECT PIN LOW
		SSPBUF = CLOCK_BURST_READ;					// INITIATE COMMUNICATION WITH SLAVE THROUGH BURST MODE  
	}	
	else if(PIE1bits.SSPIE)						// THIS BLOCK IS FOR SPI ISR CODE
	{	
		if(PIR1bits.SSPIF)	
		{
			PIR1bits.SSPIF = 0;
			if(spiState == WRITE)
			{
				if(i < 8)
					SSPBUF = bytes[i++];
				else
				{
					SLAVE_SELECT = 1;
					spiState = READ;
					reg = DUMMY;
					SLAVE_SELECT = 0;
					i = 0;
					SSPBUF = CLOCK_BURST_READ;
				}	
			}
			else
			{
				if(reg == DUMMY)				// SEND DUMMY DATA
				{
					SSPBUF = DUMMY_DATA;
					reg = SAVE;
				}
				else						// STORE THE RECEIVED DATA FROM MISO LINE
				{
					reg = DUMMY;
					bytes[i++] = SSPBUF;	
					if(i < 8)
						PIR1bits.SSPIF = 1;		// TRIGGER A SOFTWARE SPI INTERRUPT 
					else
					{
						reg = DUMMY;
						SLAVE_SELECT = 1;		// DISCONNECTED THE SLAVE
						displayResult();		// DISPLAY DATA INTO LCD
						SLAVE_SELECT = 0;
						SSPBUF = CLOCK_BURST_READ;	// INITIATE A BURST MODE READING 
						i = 0;
					}
				}
			}
		}
	}
}

#pragma code vector = 0x00008
void vector(void)
{
	_asm
		GOTO interruptFunction
	_endasm
}
#pragma code

void main(void)
{
	TRISD = 0x00;				// ALL PINS ARE OUTPUT
	TRISC = 0x90;				// MAKE SERIAL PINS AS OUTPUT + LCD CONTROL PINS OUTPUT
	SSPSTAT = 0xC0;				// CONFIGURATION OF SPI PROTOCOL
	SSPCON1 = 0x20;			
	initialization();			// INITIALIZATION OF LCD(CLEAR HOME, CURSOR OFF, 5*7 PIXEL SIZE)
	INTCONbits.GIE = 1;			// GLOBAL INTERRUPT ENABLE
	INTCONbits.PEIE = 1;		// PERIPHERAL INTERRUPT ENABLE		
	INTCONbits.INT0IE = 1;
	INTCONbits.INT0IF = 0;
	PIE1bits.SSPIE = 1;			// SPI INTERRUPT BIT
	PIR1bits.SSPIF = 0;			// INTERRUPT FLAG  
	SLAVE_SELECT = 0;
	SSPBUF = CLOCK_BURST_WRITE;		// INITIATE BURST MODE WRITE TO SLAVE
	while(TRUE);					// KEEP LOOPING
}		
void changeArrowPosition(void)		// THE	 FUNCTION ROLE'S TO HANLD ARROW POSITION 
{
	if(changePosition)			
	{
		home(ARROW_SECOND_LINE);		
		displaySpace();				// CLEAR PREVIOUS ARROW FROM THE SECOND LINE
		home(ARROW_THIRD_LINE);
		displayArrow();				// DISPLAY ARROW
	}
	else
	{
		home(ARROW_THIRD_LINE);
		displaySpace();				// CLEAR PREVIOUS ARROW FROM THE THIRD LINE
		home(ARROW_SECOND_LINE);
		displayArrow();
	}
}
void displaySpace(void)
{
	LATD = 0x20;					// 0x20 IS THE ASCII CODE OF A SPACE
	data();
}
void displayArrow(void)
{
	LATD = '>';						// ARROW CHARATCER
	data();
}
void displayParameter(void)				// THE FUNCTION ROLE'S TO DISPLAY ALL THE SUPPORTED LANGUAGES BY THE SYSTEM
{
	unsigned char strings[3][16] = {"     LANGUAGE :", "      ENGLISH", "      FRENSH"}, i = 0;
	while(i < 3)
	{
		displayString(strings[i]);
		if(i == 0)
			home(0xC0);
		else if(i == 1)
			home(0x94);
		++i;
	}
	home(0xC4);
	displayArrow();
}
void clearHome(void)
{
	LATD = 0x01;
	command();
	T0CON = 0x80;
	TMR0H = 0xF9;
	TMR0L = 0x8E;	
	timerZero();
}
void home(unsigned char value)		// THIS FUNCTION IS USED TO CHANGE THE POSITION OF THE CURSOR(SECOND LINE, FIRST LINE)
{
	LATD = value;
	command();
	busyFlag();
}
void displayTime(void)				// THIS IS USED FOR DISPLAYING TIME 
{
	signed char m = 2;
	home(0xC6);
	while(m >= 0)
	{
		LATD = 0x30 + ((bytes[m] & 0xF0) >> 4);		// EXTRACT THE HIGH BYTE TO DISPLAY IT
		data();
		LATD = 0x30 + (bytes[m] & 0x0F);			// EXTRACT THE LOW BYTE TO DISPLAY IT
		data();
		if(m != 0)									// DISPLAY TIME IN THIS FORMAT : x:x:x
		{
			LATD = ':';			
			data();
		}
		--m;
	}
}
void displayString(const unsigned char* string)
{	
	unsigned char i = 0;
	for(; string[i] != '\0'; ++i)
	{
		LATD = string[i];
		data();
	}
}
void displayDate(void)			// THIS FUNCTION DISPLAYS DATE : x/x/20x
{		
	unsigned char m = (bytes[5] - 1), n = 0, date = bytes[3], month = bytes[4], year = bytes[6];
	home(0x94);
	if(m > 7)
		m = 0;
	if(!changePosition)
		displayString(daysInEnglish[m]);
	else
		displayString(daysInFrensh[m]);
	LATD = 0x30 + ((date & 0xF0) >> 4);
	data();
	LATD = 0x30 + (date & 0x0F);
	data();
	LATD = '/';
	data();
	LATD = 0x30 + ((month & 0xF0) >> 4);
	data();
	LATD = 0x30 + (month & 0x0F);
	data();
	LATD = '/';
	data();
	LATD = 0x32;			
	data();	
	LATD = 0x30;
	data();
	LATD = 0x30 + ((year & 0xF0) >> 4);
	data();
	LATD = 0x30 + (year & 0x0F);
	data();
	displaySpace();
	displaySpace();
	displaySpace();
}
void displayInformation(void)			// THIS FUNCTION DISPLAYS INFORMATIONS
{
	unsigned char inforEnglish[] = "DATE & TIME", i = 0, inforFrensh[] = "TEMPS & DATE";
	home(0x84);
	if(!changePosition)
		displayString(inforEnglish);
	else
		displayString(inforFrensh);
}
void displayResult(void)
{
	displayInformation();
	displayTime();
	displayDate();
}
void initialization(void)		
{
	LATD = 0x38;
	command();
	delay250ms();
	LATD = 0x01;
	command();
	delay250ms();
	LATD = 0x0C;
	command();
	delay250ms();
}
void delay250ms(void)
{
	T0CON = 0x01;
	TMR0H = 0x0B;
	TMR0L = 0xBC;
	timerZero();		
}
void delay3us(void)				
{
	T0CON = 0x48;
	TMR0L = 253;
	timerZero();
}
void command(void)
{
	RS_PIN = 0;
	RW_PIN = 0;
	EN_PIN = 1;
	delay3us();
	EN_PIN = 0;
}
void data(void)
{
	RS_PIN = 1;
	RW_PIN = 0;
	EN_PIN = 1;
	delay3us();
	EN_PIN = 0;
	busyFlag();
}
void busyFlag(void)
{
	RS_PIN = 0;
	RW_PIN = 1;	
	TRISDbits.TRISD7 = 1;
	do
	{
		EN_PIN = 1;
		delay3us();
		EN_PIN = 0;
	}while(PORTDbits.RD7 == 1);
	EN_PIN = 0;
	TRISDbits.TRISD7 = 0;
}
void timerZero(void)
{
	INTCONbits.TMR0IF = 0;
	T0CONbits.TMR0ON = 1;
	while(INTCONbits.TMR0IF == 0);
	INTCONbits.TMR0IF = 0;
	T0CONbits.TMR0ON = 0;	
}
