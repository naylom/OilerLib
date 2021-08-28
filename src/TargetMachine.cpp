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
/// <param name="uiActiveUnitTarget">threshold, in seconds, beyond which target machine is ready to be oiled</param>
/// <param name="uiWorkUnitTarget">threshold, in units, beyond which the target machine is ready to be oiled</param>
/// <returns>false if unable to add valid pin to pin change interrupt handling, else true</returns>
bool TargetMachineClass::AddFeatures ( uint8_t uiActivePin, uint8_t uiWorkPin, uint8_t uiActiveUnitTarget, uint8_t uiWorkUnitTarget )
{
	bool bResult = true;

	m_ulTargetSecs	= uiActiveUnitTarget;
	m_ulTargetUnits = uiWorkUnitTarget;
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
/// checks if the target machine has equalled or exceeded the threshold set for units of work done
/// </summary>
/// <param name="">none</param>
/// <returns>true if threshold met or exceeded, else false</returns>
bool TargetMachineClass::MachineUnitsDone ( void )
{
	bool bResult = false;
	if ( m_ulWorkUnitCount >= m_ulTargetUnits )
	{
		bResult = true;
	}
	return bResult;
}

/// <summary>
/// checks if the target machine has met or exceeded the threshold set for time with power
/// </summary>
/// <param name="">none</param>
/// <returns>true if threshold met or exceeded, else false</returns>
bool TargetMachineClass::MachinePoweredTimeExpired ( void )
{
	bool bResult = false;
	// Check time is up to date
	if ( m_Active == ACTIVE )
	{
		// add time to now and check if passed threshold
		uint32_t tNow = millis ();
		IncActiveTime ( tNow );
		m_timeActiveStarted = tNow;
	}
	if ( m_timeActive >= m_ulTargetSecs * 1000 )
	{
		bResult = true;
	}
	return bResult;
}

/// <summary>
/// checks of the target machine is ready to be oiled, ie has met its threshold
/// </summary>
/// <param name="">none</param>
/// <returns>returns TargetMachineClass::eMachineState to reflect if machine is ready or not</returns>
TargetMachineClass::eMachineState TargetMachineClass::IsReady ( void )
{
	// Check time is up to date
	if ( m_Active == ACTIVE )
	{
		// add time to now and check if passed threshold
		uint32_t tNow = millis ();
		IncActiveTime ( tNow );
		m_timeActiveStarted = tNow;
	}
	return m_State;
}

/// <summary>
/// Checks if TargetMachine has exceeded AlertThreshold in work units
/// </summary>
/// <param name="uiAlertThreshold">Value of alert threshold in units of work</param>
/// <returns>true if exceeded threshold</returns>
bool TargetMachineClass::IsWorkAlert ( uint16_t uiAlertThreshold )
{
	bool bResult = false;

	if ( m_ulWorkUnitCount >= uiAlertThreshold )
	{
		bResult = true;
	}

	return bResult;
}

/// <summary>
/// Checks if TargetMachine has exceeded AlertThreshold in seconds
/// </summary>
/// <param name="uiAlertThreshold">Value of alert threshold in seconds</param>
/// <returns>true if exceeded threshold</returns>
bool TargetMachineClass::IsTimeAlert ( uint16_t uiAlertThreshold )
{
	bool bResult = false;
	// Ensure time is updated before returning value
	IsReady ();
	if ( m_timeActive >= uiAlertThreshold * 1000 )
	{
		bResult = true;
	}

	return bResult;
}

/// <summary>
/// Gets the number of seconds the targetmachine has been with power since monitoring was last started.
/// </summary>
/// <param name="">none</param>
/// <returns>number of seconds</returns>
uint32_t TargetMachineClass::GetActiveTime ( void )
{
	// Ensure time is updated before returning value
	IsReady ();
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
/// Returns the number of seconds the machince should have power before the motors should start oiling
/// </summary>
/// <param name="">None</param>
/// <returns>target number of seconds</returns>
uint32_t TargetMachineClass::GetActiveTimeTarget ( void )
{
	return m_ulTargetSecs;
}

/// <summary>
/// return the target number of work units (eg revs) the machine should accomplish after which it should be oiled
/// </summary>
/// <param name="">None</param>
/// <returns>target number of units</returns>
uint32_t TargetMachineClass::GetWorkUnitTarget ( void )
{
	return m_ulTargetUnits;
}

/// <summary>
/// add active time in milliseconds to total since machine became active and set state to ready if threshold exceeded
/// </summary>
/// <param name="tNow">current time in milliseconds</param>
void TargetMachineClass::IncActiveTime ( uint32_t tNow )
{
	m_timeActive += ( tNow - m_timeActiveStarted );
	if ( m_timeActive >= m_ulTargetSecs * 1000 )
	{
		m_State = READY;
	}
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
/// Increments count of work units completed and then checks if threshold met. If met the state is updated to READY
/// </summary>
/// <param name="ulIncAmount">number of units</param>
void TargetMachineClass::IncWorkUnit ( uint32_t ulIncAmount )
{
	m_ulWorkUnitCount += ulIncAmount;
	if ( m_ulWorkUnitCount >= m_ulTargetUnits )
	{
		m_State = READY;
	}
}

/// <summary>
/// Updates the threshold for time with power after which machine becones ready for oiling
/// </summary>
/// <param name="ulTargetSecs">Threshold number of seconds</param>
/// <returns>false if active pin set to NOT_A_PIN, else true</returns>
bool TargetMachineClass::SetActiveTimeTarget ( uint32_t ulTargetSecs )
{
	bool bResult = false;
	if ( m_uiActivePin != NOT_A_PIN )
	{
		m_ulTargetSecs = ulTargetSecs;
		bResult = true;
	}
	return bResult;
}

/// <summary>
/// Updates the threshold for units of work (e.g. revs) after which machine becomes ready for oiling
/// </summary>
/// <param name="ulTargetUnits">Threshold number of units</param>
/// <returns>false if active pin set to NOT_A_PIN, else true</returns>
bool TargetMachineClass::SetWorkTarget ( uint32_t ulTargetUnits )
{
	bool bResult = false;
	if ( m_uiWorkPin != NOT_A_PIN )
	{
		m_ulTargetUnits = ulTargetUnits;
		bResult = true;
	}
	return bResult;
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
