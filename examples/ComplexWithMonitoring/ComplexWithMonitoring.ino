/*
	OilerLib Example sketch - ComplexWithMonitoring

	Author:	Mark Naylor, June 2021

	Description

	This sample sketch demonstrates using the OilerLib library to:
		control two pumps using stepper motors using a ULN2003 stepper driver
		respond to motor related input sensors that indicates the pump has output oil and pause the related motor when configured amount of oil is delivered
		restart motors after a specified elapsed time
		raise an alert signal if a pump is delayed in delivering oil
		respond to inputs from the machine being oiled indicating the machine is powered on and when the machine has completed work (eg its spindle has turned)
		use the above inputs to additionally be able to restart the pumps after the machine has been powered for a given time or when it has rotated a given number of times.

	The sketch has a basic menu that allows the user to control the Oiler and to see the state of the Oiler.
	This menu uses ANSI cursor positioning commands not supported by the standard Arduino Serial monitor. To see this properly use a terminal emulator such as the free PuTTY program
*/
#include "OilerLib.h"


#define NUM_MOTORS						2			// number of motors used to deliver oil

#define OILED_DEVICE_ACTIVE_PIN1		17			// digital pin which will go high when drips sent from motor 1
#define OILED_DEVICE_ACTIVE_PIN2		3			// digital pin which will go high when drips sent from motor 2
#define STEPPER_SPEED_DEFAULT			800			// works well with ULN2003 stepper driver
#define PUMP1_NUM_DRIPS					3			// number of drips after which motor 1 is paused
#define PUMP2_NUM_DRIPS					1			// number of drips after which motor 2 is paused

#define	MACHINE_ACTIVE_PIN				12			// digital Pin used to indicate target machine is doing work, set to NOT_A_PIN if this feature is not implemented
#define	MACHINE_ACTIVE_TIME_TARGET		30			// Number of seconds of active time after which target machine is ready to be oiled
#define	MACHINE_WORK_PIN				13			// digital Pin which signals when a unit of work by target machine is completed, set to NOT_A_PIN if not implemented
#define	MACHINE_WORK_UNITS_TARGET		3			// Number of signals that indicates machine is ready (eg how many revolutions of spindle)

#define ALERT_PIN						19			// digital Pin to signal if not completed oiling in multiple of oiler start target (eg elapsed time / revs / powered on time)
#define ALERT_THRESHOLD					2			// Example only - set to twice normal target

#define ELAPSED_TIME_SECS				30			// Example only - restart paused pumps after 30 seconds
#define POWERED_TIME_SECS				20			// Example only - restart paused pumps after 20 seconds of target machine having power
#define TARGETMACHINE_WORK				3			// Example only - restart paused pumps after target machine has completed 3 units of work (e.g. 3 spindle revolutions)

