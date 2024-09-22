#include <p18f452.h>
#pragma config WDT = OFF


#define 	RS_PIN				LATCbits.LATC0
#define 	RW_PIN 				LATCbits.LATC1
#define 	EN_PIN				LATCbits.LATC2
#define 	SS					LATCbits.LATC6

#define 	SIZE				0x08
#define 	burstModeWrite		0x3F			
#define 	burstModeRead 		0xBF
#define 	dummyData			0x00




#define DAY_INDEX				0x05		
#define MINUTE_INDEX			0x01
#define HOUR_INDEX				0x02
#define MONTH_INDEX				0x04			
#define YEAR_INDEX				0x06	
#define DATE_INDEX				0x03
#define SECOND_INDEX			0x00

unsigned char clockValues[] = {0x00, 0x59, 0x23, 0x31, 0x12, 0x01, 0x24, 0x80}, trackValue = 0;    // initialization of each rtc register

unsigned char clockReadValues[SIZE];


unsigned char days[7][20] = {"SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"};	// array that contains days of week

enum spiState
{
	RTC_WRITE,
	RTC_READ
}SPI;

enum dataState
{
	BURST_MODE,
	DUMMY_DATA,
	REAL_DATA

}dataKind;

void initialization(void);			
void delay250ms(void);		
void delay3us(void);			
void command(void);
void busyFlag(void);				
void data(void);	
void timerZero(void);
void lcdDisplay(void);
void displayDate(void);
void displayTime(void);

#pragma interrupt myFunction
void myFunction(void)
{
	if(PIR1bits.SSPIF)
	{
		PIR1bits.SSPIF = 0;
		if(SPI == RTC_WRITE)					// writing into rtc registers
		{
			SSPBUF = clockValues[trackValue++];
			if(trackValue == 8)
				SPI = RTC_READ;
		}
		else if(SPI == RTC_READ)				// reading from rtc registers
		{
			if(dataKind == BURST_MODE)	
			{
				trackValue = 0x00;
				SS = 1;
				SS = 0;
				SSPBUF = burstModeRead;
				dataKind = DUMMY_DATA;
			}
			else if(dataKind == DUMMY_DATA)		// sending dummy data through mosi line
			{
				SSPBUF = dummyData;
				dataKind = REAL_DATA;
			}
			else
			{
				dataKind = DUMMY_DATA;
				clockReadValues[trackValue++] = SSPBUF;		// read the data from miso line
				if(trackValue == 0x08)
				{
					SS = 1;									// disable slave select
					dataKind = BURST_MODE;	
					lcdDisplay();							// display time and date
				}
				PIR1bits.SSPIF = 1;							// trigger spi interrupt forever
			}
		}
	}
}


#pragma code myVector = 0x00008
void myVector(void)
{
	_asm
		GOTO myFunction
	_endasm
}
#pragma code


void main(void)
{
	TRISC = 0x90;					
	TRISD = 0x00;
	SPI = RTC_WRITE;
	SSPCON1 = 0x21;					// CONFIGURATION OF SPI
	SSPSTAT = 0xC0;					
	dataKind = BURST_MODE;			
	initialization();
	INTCONbits.GIE = 1;
	INTCONbits.PEIE = 1;
	PIE1bits.SSPIE = 1;			// SPI INTERRUPT MODE
	PIR1bits.SSPIF = 0;
	SS = 0;						// ENABLE SLAVE SELECT
	SSPBUF = burstModeWrite;	// BURST MODE INITIALIZATION 
	while(1);
}


void lcdDisplay(void)
{
	static unsigned char state = 0x00, oldSecond;
	if(state == 0x00)
	{
		state = 0x01;
		oldSecond = clockReadValues[SECOND_INDEX];
		displayDate();
		displayTime();
	}
	else
	{
		if(oldSecond != clockReadValues[SECOND_INDEX])				// DISPLAY DATA ONLY WHEN THE MINUTE CHANGES
		{
			oldSecond = clockReadValues[SECOND_INDEX];
			LATD = 0x80;
			command();	
			displayDate();								// UPDATE DATE
			displayTime();			// UPDATE TIME
		}
	}
}

void displayDate(void)
{
	unsigned char i = clockReadValues[DAY_INDEX], j = 0;
	while(days[i-1][j] != '\0')
	{
		LATD = days[i-1][j++];
		data();
	}
	LATD = 0x20;
	data();


	//converting a BCD value to an ascii code in order to be displayed in lcd


	LATD = ((clockReadValues[DATE_INDEX] & 0xF0) >> 4) + 0x30;			// get the highest nibble	
	data();
	LATD = (clockReadValues[DATE_INDEX] & 0x0F) + 0x30;					// get the lowest nibble
	data();
	LATD = '/';		// separate between two values 
	data();
	LATD = ((clockReadValues[MONTH_INDEX] & 0xF0) >> 4) + 0x30;				
	data();
	LATD = (clockReadValues[MONTH_INDEX] & 0x0F) + 0x30;		
	data();
	LATD = '/';
	data();
	LATD = 0x32;			// adding 2 and 0 to concatinate them with the year from rtc to build 2024
	data();
	LATD = 0x30;
	data();
	LATD = ((clockReadValues[YEAR_INDEX] & 0xF0) >> 4) + 0x30;		
	data();
	LATD = (clockReadValues[YEAR_INDEX] & 0x0F) + 0x30;		
	data();
	LATD = 0xC0;				// jump second line
	command();
	busyFlag();
}
void displayTime(void)
{
	signed char i = 2, state = 0x00;
	while( i >= 0)
	{
		LATD = 0x30 + ((clockReadValues[i] & 0xF0) >> 4);	
		data();
		LATD = 0x30 + (clockReadValues[i] & 0x0F);	
		data();	
		if( i != 0)
		{
			LATD = ':';
			data();
		}
		--i;	
	}
}

void initialization(void)				// lcd initialization
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
void delay3us(void)					// 3 Us for EN_PIN pulse
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