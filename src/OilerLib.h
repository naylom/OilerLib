//
//  OilerLib.h
// 
// (c) Mark Naylor June 2021
//
//	This class is encapsulates the main functionality of this project
//	The oiler consists of one or more motors and for each motor it has an associated input signal that tells when the motor has completed a unit of work. 
//  In this case a unit of work is a drop of oil.
// 
//  At its simplest the oiler will output oil on a timed basis, if the oiler is provided with a TargetMachine object that represents the machine being oiled, it will 
//  use this object to detect elapsed time the machine is active (ie ignore idle time) and / or when the TargetMachine has done an amount of work. In our lathe example this is 
//  tied to the number of revolutions it does.
//
//  Changelog
//	Ver 0.3 14/6/21	Changed the interrupt associated with motor outputting oil to trigger on a FALLING signal not whenever HIGH
//					Added code to ignore signals from sensor measuring motor output if not a gap of at least DEBOUNCE_THRESHOLD millseconds
//					Fixed bug where second motor would not get correct interrupt routine set up
//
//	Ver 0.4 14/6/21	Machine can now be queried if number of work units (eg revs of lathe spindle) or time machine has power has exceeded its related threshold and is ready to be oiled
//					This allows Oiler to only start oiling if the relevant metric is met for the Oiler mode
//
//	Ver 0.5 14/6/21	Oiler now restarts each motor separately after elapsed time so that if one motor takes longer to complete its work (drip oil) other motors will not be held up.
//
//	Ver 0.6 14/6/21	Added functionality to optionally specify pin to signalled if oiler has not oiled in multiple of target mode threshold eg twice elapsed time or three times spindle revs
//	Ver 1.0	17/6/21 remove dependency on attachInterrupt as too few options, input signals now monitored with Pin Change Interrupts
//					this allows more motors, so increased to 6
//	Ver 1.1 18/6/21 Converted to arduino library (OilerLib). Made public functions calls simpler to use in line with arduino guidelines. No new functionality
//
#ifndef _OILERLIB_h
#define _OILERLIB_h

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include "WProgram.h"
#endif

#define		OILER_VERSION				1.2

#define		MAX_MOTORS					6											// MAX the oiler can support
#define		MOTOR_WORK_SIGNAL_MODE		FALLING										// Change in signal when motor output (eg oil seen) is signalled
#define		MOTOR_WORK_SIGNAL_PINMODE	INPUT_PULLUP
#define		ALERT_PIN_ERROR_STATE		HIGH										// dewfault value
#define		TIME_BETWEEN_OILING			30											// default value  - In seconds
#define		NUM_MOTOR_WORK_EVENTS		3											// number of motor outputs (oil drips) after which motor is stopped and restarts waiting for mode threshold to occur
#define		DEBOUNCE_THRESHOLD			150UL										// milliseconds, increase if drip sensor is registering too many drips per single drip

#include "PCIHandler.h"
#include "RelayMotor.h"
#include "FourPinStepperMotor.h"
#include "TargetMachine.h"

class OilerClass
{
public:

	OilerClass ( TargetMachineClass* pMachine = NULL );
	// Operations
	bool				On ();														// Start all motors
	void				Off ();														// Stop all motors

