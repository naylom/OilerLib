// RelayMotor.cpp
// 
// (c) Mark Naylor June 2021
// 
// RelayMotor class, derivative of MotorClass for driving a DC motor via a relay
// 

#include "RelayMotor.h"

RelayMotorClass::RelayMotorClass ( uint8_t uiRelayPin, uint8_t uiWorkPin, uint32_t ulWorkThreshold, uint32_t ulDebouncems, uint32_t ulTimeThreshold ) : OilerMotorClass ( uiWorkPin, ulWorkThreshold, ulDebouncems, 0, ulTimeThreshold )
{
	SetDirection ( FORWARD );
	m_uiRelayPin = uiRelayPin;
	pinMode ( m_uiRelayPin, OUTPUT );
}

/// <summary>
/// Motor specific function to idle motor
/// </summary>
void RelayMotorClass::Idle ()
{
	// Idle motor - for relay this means same as power off
	PowerOff ();
}

/// <summary>
/// Motor specific function to Start motor
/// </summary>
void RelayMotorClass::Start ()
{
	// Start motor - for relay this means switch on
	digitalWrite ( m_uiRelayPin, HIGH );
}

/// <summary>
/// Motor specific function to power off
/// </summary>
void RelayMotorClass::PowerOff ()
{
	// power off motor - for relay this means switch off
	digitalWrite ( m_uiRelayPin, LOW );
}


void RelayMotorClass::SetDirection ( eDirection Direction )
{
	// Save requested direction
	MotorClass::SetDirection ( Direction );
}

