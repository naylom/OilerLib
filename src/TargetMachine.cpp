//
// TargetMachine.cpp
//
// (c) Mark Naylor 2021
//
// This class encapsulates the machine that is being oiled
//
// It has two optional attributes: 
//		A signal that indicates the machine is active
//		A signal that indicates the machine has completed a unit of work
//
//	In our example machine - a metal working lathe, the active signal indicates the machine is moving and the unit of work is a completed full rototation of the lather spindle
//
// The class keeps track of active time and number of units of work completed. These are optional inputs for the Oiler class to refine when it delivers oil.
//
#include "PCIHandler.h"
#include "TargetMachine.h"

/// <summary>
/// Routine to be called if the target machine active (has power) pin is signalled - called by interrupt
/// </summary>
/// <param name="">none</param>
void MachineActiveSignal ( void )
{
	TheMachine.CheckActivity ();
}

/// <summary>
/// Routine to be called if MACHINE_WORK_PIN is signalled - called by interrupt
/// </summary>
/// <param name="">none</param>
void MachineWorkUnitSignal ( void )
{
	TheMachine.IncWorkUnit ( 1 );
}

// Class routines
TargetMachineClass::TargetMachineClass ( void )
{
	m_uiWorkPin			= NOT_A_PIN;
	m_uiActivePin		= NOT_A_PIN;
	m_State				= NOT_READY;
	m_Active			= IDLE;
	m_uiActivePinMode	= MACHINE_ACTIVE_PIN_MODE;		// set default value
	m_uiWorkPinMode		= MACHINE_WORK_PIN_MODE;		// set default value
	m_uiActiveState		= MACHINE_ACTIVE_STATE;			// set default value
}

/// <summary>
/// Called to configure machine object with details of which features it supports
/// </summary>
/// <param name="uiActivePin">digital pin that is signalled whilst target machine is active (e.g. has power),use NOT_A_PIN if feature not implemented</param>
/// <param name="uiWorkPin">digital pin that is signalled each time the target machine does a unit of work (e.g a rev of a lathe), use NOT_A_PIN if feature not implemented</param>
/// <returns>false if unable to add valid pin to pin change interrupt handling, else true</returns>
bool TargetMachineClass::AddFeatures ( uint8_t uiActivePin, uint8_t uiWorkPin )
{
	bool bResult = true;

	m_uiActivePin	= uiActivePin;
	m_uiWorkPin		= uiWorkPin;

	if ( uiActivePin == NOT_A_PIN && uiWorkPin == NOT_A_PIN )
	{
		m_State = NO_FEATURES;
	}
	RestartMonitoring ();

	if ( uiActivePin != NOT_A_PIN )
	{
		if ( PCIHandler.AddPin ( uiActivePin, MachineActiveSignal, MACHINE_ACTIVE_PIN_SIGNAL, m_uiActivePinMode ) == false )
		{
			bResult = false;
		}
	}
	if ( uiWorkPin != NOT_A_PIN )
	{
		if ( PCIHandler.AddPin ( uiWorkPin, MachineWorkUnitSignal, MACHINE_WORK_PIN_SIGNAL, m_uiWorkPinMode ) == false )
		{
			bResult = false;
		}
	}
	return bResult;
}

/// <summary>
/// Checks if target machine is configured with at least one valid work or activity digital pin and if so starts monitoring pin(s) 
/// </summary>
/// <param name="">none</param>
void TargetMachineClass::RestartMonitoring ( void )
{
	m_timeActive = 0UL;
	m_ulWorkUnitCount = 0UL;
	if ( m_State != NO_FEATURES )
	{
		m_State = NOT_READY;
		m_Active = m_uiActivePin == NOT_A_PIN ? IDLE : digitalRead ( m_uiActivePin ) == m_uiActiveState ? ACTIVE : IDLE;
		if ( m_Active == ACTIVE )
		{
			m_timeActiveStarted = millis ();
		}
	}
}

/// <summary>
/// called when active signal changes state. If target machine is active (has power) and if so remembers start time, if not active calculates how much time was with power and adds to count 
/// </summary>
/// <param name="">none</param>
void TargetMachineClass::CheckActivity ( void )
{
	// see how machine has changed state
	if ( digitalRead ( m_uiActivePin ) == m_uiActiveState )
	{
		// machine gone active so remember when this started
		GoneActive ( millis () );
	}
	else
	{
		// machine gone idle so calc time was active and save it
		IncActiveTime ( millis () );
	}
}

