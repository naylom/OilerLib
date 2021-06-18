//
//  PCIHandler.cpp
// 
// (c) Mark Naylor June 2021
//
//	This class is implements the handling of Pin Change Interrupt functionality, note this will handle all ports that can generate a PCI interrupt and therefore may be incompatible
//  with other libraries / code that attempts to do the same
//

#include "PCIHandler.h"

PCIHandlerClass::PCIHandlerClass ()
{
}

// This static function is called by a PCINT interrupt ISR
void PCIHandlerClass::CheckPortPins ( uint8_t uiPortIdGeneratingInterrupt )
{
	uint8_t uiCurrentPCIReg = *portInputRegister ( uiPortIdGeneratingInterrupt );	// Get state of pins on this port
	// See what pins have changed
	uint8_t uiChangedPins = uiCurrentPCIReg ^ m_PCintLastValues [ uiPortIdGeneratingInterrupt - 2 ];

	// Check if any of these pins relate to one we are montoring
	InvokeCallback ( uiChangedPins, uiPortIdGeneratingInterrupt );
	// Save latest port values
	m_PCintLastValues [ uiPortIdGeneratingInterrupt - 2 ] = uiCurrentPCIReg;

}

void PCIHandlerClass::InvokeCallback ( uint8_t uiChangedPins, uint8_t uiPortIdGeneratingInterrupt )
{
	// check each entry to see if we need to invoke its callback
	for ( uint8_t i = 0; i < PCIData::m_uiPinCount; i++ )
	{
		// if a pin on this port
		if ( m_PinInfo [ i ].uiPinPort == uiPortIdGeneratingInterrupt )
		{
			// if this pin has signalled 
			if ( uiChangedPins & digitalPinToBitMask ( m_PinInfo [ i ].uiPinNum ) )
			{
				// if pin has changed as per required mode
				uint8_t uiCurrentPinState = digitalRead ( m_PinInfo [ i ].uiPinNum );
				switch ( m_PinInfo [ i ].uiMode )
				{
					case RISING:
						if ( m_PinInfo [ i ].uiLastState == LOW && uiCurrentPinState == HIGH )
						{
							m_PinInfo [ i ].pCallBack ();
						}
						break;

					case FALLING:
						if ( m_PinInfo [ i ].uiLastState == HIGH && uiCurrentPinState == LOW )
						{
							m_PinInfo [ i ].pCallBack ();
						}
						break;

					case CHANGE:
						m_PinInfo [ i ].pCallBack ();
						break;

					default:
						// ignore
						break;

				}
				m_PinInfo [ i ].uiLastState = uiCurrentPinState;
			}
		}
	}
}

// Pin Change Interrupt routines, Arduino Uno mcu has 3 ports each handles a different set of pins and each port can generate a unique interrupt for the pins it covers
ISR ( PCINT0_vect )
{
	PCIHandlerClass::CheckPortPins ( 2 );		// Port 2 interrupted
}
ISR ( PCINT1_vect )
{
	PCIHandlerClass::CheckPortPins ( 3 );		// Port 3 interrupted
}
ISR ( PCINT2_vect )
{
	PCIHandlerClass::CheckPortPins ( 4 );		// Port 4 interrupted
}

PCIHandlerClass  PCIHandler;
volatile uint8_t PCIHandlerClass::m_PCintLastValues [ NUM_PCI_PORTS ];
uint8_t	PCIData::m_uiPinCount = 0;
PCIData::PININFO PCIData::m_PinInfo [ MAX_PCI_PINS ];

PCIData::PCIData ( void )
{
	m_uiPinCount = 0;
}

bool PCIData::AddPin ( uint8_t uiDigitalPinNum, InterruptCallback pInterruptFn, uint8_t uiState, uint8_t uiMode )
{
	bool bResult = false;
	if ( !IsPinPresent ( uiDigitalPinNum ) && !IsFull () && ( uiState == FALLING || uiState == RISING || uiState == CHANGE ) )
	{
		m_PinInfo [ m_uiPinCount ].uiPinNum = uiDigitalPinNum;
		m_PinInfo [ m_uiPinCount ].pCallBack = pInterruptFn;
		m_PinInfo [ m_uiPinCount ].uiMode = uiState;
		m_PinInfo [ m_uiPinCount ].uiLastState = digitalRead ( uiDigitalPinNum );
		m_PinInfo [ m_uiPinCount ].uiPinPort = digitalPinToPort ( uiDigitalPinNum );				// NB digitalPinToPort returns 2,3 or 4
		pinMode ( uiDigitalPinNum, uiMode );
		m_uiPinCount++;
		EnablePCI ( uiDigitalPinNum );
		bResult = true;
	}
	return bResult;
}

void PCIData::EnablePCI ( uint8_t uiPin )
{
	// enable PCI interrupts and set pin
	*digitalPinToPCMSK ( uiPin ) |= ( 1 << digitalPinToPCMSKbit ( uiPin ) );
	*digitalPinToPCICR ( uiPin ) |= ( 1 << digitalPinToPCICRbit ( uiPin ) );
}

InterruptCallback PCIData::GetCallback ( uint8_t uiPin )
{
	InterruptCallback pResult = 0;

	for ( uint8_t i = 0; i < m_uiPinCount; i++ )
	{
		if ( m_PinInfo [ i ].uiPinNum == uiPin )
		{
			pResult = m_PinInfo [ i ].pCallBack;
			break;
		}
	}
	return pResult;
}

bool PCIData::IsFull ()
{
	return !( m_uiPinCount < MAX_PCI_PINS );
}

bool PCIData::IsPinPresent ( uint8_t uiPin )
{
	bool bResult = false;

	for ( uint8_t i = 0; i < m_uiPinCount; i++ )
	{
		if ( m_PinInfo [ i ].uiPinNum == uiPin )
		{
			bResult = true;
			break;
		}
	}
	return bResult;;
}
