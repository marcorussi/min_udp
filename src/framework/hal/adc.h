/*
 * The MIT License (MIT)
 *
 * Copyright (c) [2015] [Marco Russi]
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

/*
 * This file adc.h represents the header file of the ADC component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


#include "../fw_common.h"


/* Result buffer length */
#define U32_SAMPLES_BUFFER_LENGTH   4


/* Defines of analog channels masks */
#define ADC_CHN_AN0             (0x0001)
#define ADC_CHN_AN1             (0x0002)
#define ADC_CHN_AN2             (0x0004)
#define ADC_CHN_AN3             (0x0008)
#define ADC_CHN_AN4             (0x0010)
#define ADC_CHN_AN5             (0x0020)
#define ADC_CHN_AN6             (0x0040)
#define ADC_CHN_AN7             (0x0080)
#define ADC_CHN_AN8             (0x0100)
#define ADC_CHN_AN9             (0x0200)
#define ADC_CHN_AN10            (0x0400)
#define ADC_CHN_AN11            (0x0800)
#define ADC_CHN_AN12            (0x1000)
#define ADC_CHN_AN13            (0x2000)
#define ADC_CHN_AN14            (0x4000)
#define ADC_CHN_AN15            (0x8000)


/* acquisition mode enum */
typedef enum
{
   ADC_MODE_MANUAL_ACQ,
   ADC_MODE_AUTO_ACQ,
   ADC_MODE_CHECK
} ADC_ke_AcquisitionMode;


/* MANUAL MODE command enum */
typedef enum
{
   ADC_MAN_START_ACQ_CMD,
   ADC_MAN_STOP_ACQ_CMD,
   ADC_MAN_COMMAND_CHECK
} ADC_ke_ManualCommand;


/* ADC samples buffer */
EXTERN uint32 ADC_aui32ResultsBuffer[U32_SAMPLES_BUFFER_LENGTH];


EXTERN void     ADC_Init                ( ADC_ke_AcquisitionMode );

EXTERN void     ADC_PeriodicTask        ( void );

EXTERN void     ADC_StartChnAcq         ( uint16, uint8 );

EXTERN boolean  ADC_CheckIsAcqFinished  ( void );
