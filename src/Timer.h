// Timer.h
//
// This defines a timer class that encapsulates a microsecond timer that has functions to call at given intervals
//
// (c) Mark Naylor June 2021
//

#ifndef _TIMER_h
#define _TIMER_h

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include "WProgram.h"
#endif
#define MAX_CALLBACKS	8
#define RESOLUTION		2000		// ticks per sec

typedef void ( *TimerCallback )( void );

class TimerClass
{
public:
	TimerClass ( void );
	bool		AddCallBack ( TimerCallback Routine, uint32_t uiInterval );
	bool		RemoveCallBack ( TimerCallback Routine );
	uint32_t	GetInterval ( uint8_t uiIndex );
	TimerCallback GetCallback ( uint8_t uiIndex );
	void		ClearAllCallBacks ( void );
	uint8_t		GetNumCallbacks ( void );



protected:
	uint8_t			m_uiCallbackCount;
	TimerCallback	m_aFunctions [ MAX_CALLBACKS ];
	uint32_t		m_aFunctionIntervals [ MAX_CALLBACKS ];
};

extern TimerClass TheTimer;

#endif

