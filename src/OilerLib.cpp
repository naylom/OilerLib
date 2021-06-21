// 
//  Oiler.cpp
// 
// (c) Mark Naylor June 2021

#include "OilerLib.h"
#include "Timer.h"
#include "PCIHandler.h"

void Motor1WorkSignal ( void )
{
	static uint32_t ulLastSignal = 0;

	if ( TheOiler.MotorWork ( ulLastSignal, 0 ) == true )
	{
		// signal was accepted
		ulLastSignal = millis ();
	}
}

void Motor2WorkSignal ( void )
{
	static uint32_t ulLastSignal = 0;

	if ( TheOiler.MotorWork ( ulLastSignal, 1 ) == true )
	{
		// signal was accepted
		ulLastSignal = millis ();
	}
}

void Motor3WorkSignal ( void )
{
	static uint32_t ulLastSignal = 0;

	if ( TheOiler.MotorWork ( ulLastSignal, 2 ) == true )
	{
		// signal was accepted
		ulLastSignal = millis ();
	}
}

void Motor4WorkSignal ( void )
{
	static uint32_t ulLastSignal = 0;

	if ( TheOiler.MotorWork ( ulLastSignal, 3 ) == true )
	{
		// signal was accepted
		ulLastSignal = millis ();
	}
}

void Motor5WorkSignal ( void )
{
	static uint32_t ulLastSignal = 0;

	if ( TheOiler.MotorWork ( ulLastSignal, 4 ) == true )
	{
		// signal was accepted
		ulLastSignal = millis ();
	}
}

void Motor6WorkSignal ( void )
{
	static uint32_t ulLastSignal = 0;

	if ( TheOiler.MotorWork ( ulLastSignal, 5 ) == true )
	{
		// signal was accepted
		ulLastSignal = millis ();
	}
}
// list of ISRs for each possible motor
typedef void ( *Callback )( void );
struct
{
	Callback MotorWorkCallback [ MAX_MOTORS ];
} MotorISRs =
{
	Motor1WorkSignal,
	Motor2WorkSignal,
	Motor3WorkSignal,
	Motor4WorkSignal,
	Motor5WorkSignal,
	Motor6WorkSignal
};

void OilerTmerCallback ( void )
{
	if ( TheOiler.IsOff () == false )
	{
		// Invoked once per second to check if oiler needs starting
		if ( TheOiler.IsMonitoringTime () )
		{
			TheOiler.CheckElapsedTime ();
		}
		else if ( TheOiler.IsMonitoringTargetPower() || TheOiler.IsMonitoringTargetWork() )
		{
			TheOiler.CheckTargetReady ();
		}
	}
}

OilerClass::OilerClass ( TargetMachineClass* pMachine )
{
	m_pMachine				= pMachine;
	m_OilerMode				= ON_TIME;
	m_OilerStatus			= OFF;
	m_ulOilTime				= TIME_BETWEEN_OILING;
	m_Motors.uiNumMotors	= 0;
	m_uiAlertPin			= NOT_A_PIN;
	m_ulAlertMultiple		= 0UL;
	m_uiALertOnValue		= ALERT_PIN_ERROR_STATE;
}

bool OilerClass::On ()
{
	// can only start if > 0 motors!
	bool bResult = false;
	if ( m_Motors.uiNumMotors > 0 )
	{
		for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
		{
			m_Motors.MotorInfo [ i ].Motor->On ();
			m_Motors.MotorInfo [ i ].uiWorkCount = 0;
		}
		// if we have a machine, start monitoring
		if ( m_OilerMode != ON_TIME && m_pMachine != NULL )
		{
			m_pMachine->RestartMonitoring ();
		}

		TheTimer.AddCallBack ( OilerTmerCallback, RESOLUTION );		// callback once per sec
		m_OilerStatus = OILING;
		bResult = true;
	}
	return bResult;
}

void OilerClass::Off ()
{
	for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
	{
		m_Motors.MotorInfo [ i ].Motor->Off ();
	}
	m_OilerStatus = OFF;
	m_timeOilerStopped = millis ();
}

