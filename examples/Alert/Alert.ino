/*
	OilerLib Example sketch - Alert

	Author:	Mark Naylor, June 2021

	Description

	This sample sketch demonstrates using the OilerLib library to:
		control two pumps using relay driven motors
		respond to motor related input sensors that indicates the pump has output oil and pause the related motor when configured amount of oil is delivered
		restart motors after a specified elapsed time
		raise an alert signal if a pump is delayed in delivering oil
*/
#include "OilerLib.h"

#define OILED_DEVICE_ACTIVE_PIN1		17			// digital pin which will go high when drips sent from motor 1
#define OILED_DEVICE_ACTIVE_PIN2		3			// digital pin which will go high when drips sent from motor 2

#define PUMP1_NUM_DRIPS					3			// number of drips after which motor 1 is paused
#define PUMP2_NUM_DRIPS					1			// number of drips after which motor 2 is paused

#define ALERT_PIN						19			// digital Pin to signal if not completed oiling in multiple of oiler start target (eg elapsed time / revs / powered on time), set to NOT_A_PIN if only using software monitoring as in loop() below

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
	// add second 1 pin relay motor on Pin 5
	if ( TheOiler.AddMotor ( 5, OILED_DEVICE_ACTIVE_PIN2, PUMP2_NUM_DRIPS ) == false )
	{
		Serial.println ( F ( "Unable to add relay motor to oiler, stopped" ) );
		while ( 1 );
	}
	// Set the oiler to restart pumps every 20 seconds
	if ( TheOiler.SetStartEventToTime ( 20 ) == false )
	{
		Serial.println ( F ( "Unable to set oiler to restart pumps after 20 seconds, stopped" ) );
		while ( 1 );
	}

	// Configure Alert if oil is delayed by 3x restart time (ie 60 secs), note this applies independently to all motors so if one is working correctly and second runs out of oil the alert will be generated 
	// when the second pump fails the 3 x 20 threshold.
	TheOiler.SetAlert ( ALERT_PIN, 3 );

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
	static bool bLastAlertState = TheOiler.IsAlert ();

	if ( TheOiler.IsAlert () != bLastAlertState )
	{
		// changed
		bLastAlertState = bLastAlertState == true ? false : true;
		if ( bLastAlertState )
		{
			Serial.println ( F ( "Alert is true" ) );
		}
		else
		{
			Serial.println ( F ( "Alert is false" ) );
		}
	}
	delay ( 500 );
}
