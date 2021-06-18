// 
// 
// This implements a timer class that encapsulates a microsecond timer that has functions to call at given intervals. Its set to a resolution of 2000 ticks per sec
//
// (c) Mark Naylor June 2021
// 

#include "Timer.h"

TimerClass::TimerClass ( void )
{
	m_uiCallbackCount = 0;
	/*
	*	This code is designed for Arduino Uno only!!!!
	*
	*/
	noInterrupts ();

	//set timer2 interrupt at 4kHz
	TCCR2A = 0;				// set entire TCCR2A register to 0
	TCCR2B = 0;				// same for TCCR2B
	TCNT2 = 0;				//initialize counter value to 0

	// set compare match register for 4khz increments
	OCR2A = 108;				// = (16*10^6) / (4000*64) - 1 (must be <256)

	// turn on CTC mode
	TCCR2A |= ( 1 << WGM21 );
	// Set CS21 bit & CS20 bit for 64 prescaler
	//TCCR2B |= ( 1 << CS21 );
	// TCCR2B |= ( 1 << CS20 );
	// enable timer compare interrupt
	TIMSK2 |= ( 1 << OCIE2A );

	interrupts ();
}

bool TimerClass::AddCallBack ( TimerCallback Routine, uint32_t ulInterval )
{
	bool bResult = false;

	if ( m_uiCallbackCount < MAX_CALLBACKS )
	{
		// check callback not already registered
		for ( uint8_t i = 0; i < m_uiCallbackCount; i++ )
		{
			// look for match
			if ( m_aFunctions [ i ] == Routine )
			{
				// already here, so quit
				return bResult;
			}
		}
		m_aFunctions [ m_uiCallbackCount ] = Routine;
		m_aFunctionIntervals [ m_uiCallbackCount ] = ulInterval;
		m_uiCallbackCount++;

		bResult = true;
	}
	return bResult;
}

bool TimerClass::RemoveCallBack ( TimerCallback Routine )
{
	bool bResult = false;
	if ( m_uiCallbackCount > 0 )
	{
		for ( uint16_t i = 0; i < m_uiCallbackCount; i++ )
		{
			// look for match
			if ( m_aFunctions [ i ] == Routine )
			{
				// match found
				// overwrite with last entry
				noInterrupts ();
				m_aFunctions [ i ] = m_aFunctions [ m_uiCallbackCount ];
				m_aFunctionIntervals [ i ] = m_aFunctionIntervals [ m_uiCallbackCount ];
				m_uiCallbackCount--;
				interrupts ();
				bResult = true;
				break;
			}
		}
	}
	return bResult;
}

uint32_t TimerClass::GetInterval ( uint8_t uiIndex )
{
	return m_aFunctionIntervals [ uiIndex ];
}

TimerCallback	TimerClass::GetCallback ( uint8_t uiIndex )
{
	return m_aFunctions [ uiIndex ];
}

void TimerClass::ClearAllCallBacks ( void )
{
	m_uiCallbackCount = 0;
}

uint8_t TimerClass::GetNumCallbacks ( void )
{
	return m_uiCallbackCount;
}

// Interrupt routine called by system timer
//ISR ( TIMER1_OVF_vect )
ISR ( TIMER2_COMPA_vect )
{
	static uint32_t ulCount = 0;
	uint8_t uiNumCallbacks = TheTimer.GetNumCallbacks ();

	for ( uint8_t i = 0; i < uiNumCallbacks; i++ )
	{
		if ( ( ulCount % TheTimer.GetInterval ( i ) ) == 0 )
		{
			TheTimer.GetCallback ( i ) ( );
		}
	}
	ulCount++;
	TCNT2 = 0;		// Shouldn't be necessary!
}

TimerClass TheTimer;