bool OilerClass::AddMotor ( uint8_t uiPin1, uint8_t uiPin2, uint8_t uiPin3, uint8_t uiPin4, uint32_t ulSpeed, uint8_t uiWorkPin, uint8_t uiWorkTarget )
{
	bool bResult = false;
	if ( m_Motors.uiNumMotors < MAX_MOTORS /* && digitalPinToInterrupt (uiWorkPin) != NOT_AN_INTERRUPT */ )
	{
		// space to add another motor
		m_Motors.MotorInfo [ m_Motors.uiNumMotors ].Motor = ( MotorClass* )new FourPinStepperMotorClass ( uiPin1, uiPin2, uiPin3, uiPin4, ulSpeed );
		SetupMotorPins ( uiWorkPin, uiWorkTarget );
		bResult = true;
	}
	return bResult;
}

bool OilerClass::AddMotor ( uint8_t uiPin, uint8_t uiWorkPin, uint8_t uiWorkTarget )
{
	bool bResult = false;
	if ( m_Motors.uiNumMotors < MAX_MOTORS /* && digitalPinToInterrupt (uiWorkPin) != NOT_AN_INTERRUPT */ )
	{
		// space to add another motor
		m_Motors.MotorInfo [ m_Motors.uiNumMotors ].Motor = ( MotorClass* )new RelayMotorClass ( uiPin );
		SetupMotorPins ( uiWorkPin, uiWorkTarget );
		bResult = true;
	}
	return bResult;
}

void OilerClass::SetupMotorPins ( uint8_t uiWorkPin, uint8_t uiWorkTarget )
{
	m_Motors.MotorInfo [ m_Motors.uiNumMotors ].uiWorkPin = uiWorkPin;
	m_Motors.MotorInfo [ m_Motors.uiNumMotors ].uiWorkTarget = uiWorkTarget;
	m_Motors.MotorInfo [ m_Motors.uiNumMotors ].uiWorkDebounce = DEBOUNCE_THRESHOLD;			// set default
	PCIHandler.AddPin ( uiWorkPin, MotorISRs.MotorWorkCallback [ m_Motors.uiNumMotors ], MOTOR_WORK_SIGNAL_MODE, MOTOR_WORK_SIGNAL_PINMODE );
	m_Motors.uiNumMotors++;
}

void	OilerClass::AddMachine ( TargetMachineClass* pMachine )
{
	m_pMachine = pMachine;
}

bool	OilerClass::SetAlert ( uint8_t uiAlertPin, uint32_t ulAlertMultiple )
{
	bool bResult = false;

	if ( uiAlertPin != NOT_A_PIN )
	{
		m_uiAlertPin = uiAlertPin;
		pinMode ( m_uiAlertPin, OUTPUT );
		ClearError ();
		m_ulAlertMultiple = ulAlertMultiple;
		bResult = true;
	}

	return bResult;
}

bool OilerClass::SetAlertLevel ( uint8_t uiLevel )
{
	bool bResult = false;
	if ( uiLevel == HIGH || uiLevel == LOW )
	{
		// if changed
		if ( m_uiALertOnValue != uiLevel )
		{
			digitalWrite ( m_uiAlertPin, uiLevel );
			m_uiALertOnValue = uiLevel;
		}
		bResult = true;
	}
	return bResult;
}

bool OilerClass::IsOiling ( void )
{
	return m_OilerStatus == OILING;
}

bool OilerClass::IsOff ( void )
{
	return m_OilerStatus == OFF;
}

bool OilerClass::IsIdle ()
{
	return m_OilerStatus == OFF;
}

bool OilerClass::IsMonitoringTime ( void )
{
	return m_OilerMode == ON_TIME;
}

bool OilerClass::IsMonitoringTargetPower ( void )
{
	return m_OilerMode == ON_POWERED_TIME;
}

bool OilerClass::IsMonitoringTargetWork ( void )
{
	return m_OilerMode == ON_TARGET_ACTIVITY;
}

bool OilerClass::MotorWork ( uint32_t ulLastSignalTime, uint8_t uiMotorIndex )
{
	bool bResult = false;
	// Check if this signal is within the debounce period
	if ( millis () - ulLastSignalTime >= m_Motors.MotorInfo [ uiMotorIndex ].uiWorkDebounce )
	{
		bResult = true;
		// One of Oiler motors has completed work
		m_Motors.MotorInfo [ uiMotorIndex ].uiWorkCount++;
		// check if it has hit target
		if ( m_Motors.MotorInfo [ uiMotorIndex ].uiWorkCount >= NUM_MOTOR_WORK_EVENTS )
		{
			// hit target, stop motor
			m_Motors.MotorInfo [ uiMotorIndex ].Motor->Off ();

			// have we stopped all motors
			if ( AllMotorsStopped () )
			{
				ClearError ();
				// restart monitoring, if we have a machine
				if ( m_pMachine != NULL && m_OilerMode != ON_TIME )
				{
					m_pMachine->RestartMonitoring ();
					m_timeOilerStopped = millis ();
				}
				// reset start time count
				if ( m_OilerMode == ON_TIME )
				{
					m_timeOilerStopped = millis ();
				}
				m_OilerStatus = IDLE;
			}
		}
	}
	return bResult;
}