void setup ()
{
	Serial.begin ( 19200 );
	while ( !Serial );
	ClearScreen ();

	// Add 4 pin stepper motor to Oiler connected on pins 4,5,6,7, 
	if ( TheOiler.AddMotor ( 4, 5, 6, 7, STEPPER_SPEED_DEFAULT, OILED_DEVICE_ACTIVE_PIN1, PUMP1_NUM_DRIPS ) == false )
	{
		Error ( F ( "Unable to add stepper motor to oiler, stopped" ) );
		while ( 1 );
	}
	// Add second motor on pin 8,9,10,11
	if ( TheOiler.AddMotor ( 8, 9, 10, 11, STEPPER_SPEED_DEFAULT, OILED_DEVICE_ACTIVE_PIN2, PUMP2_NUM_DRIPS ) == false )
	{
		Error ( F ( "Unable to add stepper motor to oiler, stopped" ) );
		while ( 1 );
	}

	/*
	*		Alternatively if using relays to drive a dc motor replace the above with the code commented out below
	*/
	/*
	// add 1 pin relay motor on Pin 4
	if ( TheOiler.AddMotor ( 4, OILED_DEVICE_ACTIVE_PIN1, PUMP1_NUM_DRIPS ) == false )
	{
		Error ( F ( "Unable to add relay motor to oiler, stopped" ) );
		while ( 1 );
	}
	// add second 1 pin relay motor on Pin 5
	if ( TheOiler.AddMotor ( 5, OILED_DEVICE_ACTIVE_PIN2, PUMP2_NUM_DRIPS ) == false )
	{
		Error ( F ( "Unable to add relay motor to oiler, stopped" ) );
		while ( 1 );
	}
	*/

	// This next step is optional, the Oiler will work without it. It simply gives a better experience.
	// TheMachine represents the machine being oiled and uses two pins to signal when the machine is powered on (Active) and also when it has completed a 
	// unit of work (e.g. a revolution of a spindle). See TargetMachine.h for configuration of these pins (eg INPUT vs INPUT_PULLUP or HIGH vs LOW)
	// as well as the number of units of work and elapsed time of machine being powered on after which it is ready to be oiled.
	// Note that its not necessary to have both configured if the feature is not available

	// Since we have one, we configure its pins and targets and then add to TheOiler as follows:
	if ( TheMachine.AddFeatures ( MACHINE_ACTIVE_PIN, MACHINE_WORK_PIN, MACHINE_ACTIVE_TIME_TARGET, MACHINE_WORK_UNITS_TARGET ) == false )
	{
		Error ( F ( "Unable to configure TargetMachine" ) );
		while ( 1 );
	}

	TheOiler.AddMachine ( &TheMachine );

	// Demonstrate how to turn on functionality that will generate a signal if oiling takes too long
	if ( TheOiler.SetAlert ( ALERT_PIN, ALERT_THRESHOLD ) == false )
	{
		Error ( F ( "Unable to add Alert feature to oiler, stopped" ) );
		while ( 1 );
	}

	// By default the oiler will work on an elapsed time basis

	DisplayMenu ();
}
void loop ()
{

	// All work should happen in the background
	// loop can be used to control oiler or do other functions as below

	if ( Serial.available () > 0 )
	{
		switch ( Serial.read () )
		{
			case '1':	// On
				if ( TheOiler.On () == false )
				{
					Error ( F ( "Unable to start oiler, stopped" ) );
					while ( 1 );
				}
				else
				{
					DisplayOilerStatus ( F ( "Oiler Started" ) );
				}
				break;

			case '2':	// Off
				TheOiler.Off ();
				DisplayOilerStatus ( F ( "Oiler Stopped" ) );
				break;

			case '3':	// Motors Forward
				TheOiler.SetMotorsForward ();
				DisplayOilerStatus ( F ( "Oiler moving forward" ) );
				break;

			case '4':	// Motors Backward
				TheOiler.SetMotorsBackward ();
				DisplayOilerStatus ( F ( "Oiler moving backward" ) );
				break;

			case '5': // TIME_ONLY - basic mode -  oil every n secs regardless
				if ( TheOiler.SetStartEventToTime ( ELAPSED_TIME_SECS ) )
				{
					DisplayOilerStatus ( F ( "Oiler in basic elapsed time mode" ) );
				}
				else
				{
					Error ( F ( "Unable to set basic elapsed time mode" ) );
				}
				break;

			case '6': // POWERED_ON - adv mode - oil every n secs target machine is powered on
				if ( TheOiler.SetStartEventToTargetActiveTime ( POWERED_TIME_SECS ) )
				{
					DisplayOilerStatus ( F ( "Oiler in advanced target machine powered time mode" ) );
				}
				else
				{
					Error ( F ( "Unable to set target machine powered time mode" ) );
				}
				break;

			case '7': // WORK_UNITS - adv mode - oil every 3 units done by target machine, eg every 3 spindle revs
				if ( TheOiler.SetStartEventToTargetWork ( TARGETMACHINE_WORK ) )
				{
					DisplayOilerStatus ( F ( "Oiler in advanced target machine work mode" ) );
				}
				else
				{
					Error ( F ( "Unable to set ON_TARGET_ACTIVITY mode" ) );
				}
				break;

			default:
				break;
		}
	}
	// display work units per motor
	DisplayStats ();
}

