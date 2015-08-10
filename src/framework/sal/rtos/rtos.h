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
 * This file rtos.h represents the header file of the RTOS component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


#ifndef _RTOS_INCLUDED_          /* Switch around the header file only */
#define _RTOS_INCLUDED_          /* to load once.                      */


/*==============================================================================
    Inclusions
==============================================================================*/
/* This inclusion is for other modules that include this component */
#include "rtos_cfg.h"            /* component config header file */


/*==============================================================================
    Exported Types
==============================================================================*/
/* Callback timers IDs */
typedef enum
{
    RTOS_CB_ID_1,
    RTOS_CB_ID_2,
    RTOS_CB_ID_3,
    RTOS_CB_ID_4,
    RTOS_CB_ID_5,
    RTOS_CB_ID_MAX_NUM,
    RTOS_CB_ID_CHECK = RTOS_CB_ID_MAX_NUM
} RTOS_ke_CallbackID;

/* Callback types */
typedef enum
{
    RTOS_CB_TYPE_SINGLE,
    RTOS_CB_TYPE_PERIODIC,
    RTOS_CB_TYPE_CHECK
} RTOS_ke_CallbackType;


/*==============================================================================
   Exported Defines
==============================================================================*/

/* Tick timer period */
#define RTOS_UL_TICK_PERIOD_US          ((uint32)10000)      /* 10 ms */

/* Tick timer period */
#define RTOS_UL_TASKS_PERIOD_MS         ((uint32)50)        /* 50 ms */

/* Tick periods per second */
#define RTOS_UL_TICK_PER_SEC            ((uint32)(UL_1000000 / RTOS_UL_TICK_PERIOD_US))


/*==============================================================================
   Exported Macros
==============================================================================*/

#define RTOS_TickPerSecond()            (RTOS_UL_TICK_PER_SEC)


/*==============================================================================
    Prototypes
==============================================================================*/
EXTERN void     RTOS_SetCallback            (RTOS_ke_CallbackID, RTOS_ke_CallbackType, uint32, void *);
EXTERN void     RTOS_StopCallback           (RTOS_ke_CallbackID);
EXTERN void     RTOS_TickTimerCallback      (void);
EXTERN void     RTOS_stopOperation          (void);
EXTERN void     RTOS_startOperation         (RTOS_CFG_ke_states);
EXTERN void     RTOS_executeTask            (void);
EXTERN uint32   RTOS_tickCountGet           (void);

EXTERN void RTOS_CallbackTemp ( void );


#endif

/* END OF FILE rtos.h */