/// <summary>
/// If currently active updates powered time calc
/// </summary>
/// <param name="">none</param>
/// <returns>Nothing</returns>
void TargetMachineClass::UpdatePoweredTime ( void )
{
	// Check time is up to date
	if ( m_Active == ACTIVE )
	{
		// add time to now and check if passed threshold
		uint32_t tNow = millis ();
		IncActiveTime ( tNow );
		m_timeActiveStarted = tNow;
	}
}

/// <summary>
/// Gets the number of seconds the targetmachine has been with power since monitoring was last started.
/// </summary>
/// <param name="">none</param>
/// <returns>number of seconds</returns>
uint32_t TargetMachineClass::GetActiveTime ( void )
{
	// Ensure time is updated before returning value
	UpdatePoweredTime ();
	return m_timeActive / 1000;
}

/// <summary>
/// Gets the number of work units (e.g. lathe revs) since monitoring was last restarted
/// </summary>
/// <param name="">none</param>
/// <returns>units of work</returns>
uint32_t TargetMachineClass::GetWorkUnits ( void )
{
	return m_ulWorkUnitCount;
}

/// <summary>
/// add active time in milliseconds to total since machine became active 
/// </summary>
/// <param name="tNow">current time in milliseconds</param>
void TargetMachineClass::IncActiveTime ( uint32_t tNow )
{
	m_timeActive += ( tNow - m_timeActiveStarted );

	m_Active = digitalRead ( m_uiActivePin ) == m_uiActiveState ? ACTIVE : IDLE;
}

/// <summary>
/// called to update state and remember when targetmachine got power
/// </summary>
/// <param name="tNow">time in milliseconds</param>
void TargetMachineClass::GoneActive ( uint32_t tNow )
{
	m_Active = ACTIVE;
	m_timeActiveStarted = tNow;
}

/// <summary>
/// Increments count of work units completed and then checks if threshold met. 
/// </summary>
/// <param name="ulIncAmount">number of units</param>
void TargetMachineClass::IncWorkUnit ( uint32_t ulIncAmount )
{
	m_ulWorkUnitCount += ulIncAmount;
}

/// <summary>
/// Sets the pinmode of the digital pin used to signal when the TargetMachine has power
/// </summary>
/// <param name="uiMode">INPUT or INPUT_PULLUP</param>
/// <returns>false if uiMode is not one of required values</returns>
bool TargetMachineClass::SetActivePinMode ( uint8_t uiMode )
{
	bool bResult = false;
	if ( uiMode == INPUT || uiMode == INPUT_PULLUP )
	{
		if ( uiMode != m_uiActivePinMode )
		{
			// has changed
			pinMode ( m_uiActivePin, uiMode );
			m_uiActivePinMode = uiMode;
		}
		bResult = true;
	}
	return bResult;
}

/// <summary>
/// Sets the pinmode of the digital pin used to signal when the TargetMachine completes a unit of work
/// </summary>
/// <param name="uiMode">INPUT or INPUT_PULLUP</param>
/// <returns>false if uiMode is not one of required values</returns>
bool TargetMachineClass::SetWorkPinMode ( uint8_t uiMode )
{
	bool bResult = false;
	if ( uiMode == INPUT || uiMode == INPUT_PULLUP )
	{
		if ( uiMode != m_uiWorkPinMode )
		{
			// has changed
			pinMode ( m_uiWorkPin, uiMode );
			m_uiWorkPinMode = uiMode;
		}
		bResult = true;
	}
	return bResult;
}

/// <summary>
/// Sets the expected state of the active pin when signalling the target machine has power
/// </summary>
/// <param name="uiState">HIGH or LOW</param>
/// <returns>true if uiState is one of permitted values, else false</returns>
bool TargetMachineClass::SetActiveState ( uint8_t uiState )
{
	bool bResult = false;
	if ( uiState == HIGH || uiState == LOW )
	{
		m_uiActiveState = uiState;
		bResult = true;
	}
	return bResult;
}

TargetMachineClass TheMachine;				// Create instance