OilerClass::eStartMode OilerClass::GetStartMode ( void )
{
	return m_OilerMode;
}

OilerClass::eStatus OilerClass::GetStatus ( void )
{
	return m_OilerStatus;
}

void OilerClass::CheckTargetReady ( void )
{
	static uint32_t ulOilingFailed = 0;

	switch ( m_OilerMode )
	{
		case ON_TARGET_ACTIVITY:
			if ( m_pMachine->MachineUnitsDone () )
			{
				if ( m_OilerStatus == OILING )
				{
					// Still Oiling and machine is ready for more oil - one or more motors isn't outputting in time
					ulOilingFailed++;
					CheckError ( ulOilingFailed, 1 );
				}
				else
				{
					ulOilingFailed = 0;
				}
				On ();
				m_pMachine->RestartMonitoring ();
			}
			break;

		case ON_POWERED_TIME:
			if ( m_pMachine->MachinePoweredTimeExpired () )
			{
				if ( m_OilerStatus == OILING )
				{
					// Still Oiling and machine is ready for more oil - one or more motors isn't outputting in time
					ulOilingFailed++;
					CheckError ( ulOilingFailed, 1 );
				}
				else
				{
					ulOilingFailed = 0;
				}
				On ();
				m_pMachine->RestartMonitoring ();
			}
			break;

		default:		// do nothing, shouldn't get here
			break;
	}
}

// returns time since Oiler went idle in seconds
uint32_t OilerClass::GetTimeOilerIdle ( void )
{
	uint32_t	ulResult = 0UL;

	// if no oiler motors pumping
	if ( AllMotorsStopped () && GetStatus () != OFF )
	{
		ulResult = ( millis () - m_timeOilerStopped ) / 1000;
	}
	return ulResult;
}

uint32_t OilerClass::GetTimeSinceMotorStarted ( uint8_t uiMotorIndex )
{
	uint32_t ulResult = 0;
	if ( uiMotorIndex < m_Motors.uiNumMotors )
	{
		ulResult = m_Motors.MotorInfo [ uiMotorIndex ].Motor->GetTimeMotorRunning ();
	}
	return ulResult;
}

// called by timer callback when in ON_TIME mode to restart oiler motors if necessary
void OilerClass::CheckElapsedTime ()
{
	// check if motor should be restarted
	if ( GetStatus () != OFF )
	{
		if ( m_Motors.uiNumMotors > 0 )
		{
			uint32_t tNow = millis ();
			for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
			{
				// check if motor not running
				if ( m_Motors.MotorInfo [ i ].Motor->GetMotorState () == MotorClass::STOPPED )
				{
					// check elapsed time
					if ( ( tNow - m_Motors.MotorInfo [ i ].Motor->GetTimeMotorStopped () ) / 1000 > m_ulOilTime )
					{
						m_Motors.MotorInfo [ i ].Motor->On ();
						m_Motors.MotorInfo [ i ].uiWorkCount = 0;
					}
				}
				else
				{
					// Check motor not running beyond expected time
					CheckError ( m_Motors.MotorInfo [ i ].Motor->GetTimeMotorRunning (), m_ulOilTime );
				}
			}
		}
	}
}

void	OilerClass::CheckError ( uint32_t ulActual, uint32_t ulTarget )
{
	if ( m_ulAlertMultiple > 0 )
	{
		if ( ulActual >= ulTarget * m_ulAlertMultiple )
		{
			// Serial.print ( "Actual " ); Serial.print ( ulActual ); Serial.print ( " Target " ); Serial.println ( ulTarget );
			if ( m_uiAlertPin != NOT_A_PIN )
			{
				digitalWrite ( m_uiAlertPin, m_uiALertOnValue );
			}
		}
	}
}

void	OilerClass::ClearError ( void )
{
	if ( m_uiAlertPin != NOT_A_PIN )
	{
		digitalWrite ( m_uiAlertPin, m_uiALertOnValue == HIGH ? LOW : HIGH );
	}
}

