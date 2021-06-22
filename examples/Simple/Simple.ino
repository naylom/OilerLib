/*
	OilerLib Example sketch - Simple

	Author:	Mark Naylor, June 2021

	Description

	This sample sketch demonstrates the most basic usage of the OilerLib library to:
		control one relay driven pump
		respond to a related input sensor that indicates the pump has output oil and pause the related motor when configured amount of oil is delivered
		restart motors after a specified elapsed time
*/
#include "OilerLib.h"

#define OILED_DEVICE_ACTIVE_PIN1		17			// digital pin which will go high when drips sent from motor 1
#define PUMP1_NUM_DRIPS					3			// number of drips after which motor 1 is paused

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

	// Set the oiler to restart pumps every 20 seconds
	if ( TheOiler.SetStartEventToTime ( 20 ) == false )
	{
		Serial.println ( F ( "Unable to set oiler to restart pumps after 20 seconds, stopped" ) );
		while ( 1 );
	}
	
	// the next two function calls are unnecessary as they simply set what is the default however they demonstrate how to change these values
	// Set the motor sensor pin (that indicates drips of Oil have been seen) to INPUT mode 
	if ( TheOiler.SetMotorWorkPinMode ( 0, INPUT ) == false )
	{
		// unable to set first motor (number 0 ) input mode to INPUT
		Serial.println ( F ( "Unable to set sensor for motor 0 to INPUT, stopped" ) );
		while ( 1 );
	}
	// set minimum time between valid signals from motor sensor to 150 milliseconds
	if ( TheOiler.SetMotorSensorDebounce ( 0, 150 ) == false )
	{
		// Unable to set the debounce delay for first motor (0)
		Serial.println ( F ( "Unable to set sensor debounce value, stopped" ) );
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
}