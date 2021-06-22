/*
	OilerLib Example sketch - Debounce

	Author:	Mark Naylor, June 2021

	Description

	This sample sketch demonstrates how to set the debounce timing on a motor output sensor
	it uses a sinple example of a single dc motor controlled by a relay and allows the user to change the debounce value and see how this impacts the way oil drips are seen
	Whilst the main purpose of this is to demonstrate the API it can also be used to calibrate the sensors and the results used as part of the users final sketch.

	This menu uses ANSI cursor positioning commands not supported by the standard Arduino Serial monitor. To see this properly use a terminal emulator such as the free PuTTY program
*/
#include "OilerLib.h"

#define OILED_DEVICE_ACTIVE_PIN1		17			// digital pin which will go high when drips sent from motor 1
#define PUMP1_NUM_DRIPS					3			// number of drips after which motor 1 is paused

#define DELAY_ROW						5
#define DELAY_COL						25
#define DELAY_COL_RESULT				48
#define OILER_STATE_ROW					15
#define OILER_STATE_COL					5
#define	OILER_STATE_COL_RESULT			20

uint8_t uiMotorCount = 0;							// number of motors added

void setup ()
{
	Serial.begin ( 19200 );
	while ( !Serial );

	String Heading = F ( "\nOiler Example Sketch, Version " );
	Heading += String ( OILER_VERSION );
	Serial.println ( Heading );

	if ( TheOiler.AddMotor ( 4, 5, 6, 7, 800, OILED_DEVICE_ACTIVE_PIN1, PUMP1_NUM_DRIPS ) == false )
	// add 1 pin relay motor on Pin 4
	//if ( TheOiler.AddMotor ( 4, OILED_DEVICE_ACTIVE_PIN1, PUMP1_NUM_DRIPS ) == false )
	{
		Serial.println ( F ( "Unable to add relay motor to oiler, stopped" ) );
		while ( 1 );
	}
	else
	{
		uiMotorCount++;
	}

	// Set the oiler to restart pumps every 20 seconds
	if ( TheOiler.SetStartEventToTime ( 20 ) == false )
	{
		Serial.println ( F ( "Unable to set oiler to restart pumps after 20 seconds, stopped" ) );
		while ( 1 );
	}

	// Start Oiler
	if ( TheOiler.On () == false )
	{
		Serial.println ( F ( "Unable to start oiler, stopped" ) );
		while ( 1 );
	}
	else
	{
		Serial.println ( F ( "Oiler started" ) );
	}
	
}

void loop ()
{
	uint16_t uiMotorSelected = 0;

	while ( true )
	{
		DisplaySelectMotorMenu ();
		while ( Serial.available () <= 0 )
		{
			delay ( 50 );		// wait for input
		}
		int32_t uiInput = Serial.parseInt ();
		if ( uiInput > 0 && uiInput <= uiMotorCount )
		{
			uiMotorSelected = uiInput;
			SetDebounce ( uiMotorSelected );
		}
		else
		{
			Error ( F ( "Invalid input" ) );
		}
	}
}

void SetDebounce ( uint16_t uiMotorNum )
{
	bool bExit = false;
	int32_t ulDelay = 150;
	ResetStateInfo ();
	DisplayDebounceMenu ( uiMotorNum, ulDelay );
	while ( bExit == false )
	{
		while ( Serial.available () <= 0 )
		{
			UpdateState ( ulDelay, uiMotorNum );			// show state of Oiler
			delay ( 250 );		// wait for input
		}
		int32_t ulNewDelay = Serial.parseInt ( SKIP_NONE );
		if ( ulNewDelay == 0L )
		{
			if ( Serial.read() == 'x' )
			{
				bExit = true;
			}
		}
		else
		{
			// set delay

			if ( TheOiler.SetMotorSensorDebounce ( uiMotorNum - 1, ulNewDelay ) == false )
			{
				Error ( F ( "Unable to set the debounce entered" ) );				
			}
			else
			{
				ulDelay = ulNewDelay;
				Error ( F ( "" ) );
			}
		}
	}
}

// used to hold prior values so we can check if a change has happened and update screen if necessary
struct
{
	uint32_t		ulOldDelay;
	String			sOldOilerState;
	String			sOldMotorState;
	uint32_t		ulOldWorkCount;
} StateInfo;

void ResetStateInfo ( void )
{
	StateInfo.ulOldDelay		= 0;
	StateInfo.sOldOilerState	= F ( "" );
	StateInfo.sOldMotorState	= F ( "" );
	StateInfo.ulOldWorkCount	= 0;
}

