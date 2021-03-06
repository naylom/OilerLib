//
// TargetMachine.h
//
// (c) Mark Naylor 2021
//
// This class represents the machine that is being oiled
//
// It has two attributes: 
//		A signal that indicates the machine is active
//		A signal that indicates the machine has completed a unit of work
//
//	In our example machine - a metal working lathe, the active signal indicaates the machine is moving and the unit of work is a completed full rototation of the lather spindle
//
// The class keeps track of active time and number of units of work completed. These are optional inputs for the Oiler class to refine when it delivers oil.
//
#ifndef _TARGETMACHINE_h
#define _TARGETMACHINE_h

#include <Arduino.h>

#define		MACHINE_ACTIVE_PIN_MODE		INPUT_PULLUP		// Default value
#define		MACHINE_ACTIVE_PIN_SIGNAL	CHANGE				// Callback invoked if signal CHANGES
#define		MACHINE_ACTIVE_STATE		HIGH				// signal HIGH when machine is active, change to LOW if that is how target machine works
#define		MACHINE_WORK_PIN_MODE		INPUT_PULLUP		// Default value
#define		MACHINE_WORK_PIN_SIGNAL		FALLING				// signal FALLS when unit completed, change to RISING if that is how target machine works


typedef void ( *InterruptCallback )( void );

class TargetMachineClass
{
public:
	enum eMachineState { READY, NOT_READY, NO_FEATURES };		// Ready to be oiled or can't tell
	enum eActiveState { IDLE, ACTIVE };
	TargetMachineClass ( void );
	bool			AddFeatures ( uint8_t uiActivePin, uint8_t uiWorkPin );
	void			RestartMonitoring ( void );

	uint32_t		GetActiveTime ( void );						// Active time in secs since oiler stopped
	uint32_t		GetWorkUnits ( void );						// number of work units since oiler stopped

	void			IncWorkUnit ( uint32_t ulIncAmount );
	bool			SetActivePinMode ( uint8_t uiMode );		// set Active input pin to INPUT or INPUT_PULLUP
	bool			SetWorkPinMode ( uint8_t uiMode );			// set Work input pin to INPUT or INPUT_PULLUP
	bool			SetActiveState ( uint8_t uiState );			// set if HIGH or LOW indicates machine has power
	void			CheckActivity ( void );						// check activity after change in signal from machine

protected:
	void			UpdatePoweredTime ( void );
	void			IncActiveTime ( uint32_t tActive );

	eMachineState	m_State;
	eActiveState	m_Active;
	void			GoneActive ( uint32_t tNow );

	uint32_t		m_timeActive;								// time machine has been active since monitor reset
	uint32_t		m_timeActiveStarted;						// time machine last went active
	uint32_t		m_ulWorkUnitCount;
	uint8_t			m_uiActivePin;								// Pin used to signal when machine is active
	uint8_t			m_uiWorkPin;								// Pin used to signal when machine has completed work
	uint8_t			m_uiActivePinMode;
	uint8_t			m_uiWorkPinMode;
	uint8_t			m_uiActiveState;							// state that inidcates machine has power - must be HIGH or LOW
};

extern TargetMachineClass TheMachine;

#endif

