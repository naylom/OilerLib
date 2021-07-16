// 
//  Oiler.cpp
// 
// (c) Mark Naylor June 2021

#include "OilerLib.h"
#include "Timer.h"
#include "PCIHandler.h"
/// <summary>
/// Called by interrupt routine handling signal from pin indicating motor 1 has produced a unit of work
/// </summary>
/// <param name="">none</param>
void Motor1WorkSignal ( void )
{
	static uint32_t ulLastSignal = 0;

	if ( TheOiler.MotorWork ( ulLastSignal, 0 ) == true )
	{
		// signal was accepted
		ulLastSignal = millis ();
	}
}
/// <summary>
/// Called by interrupt routine handling signal from pin indicating motor 2 has produced a unit of work
/// </summary>
/// <param name="">none</param>
void Motor2WorkSignal ( void )
{
	static uint32_t ulLastSignal = 0;

	if ( TheOiler.MotorWork ( ulLastSignal, 1 ) == true )
	{
		// signal was accepted
		ulLastSignal = millis ();
	}
}
/// <summary>
/// Called by interrupt routine handling signal from pin indicating motor 3 has produced a unit of work
/// </summary>
/// <param name="">none</param>
void Motor3WorkSignal ( void )
{
	static uint32_t ulLastSignal = 0;

	if ( TheOiler.MotorWork ( ulLastSignal, 2 ) == true )
	{
		// signal was accepted
		ulLastSignal = millis ();
	}
}
/// <summary>
/// Called by interrupt routine handling signal from pin indicating motor 4 has produced a unit of work
/// </summary>
/// <param name="">none</param>
void Motor4WorkSignal ( void )
{
	static uint32_t ulLastSignal = 0;

	if ( TheOiler.MotorWork ( ulLastSignal, 3 ) == true )
	{
		// signal was accepted
		ulLastSignal = millis ();
	}
}
/// <summary>
/// Called by interrupt routine handling signal from pin indicating motor 5 has produced a unit of work
/// </summary>
/// <param name="">none</param>
void Motor5WorkSignal ( void )
{
	static uint32_t ulLastSignal = 0;

	if ( TheOiler.MotorWork ( ulLastSignal, 4 ) == true )
	{
		// signal was accepted
		ulLastSignal = millis ();
	}
}
/// <summary>
/// Called by interrupt routine handling signal from pin indicating motor 6 has produced a unit of work
/// </summary>
/// <param name="">none</param>
void Motor6WorkSignal ( void )
{
	static uint32_t ulLastSignal = 0;

	if ( TheOiler.MotorWork ( ulLastSignal, 5 ) == true )
	{
		// signal was accepted
		ulLastSignal = millis ();
	}
}
// list of ISRs for each motor upto max allowed
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
/// <summary>
/// Callback from interrupt based timer to check if restart event has been met and motors need restarting
/// </summary>
/// <param name=""></param>
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
/// <summary>
/// initialises oiler
/// </summary>
/// <param name="pMachine"></param>
OilerClass::OilerClass ( TargetMachineClass* pMachine )
{
	m_pMachine				= pMachine;
	m_OilerMode				= ON_TIME;					// default vvalue
	m_OilerStatus			= OFF;						
	m_ulOilTime				= TIME_BETWEEN_OILING;		//default value
	m_Motors.uiNumMotors	= 0;
	m_uiAlertPin			= NOT_A_PIN;
	m_ulAlertMultiple		= 0UL;
	m_uiALertOnValue		= ALERT_PIN_ERROR_STATE;	// default value
	m_bAlert				= false;
}
/// <summary>
/// starts all motors
/// </summary>
/// <returns></returns>
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
/// <summary>
/// turns all motors off
/// </summary>
void OilerClass::Off ()
{
	for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
	{
		m_Motors.MotorInfo [ i ].Motor->Off ();
	}
	m_OilerStatus = OFF;
	m_timeOilerStopped = millis ();
}
/// <summary>
/// Adds a new four pin stepper driver based motor
/// </summary>
/// <param name="uiPin1">digital pin used to control stepper driver</param>
/// <param name="uiPin2">digital pin used to control stepper driver</param>
/// <param name="uiPin3">digital pin used to control stepper driver</param>
/// <param name="uiPin4">digital pin used to control stepper driver</param>
/// <param name="ulSpeed">time in microseconds between updates sent to stepper motor</param>
/// <param name="uiWorkPin">digital pin that signals when motor has caused a unit of work (eg oil drip) to be produced</param>
/// <param name="uiWorkTarget">number of work units after which motor is turned off</param>
/// <returns>false if number of motors exceeds maximum, else true</returns>
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
/// <summary>
/// Adds a new relay based motor to oiler.
/// </summary>
/// <param name="uiPin">digital pin to signal to turn on relay and hence motor</param>
/// <param name="uiWorkPin">digital pin that signals when motor has caused a unit of work (eg oil drip) to be produced</param>
/// <param name="uiWorkTarget">number of work units after which motor is turned off</param>
/// <returns>false if number of motors exceeds maximum, else true</returns>
bool OilerClass::AddMotor ( uint8_t uiPin, uint8_t uiWorkPin, uint8_t uiWorkTarget )
{
	bool bResult = false;
	if ( m_Motors.uiNumMotors < MAX_MOTORS )
	{
		// space to add another motor
		m_Motors.MotorInfo [ m_Motors.uiNumMotors ].Motor = ( MotorClass* )new RelayMotorClass ( uiPin );
		SetupMotorPins ( uiWorkPin, uiWorkTarget );
		bResult = true;
	}
	return bResult;
}
/// <summary>
/// Stores information to current (last) motor about pin that signals motor has produced work (e.g. oil drip) and the target number after which it is stopped
/// </summary>
/// <param name="uiWorkPin">pin to signal work unit has been produced</param>
/// <param name="uiWorkTarget">number of units of work (eg oil drips) after which motor is stopped</param>
void OilerClass::SetupMotorPins ( uint8_t uiWorkPin, uint8_t uiWorkTarget )
{
	m_Motors.MotorInfo [ m_Motors.uiNumMotors ].uiWorkPin = uiWorkPin;
	m_Motors.MotorInfo [ m_Motors.uiNumMotors ].uiWorkTarget = uiWorkTarget;
	m_Motors.MotorInfo [ m_Motors.uiNumMotors ].uiWorkDebounce = DEBOUNCE_THRESHOLD;			// set default
	PCIHandler.AddPin ( uiWorkPin, MotorISRs.MotorWorkCallback [ m_Motors.uiNumMotors ], MOTOR_WORK_SIGNAL_MODE, MOTOR_WORK_SIGNAL_PINMODE );
	m_Motors.uiNumMotors++;
}
/// <summary>
/// Add the machine being oiled object to the oiler, prerequisite for setting restart mode to target machine powered time or units of work
/// </summary>
/// <param name="pMachine">object representing machine being oiled</param>
void	OilerClass::AddMachine ( TargetMachineClass* pMachine )
{
	m_pMachine = pMachine;
}
/// <summary>
/// Configures alert settings
/// </summary>
/// <param name="uiAlertPin">pin on which to signal when in alert state, if NOT_A_PIN no signal will be output</param>
/// <param name="ulAlertMultiple">multiple of restart threshold after which alert is generated</param>
void	OilerClass::SetAlert ( uint8_t uiAlertPin, uint32_t ulAlertMultiple )
{
	m_uiAlertPin = uiAlertPin;
	if ( uiAlertPin != NOT_A_PIN )
	{
		pinMode ( m_uiAlertPin, OUTPUT );
	}
	// no pin just do software error
	ClearError ();
	m_ulAlertMultiple = ulAlertMultiple;
}
/// <summary>
/// Sets whether the alert pin should be held HIGH or LOW when in alert state
/// </summary>
/// <param name="uiLevel">HIGH or LOW</param>
/// <returns>false if uiLevel not valid or no alert pin configured or already configured to uiLevel value, else true</returns>
bool OilerClass::SetAlertLevel ( uint8_t uiLevel )
{
	bool bResult = false;
	if ( uiLevel == HIGH || uiLevel == LOW )
	{
		// if changed and we have a pin
		if ( m_uiALertOnValue != uiLevel && m_uiAlertPin != NOT_A_PIN )
		{
			digitalWrite ( m_uiAlertPin, m_bAlert == true ? uiLevel : m_uiALertOnValue );
			m_uiALertOnValue = uiLevel;
		}
		bResult = true;
	}
	return bResult;
}
/// <summary>
/// Checks if oiler is in an alert state
/// </summary>
/// <param name="">none</param>
/// <returns>true if in alert state, else false</returns>
bool OilerClass::IsAlert ( void )
{
	return m_bAlert;
}
/// <summary>
/// Checks if the oiler is oiling
/// </summary>
/// <param name="">none</param>
/// <returns>true if oiling, else false</returns>
bool OilerClass::IsOiling ( void )
{
	return m_OilerStatus == OILING;
}
/// <summary>
/// Checks if oiler is off
/// </summary>
/// <param name="">none</param>
/// <returns>true if off, else false</returns>
bool OilerClass::IsOff ( void )
{
	return m_OilerStatus == OFF;
}
/// <summary>
/// Checks if oiler is idle
/// </summary>
/// <param name="">none</param>
/// <returns>true if idle, else false</returns>
bool OilerClass::IsIdle ( void )
{
	return m_OilerStatus == IDLE;
}
/// <summary>
/// Checks if system restart mode is monitoring elapsed time
/// </summary>
/// <param name="">none</param>
/// <returns>true if mode is ON_TIME, else false</returns>
bool OilerClass::IsMonitoringTime ( void )
{
	return m_OilerMode == ON_TIME;
}
/// <summary>
/// Checks if system restart mode is monitoring time the target machine has power
/// </summary>
/// <param name="">none</param>
/// <returns>true is mode is ON_POWERED_TIME, else false</returns>
bool OilerClass::IsMonitoringTargetPower ( void )
{
	return m_OilerMode == ON_POWERED_TIME;
}
/// <summary>
/// Checks if system restart mode is monitoring how many units of work the target machine has done
/// </summary>
/// <param name="">none</param>
/// <returns>true if mode is ON_TARGET_ACTIVITY, else false</returns>
bool OilerClass::IsMonitoringTargetWork ( void )
{
	return m_OilerMode == ON_TARGET_ACTIVITY;
}
/// <summary>
/// processes a signal that a motor has produced a unit of work e.g. oil drip. Must exceed debounce threshold to be valid
/// <para>if motor has now produced its required number of units it is turned off</para>
/// <para>if this is last of motors to be turned off, then TheOiler state is changed to OFF, any alerts are cleared and system awaits next restart event</para>
/// </summary>
/// <param name="ulLastSignalTime">time in millisends of prior signal</param>
/// <param name="uiMotorIndex">zero based index of motor whose output generated signal</param>
/// <returns>true if signal after debounce period</returns>
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
		if ( m_Motors.MotorInfo [ uiMotorIndex ].uiWorkCount >= m_Motors.MotorInfo[uiMotorIndex].uiWorkTarget )
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
/// <summary>
/// Gets the restart mode of the oiler system if set
/// </summary>
/// <param name="">none</param>
/// <returns>OilerClass::eStartMode</returns>
OilerClass::eStartMode OilerClass::GetStartMode ( void )
{
	return m_OilerMode;
}
/// <summary>
/// Gets the status of the oiler system
/// </summary>
/// <param name="">none</param>
/// <returns>OilerClass::Status</returns>
OilerClass::eStatus OilerClass::GetStatus ( void )
{
	return m_OilerStatus;
}
/// <summary>
/// checks if the machine being oiled has met its oiling restart threshold and restarts oiling if required. Also checks if alert should be generated.
/// </summary>
/// <param name="">none</param>
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

