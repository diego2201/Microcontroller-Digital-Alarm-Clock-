/*
===============================================================================
 Name        : FinalProject.c
 Author      : $(Diego Cifuentes)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
 */

/* Defining registers for I2C0 */
#define I2C0CONSET (*(volatile unsigned int *)0x4001c000)
#define I2C0STAT (*(volatile unsigned int *)0x4001c004)
#define I2C0DAT (*(volatile unsigned int *)0x4001c008)
#define I2C0ADR0 (*(volatile unsigned int *)0x4001c00C)
#define I2C0SCLH (*(volatile unsigned int *)0x4001c010)
#define I2C0SCLL (*(volatile unsigned int *)0x4001c014)
#define I2C0CONCLR (*(volatile unsigned int *)0x4001c018)

/* Defining registers for I2C0 */
#define T0TCR (*(volatile unsigned int *)0x40004004)
#define T0TC (*(volatile unsigned int *)0x40004008)

/* Defining PINSEL for I2C and DACR */
#define PINSEL1 (*(volatile unsigned int *)0x4002c004)

/* Defining Digital to Analog */
#define DACR (*(volatile unsigned int *)0x4008c000)

/* Defining register addresses for IO expander */
/* Defining as input */
#define GPIOA 0x12
#define GPIOB 0x13
/* Defining as output */
#define IODIRA 0x00
#define IODIRB 0x01

/* Defining registers for port 2 */
#define FIO2DIR (*(volatile unsigned int *)0x2009c040)
#define FIO2PIN (*(volatile unsigned int *)0x2009c054)

/* Defining registers for port 0 */
#define FIO0DIR (*(volatile unsigned int *)0x2009c000)
#define FIO0PIN (*(volatile unsigned int *)0x2009c014)

/* Defining registers for RTC */
#define PCONP (*(volatile unsigned int *)0x400FC0C4)
#define CCR (*(volatile unsigned int *)0x40024008)
#define SEC (*(volatile unsigned int *)0x40024020)
#define MIN (*(volatile unsigned int *)0x40024024)
#define HOUR (*(volatile unsigned int *)0x40024028)

/* Defining ports needed to be initialized as outputs */
const int outputPorts[] = {9, 8, 7, 0, 1, 18, 17, 15, 16, 23};

/* Defining global variables */
int sec, min, hour; //RTC values

volatile int digit;

//Flag variables
volatile int hasPressed;
volatile int flag;
volatile int check;

int aSec, aMin, aHour; //Alarm clock values

void initalize()
{
	for (int i = 0; i < sizeof(outputPorts) / sizeof(int); i++)
	{
		// Initialize LCD display pins as outputs
		FIO0DIR |= (1 << outputPorts[i]);
	}

	//Starting our RTC
	PCONP |= (1 << 9);
	CCR = 1;
	CCR &= ~(1 << 4);

	PINSEL1 |= (1 << 21); //enabling DAC by selecting AOUT on PINSEL register

	//Configuring our I2C
	PINSEL1 |= (1 << 22); //Configures SDA0
	PINSEL1 |= (1 << 24); //Configures SCL0
	I2C0SCLH = 5; //Sets high time
	I2C0SCLL = 5; //Sets low time
	I2C0CONCLR = (1 << 3);
	I2C0CONSET = (1 << 3);
	I2C0CONCLR = (1 << 6);
	I2C0CONSET = (1 << 6); //Enables I2C interface

	//Setting our flag values
	hasPressed = 0;
	flag = 0;
	check = 0;

	aSec = -1, aMin = -1, aHour= -1; //Alarm values

	//Set up for LCD display
	LCDwriteCommand(0x38);
	LCDwriteCommand(0x06);
	LCDwriteCommand(0x0c);
	LCDwriteCommand(0x01);

}

/* Function for the start functionality of I2C*/
void start()
{
	//19.9.1
	I2C0CONSET = (1 << 3); //Sets SI bit
	I2C0CONSET = (1 << 5); //Set START flag, enter master mode
	I2C0CONCLR = (1 << 3); //Clear SI bit before serial transfer can continue

	while(((I2C0CONSET >> 3) & 1) == 0)
	{
		//waits for SI bit to return to 1
	}

	I2C0CONCLR = (1 << 5); //Clears STA bit
}