// code to draw screen
#define ERROR_ROW			25
#define ERROR_COL			1
#define STATUS_LINE			24
#define STATUS_START_COL	30
#define STATS_ROW			8
#define STATS_RESULT_COL	70
#define MODE_ROW			20
#define MODE_RESULT_COL		45
#define MAX_COLS			80
#define MAX_ROWS			25

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

// following are routines to output ANSI style terminal emulation
void DisplayMenu ()
{
	String Heading = F ( "Oiler Example Sketch, Version " );
	Heading += String ( OILER_VERSION );
	COLOUR_AT ( FG_GREEN, BG_BLACK, 1, 30, Heading );
	AT ( 5, 10, F ( "1 - Turn Oiler On" ) );
	AT ( 6, 10, F ( "2 - Turn Oiler 0ff" ) );
	AT ( 7, 10, F ( "3 - Motors Forward" ) );
	AT ( 8, 10, F ( "4 - Motors Backward" ) );
	AT ( 9, 10, F ( "5 - Elapsed time mode" ) );
	AT ( 10, 10, F ( "6 - Target powered time mode" ) );
	AT ( 11, 10, F ( "7 - Target work units mode" ) );
	AT ( STATS_ROW - 1, STATS_RESULT_COL - 14, F ( "STATS" ) );
	AT ( STATS_ROW + 0, STATS_RESULT_COL - 14, F ( "Oiler Idle    N/A" ) );
	AT ( STATS_ROW + 1, STATS_RESULT_COL - 14, F ( "Motor1 Units  N/A" ) );
	AT ( STATS_ROW + 2, STATS_RESULT_COL - 14, F ( "Motor1 State  N/A" ) );
	AT ( STATS_ROW + 3, STATS_RESULT_COL - 14, F ( "Motor1 Act(s) N/A" ) );
	AT ( STATS_ROW + 4, STATS_RESULT_COL - 14, F ( "Motor2 Units  N/A" ) );
	AT ( STATS_ROW + 5, STATS_RESULT_COL - 14, F ( "Motor2 State  N/A" ) );
	AT ( STATS_ROW + 6, STATS_RESULT_COL - 14, F ( "Motor2 Act(s) N/A" ) );
	AT ( STATS_ROW + 7, STATS_RESULT_COL - 14, F ( "Machine Units N/A" ) );
	AT ( STATS_ROW + 8, STATS_RESULT_COL - 14, F ( "Machine Time  N/A" ) );
	AT ( MODE_ROW + 0, MODE_RESULT_COL - 14, F ( "Oiler awake   None" ) );
	AT ( MODE_ROW + 1, MODE_RESULT_COL - 14, F ( "Oiler Status  OFF" ) );
}
const char* Modes [] =
{
	"On elapsed time",
	"On powered time",
	"On target work",
	"NONE"
};

