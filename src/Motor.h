// Motor.h
//
// (c) 2021 Mark Naylor
//
// defines base class for motors 
//

#ifndef _MOTOR_h
#define _MOTOR_h

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include "WProgram.h"
#endif
//#include "State.h"
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

class MotorClass
{
public:
	enum			eDirection
	{
		FORWARD, 
		BACKWARD
	};
	enum			eState
	{

		STOPPED = 1, RUNNING
	};

	virtual bool	On ( void );						// Needs to be overriden to implement details of how motor is enabled
	virtual bool	Off ( void );
	uint32_t		GetTimeMotorStarted ( void );		// returns millis that it started
	uint32_t		GetTimeMotorRunning ( void );		// returns seconds it has been running, 0 if stopped
	uint32_t		GetTimeMotorStopped ( void );
	eState			GetMotorState ( void );
	uint32_t		GetSpeed ( void );
	bool			SetSpeed ( uint32_t ulSpeed );
	void			SetDirection ( eDirection eDir );

	MotorClass ( uint32_t ulSpeed );

protected:
	uint32_t	m_ulSpeed;
	uint32_t	m_ulTimeStartedms;					// Time motor was last started in ms
	uint32_t	m_ulTimeStoppedms;					// Time motor was last stopped in ms
	eState		m_eState;
	eDirection	m_eDir;
};

#endif

