/*----------------------------------------------------------------------------
 * Name:    ADC.h
 * Purpose: ADC realted definitions
 * Note(s): 
 *--------------------------------------------------------------------------*/
 
 
#ifndef __ADC_H
#define __ADC_H
 
 extern unsigned int ADCStat;
 extern unsigned int ADCValue;
 
 extern void ADC_Init (void);
 extern void ADC_IRQHandler( void );
 extern void ADC_ConversionStart (void );
 
 #endif
 
 