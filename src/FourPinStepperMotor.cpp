//
// FourPinStepperMotor.cpp
//
//	Implementation of 4 pin stepper motor as derivative of Motor. Run in half step mode.
//
// (c) Mark Naylor 2021
//

#include "FourPinStepperMotor.h"
#include "Timer.h"


uint8_t  PhaseSigs [ NUM_PHASES ][ NUM_PINS ] =
{
      { HIGH,  LOW,  LOW,  LOW },   // 0
      { HIGH, HIGH,  LOW,  LOW },   // 1
      {  LOW, HIGH,  LOW,  LOW },   // 2
      {  LOW, HIGH, HIGH,  LOW },   // 3
      {  LOW,  LOW, HIGH,  LOW },   // 4
      {  LOW,  LOW, HIGH, HIGH },   // 5
      {  LOW,  LOW,  LOW, HIGH },   // 6
      { HIGH,  LOW,  LOW, HIGH }    // 7
};


// List of 4 pin stepper motor instances used by timer callback interrupt routine
MOTOR_INSTANCES MotorInstances = { 0, {0,0,0,0,0,0} };

// The following function is called by the timer and is used to check if any 4 pin stepper motors need signals output to move to the next step
void MotorCallback ( void )
{
    for ( uint8_t i = 0; i < MotorInstances.uiCount; i++ )
    {
        MotorInstances.pMotor [ i ]->NextStep ();
    }
}

FourPinStepperMotorClass::FourPinStepperMotorClass ( uint8_t uiPin1, uint8_t uiPin2, uint8_t uiPin3, uint8_t uiPin4, uint8_t uiWorkPin, uint32_t ulWorkThreshold, uint32_t ulDebouncems, uint32_t ulSpeed, uint16_t ulTimeThreshold ) : OilerMotorClass ( uiWorkPin, ulWorkThreshold, ulDebouncems, ulSpeed, ulTimeThreshold )
{
    m_uiPins [ 0 ] = uiPin1;
    m_uiPins [ 1 ] = uiPin2;
    m_uiPins [ 2 ] = uiPin3;
    m_uiPins [ 3 ] = uiPin4;
    m_ulStepInterval = ulSpeed;
    m_uiPhase = 0;
    m_ulLastStepTime = 0;
    m_eState = STOPPED;
    MotorInstances.pMotor [ MotorInstances.uiCount++ ] = this;
    // Set pins to output to driver
    for ( uint8_t uiPin = 0; uiPin < NUM_PINS; uiPin++ )
    {
        pinMode ( m_uiPins [ uiPin ], OUTPUT );
    }
}

/// <summary>
/// Motor specific function to idle motor
/// </summary>
void FourPinStepperMotorClass::Idle ()
{
    // Idle motor, same as power off
    // PowerOff (); - not sure this is necessary, if state is not moving we won't change stepper pins so motor is then effectively idle
}

/// <summary>
/// Motor specific function to Start motor
/// </summary>
void FourPinStepperMotorClass::Start ()
{
    // Start motor - for relay this means switch on
    On ();
}

/// <summary>
/// Motor specific function to power off
/// </summary>
void FourPinStepperMotorClass::PowerOff ()
{
    // power off motor - for 4 pin stepper this means drive all signals LOW
    Off ();
}

void FourPinStepperMotorClass::SetDirection ( eDirection Direction )
{
    MotorClass::SetDirection ( Direction );
}

/// <summary>
/// Turn stepper motor on. energise pins and ensure timer is set up to make next step
/// </summary>
/// <param name="">none</param>
/// <returns>true if new timer callback created</returns>
bool FourPinStepperMotorClass::On ( void )
{
    // Set up callback to increment motor steps
    bool bResult = false;
    if ( !IsMoving() )
    {
        PowerUp ();
        OilerMotorClass::On ();

        bResult = TheTimer.AddCallBack ( MotorCallback, ( m_ulStepInterval / ( 1000000 / RESOLUTION ) + 1 ) );
    }
    return bResult;
}

bool FourPinStepperMotorClass::Off ( void )
{
    for ( uint8_t uiPin = 0; uiPin < NUM_PINS; uiPin++ )
    {
        digitalWrite ( m_uiPins [ uiPin ], LOW );
    }
    m_ulLastStepTime = micros ();
    m_eState = STOPPED;
    
    return OilerMotorClass::Off (); 
}

// decrements phase and resets to NUM_PHASES - 1 when at 0
void FourPinStepperMotorClass::StepCW ( void )
{
    MoveStepper ( ( m_uiPhase + ( NUM_PHASES - 1 ) ) % NUM_PHASES );
}

// increments phase and resets to 0 when > 7
void FourPinStepperMotorClass::StepCCW ( void )
{
    MoveStepper ( ( m_uiPhase + 1 ) % NUM_PHASES );
}

void FourPinStepperMotorClass::MoveStepper ( uint8_t uiPhase )
{
    for ( uint8_t uiPin = 0; uiPin < NUM_PINS; uiPin++ )
    {
        digitalWrite ( m_uiPins [ uiPin ], PhaseSigs [ uiPhase ][ uiPin ] );
    }
    m_uiPhase = uiPhase;
    m_ulLastStepTime = micros ();
    m_ulNextStepTime = m_ulLastStepTime + m_ulStepInterval;
}

// powers pins at current step pin config to get ready for move
void FourPinStepperMotorClass::PowerUp ( void )
{
    MoveStepper ( m_uiPhase );
}

uint32_t FourPinStepperMotorClass::GetNextStepTime ( void )
{
    return m_ulNextStepTime;
}

// send signals for next step if time has elapsed
void FourPinStepperMotorClass::NextStep ( void )
{
    if ( IsMoving() )
    {
        if ( micros () > GetNextStepTime () )
        {
            if ( m_eDir == FORWARD )
            {
                StepCW ();
            }
            else
            {
                StepCCW ();
            }
        }
    }
}