/// <summary>
/// returns time since Oiler went idle in seconds
/// </summary>
/// <param name="">none</param>
/// <returns>number of seconds since all motors stopped running</returns>
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
/// <summary>
/// Gets the number of seconds since the specified motor was last started
/// </summary>
/// <param name="uiMotorIndex">zero based index of motor to be checked</param>
/// <returns>number of seconds</returns>
uint32_t OilerClass::GetTimeSinceMotorStarted ( uint8_t uiMotorIndex )
{
	uint32_t ulResult = 0;
	if ( uiMotorIndex < m_Motors.uiNumMotors )
	{
		ulResult = m_Motors.MotorInfo [ uiMotorIndex ].Motor->GetTimeMotorRunning ();
	}
	return ulResult;
}

/// <summary>
/// called by timer callback when in ON_TIME mode to restart oiler motors if necessary
/// </summary>
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
						m_OilerStatus = OILING;
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
/// <summary>
/// Checks if an Alert state has been reached, if true then sets that state and sets the alert pin if configured
/// </summary>
/// <param name="ulActual">Current value of restart unit count (e.g. seconds, revs )</param>
/// <param name="ulTarget">threshold after which Alert state should be set</param>
void	OilerClass::CheckError ( uint32_t ulActual, uint32_t ulTarget )
{
	if ( m_ulAlertMultiple > 0 )
	{
		if ( ulActual >= ulTarget * m_ulAlertMultiple )
		{
			if ( m_uiAlertPin != NOT_A_PIN )
			{
				digitalWrite ( m_uiAlertPin, m_uiALertOnValue );
			}
			m_bAlert = true;
		}
	}
}
/// <summary>
/// Resets the Alert state and clears the alert pin if configured
/// </summary>
/// <param name="">none</param>
void	OilerClass::ClearError ( void )
{
	if ( m_uiAlertPin != NOT_A_PIN )
	{
		digitalWrite ( m_uiAlertPin, m_uiALertOnValue == HIGH ? LOW : HIGH );
	}
	m_bAlert = false;
}
/// <summary>
/// returns the number of output units e.g. oil drips sent by the specified motor since it last started running
/// </summary>
/// <param name="uiMotorNum">zero based index of motor to be checked</param>
/// <returns>Count of units</returns>
uint16_t OilerClass::GetMotorWorkCount ( uint8_t uiMotorNum )
{
	uint8_t uiResult = 0;
	if ( uiMotorNum < m_Motors.uiNumMotors )
	{
		uiResult = m_Motors.MotorInfo [ uiMotorNum ].uiWorkCount;
	}
	return uiResult;
}
/// <summary>
/// Checks if the specified motor is in a RUNNING state
/// </summary>
/// <param name="uiMotorNum">zero based index of motor to be checked</param>
/// <returns>true if RUNNING, else false</returns>
bool OilerClass::IsMotorRunning ( uint8_t uiMotorNum )
{
	return  GetMotorState (  uiMotorNum ) == MotorClass::RUNNING;
}
/// <summary>
/// Checks state of identified motor
/// </summary>
/// <param name="uiMotorNum">zero based index of motor to be checked</param>
/// <returns>MotorClass:eState of motor, returns MotorClass:STOPPED if invalid index</returns>
MotorClass::eState OilerClass::GetMotorState ( uint8_t uiMotorNum )
{
	MotorClass::eState eResult = MotorClass::STOPPED;
	if ( uiMotorNum < m_Motors.uiNumMotors )
	{
		eResult = m_Motors.MotorInfo [ uiMotorNum ].Motor->GetMotorState ();
	}
	return eResult;
}
/// <summary>
/// Checks if all configured motors are not running
/// </summary>
/// <param name="">none</param>
/// <returns>false if at least one motor is still running, else true</returns>
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
/// <summary>
/// Sets the motor restart mode. The mode determines the event that causes the library to restart the motors oiling
/// </summary>
/// <param name="Mode">This can be:
/// <para>ON_TIME - elapsed time since oiling stopped</para>
/// <para>ON_POWERED_TIME - seconds that machine being oiled has had power since oiling stopped, only valid if AddMachine() has previously been called</para>
/// <para>ON_TARGET_ACTIVITY - number of work units signalled by machine being oiled has sent since oiling stopped, only valid if AddMachine() has previously been called</para>
/// </param>
/// <param name="ulModeTarget">target value in seconds or unit count as relevant</param>
/// <returns>false if AddMachine() not previously called and Mode is ON_POWERED_TIME or ON_TARGET_ACTIVITY, else true</returns>
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

