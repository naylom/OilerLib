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
	TheOiler.MotorWork ( 0 );
}
/// <summary>
/// Called by interrupt routine handling signal from pin indicating motor 2 has produced a unit of work
/// </summary>
/// <param name="">none</param>
void Motor2WorkSignal ( void )
{
	TheOiler.MotorWork ( 1 );
}
/// <summary>
/// Called by interrupt routine handling signal from pin indicating motor 3 has produced a unit of work
/// </summary>
/// <param name="">none</param>
void Motor3WorkSignal ( void )
{
	TheOiler.MotorWork ( 2 );
}
/// <summary>
/// Called by interrupt routine handling signal from pin indicating motor 4 has produced a unit of work
/// </summary>
/// <param name="">none</param>
void Motor4WorkSignal ( void )
{
	TheOiler.MotorWork ( 3 );
}
/// <summary>
/// Called by interrupt routine handling signal from pin indicating motor 5 has produced a unit of work
/// </summary>
/// <param name="">none</param>
void Motor5WorkSignal ( void )
{
	TheOiler.MotorWork ( 4 );
}
/// <summary>
/// Called by interrupt routine handling signal from pin indicating motor 6 has produced a unit of work
/// </summary>
/// <param name="">none</param>
void Motor6WorkSignal ( void )
{
	TheOiler.MotorWork ( 5 );
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
void OilerTimerCallback ( void )
{
	TheOiler.ProcessTimerEvent ();
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
	m_uiRestartTarget		= TIME_BETWEEN_OILING;		//default value
	m_Motors.uiNumMotors	= 0;
	m_uiAlertPin			= NOT_A_PIN;
	m_ulAlertThreshold		= 0UL;
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
			OilerMotorClass* pMotor = GetOilerMotor ( i );
			if ( !pMotor->IsMoving() )
			{
				pMotor->Action ( OilerMotorClass::eOilerMotorEvents::TURN_ON, GetStartModeUnits() );
			}
		}

		TheTimer.AddCallBack ( OilerTimerCallback, RESOLUTION );		// callback once per sec
		m_OilerStatus = OILING;
		bResult = true;
	}
	return bResult;
}

/// <summary>
/// Invoke motor to process output event and if status has changed check if all motors now off.
/// </summary>
/// <param name="uiMotorIndex"></param>
void OilerClass::MotorWork ( uint8_t uiMotorIndex )
{
	OilerMotorClass* pMotor = GetOilerMotor ( uiMotorIndex );
	if ( pMotor->Action ( OilerMotorClass::eOilerMotorEvents::WORK_SEEN, GetStartModeUnits() ) )
	{
		// get here if state changed

		// check if all motors now idle
		CheckMotors ();
	}
}

