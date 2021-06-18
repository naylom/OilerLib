// RelayMotor.h
//
// (c) Mark Naylor June 2021
// 
// RelayMotor class, derivative of MotorClass for driving a DC motor via a change over relay
//

#ifndef _RELAYMOTOR_h
#define _RELAYMOTOR_h

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include "WProgram.h"
#endif
#include "Motor.h"

class RelayMotorClass : MotorClass
{
public:
	RelayMotorClass ( uint8_t uiPin );
	bool				On ( void );
	bool				Off ( void );
	void				SetDirection ( eDirection Direction );		// Does nothing for this type of motor
	MotorClass::eState	GetMotorState ( void );

protected:
	uint8_t		m_uiPin;
};

#endif