//
// FourPinStepperMotor.h
//
//	Defines 4 pin stepper motor as derivative of Motor
//
// (c) Mark Naylor 2021
//

#ifndef _4PSTEPPERMOTOR_h
#define _4PSTEPPERMOTOR_h

#include <Arduino.h>
#include "OilerMotor.h"
#include "OilerLib.h"

#define NUM_PINS        4
#define HALF_STEPS      2
#define FULL_STEPS      1
#define STEPPER_MODE    HALF_STEPS
#define NUM_PHASES      ( NUM_PINS * STEPPER_MODE )

class FourPinStepperMotorClass : public OilerMotorClass
{
public:

    FourPinStepperMotorClass ( uint8_t uiPin1, uint8_t uiPin2, uint8_t uiPin3, uint8_t uiPin4, uint8_t uiWorkPin, uint32_t ulWorkThreshold, uint32_t ulDebouncems, uint32_t ulSpeed, uint16_t ulTimeThreshold );
    void            SetDirection ( eDirection Direction );
    void            NextStep ( void );

    // Machine specific implmentation of base virtual pure functions
    void			Idle ();
    void			Start ();
    void			PowerOff ();

    bool            On ( void );
    bool            Off ( void );

protected:
                    uint8_t         m_uiPins [ NUM_PINS ];  // Array of pins used to output signals to stepper driver
    volatile        uint8_t         m_uiPhase;              // The current phase of stepper (in half mode we have 8 phases numbered 0 - 7)
                    uint32_t        m_ulStepInterval;       // the delay time between micros
                    uint32_t        m_ulLastStepTime;       // the last step time in micros
                    uint32_t        m_ulNextStepTime;       // time next step due in micros

    void            StepCW ( void );                        // Move motor 1 step in clockwise direction
    void            StepCCW ( void );                       // Move motor 1 step in conunter clock wise direction
    void            MoveStepper ( uint8_t uiPhase );        // Send stepper signals
    void            PowerUp ( void );                       // powers pins at current step pin config to get ready for move
    uint32_t        GetNextStepTime ( void );

};

// used by interrupt routine to find all instances of motor
typedef struct
{
    uint8_t                     uiCount;
    FourPinStepperMotorClass* pMotor [ MAX_MOTORS ];
} MOTOR_INSTANCES;

#endif