uint16_t OilerClass::GetMotorWorkCount ( uint8_t uiMotorNum )
{
	uint8_t uiResult = 0;
	if ( uiMotorNum < m_Motors.uiNumMotors )
	{
		uiResult = m_Motors.MotorInfo [ uiMotorNum ].uiWorkCount;
	}
	return uiResult;
}

bool OilerClass::IsMotorRunning ( uint8_t uiMotorNum )
{
		return  GetMotorState (  uiMotorNum ) == MotorClass::RUNNING;
}

MotorClass::eState OilerClass::GetMotorState ( uint8_t uiMotorNum )
{
	MotorClass::eState eResult = MotorClass::STOPPED;
	if ( uiMotorNum < m_Motors.uiNumMotors )
	{
		eResult = m_Motors.MotorInfo [ uiMotorNum ].Motor->GetMotorState ();
	}
	return eResult;
}

bool OilerClass::AllMotorsStopped ( void )
{
	bool bResult = true;
	for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
	{
		if ( m_Motors.MotorInfo [ i ].Motor->GetMotorState () == MotorClass::RUNNING )
		{
			bResult = false;
			break;
		}
	}
	return bResult;
}

bool OilerClass::SetStartMode ( eStartMode Mode, uint32_t ulModeTarget )
{
	bool bResult = false;
	switch ( Mode )
	{
		case ON_TIME:
			m_ulOilTime = ulModeTarget;
			m_OilerMode = Mode;
			bResult = true;
			break;

		case ON_POWERED_TIME:
			if ( m_pMachine != NULL )
			{
				if ( m_pMachine->SetActiveTimeTarget ( ulModeTarget ) )
				{
					m_ulOilTime = ulModeTarget;
					m_OilerMode = Mode;
					bResult = true;
				}
			}
			break;

		case ON_TARGET_ACTIVITY:
			if ( m_pMachine != NULL )
			{
				if ( m_pMachine->SetWorkTarget ( ulModeTarget ) )
				{
					m_ulWorkTarget = ulModeTarget;
					m_OilerMode = Mode;
					bResult = true;
				}
			}
			break;

		default:
			break;
	}
	return bResult;
}

// Set direction of specified motor 
void OilerClass::SetMotorsForward ( uint8_t uiMotorIndex )
{
	for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
	{
		m_Motors.MotorInfo [ i ].Motor->SetDirection ( MotorClass::FORWARD );
	}
}

bool OilerClass::SetMotorSensorDebounce ( uint8_t uiMotorIndex, uint16_t uiDelayms )
{
	bool bResult = false;
	if ( uiMotorIndex < m_Motors.uiNumMotors )
	{
		m_Motors.MotorInfo [ uiMotorIndex ].uiWorkDebounce = uiDelayms;
		bResult = true;
	}
	return bResult;
}

bool OilerClass::SetStartEventToTargetActiveTime ( uint32_t ulTargetSecs )
{
	return SetStartMode ( ON_POWERED_TIME, ulTargetSecs );
}

bool OilerClass::SetStartEventToTargetWork ( uint32_t ulTargetUnits )
{
	return SetStartMode ( ON_TARGET_ACTIVITY, ulTargetUnits );
}

bool OilerClass::SetStartEventToTime ( uint32_t ulElapsedSecs )
{
	return SetStartMode ( ON_TIME, ulElapsedSecs );
}

bool OilerClass::SetMotorWorkPinMode ( uint8_t uiMotorIndex, uint8_t uiMode )
{
	bool bResult = false;
	if ( ( uiMode == INPUT || uiMode == INPUT_PULLUP ) && uiMotorIndex < m_Motors.uiNumMotors )
	{
		pinMode ( m_Motors.MotorInfo [ 1 ].uiWorkPin, uiMode );
	}
	return bResult;
}

// Set all motors in specified direction
void OilerClass::SetMotorsForward ( void )
{
	for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
	{
		SetMotorsForward ( i );
	}
}

// Set direction of specified motor 
void OilerClass::SetMotorsBackward ( uint8_t uiMotorIndex )
{

	m_Motors.MotorInfo [ uiMotorIndex ].Motor->SetDirection ( MotorClass::BACKWARD );
}

// Set all motors in specified direction
void OilerClass::SetMotorsBackward ( void )
{
	for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
	{
		SetMotorsBackward ( i );
	}
}

OilerClass TheOiler;

