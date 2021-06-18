// Motor.cpp
//
// (c) 2021 Mark Naylor
//
// implements base class for motors to drive oiler pump
//

#include "Motor.h"

MotorClass::MotorClass ( uint32_t ulSpeed )
{
	SetSpeed ( ulSpeed );
	m_eState = STOPPED;
	m_ulTimeStartedms = 0;
	m_ulTimeStoppedms = 0;
	m_eDir = FORWARD;
}

bool MotorClass::On ( void )
{
	m_ulTimeStartedms = millis ();
	m_eState = RUNNING;
	return true;
}

bool MotorClass::Off ( void )
{
	m_ulTimeStoppedms = millis ();
	m_eState = STOPPED;
	return true;
}

uint32_t MotorClass::GetTimeMotorStarted ( void )
{
	return 	m_ulTimeStartedms;
}

uint32_t MotorClass::GetTimeMotorRunning ( void )
{
	uint32_t ulResult = 0UL;
	if ( m_eState == RUNNING )
	{
		ulResult = ( millis () - m_ulTimeStartedms ) / 1000UL;
	}
	return ulResult;
}

uint32_t MotorClass::GetTimeMotorStopped ( void )
{
	return m_ulTimeStoppedms;
}

MotorClass::eState MotorClass::GetMotorState ( void )
{
	return m_eState;
}

uint32_t MotorClass::GetSpeed ( void )
{
	return m_ulSpeed;
}

bool MotorClass::SetSpeed ( uint32_t ulSpeed )
{
	m_ulSpeed = ulSpeed;
	return false;
}

void MotorClass::SetDirection ( eDirection eDir )
{
	// Save requested direction
	m_eDir = eDir;
}


// MotorClass Motor;
