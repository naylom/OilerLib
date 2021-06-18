//
//  PCIHandler.h
// 
// (c) Mark Naylor June 2021
//
//	This class is encapsulates the handling of Pin Change Interrupt (PCI) functionality
//  This code enables users to specify a pin to be monitored using the mcu PCI functionality
//	A pin can be configured along with a requested callback routine. The pin must be identified using an Arduino digital pin number
//
//	NB This is written and tested to work on the Arduino Uno
//
#ifndef _PCIHANDLER_h
#define _PCIHANDLER_h

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include "WProgram.h"
#endif
#define		NUM_PCI_PORTS		3										// number of ports on Atmel chip on arduino Uno board that can generate a PCI
#define		MAX_PCI_PINS		8										// max number of PCI pins allowed to be monitored
typedef void ( *InterruptCallback )( void );

class PCIData
{
public:
	PCIData ( void );
	bool				AddPin ( uint8_t uiDigitalPinNum, InterruptCallback pInterruptFn, uint8_t uiState, uint8_t uiMode = INPUT_PULLUP ); // add pin to be monitored, function to be called if the signal matches mode (RISING, FALLING or  CHANGE), defaults to INPUT_PULLUP
	InterruptCallback	GetCallback ( uint8_t uiPin );

protected:
	bool				IsFull ();
	bool				IsPinPresent ( uint8_t uiPin );
	void				EnablePCI ( uint8_t uiPin );

	static struct PININFO
	{
		uint8_t				uiPinNum;									// Pin being monitored
		uint8_t				uiPinPort;									// mcu Port that pin belongs to
		uint8_t				uiMode;										// mode that (RISING, FALLING or CHANGE) if true invokes callback
		uint8_t				uiLastState;								// HIGH or LOW
		InterruptCallback	pCallBack;									// function to call when pin signals
	} m_PinInfo [ MAX_PCI_PINS ];
	static uint8_t	m_uiPinCount;										// Count of pins being monitored
};

class PCIHandlerClass : public PCIData
{
public:
	PCIHandlerClass ();
	static void	CheckPortPins ( uint8_t uiPortIdGeneratingInterrupt );		// Called when a pin on the provided port signals, checks if one that pin is of interest
	static	void	InvokeCallback ( uint8_t uiChangedPins, uint8_t uiPortIdGeneratingInterrupt );
protected:
	volatile static uint8_t m_PCintLastValues [ NUM_PCI_PORTS ];		// holds the prior PCINT pin values, used to determine when one changes.
};

extern PCIHandlerClass PCIHandler;

#endif