void UpdateState (uint32_t ulDelay, uint8_t uiMotorNum )
{
	String s;

	if ( StateInfo.ulOldDelay != ulDelay )
	{
		// has changed so update screen
		StateInfo.ulOldDelay = ulDelay;
		s = String ( ulDelay );
		ClearPartofLine ( DELAY_ROW, DELAY_COL_RESULT, 8 );
		AT ( DELAY_ROW, DELAY_COL_RESULT, s );
	}

	// show state of oiler
	if ( TheOiler.IsOff () )
	{
		s = "Off";
	}
	else 
	{
		if ( TheOiler.IsIdle () )
		{
			s = "Idle";
		}
		else
		{
			if ( TheOiler.IsOiling () )
			{
				s = "Oiling";
			}
			else
			{
				s = "?";
			}
		}
	}
	if ( s != StateInfo.sOldOilerState )
	{
		// has changed
		StateInfo.sOldOilerState = s;
		ClearPartofLine ( OILER_STATE_ROW, OILER_STATE_COL_RESULT, 8 );
		AT ( OILER_STATE_ROW, OILER_STATE_COL_RESULT, s );
	}

	// show state of selected motor, remember motors are indexed from 0
	if ( TheOiler.IsMotorRunning ( uiMotorNum - 1 ) == true )
	{
		s = F ( "running" );
	}
	else
	{
		s = F ( "stopped" );
	}
	if ( s != StateInfo.sOldMotorState )
	{
		// Has changed
		StateInfo.sOldMotorState = s;
		ClearPartofLine ( OILER_STATE_ROW + 1, OILER_STATE_COL_RESULT, 8 );
		AT ( OILER_STATE_ROW + 1, OILER_STATE_COL_RESULT, s );
	}

	// show units of work of selected motor
	uint32_t ulCount = TheOiler.GetMotorWorkCount ( uiMotorNum - 1 );
	if ( ulCount != StateInfo.ulOldWorkCount )
	{
		// Has changed
		StateInfo.ulOldWorkCount = ulCount;
		ClearPartofLine ( OILER_STATE_ROW + 2, OILER_STATE_COL_RESULT, 8 );
		AT ( OILER_STATE_ROW + 2, OILER_STATE_COL_RESULT, String ( ulCount ) );
	}
}

void DisplaySelectMotorMenu ()
{
	ClearScreen ();
	String s = F ( "Debounce Example, using Oilerlib ver " );
	s += OILER_VERSION;
	AT ( 1, 20, s );

	s = F ( "Enter Motor number (1-" );
	s = s + uiMotorCount + F ( ")" );
	AT ( 10, 25, s );
}

void DisplayDebounceMenu ( uint16_t uiMotorNum, uint32_t ulDelay )
{
	ClearScreen ();
	String s = F ( "Debounce Example, using Oilerlib ver " );
	s += OILER_VERSION;
	AT ( 1, 20, s );
	s = F ( "Setting debounce for motor " );
	s += uiMotorNum;
	AT ( 3, 25, s );

	AT ( DELAY_ROW, DELAY_COL, F ( "Debounce delay set to " ) );

	AT ( 12, 25, F ( "Enter debounce value (ms) " ) );
	AT ( 13, 25, F ( "Enter x to select another motor" ) );

	AT ( OILER_STATE_ROW , OILER_STATE_COL, F ( "Oiler is " ) );

	AT ( OILER_STATE_ROW + 1, OILER_STATE_COL, F ( "Motor is " ) );

	AT ( OILER_STATE_ROW + 2, OILER_STATE_COL, F ( "Drips seen " ) );

}
/*

				Code to manipulate the screen

*/

#define MAX_COLS			80
#define MAX_ROWS			25
#define ERROR_ROW			25
#define ERROR_COL			1

// defines for ansi terminal sequences
#define CSI				F("\x1b[")
#define SAVE_CURSOR		F("\x1b[s")
#define RESTORE_CURSOR	F("\x1b[u")
#define CLEAR_LINE		F("\x1b[2K")
#define RESET_COLOURS   F("\x1b[0m")

// colors
#define FG_BLACK		30
#define FG_RED			31
#define FG_GREEN		32
#define FG_YELLOW		33
#define FG_BLUE			34
#define FG_MAGENTA		35
#define FG_CYAN			36
#define FG_WHITE		37

#define BG_BLACK		40
#define BG_RED			41
#define BG_GREEN		42
#define BG_YELLOW		43
#define BG_BLUE			44
#define BG_MAGENTA		45
#define BG_CYAN			46
#define BG_WHITE		47
void ClearScreen ()
{
	Serial.print ( "\x1b[2J" );
}

void AT ( uint8_t row, uint8_t col, String s )
{
	row = row == 0 ? 1 : row;
	col = col == 0 ? 1 : col;
	String m = String ( CSI ) + row + String ( ";" ) + col + String ( "H" ) + s;
	Serial.print ( m );
}

void COLOUR_AT ( uint8_t FGColour, uint8_t BGColour, uint8_t row, uint8_t col, String s )
{
	// set colours
	Serial.print ( String ( CSI ) + FGColour + ";" + BGColour + "m" );
	AT ( row, col, s );
	// reset colours
	Serial.print ( RESET_COLOURS );
}

void ClearPartofLine ( uint8_t row, uint8_t start_col, uint8_t toclear )
{
	static char buf [ MAX_COLS + 1 ];
	memset ( buf, ' ', sizeof ( buf ) - 1 );
	buf [ MAX_COLS ] = 0;

	// build string of toclear spaces
	toclear %= MAX_COLS + 1;						// ensure toclear is <= MAX_COLS
	if ( start_col + toclear > MAX_COLS + 1 )
	{
		toclear = MAX_COLS - start_col + 1;
	}
	toclear = ( MAX_COLS - start_col + 1 ) % ( MAX_COLS + 1 );	// ensure toclear doesn't go past end of line
	SaveCursor ();
	buf [ toclear ] = 0;
	AT ( row, start_col, buf );
	RestoreCursor ();
}

void ClearLine ( uint8_t row )
{
	SaveCursor ();
	AT ( row, 1, String ( CLEAR_LINE ) );
	RestoreCursor ();
}
void SaveCursor ( void )
{
	Serial.print ( SAVE_CURSOR );
}

void RestoreCursor ( void )
{
	Serial.print ( RESTORE_CURSOR );
}

void Error ( String s )
{
	// Clear error line
	ClearLine ( ERROR_ROW );
	// Output new error
	COLOUR_AT ( FG_WHITE, BG_RED, ERROR_ROW, ERROR_COL, s );
}