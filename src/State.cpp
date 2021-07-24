//  State.h
// 
// (c) Mark Naylor July 2021
//
// State Table implementation
//
#include "OilerMotor.h"

/// <summary>
/// Initialise State table class with state table
/// </summary>
/// <param name="pTable">ptr to array of STATE_TABLE_ENTRIES</param>
/// <param name="uiNumEntries">number of elements in array</param>
OilerMotorClass::StateTable::StateTable ( PSTATE_TABLE_ENTRY pTable, uint16_t uiNumEntries )
{
	m_pTable = pTable;
	m_uiNumTableEntries = uiNumEntries;
	SetState ( 0 );
}

/// <summary>
/// Returns current state
/// </summary>
/// <param name="">none</param>
/// <returns>state</returns>
uint16_t OilerMotorClass::StateTable::GetCurrentState ( void )
{
	return m_uiCurrentState;
}

/// <summary>
/// Sets the current state
/// </summary>
/// <param name="uiNewState">new state id</param>
void OilerMotorClass::StateTable::SetState ( uint16_t uiNewState )
{
	m_uiCurrentState = uiNewState;
}

/// <summary>
/// Called to process a new event. If match not found, does nothing
/// </summary>
/// <param name="pMotor">reference to motor instance</param>
/// <param name="uiEventId">Id of event to process</param>
/// <returns>true if state changes, else false</returns>
bool OilerMotorClass::StateTable::ProcessEvent ( OilerMotorClass* pMotor, uint16_t uiEventId )
{
	bool bResult = false;
	for ( unsigned int i = 0; i < m_uiNumTableEntries; i++ )
	{
		if ( m_pTable [ i ].uiCurrentState == m_uiCurrentState )
		{
			if ( m_pTable [ i ].uiEventId == uiEventId )
			{
				OilerMotorStateCallback fn = m_pTable [ i ].FnState;
				uint16_t t = CALL_MEMBER_FN ( *pMotor, fn )();
				bResult = t == m_uiCurrentState ? false : true;
				SetState ( t );
				break;
			}
		}
	}
	return bResult;
}



