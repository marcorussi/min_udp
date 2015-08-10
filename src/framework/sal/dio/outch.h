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
 * This file outch.h represents the header file of the output channels component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/

#ifndef _OUTCH_INCLUDED_         /* switch to read the header file only */
#define _OUTCH_INCLUDED_         /* one time */

#include "../../fw_common.h"             /* general declarations */


/* Blinking period in ms */
#define OUTCH_BLINK_PERIOD_VALUE_MS     ((uint16)1000)  /* 1000 ms */
/* Blinking DC in % */
#define OUTCH_BLINK_DC_VALUE_PERCENT    ((uint16)20)    /* 50 % */


/* Exported Types */
/* Channel state enum */
/* ATTENTION: this enum should be aligned with PWM channels enum and PORT pin number enum */
typedef enum
{
    OUTCH_KE_FIRST_CH
   ,OUTCH_KE_CHANNEL_1 = OUTCH_KE_FIRST_CH
   ,OUTCH_KE_FIRST_ILL_CH = OUTCH_KE_CHANNEL_1
   ,OUTCH_KE_ILL_CH_1 = OUTCH_KE_FIRST_ILL_CH
   ,OUTCH_KE_CHANNEL_2
   ,OUTCH_KE_ILL_CH_2 = OUTCH_KE_CHANNEL_2
   ,OUTCH_KE_CHANNEL_3
   ,OUTCH_KE_ILL_CH_3 = OUTCH_KE_CHANNEL_3
   ,OUTCH_KE_LAST_ILL_CH = OUTCH_KE_ILL_CH_3
   ,OUTCH_KE_ILL_CH_CHECK
   ,OUTCH_KE_FIRST_DIG_CH = OUTCH_KE_ILL_CH_CHECK
   ,OUTCH_KE_CHANNEL_4 = OUTCH_KE_FIRST_DIG_CH
   ,OUTCH_KE_CHANNEL_5
   ,OUTCH_KE_LAST_CH = OUTCH_KE_CHANNEL_5
   ,OUTCH_KE_CH_CHECK
} OUTCH_ke_Channels;

/* Channels enum */
typedef enum
{
    OUTCH_KE_FIRST_CH_STATE
   ,OUTCH_KE_CH_TURN_OFF = OUTCH_KE_FIRST_CH_STATE
   ,OUTCH_KE_CH_BLINKING
   ,OUTCH_KE_CH_TOGGLE
   ,OUTCH_KE_CH_TURN_ON
   ,OUTCH_KE_LAST_CH_STATE = OUTCH_KE_CH_TURN_ON
   ,OUTCH_KE_CH_STATE_CHECK
} OUTCH_ke_ChState;


typedef enum
{
    OUTCH_KE_FIRST_ILL_LEVEL,
    OUTCH_KE_ILL_LEVEL_1 = OUTCH_KE_FIRST_ILL_LEVEL,
    OUTCH_KE_ILL_LEVEL_2,
    OUTCH_KE_ILL_LEVEL_3,
    OUTCH_KE_ILL_LEVEL_4,
    OUTCH_KE_ILL_LEVEL_5,
    OUTCH_KE_LAST_ILL_LEVEL = OUTCH_KE_ILL_LEVEL_5,
    OUTCH_KE_ILL_LEVEL_CHECK
} OUTCH_ke_IllLevel;


/*==============================================================================
   Exported Variables
==============================================================================*/


/*==============================================================================
    Exported Functions Prototypes
==============================================================================*/
EXTERN void OUTCH_Init                (void);
EXTERN void OUTCH_SetChannelStatus    (OUTCH_ke_Channels, OUTCH_ke_ChState);
EXTERN void OUTCH_SetIlluminationLevel(OUTCH_ke_Channels, OUTCH_ke_IllLevel);
EXTERN void OUTCH_ManageBlinking      (void);
EXTERN void OUTCH_PeriodicTask        (void);


#endif

/* END OF FILE illum_appl.h */