/// <summary>
/// Sets specified motor to move forwards
/// </summary>
/// <param name="uiMotorIndex">zero based index of motor to change</param>
void OilerClass::SetMotorsForward ( uint8_t uiMotorIndex )
{
	m_Motors.MotorInfo [ uiMotorIndex ].Motor->SetDirection ( MotorClass::FORWARD );
}
/// <summary>
///  Sets the minimum time in milliseconds between signals before signal will be considered valid
/// </summary>
/// <param name="uiMotorIndex">zero based index of motor being set</param>
/// <param name="uiDelayms">delay time in milliseconds</param>
/// <returns>true if valid motor index else false</returns>
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
		pinMode ( m_Motors.MotorInfo [ uiMotorIndex ].uiWorkPin, uiMode );
		bResult = true;
	}
	return bResult;
}

/// <summary>
/// Set all motors in forward direction
/// </summary>
/// <param name="">none</param>
void OilerClass::SetMotorsForward ( void )
{
	for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
	{
		SetMotorsForward ( i );
	}
}

/// <summary>
/// Sets specified motor to move backwards
/// </summary>
/// <param name="uiMotorIndex"> zero based index of motor to set</param>
void OilerClass::SetMotorsBackward ( uint8_t uiMotorIndex )
{
	m_Motors.MotorInfo [ uiMotorIndex ].Motor->SetDirection ( MotorClass::BACKWARD );
}


/// <summary>
/// Set all motors in backward direction
/// </summary>
void OilerClass::SetMotorsBackward ( void )
{
	for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
	{
		SetMotorsBackward ( i );
	}
}

OilerClass TheOiler;