void DisplayStats ( void )
{
	static uint8_t						uiLastCount [ NUM_MOTORS ];
	static bool							bLastState [ NUM_MOTORS ];
	static uint32_t						ulLastMotorRunTime [ NUM_MOTORS ];
	static uint32_t						ulLastIdleSecs = 0UL;
	static uint32_t						ulLastMachineUnits = 0UL;
	static uint32_t						ulLastMachineIdleSecs = 0UL;
	static String						sLastState;
	static String						sLastMode = F ( "None" );

	uint32_t ulIdleSecs = TheOiler.GetTimeOilerIdle ();
	if ( ulIdleSecs != ulLastIdleSecs )
	{
		ulLastIdleSecs = ulIdleSecs;
		ClearPartofLine ( STATS_ROW, STATS_RESULT_COL, MAX_COLS - STATS_RESULT_COL );
		AT ( STATS_ROW, STATS_RESULT_COL, String ( ulIdleSecs ) );
	}
	for ( int i = 0; i < NUM_MOTORS; i++ )
	{
		uint8_t uiWorkDone = TheOiler.GetMotorWorkCount ( i );
		if ( uiWorkDone != uiLastCount [ i ] )
		{
			uiLastCount [ i ] = uiWorkDone;
			ClearPartofLine ( STATS_ROW + i * 2 + 1, STATS_RESULT_COL, MAX_COLS - STATS_RESULT_COL );
			AT ( STATS_ROW + i * 2 + 1, STATS_RESULT_COL, String ( uiWorkDone ) );
		}

		bool bResult = TheOiler.IsMotorRunning ( i );
		if ( bResult != bLastState [ i ] )
		{
			bLastState [ i ] = bResult;
			ClearPartofLine ( STATS_ROW + i * 2 + 2, STATS_RESULT_COL, MAX_COLS - STATS_RESULT_COL );
			AT ( STATS_ROW + i * 2 + 2, STATS_RESULT_COL, bResult == true ? F ( "Running" ) : F ( "Stopped" ) );
		}

		uint32_t ulResult = TheOiler.GetTimeSinceMotorStarted ( i );
		if ( ulResult != ulLastMotorRunTime [ i ] )
		{
			ulLastMotorRunTime [ i ] = ulResult;
			ClearPartofLine ( STATS_ROW + i * 2 + 3, STATS_RESULT_COL, MAX_COLS - STATS_RESULT_COL );
			AT ( STATS_ROW + i * 2 + 3, STATS_RESULT_COL, String ( ulResult ) );
		}
	}

	// Update machine info if necessary
	uint32_t ulMachineUnits = TheMachine.GetWorkUnits ();
	if ( ulMachineUnits != ulLastMachineUnits )
	{
		ulLastMachineUnits = ulMachineUnits;
		ClearPartofLine ( STATS_ROW + 7, STATS_RESULT_COL, MAX_COLS - STATS_RESULT_COL );
		AT ( STATS_ROW + 7, STATS_RESULT_COL, String ( ulMachineUnits ) );
	}

	uint32_t ulMachineIdleSecs = TheMachine.GetActiveTime ();
	if ( ulMachineIdleSecs != ulLastMachineIdleSecs )
	{
		ulLastMachineIdleSecs = ulMachineIdleSecs;
		ClearPartofLine ( STATS_ROW + 8, STATS_RESULT_COL, MAX_COLS - STATS_RESULT_COL );
		AT ( STATS_ROW + 8, STATS_RESULT_COL, String ( ulMachineIdleSecs ) );
	}

	// Update mode and status if necessary
	String sMode;
	if ( TheOiler.IsMonitoringTime () )
	{
		sMode = F ( "on Elapsed time" );
	}
	else if ( TheOiler.IsMonitoringTargetPower() )
	{
		sMode = F ( "on Target power" );
	}
	else if ( TheOiler.IsMonitoringTargetWork() )
	{
		sMode = F ( "on Target work" );
	}
	if ( sMode != sLastMode )
	{
		sLastMode = sMode;
		ClearPartofLine ( MODE_ROW + 0, MODE_RESULT_COL, MAX_COLS - MODE_RESULT_COL );
		AT ( MODE_ROW + 0, MODE_RESULT_COL, sMode );
	}

	String sState;
	if ( TheOiler.IsIdle () )
	{
		sState = F ( "Idle" );
	} 
	else if ( TheOiler.IsOff () )
	{
		sState = F ( "Off" );
	} 
	else if ( TheOiler.IsOiling () )
	{
		sState = F ( "Oiling" );
	}

	if ( sLastState != sState )
	{
		sLastState = sState;
		ClearPartofLine ( MODE_ROW + 1, MODE_RESULT_COL, MAX_COLS - MODE_RESULT_COL );
		AT ( MODE_ROW + 1, MODE_RESULT_COL, sState );
	}
}

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

void DisplayOilerStatus ( String s )
{
	ClearPartofLine ( STATUS_LINE, STATUS_START_COL, MAX_COLS - STATUS_START_COL + 1 );
	AT ( STATUS_LINE, STATUS_START_COL, s );
}