/* Function for the write functionality of I2C*/
void write(int data)
{
	I2C0DAT = data;
	I2C0CONCLR = (1 << 3); //Clears SI bit

	while(((I2C0CONSET >> 3) & 1) == 0)
	{
		//waits for SI bit to return to 1
	}
}

/* Function for the read functionality of I2C*/
int read()
{
	int result;
	I2C0CONCLR = (1 << 3); //Clears SI bit
	I2C0CONCLR = (1 << 2); //Clears Ack bit
	while(((I2C0CONSET >> 3) & 1) == 0)
	{
		//waits for SI bit to return to 1
	}

	result = I2C0DAT;
	return result;
}

/* Function for the stop functionality of I2C*/
void stop()
{
	I2C0CONSET = (1 << 4); // set STO to request stop condition
	I2C0CONCLR = (1 << 3); // clear SI to activate state machine
	while(((I2C0CONSET >> 4) & 1) == 1)
	{
		//waits for STO bit to return to 0
	}
}

/* Function to check if the switch has been pressed*/
void checkSwitches()
{
	int switchState; //Variable to store switch state

	start();
	write(0b01000000); //The device op code
	write(GPIOA); //register
	start();
	write(0b01000001); //op code but with 1 at end to define read
	switchState = read(); //Calls read function and stores
	stop();

	if((switchState >> 7) & 1)
	{
		hasPressed = 1;
		pWait(0.001); //Wait for switch debounce
	}
	else if((switchState >> 6) & 1)
	{
		hasPressed = 2;
		pWait(0.001); //Wait for switch debounce
	}
	else if((switchState >> 5) & 1)
	{
		hasPressed = 3;
		pWait(0.001); //Wait for switch debounce
	}
	else if((switchState >> 4) & 1)
	{
		//Reset our flag variables
		flag = 0;
		hasPressed = 0;
		check = 0;
		pWait(0.001); //Wait for switch debounce
	}

}

/* Function to clear the LCD screen */
void clearScreen()
{
	LCDwriteCommand(0x80);
	LCDwriteString("                    ");
	LCDwriteCommand(0xC0);
	LCDwriteString("                    ");
	LCDwriteCommand(0x94);
	LCDwriteString("                    ");
	LCDwriteCommand(0xD4);
	LCDwriteString("                    ");
}

/* Function to output to multiple pins */
void busOut(int val)
{
	const int outputPorts[] = {7, 0, 1, 18, 17, 15, 16, 23};

	for (int i = 0; i < sizeof(outputPorts) / sizeof(int); i++)
	{
		if ((val >> i) & 1)
		{
			FIO0PIN |= (1 << outputPorts[i]);
		}
		else
		{
			FIO0PIN &= ~(1 << outputPorts[i]);
		}
	}
}

/* Wait function based off of MicroP code */
void pWait(float sec)
{
	volatile int count = sec * 21.33e6;
	while(count > 0)
	{
		count--;
	}
}

/* Function to write command to LCD display (based off of slides) */
void LCDwriteCommand(int command)
{
	busOut(command);
	//Drive RS low
	FIO0PIN &= ~(1 << 9);

	//Drive E High
	FIO0PIN |= (1 << 8);

	//Drive E low
	FIO0PIN &= ~(1 << 8);

	pWait(0.0001);
}

/* Function to write data to LCD display */
void LCDwriteData(int data)
{
	busOut(data);
	//Drive RS high
	FIO0PIN |= (1 << 9);

	//Drive E High
	FIO0PIN|= (1 << 8);

	//Drive E high then low
	FIO0PIN &= ~(1 << 8);

	pWait(0.0001);
}

/* Function to write string to LCD display */
void LCDwriteString(char string[])
{
	//For loop to write passed in string until null terminator
	for (int i = 0; string[i] != '\0'; i++)
	{
		LCDwriteData(string[i]);
	}
}

