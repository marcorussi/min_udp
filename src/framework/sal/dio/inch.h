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
 * This file inch.h represents the header file of the input channels component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


#ifndef _INCH_INCLUDED_         /* switch to read the header file only */
#define _INCH_INCLUDED_         /* one time */


/*==============================================================================
    Inclusions
==============================================================================*/
#include "../../fw_common.h"             /* general declarations */


/* Debounce time value in ms */
#define INCH_DEB_TIME_VALUE_MS              (20)    /* 20 ms */
/* Stuck time value in ms */
#define INCH_STUCK_TIME_VALUE_MS            (5000)  /* 5 s */


/*==============================================================================
    Exported Types
==============================================================================*/
/* Input channel transitions enum */
typedef enum
{
    INCH_KE_FIRST_TRANS
   ,INCH_KE_NO_TRANS = INCH_KE_FIRST_TRANS
   ,INCH_KE_RISING_EDGE
   ,INCH_KE_FALLING_EDGE
   ,INCH_KE_LAST_TRANS = INCH_KE_FALLING_EDGE
   ,INCH_KE_TRANS_CHECK
} INCH_ke_ChannelTrans;

/* Input channel status enum */
typedef enum
{
    INCH_KE_FIRST_STATUS
   ,INCH_KE_STATUS_NOT_PRESSED = INCH_KE_FIRST_STATUS
   ,INCH_KE_STATUS_PRESSED
   ,INCH_KE_STATUS_STUCK
   ,INCH_KE_LAST_STATUS = INCH_KE_STATUS_STUCK
   ,INCH_KE_STATUS_CHECK
} INCH_ke_ChannelStatus;

/* Buttons enum.
 * ATTENTION: this enum shall be order aligned as ADC_ke_adcChannel enum,
 * the offset value is defined in buttons_appl.c */
typedef enum
{
    INCH_KE_FIRST_CHANNEL
   ,INCH_KE_CHANNEL_1 = INCH_KE_FIRST_CHANNEL
   ,INCH_KE_CHANNEL_2
   ,INCH_KE_CHANNEL_3
   ,INCH_KE_CHANNEL_4
   ,INCH_KE_LAST_CHANNEL = INCH_KE_CHANNEL_4
   ,INCH_KE_CHANNELS_CHECK
} INCH_ke_Channels;


/*==============================================================================
    Exported Functions Prototypes
==============================================================================*/

EXTERN void                     INCH_Init                   ( void );
EXTERN void                     INCH_PeriodicTask           ( void );
EXTERN INCH_ke_ChannelTrans     INCH_GetChannelTransition   ( INCH_ke_Channels );
EXTERN INCH_ke_ChannelStatus    INCH_GetChannelStatus       ( INCH_ke_Channels );


#endif


/* END OF FILE buttons_appl.h */
