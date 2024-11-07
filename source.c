#include <p18f452.h>
#pragma config WDT = OFF

#define TRUE 0x01
#define FALSE 0x00
#define numberDay 0x07
#define stringLength 5


#define 	RS_PIN				LATCbits.LATC0		// RS PIN CONNECTED TO RC0
#define 	RW_PIN 				LATCbits.LATC1		// RW PIN CONNECTED TO RC1
#define 	EN_PIN				LATCbits.LATC2		// ENABLE PIN CONNECTED TO RC2


#define CLOCK_BURST_WRITE 0x3F						// BURST MODE FOR WRITING
#define CLOCK_BURST_READ 0xBF						// BURST MODE FOR READING
#define DUMMY_DATA 	0x00							// DUMMY DATA 
#define CONTROL_WRITE 0x0F							// CONTROL REGISTER USED TO DEFINED THE WP BIT
#define SLAVE_SELECT LATCbits.LATC6					// SLAVE SELECT PIN IS CONNECTED TO RC6

volatile unsigned char bytes[] = {0x50, 0x59, 0x23, 0x29, 0x02, 0x07, 0x24, 0x00}, i = 0;	// THIS ARRAY SAVES THE VALUES OF RTC WHEN A READ OCCURS
unsigned char daysInEnglish[numberDay][stringLength] = {"SUN ", "MON ", "TUE ", "WED ", "THU ", "FRI ", "SAT "};	// THIS ARRAY CONTAINS ABBREVIATION OF DAYS NAME
unsigned char daysInFrensh[numberDay][stringLength] = {"DIM ", "LUN ", "MAR ", "MER ", "JEU ", "VEN " , "SAM "};
volatile unsigned char changeLanguage = 0;


enum state				// ENUMERATED TYPE TO SPECIFY THE STATE OF SPI
{
	READ = 0, 			
	WRITE = 1
}spiState = WRITE;

enum registers			// ENUMERATED TYPE TO SPECIFY EITHER SAVE MODE OR DUMMY MODE
{
	DUMMY,
	SAVE
}reg;

void displayResult(void);
void initialization(void);			
void delay250ms(void);		
void delay3us(void);			
void command(void);
void busyFlag(void);				
void data(void);	
void timerZero(void);
void home(unsigned char value);
void displayTime(void);
void displayDate(void);
void clearSecondLine(void);

#pragma interrupt interruptFunction
void interruptFunction(void)
{
	if(INTCONbits.INT0IF)
	{
		INTCONbits.INT0IF = 0;
		changeLanguage = ~changeLanguage;
	}
	else if(PIR1bits.SSPIF)	
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
	SLAVE_SELECT = 0;			// PULL LOW THE SLAVE TO START COMMUNICATION
	SSPBUF = CLOCK_BURST_WRITE;		// SEND A COMMAND FOR  BURST MODE FOR WRITING 
	while(TRUE);					// KEEP LOOPING
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
	home(0x84);
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
void displayDate(void)			// THIS FUNCTION DISPLAYS DATE : x/x/20x
{		
	unsigned char m = (bytes[5] - 1), n = 0, date = bytes[3], month = bytes[4], year = bytes[6];
	home(0xC0);
	if(m > 7)
		m = 0;
	home(0xC0);
	if(!changeLanguage)
	{
		for(; daysInEnglish[m][n] != '\0'; ++n)
		{
			LATD = daysInEnglish[m][n];
			data();
		}	
	}
	else
	{
		for(; daysInFrensh[m][n] != '\0'; ++n)
		{
			LATD = daysInFrensh[m][n];
			data();
		}
	}
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
}
void displayResult(void)
{
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
		EN_PIN = 0;
		delay3us();
		EN_PIN = 1;
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