/* Function to convert ints to hex */
int segConvert(int num)
{
	switch(num)
	{
	case 0: digit = 0x30;
	break;
	case 1: digit = 0x31;
	break;
	case 2: digit = 0x32;
	break;
	case 3: digit = 0x33;
	break;
	case 4: digit = 0x34;
	break;
	case 5: digit = 0x35;
	break;
	case 6: digit = 0x36;
	break;
	case 7: digit = 0x37;
	break;
	case 8: digit = 0x38;
	break;
	case 9: digit = 0x39;
	break;
	}
	return digit;
}

/* Function to format and display time */
void time(int format)
{
	//If it is the first time running from another menu
	if(flag == 0)
	{
		clearScreen(); //Clear the screen
		flag = 1; //Set the flag
	}

	//Sets our RTC values
	sec = SEC;
	min = MIN;

	//If format is 0 we display in standard time (12 hour)
	if(format == 0)
	{
		hour = convertTime();
		LCDwriteCommand(0x86);
		LCDwriteData(segConvert(hour/10));
		LCDwriteData(segConvert(hour%10));
		LCDwriteString(":");
		LCDwriteData(segConvert(min/10));
		LCDwriteData(segConvert(min%10));
		LCDwriteString(":");
		LCDwriteData(segConvert(sec/10));
		LCDwriteData(segConvert(sec%10));
		if(HOUR > 12)
		{
			LCDwriteString(" PM");
		}
		else
		{
			LCDwriteString(" AM");
		}
	}
	else if(format == 1)
	{
		//Else we write in military time
		hour = HOUR;
		LCDwriteCommand(0x86);
		LCDwriteData(segConvert(hour/10));
		LCDwriteData(segConvert(hour%10));
		LCDwriteString(":");
		LCDwriteData(segConvert(min/10));
		LCDwriteData(segConvert(min%10));
		LCDwriteString(":");
		LCDwriteData(segConvert(sec/10));
		LCDwriteData(segConvert(sec%10));
	}
	else if(format == 2)
	{
		//This is done in order to display alarm time values
		LCDwriteCommand(0x9A);
		LCDwriteData(segConvert(aHour/10));
		LCDwriteData(segConvert(aHour%10));
		LCDwriteString(":");
		LCDwriteData(segConvert(aMin/10));
		LCDwriteData(segConvert(aMin%10));
		LCDwriteString(":");
		LCDwriteData(segConvert(aSec/10));
		LCDwriteData(segConvert(aSec%10));
	}
}

/* Function to allow user to set the time */
void setTime()
{
	int switchState; //Variable to store switch state

	//If it is the first time running from another menu
	if(flag == 0)
	{
		clearScreen(); //Clear the screen
		flag = 1; //Set the flag
	}

	pWait(0.001); //Wait for switch debounce
	LCDwriteCommand(0x80);
	LCDwriteString("Set:");
	flag = 1;

	//While loop to run for user input to set time
	while(1)
	{
		time(1);

		start();
		write(0b01000000); //The device op code
		write(GPIOA); //register
		start();
		write(0b01000001); //op code but with 1 at end to define read
		switchState = read(); //Calls read function and stores
		stop();

		//Various conditional statements to check which value to increment
		//Same logic for the first three check statements
		if((switchState >> 7) & 1)
		{
			if(HOUR == 24) //If we are at the limit
			{
				HOUR = 0; //Set back to zero
			}
			else
			{
				HOUR++; //Else increment
			}
			pWait(0.0001);

		}
		else if((switchState >> 6) & 1)
		{
			if(MIN == 59)
			{
				MIN = 0;
			}
			else
			{
				MIN++;
			}
			pWait(0.0001);

		}
		else if((switchState >> 5) & 1)
		{
			if(SEC == 59)
			{
				SEC = 0;
			}
			else
			{
				SEC++;
			}
			pWait(0.0001);
		}
		else if((switchState >> 4) & 1)
		{
			//The last switch is to set the time and exit
			flag = 0; //Reset respective flags
			hasPressed = 0; //Open menu upon exiting
			pWait(0.003);
			break; //Break out of while loop
		}
	}
}

