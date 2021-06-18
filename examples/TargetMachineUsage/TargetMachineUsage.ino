/*
	OilerLib Example sketch - TargetMachineUsage

	Author:	Mark Naylor, June 2021

	Description

	This sample sketch demonstrates how to use signals from the machine being oiled to get enhanced features from the OilerLib library:
		This sets up one relay driven pump
		respond to a related input sensor that indicates the pump has output oil and pause the related motor when configured amount of oil is delivered
		restart pump after a target machine indicates work has been done or after target machine has had power for a set number of seconds
*/
#include "OilerLib.h"

#define OILED_DEVICE_ACTIVE_PIN1		17			// digital pin which will go high when drips sent from motor 1
#define PUMP1_NUM_DRIPS					3			// number of drips after which motor 1 is paused

#define	HAS_POWER_PIN					10
#define DONE_WORK_PIN					11
#define	POWER_SECONDS_THRESHOLD			60			// restart oil pumps evry 60 seconds target machine has power (if machine powered down, don't oil)
#define WORK_THRESHOLD					100			// restart oil pumps every 100 signals - e.g. if attached to lathe spindle encoder, every 100 revolutions of spindle

void setup ()
{
	Serial.begin ( 19200 );
	while ( !Serial );

	String Heading = F ( "\nOiler Example Sketch, Version " );
	Heading += String ( OILER_VERSION );
	Serial.println ( Heading );

	// add 1 pin relay motor on Pin 4
	if ( TheOiler.AddMotor ( 4, OILED_DEVICE_ACTIVE_PIN1, PUMP1_NUM_DRIPS ) == false )
	{
		Serial.println ( F ( "Unable to add relay motor to oiler, stopped" ) );
		while ( 1 );
	}
	
	// configure TheMachine
	if ( TheMachine.AddFeatures ( HAS_POWER_PIN, DONE_WORK_PIN, POWER_SECONDS_THRESHOLD, WORK_THRESHOLD ) == false )
	{
		Serial.println ( F ( "Unable to configure TheMachine, stopped" ) );
		while ( 1 );
	}
	// if you don't have one of these signals from the machine being oiled, then the alternatives below could be used instead
	//TheMachine.AddFeatures ( NOT_A_PIN, DONE_WORK_PIN, 0, WORK_THRESHOLD );				// no 'has power' output from target machine
	//TheMachine.AddFeatures ( HAS_POWER_PIN, NOT_A_PIN, POWER_SECONDS_THRESHOLD, 0 );	// no 'work done' output from target machine

	// Add TheMachine to TheOiler
	TheOiler.AddMachine ( &TheMachine );
	
	// Set the oiler to restart pumps when target machine has been powered on for 20 seconds
	if ( TheOiler.SetStartEventToTargetActiveTime ( 20 ) == false )
	{
		Serial.println ( F ( "Unable to set oiler to restart pumps after 20 seconds, stopped" ) );
		while ( 1 );
	}
	else
	{
		Serial.println ( F ( "Now will restart pumps after target machine has had power for 20 seconds" ) );
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
	delay ( 30 * 1000 );		// wait for 30 secs
	delay ( 30 * 1000 );		// wait for 30 secs

	// switch to restart pumps after target machine completes another 100 revs
	if ( TheOiler.SetStartEventToTargetWork ( 100 ) == false )
	{
		Serial.println ( F ( "Unable to set oiler to restart pumps after 100 work units, stopped" ) );
		while ( 1 );
	}
	else
	{
		Serial.println ( F ( "Now will restart pumps after target machine has completed 100 units of work" )  );
	}
}

void loop ()
{
}