	void				AddMachine ( TargetMachineClass* pMachine );				// optionally called to inform oiler we have a target machine that can be queried
	bool				AddMotor ( uint8_t uiPin1, uint8_t uiPin2, uint8_t uiPin3, uint8_t uiPin4, uint32_t ulSpeed, uint8_t uiWorkPin, uint8_t uiWorkTarget = NUM_MOTOR_WORK_EVENTS );		// FourPin Stepper version
	bool				AddMotor ( uint8_t uiPin, uint8_t uiWorkPin, uint8_t uiWorkTarget = NUM_MOTOR_WORK_EVENTS );		// 1 pin relay version
	bool				SetAlert ( uint8_t uiAlertPin, uint32_t uiAlertMultiple );	// Set the pin to be signalled when oiling is delayed.
	bool				SetAlertLevel ( uint8_t uiLevel );							// Set level of alert pin when in Alert State
	void				SetMotorsBackward ( void );									// Set direction of all motors
	void				SetMotorsBackward ( uint8_t uiMotorIndex );					// set direction of specified motor
	void				SetMotorsForward ( void );									// Set direction of all motors
	void				SetMotorsForward ( uint8_t uiMotorIndex );					// Set direction of specified motor
	bool				SetMotorWorkPinMode ( uint8_t uiMotorIndex, uint8_t uiMode );	// set mode to INPUT or INPUT_PULLUP for input sensor of specified motor
	bool				SetStartEventToTargetActiveTime ( uint32_t ulTargetSecs );	// set time target machine (eg lathe) has power to be event that causes motors to restart oiling
	bool				SetStartEventToTargetWork ( uint32_t ulTargetUnits );		// set amount of work done by target machine ( eg lathe) to be event that causes motors to restart oiling
	bool				SetStartEventToTime ( uint32_t ulElapsedSecs );				// set elapsed time to be event that causes motors to restart oiling
	// Queries
	bool				AllMotorsStopped ( void );									// true if no motors active
	bool				IsIdle ();													// true if all motors are paused waiting for event to start again
	bool				IsOff ( void );												// true if system is off
	bool				IsOiling ( void );											// true if at least one motor is running (ie pumping oil)
	bool				IsMonitoringTargetPower ( void );							// true if oiling starts as a result of target machine having power for configured seconds
	bool				IsMonitoringTargetWork ( void );							// true if monitoring target machine completing configured units of work (e.g. number of revs of lathe spindle)
	bool				IsMonitoringTime ( void );									// true if oiling starts as a result of configured number of seconds elapsing
	bool				IsMotorRunning ( uint8_t uiMotorNum );						// true if specified motor is running

	uint16_t			GetMotorWorkCount ( uint8_t uiMotorNum );					// get number of work units (oil drips) seen from specified motor since last told to start
	uint32_t			GetTimeOilerIdle ( void );									// returns time in seconds the Oiler has been idle (all motors off)
	uint32_t			GetTimeSinceMotorStarted ( uint8_t uiMotorIndex );			// returns time in seconds since motor started


/*---------------------- INTERNAL USE - DO NOT USE -----------------------------------*/
	void				MotorWork ( uint8_t uiMotorIndex );							// Used internally by interrupt handler to capture signal from a motor sensor when output (oil) is seen
	void				CheckTargetReady ( void );									// Checks if target is ready for oil
	void				CheckElapsedTime ( void );									// Checks time running since oiler last finished - this is the basic version not using TargetMachine

protected:
	enum eStartMode { ON_TIME = 0, ON_POWERED_TIME, ON_TARGET_ACTIVITY, NONE };
	enum eStatus { OILING = 0, OFF, IDLE };											// IDLE => waiting for start event

	void				CheckError ( uint32_t Actual, uint32_t Target );
	void				ClearError ( void );
	void				SetupMotorPins ( uint8_t uiWorkPin, uint8_t uiWorkTarget );
	MotorClass::eState	GetMotorState ( uint8_t uiMotorNum );						// get state of specified motor
	eStartMode			GetStartMode ( void );
	eStatus				GetStatus ( void );
	bool				SetStartMode ( eStartMode Mode, uint32_t uiModeTarget );

	eStartMode			m_OilerMode;
	eStatus				m_OilerStatus;
	TargetMachineClass* m_pMachine;
	uint32_t			m_timeOilerStopped;
	uint8_t				m_uiAlertPin;												// pin to signal if Alert to be generated
	uint16_t			m_ulAlertMultiple;											// Multiple of metric used to restart Oiler if motors are running in excess of AlertMultiple * metric
	uint8_t				m_uiALertOnValue;											// value to set pin when alert in on

	union																			// These values are mutually exclsuive so use same storage
	{
		uint32_t m_ulOilTime;
		uint32_t m_ulWorkTarget;
	};
	typedef struct
	{
		uint8_t					uiWorkPin;											// Pin that signals when motor has completed a unit of work e.g. a drip of oil
		MotorClass*				Motor;												// ptr to type of motor class
		uint16_t				uiWorkCount;										// Number of work units (oil drips) seen
		uint8_t					uiWorkTarget;										// Target number of work units (oil drips) from motor after which it is stopped
		uint16_t				uiAlertThreshold;									// if motor has been running in excess of threshold then alert will be signalled, 0 = no threshold
	} MOTOR_INFO;
	struct																			
	{
		uint8_t					uiNumMotors;
		MOTOR_INFO				MotorInfo [ MAX_MOTORS ];
	}	m_Motors;																	// keep track of each motor used by oiler
};

extern OilerClass TheOiler;

#endif