/* Function to set alarm time to */
void setAlarm()
{
	int switchState; //Variable to store switch state
	pWait(0.001); //Wait for switch debounce

	//Same logic as setTime();
	while(check == 0)
	{
		time(1);

		start();
		write(0b01000000); //The device op code
		write(GPIOA); //register
		start();
		write(0b01000001); //op code but with 1 at end to define read
		switchState = read(); //Calls read function and stores
		stop();

		if((switchState >> 7) & 1)
		{
			if(aHour == 24)
			{
				aHour = 0;
			}
			else
			{
				aHour++;
			}
			pWait(0.0001);
		}
		else if((switchState >> 6) & 1)
		{
			if(aMin == 59)
			{
				aMin = 0;
			}
			else
			{
				aMin++;
			}
			pWait(0.0001);
		}
		else if((switchState >> 5) & 1)
		{
			if(aSec == 59)
			{
				aSec = 0;
			}
			else
			{
				aSec++;
			}
			pWait(0.0001);
		}
		else if((switchState >> 4) & 1)
		{
			//The last switch is to set the time and exit
			check = 1;
			flag = 0;
			pWait(0.005);
		}
		LCDwriteCommand(0x94);
		LCDwriteString("Set:");
		time(2);
	}
}

/* Function to convert military to standard */
int convertTime()
{
	int hour = 0;
	if(HOUR > 12)
	{
		hour = HOUR - 12;
	}
	else
	{
		hour = HOUR;
	}

	return hour;
}

/* Function to simulate wait based off timer register */
void wait(float us)
{
	T0TCR|= (1<<0);
	T0TC = 0;
	while(T0TC < us);
}

/* Function to generate our square wave/sound */
void soundNote(int time, int halfperiod)
{
	int val = 512;
	for (int i = 0; i < time; i++)
	{
		DACR = val << 6; //Sets our Analog output to our current amplitude
		wait(halfperiod); //Waits for the specified time
		DACR = 0; //Turns off the Analog output
		wait(halfperiod); //Waits for the specified time
		val --;
	}
}

/* Function to display our menu */
void menu()
{
	LCDwriteCommand(0x80);
	LCDwriteString("    ");
	LCDwriteCommand(0x86);
	LCDwriteData(segConvert(HOUR/10));
	LCDwriteData(segConvert(HOUR%10));
	LCDwriteString(":");
	LCDwriteData(segConvert(MIN/10));
	LCDwriteData(segConvert(MIN%10));
	LCDwriteString(":");
	LCDwriteData(segConvert(SEC/10));
	LCDwriteData(segConvert(SEC%10));
	LCDwriteCommand(0x8F);
	LCDwriteString("   ");
	LCDwriteCommand(0xC0);
	LCDwriteString(" 1)  Standard Time");
	LCDwriteCommand(0x94);
	LCDwriteString(" 2)  Set Alarm");
	LCDwriteCommand(0xD4);
	LCDwriteString(" 3)  Set Time");
}

/* Function to check alarm condition and create melody */
void alarm()
{
	//Check statement to see if all our values match alarm values
	if((HOUR == aHour) && (MIN == aMin) && (SEC == aSec))
	{
		//Calls time() in between sounds to keep updating time to user
		hasPressed = 0; //Reset to menu upon completion
		aSec = 0, aMin = 0, aHour= 0; //Alarm values
		time(1);
		soundNote(100,1276); //G4
		time(1);
		soundNote(200,1136); //A4
		time(1);
		soundNote(100,1276); //G4
		time(1);
		soundNote(200,1517); //E4
		time(1);
		soundNote(200,956);  //C5
		time(1);
		soundNote(200,1136); //A4
		time(1);
		soundNote(400,1276); //G4

		time(1);
		soundNote(200,1276); //G4
		time(1);
		soundNote(200,1136); //A4
		time(1);
		soundNote(200,1276); //G4
		time(1);
		soundNote(200,1136); //A4
		time(1);
		soundNote(200,1276); //G4
		time(1);
		soundNote(200,956);  //C5
		time(1);
		soundNote(400,1012); //B4

		time(1);
		soundNote(200,1432); //F4
		time(1);
		soundNote(200,1276); //G4
		time(1);
		soundNote(200,1432); //F4
		time(1);
		soundNote(200,1703); //D4
		time(1);
		soundNote(200,956);  //C5
		time(1);
		soundNote(200,1136); //A4
		time(1);
		soundNote(400,1276); //G4
		time(1);

		soundNote(200,1276); //G4
		time(1);
		soundNote(200,1136); //A4
		time(1);
		soundNote(200,1276); //G4
		time(1);
		soundNote(200,1136); //A4
		time(1);
		soundNote(200,1276); //G4
		time(1);
		soundNote(200,1136); //A4
		time(1);
		soundNote(500,1517); //E4
		time(1);
	}
}

