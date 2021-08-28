#include "OilerMotor.h"
// OilerMotor.cpp
//
// (c) 2021 Mark Naylor
//
// implements base class for motors driving an oiler, extends MotorClass and adds an output feature that tracks units of work (i.e. oil drips) from motor
//
#include "OilerMotor.h"

OilerMotorClass::OilerMotorClass ( uint8_t uiWorkPin, uint32_t ulThreshold, uint32_t ulDebouncems, uint32_t ulSpeed, uint16_t uiTimeThreshold ) : MotorClass ( ulSpeed ), m_MotorState ( &MotorTable [ 0 ], ( sizeof ( MotorTable ) / sizeof ( MotorTable [ 0 ] ) ) )
{
	m_uiWorkPin = uiWorkPin;
	m_ulLastWorkSignal = 0UL;
	m_ulMachineUnitsAtStart = 0UL;
	m_ulMachineUnitsAtIdle = 0UL;
	m_ulMachinePowerTimeAtStart = 0UL;
	m_ulMachinePowerTimeAtIdle = 0UL;
	SetWorkThreshold ( ulThreshold );
	SetDebouncems ( ulDebouncems );
	SetTimeThreshold ( uiTimeThreshold );
}

bool OilerMotorClass::On ( void )
{
	ResetWorkUnits ();
	return MotorClass::On ();
}

bool OilerMotorClass::Off ( void )
{
	return MotorClass::Off ();
}

void OilerMotorClass::IncWorkUnits ( uint16_t uiNewUnits )
{
	m_uiWorkCount += uiNewUnits;
}

void OilerMotorClass::ResetWorkUnits ( void )
{
	m_uiWorkCount = 0;
}

uint16_t OilerMotorClass::GetWorkUnits ()
{
	return m_uiWorkCount;
}

void OilerMotorClass::SetDebouncems ( uint32_t ulDebouncems )
{
	m_ulDebounceMin = ulDebouncems;
}

void OilerMotorClass::SetWorkThreshold ( uint32_t ulWorkThreshold )
{
	m_ulWorkThreshold = ulWorkThreshold;
}

void OilerMotorClass::SetTimeThreshold ( uint16_t uiTimeThresholdSec )
{
	m_uiTimeThresholdSec = uiTimeThresholdSec;
}

void OilerMotorClass::SetMachineUnitsAtStart ( uint32_t ulMachineUnitsAtStart )
{
	m_ulMachineUnitsAtStart = ulMachineUnitsAtStart;
}

void OilerMotorClass::SetMachineUnitsAtIdle ( uint32_t ulMachineUnitsAtIdle )
{
	m_ulMachineUnitsAtIdle = ulMachineUnitsAtIdle;
}

void OilerMotorClass::SetMachinePowerTimeAtStart ( uint32_t ulMachinePowerTimeAtStart )
{
	m_ulMachinePowerTimeAtStart = ulMachinePowerTimeAtStart;
}

void OilerMotorClass::SetMachinePowerTimeAtIdle ( uint32_t ulMachinePowerTimeAtIdle )
{
	m_ulMachinePowerTimeAtIdle = ulMachinePowerTimeAtIdle;
}

uint32_t OilerMotorClass::GetMachineUnitsAtStart ( void )
{
	return m_ulMachineUnitsAtStart;
}

uint32_t OilerMotorClass::GetMachineUnitsAtIdle ( void )
{
	return m_ulMachineUnitsAtIdle;
}

uint32_t OilerMotorClass::GetMachinePowerTimeAtStart ( void )
{
	return m_ulMachinePowerTimeAtStart;
}

uint32_t OilerMotorClass::GetMachinePowerTimeAtIdle ( void )
{
	return m_ulMachinePowerTimeAtIdle;
}

bool OilerMotorClass::Action ( eOilerMotorEvents eAction )
{
	return m_MotorState.ProcessEvent ( this, eAction );
}


/// <summary>
/// Get the state table state of motor
/// </summary>
/// <returns>current state</returns>
OilerMotorClass::eOilerMotorState OilerMotorClass::GetOilerMotorState ()
{
	return (eOilerMotorState)m_MotorState.GetCurrentState ();
}

/// <summary>
/// Checks if motor is idle
/// </summary>
/// <returns>true if idle, else false</returns>
bool OilerMotorClass::IsIdle ()
{
	return GetOilerMotorState () == IDLE ? true : false;
}

/// <summary>
/// Checks if motor is moving
/// </summary>
/// <returns>true if moving, else false</returns>
bool OilerMotorClass::IsMoving ()
{
	return GetOilerMotorState () == MOVING ? true : false;
}

/// <summary>
/// Checks if motor is off
/// </summary>
/// <returns>true if off, else false</returns>
bool OilerMotorClass::IsOff ()
{
	return GetOilerMotorState () == OFF ? true : false;
}

/// <summary>
/// State table functon called to start motor moving
/// </summary>
/// <returns>new state</returns>
uint16_t OilerMotorClass::TurnOn ()
{
	On ();				// Update status
	Start ();			// physically start motor
	return MOVING;		// new state
}

/// <summary>
/// State table function called to turn off motor
/// </summary>
/// <returns>new state</returns>
uint16_t OilerMotorClass::TurnOff ()
{
	Off ();				// Update status
	PowerOff ();		// physically turn off
	return OFF;			// new state
}

/// <summary>
/// Checks if we have equalled or exceeded the threshold set for units of work (oil drips) and idles motor if true
/// </summary>
/// <returns>new state or existing state</returns>
uint16_t OilerMotorClass::CheckWork ()
{
	uint16_t uiResult;

	// check for spurious signal
	if ( millis () - m_ulLastWorkSignal >= m_ulDebounceMin )
	{
		IncWorkUnits ( 1 );
		if ( m_uiWorkCount >= m_ulWorkThreshold )
		{
			uiResult = IDLE;			// new state
			Idle ();					// Physically idle motor
			OilerMotorClass::Off ();	// update class, don't invoke
		}
		else
		{
			// not met threshold
			uiResult = DoNothing ();
		}
	}
	else
	{
		// ignore bad signal
		uiResult = DoNothing ();
	}
	return uiResult;
}

/// <summary>
/// Checks to see if the motor should restart after a set time has elapsed
/// </summary>
/// <returns></returns>
uint16_t OilerMotorClass::CheckTime ()
{
	uint16_t uiResult;
	//if ( ( millis() - MotorClass::GetTimeMotorStopped () ) / 1000 >= m_uiTimeThresholdSec )
	if ( ( millis () - MotorClass::GetTimeMotorStarted () ) / 1000 >= m_uiTimeThresholdSec )		// use time started so a slow motor restarts from start time not when it finished dripping oil
	{
		// time to start motor
		uiResult = TurnOn ();
	}
	else
	{
		uiResult = DoNothing ();
	}
	return uiResult;
}

/// <summary>
/// Keeps state of state machine unchanged
/// </summary>
/// <returns>existing state</returns>
uint16_t OilerMotorClass::DoNothing ()
{
	return m_MotorState.GetCurrentState ();
}