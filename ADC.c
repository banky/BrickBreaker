/*----------------------------------------------------------------------------
 * Name:    ADC.c
 * Purpose: ADC related functions
 * Note(s): 
 *----------------------------------------------------------------------------*/

#include "LPC17xx.H"                         /* LPC17xx definitions           */
#include "ADC.h"
#include <stdio.h>


/*----------------------------------------------------------------------------
  initialize ADC Pins
 *----------------------------------------------------------------------------*/
void ADC_Init (void) {
	
  LPC_SC->PCONP     |= (1 << 12);            /* enable power to GPIO & IOCON  */

	//See table 81, setting GPIO Port 0.25 to AD0.2 mode
  LPC_PINCON->PINSEL1 &= ~( 0x3 << 18 ); //clear bits
	LPC_PINCON->PINSEL1 |=  ( 0x1 << 18 ); //set bits
	
	LPC_ADC->ADCR = ( 1 <<  2 ) | // Select the second channel
									( 4 <<  8 ) | // ADC clock is 25MHz/(4+1)
									( 0 << 24 ) | // Do not start the conversion yet
									( 1 << 21 );  // Enable ADC

	LPC_ADC->ADINTEN = ( 1 <<  8);  //Enable interrupts for all ADC channels	
}


/*----------------------------------------------------------------------------
  ADC Interrupt Handler Routine
 *----------------------------------------------------------------------------*/
void ADC_IRQHandler( void ) 
{
	// Read ADC Status clears the interrupt condition
	ADCStat = LPC_ADC->ADSTAT;
	ADCValue = (LPC_ADC->ADGDR >> 4) & 0xFFF;
}


/*----------------------------------------------------------------------------
  Start ADC conversion 
 *----------------------------------------------------------------------------*/
/*NOTE: You need to write this funciton if using the ADC */
void ADC_ConverstionStart (void )
{
	LPC_ADC->ADCR |= ( 1<< 24 );
}

