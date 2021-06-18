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


// #define IsInThisPCIR( digitalPin, Port ) ( digitalPinToPort ( digitalPin ) -  2 == Port ? true: false)

// Routine to be called if the target machine active pin is signalled - called by interrupt
void MachineActiveSignal ( void )
{
	TheMachine.CheckActivity ();
}

// Routine to be called if MACHINE_WORK_PIN is signalled - called by interrupt
void MachineWorkUnitSignal ( void )
{
	TheMachine.IncWorkUnit ( 1 );
}

// Class routines
TargetMachineClass::TargetMachineClass ( void )
{
	m_uiWorkPin = NOT_A_PIN;
	m_uiActivitePin = NOT_A_PIN;
	m_State = NOT_READY;
	m_Active = IDLE;
}

bool TargetMachineClass::AddFeatures ( uint8_t uiActivePin, uint8_t uiWorkPin, uint8_t uiActiveUnitTarget, uint8_t uiWorkUnitTarget )
{
	bool bResult = true;

	m_ulTargetSecs = uiActiveUnitTarget;
	m_ulTargetUnits = uiWorkUnitTarget;
	m_uiActivitePin = uiActivePin;
	m_uiWorkPin = uiWorkPin;

	if ( uiActivePin == NOT_A_PIN && uiWorkPin == NOT_A_PIN )
	{
		m_State = NO_FEATURES;
	}
	RestartMonitoring ();

	if ( uiActivePin != NOT_A_PIN )
	{
		if ( PCIHandler.AddPin ( uiActivePin, MachineActiveSignal, MACHINE_ACTIVE_PIN_SIGNAL, MACHINE_ACTIVE_PIN_MODE ) == false )
		{
			bResult = false;
		}
	}
	if ( uiWorkPin != NOT_A_PIN )
	{
		if ( PCIHandler.AddPin ( uiWorkPin, MachineWorkUnitSignal, MACHINE_WORK_PIN_SIGNAL, MACHINE_WORK_PIN_MODE ) == false )
		{
			bResult = false;
		}
	}
	return bResult;
}

void TargetMachineClass::RestartMonitoring ( void )
{
	m_timeActive = 0;
	m_ulWorkUnitCount = 0;
	if ( m_State != NO_FEATURES )
	{
		m_State = NOT_READY;
		m_Active = m_uiActivitePin == NOT_A_PIN ? IDLE : digitalRead ( m_uiActivitePin ) == MACHINE_ACTIVE_STATE ? ACTIVE : IDLE;
		if ( m_Active == ACTIVE )
		{
			m_timeActiveStarted = millis ();
		}
	}
}

void TargetMachineClass::CheckActivity ( void )
{
	// see how machine has changed state
	if ( digitalRead ( m_uiActivitePin ) == MACHINE_ACTIVE_STATE )
	{
		// machine gone active so remember when this started
		TheMachine.GoneActive ( millis () );
	}
	else
	{
		// machine gone idle so calc time was active and save it
		TheMachine.IncActiveTime ( millis () );
	}
}

bool TargetMachineClass::MachineUnitsDone ( void )
{
	bool bResult = false;
	if ( m_ulWorkUnitCount >= m_ulTargetUnits )
	{
		bResult = true;
	}
	return bResult;
}

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
	if ( m_timeActive / 1000 >= m_ulTargetSecs )
	{
		bResult = true;
	}
	return bResult;
}

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

uint32_t TargetMachineClass::GetActiveTime ( void )
{
	return m_timeActive / 1000;
}

uint32_t TargetMachineClass::GetWorkUnits ( void )
{
	return m_ulWorkUnitCount;
}

// add active time in milliseconds to total since machine became active
void TargetMachineClass::IncActiveTime ( uint32_t tNow )
{
	m_timeActive += ( tNow - m_timeActiveStarted );
	if ( m_timeActive / 1000 >= m_ulTargetSecs )
	{
		m_State = READY;
	}
	m_Active = digitalRead ( m_uiActivitePin ) == MACHINE_ACTIVE_STATE ? ACTIVE : IDLE;
}

void TargetMachineClass::GoneActive ( uint32_t tNow )
{
	m_Active = ACTIVE;
	m_timeActiveStarted = tNow;
}

void TargetMachineClass::IncWorkUnit ( uint32_t ulIncAmoount )
{
	m_ulWorkUnitCount += ulIncAmoount;
	if ( m_ulWorkUnitCount >= m_ulTargetUnits )
	{
		m_State = READY;
	}
}

bool TargetMachineClass::SetActiveTimeTarget ( uint32_t ulTargetSecs )
{
	bool bResult = false;
	if ( m_uiActivitePin != NOT_A_PIN )
	{
		m_ulTargetSecs = ulTargetSecs;
		bResult = true;
	}
	return bResult;
}

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

TargetMachineClass TheMachine;				// Create instance
