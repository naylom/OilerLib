#include "OilerMotor.h"
// OilerMotor.cpp
//
// (c) 2021 Mark Naylor
//
// implements base class for motors driving an oiler, extends MotorClass and adds an output feature that tracks units of work (i.e. oil drips) from motor
//
#include "OilerMotor.h"

OilerMotorClass::OilerMotorClass ( uint8_t uiWorkPin, uint32_t ulThreshold, uint32_t ulDebouncems, uint32_t ulSpeed, uint16_t uiRestartThreshold ) : MotorClass ( ulSpeed ), m_MotorState ( &MotorTable [ 0 ], ( sizeof ( MotorTable ) / sizeof ( MotorTable [ 0 ] ) ) )
{
	m_uiWorkPin					= uiWorkPin;
	m_ulLastWorkSignal			= 0UL;
	m_bError					= false;
	SetModeMetricAtStart ( 0UL );
	SetModeMetricAtIdle ( 0UL );
	SetWorkThreshold ( ulThreshold );
	SetDebouncems ( ulDebouncems );
	SetRestartThreshold ( uiRestartThreshold );
	SetAlertThreshold ( 0UL );
	ResetWorkUnits ();
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

void OilerMotorClass::SetRestartThreshold ( uint16_t uiRestartValue )
{
	m_uiRestartValue = uiRestartValue;
}

void OilerMotorClass::SetAlertThreshold ( uint32_t ulAlertThreshold )
{
	m_ulAlertThreshold = ulAlertThreshold;
}

void OilerMotorClass::SetModeMetricAtStart ( uint32_t ulMetric )
{
	m_ulModeMetricAtStart = ulMetric;
}

void OilerMotorClass::SetModeMetricAtIdle ( uint32_t ulMetric )
{
	m_ulModeMetricAtIdle = ulMetric;
}

inline uint32_t OilerMotorClass::GetModeMetricAtStart ( void )
{
	return m_ulModeMetricAtStart;
}

inline uint32_t OilerMotorClass::GetModeMetricAtIdle ( void )
{
	return m_ulModeMetricAtIdle;
}

bool OilerMotorClass::Action ( eOilerMotorEvents eAction, uint32_t ulParam )
{
	return m_MotorState.ProcessEvent ( this, eAction, ulParam );
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
/// returns true if the motor has not completed its work output before set threshold
/// </summary>
/// <returns>true if work delayed else false</returns>
bool OilerMotorClass::IsInError ()
{
	return m_bError;
}

/// <summary>
/// State table function called to start motor moving
/// </summary>
/// <returns>new state</returns>
uint16_t OilerMotorClass::TurnOn ( uint32_t ulParam )
{
	On ();								// Update status
	Start ();							// physically start motor
	SetModeMetricAtStart ( ulParam );
	return MOVING;						// new state
}

/// <summary>
/// State table function called to turn off motor
/// </summary>
/// <returns>new state</returns>
uint16_t OilerMotorClass::TurnOff ( uint32_t ulParam )
{
	Off ();								// Update status
	PowerOff ();						// physically turn off
	return OFF;							// new state
}

/// <summary>
/// Checks if we have equalled or exceeded the threshold set for units of work (oil drips) and idles motor if true
/// </summary>
/// <returns>new state or existing state</returns>
uint16_t OilerMotorClass::CheckWork ( uint32_t ulParam )
{
	uint16_t uiResult;

	// check for spurious signal
	if ( millis () - m_ulLastWorkSignal >= m_ulDebounceMin )
	{
		IncWorkUnits ( 1 );
		if ( GetWorkUnits() >= m_ulWorkThreshold )
		{
			uiResult = IDLE;			// new state
			Idle ();					// Physically idle motor
			OilerMotorClass::Off ();	// update class, don't invoke
			SetModeMetricAtIdle ( ulParam );
			m_bError = false;
		}
		else
		{
			// not met threshold
			uiResult = DoNothing (0);
		}
	}
	else
	{
		// ignore bad signal
		uiResult = DoNothing (0);
	}
	return uiResult;
}
/// <summary>
/// Called to set error state if still oiling beyond alert threshold
/// </summary>
/// <param name="ulParam">Current value of metric to be measured against alert threshold</param>
/// <returns>false, does not change OilerMotor processing state</returns>
uint16_t OilerMotorClass::CheckAlert ( uint32_t ulParam )
{
	m_bError = ( ulParam - GetModeMetricAtStart() ) >= m_ulAlertThreshold ? true : false;
	return DoNothing ( 0UL );
}

/// <summary>
/// Checks to see if the motor should restart after a set time has elapsed
/// </summary>
/// <returns></returns>
uint16_t OilerMotorClass::CheckRestart ( uint32_t ulParam )
{
	uint16_t uiResult;

	if ( ( ulParam - GetModeMetricAtStart () ) >= m_uiRestartValue )
	{
		// time to start motor
		uiResult = TurnOn ( ulParam );
	}
	else
	{
		uiResult = DoNothing (0);
	}
	return uiResult;
}

/// <summary>
/// Keeps state of state machine unchanged
/// </summary>
/// <returns>existing state</returns>
uint16_t OilerMotorClass::DoNothing ( uint32_t ulParam )
{
	return m_MotorState.GetCurrentState ();
}