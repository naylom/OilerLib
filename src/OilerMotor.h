// OilerMotor.h
//
// (c) 2021 Mark Naylor
//
// defines class for motors driving an oiler, extends MotorClass and adds support for an input pin that signals when units of work (i.e. oil drips) are seen from from motor
// functionally adds support to:
//		only count work units that are separated by a specified debounce time (in milliseconds)
//		idle the motor when a specified threshold of work units is met
//		restart the motor after a specified time (in seconds) is passed
//
// This class implements a state table to control how the motor state changes in response to external events (e.g. signal work done, timer tick, on or off request
//
// Different types of motos used to do oiling e.g. a stepper motor or simple dc motor controlled by a relay switch derive from this class and override specific
// functions to idle, stop and start the motor.
//
#ifndef _OILER_MOTOR_h
#define _OILER_MOTOR_h

#include "Motor.h"

class OilerMotorClass : public MotorClass
{
protected:
	class StateTable
	{
		typedef uint16_t ( OilerMotorClass::*OilerMotorStateCallback )( );
	public:
		typedef struct
		{
			uint16_t				uiCurrentState;				// Current state
			uint16_t				uiEventId;					// Event that happened
			OilerMotorStateCallback	FnState;					// function to be called to process this event when in this state
		} STATE_TABLE_ENTRY, * PSTATE_TABLE_ENTRY;

		StateTable ( PSTATE_TABLE_ENTRY pTable, uint16_t uiNumEntries );
		uint16_t	GetCurrentState ( void );
		bool		ProcessEvent ( OilerMotorClass* pMotor, uint16_t uiEventId );


	protected:
		PSTATE_TABLE_ENTRY	m_pTable;				// ptr to array of table entries
		uint16_t			m_uiNumTableEntries;	// number of table entries
		uint16_t			m_uiCurrentState;		// Current State

		void		SetState ( uint16_t uiNewState );
	};

public:
	// State table functions
	virtual uint16_t		TurnOn ();							// function called to turn motor on and return new state
	virtual uint16_t		TurnOff ();							// function called to turn motor off and return new state
	virtual uint16_t		CheckWork ();						// function called to check if sufficient oil has been produced and to idle motor if it has
	virtual uint16_t		CheckTime ();						// function called to check if time passed implies oiler not working
	virtual uint16_t		DoNothing ();

	// abstract function that derived classes must implement
	virtual void			Idle () = 0;									// Idle motor, still energised but not moving
	virtual void			Start () = 0;									// Start motor
	virtual void			PowerOff () = 0;								// power off motor

protected:
	uint8_t		m_uiWorkPin;			// input Pin that indicates when a unit of work has been seen
	uint32_t	m_ulWorkThreshold;		// number of units to be seen before idling motor
	uint32_t	m_ulDebounceMin;		// number of milliseconds that must elapse before a subsequent workpin signal is treated as real
	uint16_t	m_uiWorkCount;			// number of work units seen since last reset
	uint32_t	m_ulLastWorkSignal;		// time of last signal in millis
	uint16_t	m_uiTimeThresholdSec;	// time after which idle motor should restart
	StateTable	m_MotorState;

/*
*	Oiler Motor State table
*/

public:
	enum eOilerMotorState : uint16_t						// States motor can be in
	{
		OFF = 0,			// OFF => not energised, default state at start must be 0
		IDLE,				// IDLE imples not moving but is being held stationary
		MOVING
	};

	enum eOilerMotorEvents : uint16_t						// events that can change motor state
	{
		TURN_ON = 0,		// Request to turn on
		TURN_OFF,			// Request to turn off
		WORK_SEEN,			// Output seen
		TIMER				// Timer check
	};
private:
	StateTable::STATE_TABLE_ENTRY	MotorTable [ 12 ]
	{
		{ OFF,		TURN_ON,	&OilerMotorClass::TurnOn },				// start motor moving
		{ OFF,		TURN_OFF,	&OilerMotorClass::DoNothing },			// if off no need to turn off, ignore
		{ OFF,		WORK_SEEN,	&OilerMotorClass::DoNothing },			// if off and oil drips output, ignore
		{ OFF,		TIMER,		&OilerMotorClass::DoNothing },			// if off ignore time
		{ MOVING,	TURN_ON,	&OilerMotorClass::DoNothing },			// if moving no need to start moving, ignore
		{ MOVING,	TURN_OFF,	&OilerMotorClass::TurnOff },			// if moving, turn off
		{ MOVING,	WORK_SEEN,	&OilerMotorClass::CheckWork },			// if moving and oil drip see check if enough produced and idle motor
		{ MOVING,	TIMER,		&OilerMotorClass::DoNothing },			// if moving, nothing to do
		{ IDLE,		TURN_ON,	&OilerMotorClass::TurnOn },				// start motor moving
		{ IDLE,		TURN_OFF,	&OilerMotorClass::TurnOff },			// turn off
		{ IDLE,		WORK_SEEN,	&OilerMotorClass::DoNothing },			// oil produced whilst idle - ignore
		{ IDLE,		TIMER,		&OilerMotorClass::CheckTime }			// see if we need to restart based on time idle
	};

public:
	OilerMotorClass ( uint8_t uiWorkPin, uint32_t ulThreshold, uint32_t ulDebouncems, uint32_t ulSpeed, uint16_t ulTimeThreshold );
	virtual bool On ( void );
	virtual bool Off ( void );
	void IncWorkUnits ( uint16_t uiNewUnits = 1 );
	void ResetWorkUnits ( void );
	void SetDebouncems ( uint32_t ulDebouncems );
	void SetWorkThreshold ( uint32_t ulWorkThreshold );
	void SetTimeThreshold ( uint16_t uiTimeThresholdSec );
	bool Action ( eOilerMotorEvents eAction );
	uint16_t GetWorkUnits ();
	eOilerMotorState GetOilerMotorState ();
	bool IsIdle ();
	bool IsMoving ();
	bool IsOff ();
};
#endif