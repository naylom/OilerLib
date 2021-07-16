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

/// <summary>
/// adds a callback routine to be called at specified interval
/// </summary>
/// <param name="Routine">address of callback routine with signature of void func ( void )</param>
/// <param name="ulInterval">number of 1/4000 sec ticks after which callback should be invoked</param>
/// <returns>true if max number of callbacks not exceeded and this callback is not alreasy registered. else false</returns>
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

/// <summary>
/// Removes specified callback from configured list
/// </summary>
/// <param name="Routine">address of callback routine with signature of void func ( void )</param>
/// <returns>true if callback removed successfully, else false</returns>
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

/// <summary>
/// Gets the number of 1/4000 sec ticks required to pass before the callback whose index is provided is invoked
/// </summary>
/// <param name="uiIndex">zero based index of callback list entry required</param>
/// <returns>number of 1/4000 sec ticks</returns>
uint32_t TimerClass::GetInterval ( uint8_t uiIndex )
{
	return m_aFunctionIntervals [ uiIndex ];
}

/// <summary>
/// Get the address of the callback whose index is provided
/// </summary>
/// <param name="uiIndex">zero based index of callback list entry required</param>
/// <returns>callback function address</returns>
TimerCallback	TimerClass::GetCallback ( uint8_t uiIndex )
{
	return m_aFunctions [ uiIndex ];
}

/// <summary>
/// Clears callback list
/// </summary>
/// <param name="">none</param>
void TimerClass::ClearAllCallBacks ( void )
{
	m_uiCallbackCount = 0;
}

/// <summary>
/// Gets the number of callbacks that have been configured
/// </summary>
/// <param name="">none</param>
/// <returns>number of callbacks in list</returns>
uint8_t TimerClass::GetNumCallbacks ( void )
{
	return m_uiCallbackCount;
}

// Interrupt routine called by system timer
//ISR ( TIMER1_OVF_vect )

/// <summary>
/// hardware Timer2 interrupt - called every 1/4000 second. Checks if any configured callback is due to be called and invokes if necessary
/// </summary>
/// <param name="">none</param>
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

