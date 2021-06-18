// RelayMotor.cpp
// 
// (c) Mark Naylor June 2021
// 
// RelayMotor class, derivative of MotorClass for driving a DC motor via a relay
// 

#include "RelayMotor.h"

RelayMotorClass::RelayMotorClass ( uint8_t uiPin ) : MotorClass ( 0 )
{
	SetDirection ( FORWARD );
	m_uiPin = uiPin;
	pinMode ( m_uiPin, OUTPUT );
}

bool RelayMotorClass::On ( void )
{
	digitalWrite ( m_uiPin, HIGH );
	MotorClass::On ();
	return true;
}

bool RelayMotorClass::Off ( void )
{
	digitalWrite ( m_uiPin, LOW );
	MotorClass::Off ();
	return true;
}

void RelayMotorClass::SetDirection ( eDirection Direction )
{
	// Save requested direction
	MotorClass::SetDirection ( Direction );
}

MotorClass::eState RelayMotorClass::GetMotorState ( void )
{
	return MotorClass::GetMotorState ();
}