/* Function to check alarm condition and create melody */
void chime()
{
	//Various conditionals to check if we are at the respective time
	//and plays the respective chime pattern
	if(MIN == 15 && SEC == 0)
	{
		time(1);
		soundNote(200,1204); //G#4
		time(1);
		soundNote(200,1351); //F#4
		time(1);
		soundNote(200,1517); //E4
		time(1);
		soundNote(400,2025); //B3
	}
	else if(MIN == 30 && SEC == 0)
	{
		time(1);
		soundNote(200,1517); //E4
		time(1);
		soundNote(200,1204); //G#4
		time(1);
		soundNote(200,1351); //F#4
		time(1);
		soundNote(400,2025); //B3
		time(1);
		soundNote(200,1517); //E4
		time(1);
		soundNote(200,1351); //F#4
		time(1);
		soundNote(200,1204); //G#4
		time(1);
		soundNote(400,1517); //E4
	}
	else if(MIN == 45 && SEC == 0)
	{
		time(1);
		soundNote(200,1204); //G#4
		time(1);
		soundNote(200,1517); //E4
		time(1);
		soundNote(200,1351); //F#4
		time(1);
		soundNote(400,2025); //B3
		time(1);
		soundNote(200,2025); //B3
		time(1);
		soundNote(200,1351); //F#4
		time(1);
		soundNote(200,1204); //G#
		time(1);
		soundNote(400,1517); //E4
		time(1);
		soundNote(200,1204); //G#4
		time(1);
		soundNote(200,1351); //F#4
		time(1);
		soundNote(200,1517); //E4
		time(1);
		soundNote(400,2025); //B3
	}
	else if(MIN == 0 && SEC == 0)
	{
		//Calls convertTime() to get the correct amount of times
		//the bell should ring
		int tocks = convertTime();

		time(1);
		soundNote(200,1517); //E4
		time(1);
		soundNote(200,1204); //G#4
		time(1);
		soundNote(200,1351); //F#4
		time(1);
		soundNote(400,2025); //B3
		time(1);
		soundNote(200,1517); //E4
		time(1);
		soundNote(200,1351); //F#4
		time(1);
		soundNote(200,1204); //G#4
		time(1);
		soundNote(400,1517); //E4
		time(1);
		soundNote(200,1204); //G#4
		time(1);
		soundNote(200,1517); //E4
		time(1);
		soundNote(200,1351); //F#4
		time(1);
		soundNote(400,2025); //B3
		time(1);
		soundNote(200,2025); //B3
		time(1);
		soundNote(200,1351); //F#4
		time(1);
		soundNote(200,1204); //G#4
		time(1);
		soundNote(400,1517); //E4
		time(1);

		//For loop to run as many tocks
		for(int i = 0; i < tocks; i++)
		{
			time(1);
			soundNote(400,3034); //E3
		}
		hasPressed = 0;
	}

}

int main(void)
{
	initalize(); //Initalizes all respective registers and variables

	while(1)
	{
		//Calls the following three to check if their respective conditions are met
		checkSwitches();
		chime();
		alarm();

		//If we are in state zero
		if(hasPressed == 0)
		{
			menu(); //Display menu
		}

		//If we are in state 1
		if(hasPressed == 1)
		{
			time(0); //Display standard time
		}
		else if(hasPressed == 2)
		{
			//If in state 2
			setAlarm(); //Ask user to set alarm
		}
		else if(hasPressed == 3)
		{
			//If in state 3
			setTime(); //Ask user to set time
		}
	}
	return 0;
}
