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
#include "OilerMotor.h"

class RelayMotorClass : public OilerMotorClass
{
public:
						RelayMotorClass ( uint8_t uiPin, uint8_t uiWorkPin, uint32_t ulWorkThreshold, uint32_t ulDebouncems, uint32_t ulTimeThreshold );
	void				Idle ();
	void				Start ();
	void				PowerOff ();

	void				SetDirection ( eDirection Direction );		// Does nothing for this type of motor

protected:
	uint8_t		m_uiRelayPin;										// Pin that controls relay switch
};

#endif