/// <summary>
/// turns all motors off
/// </summary>
void OilerClass::Off ()
{
	uint32_t ulModeUnitsNow = GetStartModeUnits ();
	for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
	{
		OilerMotorClass* pMotor = GetOilerMotor ( i );
		pMotor->Action ( OilerMotorClass::eOilerMotorEvents::TURN_OFF, ulModeUnitsNow );
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
	if ( m_Motors.uiNumMotors < MAX_MOTORS )
	{
		// space to add another motor
		m_Motors.MotorInfo [ m_Motors.uiNumMotors ].Motor = new FourPinStepperMotorClass ( uiPin1, uiPin2, uiPin3, uiPin4, uiWorkPin, uiWorkTarget, DEBOUNCE_THRESHOLD, ulSpeed, m_uiRestartTarget );
		if ( m_ulAlertThreshold > 0UL )
		{
			m_Motors.MotorInfo [ m_Motors.uiNumMotors ].Motor->SetAlertThreshold ( m_ulAlertThreshold );
		}
		SetupMotorPins ( uiWorkPin, uiWorkTarget );
		m_Motors.uiNumMotors++;
		bResult = true;
	}
	return bResult;
}
/// <summary>
/// Adds a new relay based motor to oiler.
/// </summary>
/// <param name="uiRelayPin">digital pin to signal to turn on relay and hence motor</param>
/// <param name="uiWorkPin">digital pin that signals when motor has caused a unit of work (eg oil drip) to be produced</param>
/// <param name="uiWorkTarget">number of work units after which motor is turned off</param>
/// <returns>false if number of motors exceeds maximum, else true</returns>
bool OilerClass::AddMotor ( uint8_t uiRelayPin, uint8_t uiWorkPin, uint8_t uiWorkTarget )
{
	bool bResult = false;
	if ( m_Motors.uiNumMotors < MAX_MOTORS )
	{
		// space to add another motor
		m_Motors.MotorInfo [ m_Motors.uiNumMotors ].Motor = new RelayMotorClass ( uiRelayPin, uiWorkPin, uiWorkTarget, DEBOUNCE_THRESHOLD, m_uiRestartTarget );
		if ( m_ulAlertThreshold > 0UL )
		{
			m_Motors.MotorInfo [ m_Motors.uiNumMotors ].Motor->SetAlertThreshold ( m_ulAlertThreshold );
		}
		SetupMotorPins ( uiWorkPin, uiWorkTarget );
		m_Motors.uiNumMotors++;
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
	PCIHandler.AddPin ( uiWorkPin, MotorISRs.MotorWorkCallback [ m_Motors.uiNumMotors ], MOTOR_WORK_SIGNAL_MODE, MOTOR_WORK_SIGNAL_PINMODE );
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
void	OilerClass::SetAlert ( uint8_t uiAlertPin, uint32_t ulAlertThreshold )
{
	m_uiAlertPin = uiAlertPin;
	if ( uiAlertPin != NOT_A_PIN )
	{
		pinMode ( m_uiAlertPin, OUTPUT );
	}
	// no pin just do software error
	ClearError ();
	SetAlertThreshold ( ulAlertThreshold );			// ensure all configured motors know the new alert threshold
}
/// <summary>
/// if changed, updates all oilermotors with new threshold
/// </summary>
/// <param name="uiAlertThreshold">new alert threshold</param>
void OilerClass::SetAlertThreshold ( uint32_t ulAlertThreshold )
{
	if ( ulAlertThreshold != m_ulAlertThreshold )
	{
		m_ulAlertThreshold = ulAlertThreshold;
		for ( uint8_t i = 0; i < m_Motors.uiNumMotors ; i++ )
		{
			GetOilerMotor ( i )->SetAlertThreshold ( ulAlertThreshold );
		}
	}
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
/// checks if all motors are idle and updates status as necessary, called from timer interrupt
/// </summary>
void OilerClass::CheckMotors ()
{
	// have we stopped all motors
	if ( IsOiling () )
	{
		if ( AllMotorsStopped () )
		{
			m_timeOilerStopped = millis ();
			m_OilerStatus = IDLE;
		}
	}
	bool bAlert = false;
	for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
	{
		if ( GetOilerMotor ( i )->IsInError () )
		{
			bAlert = true;
			break;
		}
	}
	if ( bAlert )
	{
		SetError ();
	}
	else
	{
		ClearError ();
	}
}
/// <summary>
/// Gets the restart mode of the oiler system if set
/// </summary>
/// <param name="">none</param>
/// <returns>OilerClass::eStartMode</returns>
inline OilerClass::eStartMode OilerClass::GetStartMode ( void )
{
	return m_OilerMode;
}
/// <summary>
/// Gets the status of the oiler system
/// </summary>
/// <param name="">none</param>
/// <returns>OilerClass::Status</returns>
inline OilerClass::eStatus OilerClass::GetStatus ( void )
{
	return m_OilerStatus;
}
/// <summary>
/// Gets the current value of the units being measured for the current start mode
/// </summary>
/// <param name="">None</param>
/// <returns>units</returns>
uint32_t OilerClass::GetStartModeUnits ( void )
{
	uint32_t ulResult = 0UL;
	switch ( GetStartMode() )
	{
		case eStartMode::ON_TIME:
			ulResult = millis () / 1000;		// in seconds
			break;

		case eStartMode::ON_POWERED_TIME:
			ulResult = m_pMachine->GetActiveTime ();
			break;

		case eStartMode::ON_TARGET_ACTIVITY:
			ulResult = m_pMachine->GetWorkUnits ();
			break;

		default:
			break;
	}
	return ulResult;
}
/// <summary>
/// Gets the number of seconds the coinfigured machine has had power to date
/// </summary>
/// <param name="">None</param>
/// <returns>returns 0 if no machine configured else the number of seconds of powered time</returns>
uint32_t OilerClass::GetMachinePoweredOnTime ( void )
{
	uint32_t ulResult = 0UL;

	if ( m_pMachine )
	{
		ulResult = m_pMachine->GetActiveTime ();
	}
	return ulResult;
}

/// <summary>
/// Returns the number of machine units done to date or 0 if no machine is configured
/// </summary>
/// <param name="">None</param>
/// <returns>number of work units done or 0 if no machine configured</returns>
uint32_t OilerClass::GetMachineUnitCount ( void )
{
	uint32_t ulResult = 0UL;

	if ( m_pMachine )
	{
		ulResult = m_pMachine->GetWorkUnits ();
	}

	return ulResult;
}
/// <summary>
/// Called from periodic time interrupt to see check if motors state need changing
/// </summary>
/// <param name="">None</param>
void OilerClass::ProcessTimerEvent ( void )
{
	// if Oiler is on
	if ( IsOff () == false )
	{
		// get current value of units being measured for this more ie elapsed time, powered time or active work units (eg revs)
		uint32_t ulModeUnits = GetStartModeUnits ();

		// motors configured
		for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
		{
			OilerMotorClass* pMotor = GetOilerMotor ( i );
			if ( pMotor->Action ( OilerMotorClass::eOilerMotorEvents::TIMER, ulModeUnits ) )
			{
				// state changed
				if ( pMotor->IsMoving () )
				{
					m_OilerStatus = OILING;
				}
			}
			// check if in error state
			if ( pMotor->IsInError () )
			{
				SetError ();
			}
		}
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
		ulResult = GetOilerMotor ( uiMotorIndex )->GetTimeMotorRunning ();
	}
	return ulResult;
}

/// <summary>
/// Get pointer to instance of OilerMotorClass
/// </summary>
/// <param name="uiMotorIndex">zero based index of oiler motor class instance</param>
/// <returns>pointer to OilerMotorClass</returns>
inline OilerMotorClass* OilerClass::GetOilerMotor ( uint8_t uiMotorIndex )
{
	return m_Motors.MotorInfo[ uiMotorIndex ].Motor;
}

/// <summary>
/// Set the alert error state and pin if configured
/// </summary>
void OilerClass::SetError ()
{
	if ( m_uiAlertPin != NOT_A_PIN )
	{
		digitalWrite ( m_uiAlertPin, m_uiALertOnValue );
	}
	m_bAlert = true;
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
	uint16_t uiResult = 0;
	if ( uiMotorNum < m_Motors.uiNumMotors )
	{
		uiResult = GetOilerMotor ( uiMotorNum )->GetWorkUnits();
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
	return  GetMotorState (  uiMotorNum ) == OilerMotorClass::MOVING;
}
/// <summary>
/// Checks state of identified motor
/// </summary>
/// <param name="uiMotorNum">zero based index of motor to be checked</param>
/// <returns>MotorClass:eState of motor, returns MotorClass:STOPPED if invalid index</returns>
OilerMotorClass::eOilerMotorState OilerClass::GetMotorState ( uint8_t uiMotorNum )
{
	OilerMotorClass::eOilerMotorState eResult = OilerMotorClass::OFF;
	if ( uiMotorNum < m_Motors.uiNumMotors )
	{
		eResult = GetOilerMotor ( uiMotorNum )->GetOilerMotorState ();
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
		if ( GetOilerMotor ( i )->IsMoving() )
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
bool OilerClass::SetStartMode ( eStartMode Mode, uint16_t uiModeTarget ) 
{
	bool bResult = false;
	switch ( Mode )
	{
		case ON_POWERED_TIME:
		case ON_TARGET_ACTIVITY:
			if ( m_pMachine == NULL )
			{
				break;
			}

		case ON_TIME:
			// ensure all oilermotors have this restart value
			for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
			{
				GetOilerMotor ( i )->SetRestartThreshold ( uiModeTarget );
			}
			m_uiRestartTarget = uiModeTarget;		// save restart valuue
			m_OilerMode = Mode;
			bResult = true;
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
		GetOilerMotor ( uiMotorIndex )->SetDebouncems ( uiDelayms );
		bResult = true;
	}
	return bResult;
}

bool OilerClass::SetStartEventToTargetActiveTime ( uint16_t uiTargetSecs )
{
	return SetStartMode ( ON_POWERED_TIME, uiTargetSecs );
}

bool OilerClass::SetStartEventToTargetWork ( uint16_t uiTargetUnits )
{
	return SetStartMode ( ON_TARGET_ACTIVITY, uiTargetUnits );
}

bool OilerClass::SetStartEventToTime ( uint16_t uiElapsedSecs )
{
	return SetStartMode ( ON_TIME, uiElapsedSecs );
}

bool OilerClass::SetStopTarget ( uint8_t uiWorkTarget, uint8_t uiMotorIndex )
{
	bool bResult = false;
	
	if ( uiWorkTarget > 0 )
	{
		if ( uiMotorIndex == MAX_MOTORS )
		{
			// do all
			for ( uint8_t i = 0; i < m_Motors.uiNumMotors; i++ )
			{
				GetOilerMotor ( i )->SetWorkThreshold ( uiWorkTarget );
			}
		}
		else
		{
			GetOilerMotor ( uiMotorIndex )->SetWorkThreshold ( uiWorkTarget );
		}
		bResult = true;
	}
	return bResult;
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
	GetOilerMotor ( uiMotorIndex )->SetDirection ( MotorClass::BACKWARD );
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

