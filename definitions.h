#ifndef _HEADER
#define _HEADER

#define TRUE 0x01
#define FALSE 0x00
#define numberDay 0x07
#define stringLength 11


#define ARROW_SECOND_LINE 0xC4
#define ARROW_THIRD_LINE 0x98

#define 	RS_PIN				LATCbits.LATC0		// RS PIN CONNECTED TO RC0
#define 	RW_PIN 				LATCbits.LATC1		// RW PIN CONNECTED TO RC1
#define 	EN_PIN				LATCbits.LATC2		// ENABLE PIN CONNECTED TO RC2


#define CLOCK_BURST_WRITE 0x3F						// BURST MODE FOR WRITING
#define CLOCK_BURST_READ 0xBF						// BURST MODE FOR READING
#define DUMMY_DATA 	0x00							// DUMMY DATA 
#define CONTROL_WRITE 0x0F							// CONTROL REGISTER USED TO DEFINED THE WP BIT
#define SLAVE_SELECT LATCbits.LATC6					// SLAVE SELECT PIN IS CONNECTED TO RC6

volatile unsigned char bytes[] = {0x50, 0x59, 0x23, 0x31, 0x01, 0x05, 0x24, 0x00}, i = 0;	// THIS ARRAY SAVES THE VALUES OF RTC WHEN A READ OCCURS
unsigned char daysInEnglish[numberDay][stringLength] = {"SUNDAY ", "MONDAY ", "TUESDAY ", 
"WEDNESDAY ", "THURSDAY ", "FRIDAY ", "SATURDAY "};	// THIS ARRAY CONTAINS ABBREVIATION OF DAYS NAME
unsigned char daysInFrensh[numberDay][stringLength] = {"DIMANCHE ", "LUNDI ", "MARDI ", "MERCREDI ", "JEUDI ", "VENDREDI ", "SAMEDI "};
unsigned char changePosition = 0, choose = 0;


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

void displayArrow(void);
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
void displayParameter(void);
void changeArrowPosition(void);
void clearHome(void);
void displayInformation(void);
void displaySpace(void);
void displayString(const unsigned char* string);
#endif