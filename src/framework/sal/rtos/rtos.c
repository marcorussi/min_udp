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
 * This file rtos.c represents the source file of the RTOS component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


// TODO: modify callbacks implementation in the same way of tasks (use flag end manage the counter in the tick int only)
// TODO: state switch function shall return a valid value. Do not check it in the execution task function


/* ------------ Inclusions -------------- */

#include <stddef.h>
#include "../../fw_common.h"    /* common file */

#include "../../hal/tmr.h"      /* component timer header file */

//#include "../../hal/port.h"

#include "rtos_cfg.h"           /* component config header file */
#include "rtos.h"               /* component header file */




/* ----------- Local constants definitions -------------- */

/* Tasks call counter timeout value */
#define UL_TASK_COUNTER_TIMEOUT         ((ulong)((RTOS_UL_TASKS_PERIOD_MS * UL_1000) / RTOS_UL_TICK_PERIOD_US))

/* Max callback time value in ms */
#define U32_CALLBACK_MAX_VALUE_MS       ((uint32)10000)     /* 10 s */

/* First task index */
#define U8_FIRST_TASK_INDEX_VALUE       0




/* ------------- Local typedef definitions ------------- */

/* tick timer enum definition */
typedef enum
{
    KE_TICK_TIMER_NOT_ELAPSED,
    KE_TICK_TIMER_ELAPSED
} KE_TICK_TIMER_STATUS;




/* ------------- Local variables declaration --------------- */

/* store actual RTOS state (from switch_state) */
LOCAL uint8 rtosActualState_u8;

/* store actual tick timer status */
LOCAL KE_TICK_TIMER_STATUS keTickTimerStatus;

/* store counters value of tasks call */
LOCAL uint32 aui32TaskCounters;

/* store expired flag of all callbacks */
LOCAL uint32 abCallbackExpired[RTOS_CB_ID_MAX_NUM] =
{
    B_FALSE,
    B_FALSE,
    B_FALSE,
    B_FALSE,
    B_FALSE
};

/* store enable flag of all callbacks */
LOCAL uint32 abCallbackEnabled[RTOS_CB_ID_MAX_NUM] =
{
    B_FALSE,
    B_FALSE,
    B_FALSE,
    B_FALSE,
    B_FALSE
};

/* store counters value of all callbacks */
LOCAL uint32 aui32CallbackCounters[RTOS_CB_ID_MAX_NUM];

/* store timeout value of all callbacks */
LOCAL uint32 aui32CallbackTimeout[RTOS_CB_ID_MAX_NUM];

/* store callback function pointers */
LOCAL callback_ptr_t apvCallbackFunctions[RTOS_CB_ID_MAX_NUM] =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/* RTOS tick counter */
LOCAL uint32 ui32TickCount = UL_NULL;

/* RTOS tick overflow counter */
LOCAL uint16 ui16TickOverflow = US_NULL;




/* ------------- Local functions prototypes ------------- */

LOCAL uint8 getNewStateToSwitch( uint8 );




/* --------------- Exported functions ---------------- */

/* set and start callback */
EXPORTED void RTOS_SetCallback (RTOS_ke_CallbackID eCallbackID, RTOS_ke_CallbackType eCallbackType, uint32 ui32TimerPeriodMs, void * pCallbackFunction)
{
    /* It is important to update timeout variable first and then counter
     * variable because an eventual tick interrupt can decrement counter value
     * during this operation. Because the timeout value is updated only
     * in periodic mode, this temp variable is necessary */
    uint32 ui32TempCounter;

    if((eCallbackID < RTOS_CB_ID_CHECK)
    && (eCallbackType < RTOS_CB_TYPE_CHECK)
    && (ui32TimerPeriodMs <= U32_CALLBACK_MAX_VALUE_MS)
    && (pCallbackFunction != NULL))
    {
        /* calculate counter value */
        ui32TempCounter = (uint32)((ui32TimerPeriodMs * UL_1000) / RTOS_UL_TICK_PERIOD_US);

        /* if periodic callback request */
        if(RTOS_CB_TYPE_PERIODIC == eCallbackType)
        {
            /* store timeout counter value */
            aui32CallbackTimeout[eCallbackID] = ui32TempCounter;
        }

         /* store counter value */
        aui32CallbackCounters[eCallbackID] = ui32TempCounter;

        /* store callback function pointer */
        apvCallbackFunctions[eCallbackID] = pCallbackFunction;

        /* callback enabled. do it as last operation */
        abCallbackEnabled[eCallbackID] = B_TRUE;
    }
    else
    {
        /* invalid parameters */
    }
}


/* stop callback */
EXPORTED void RTOS_StopCallback (RTOS_ke_CallbackID eCallbackID)
{
    if(eCallbackID < RTOS_CB_ID_CHECK)
    {
        /* callback disabled. do it as first operation */
        abCallbackEnabled[eCallbackID] = B_FALSE;

        /* clear timeout counter value */
        aui32CallbackTimeout[eCallbackID] = UL_NULL;

        /* clear callback counter value */
        aui32CallbackCounters[eCallbackID] = UL_NULL;

        /* clear callback function pointer */
        apvCallbackFunctions[eCallbackID] = NULL_PTR;
    }
    else
    {
        /* invalid parameters */
    }
}


/* Manage RTOS tick timer */
EXPORTED void RTOS_TickTimerCallback( void )
{
    uint8 ui8CallbackIndex;

    ui32TickCount++;

    /* Check for the roll over */
    if( ui32TickCount == UL_NULL )
    {
        /* Indicate the overflow */
        ui16TickOverflow++;
    }

    /* decrement callback counter for each enabled callback */
    for(ui8CallbackIndex = UC_NULL; ui8CallbackIndex < RTOS_CB_ID_CHECK; ui8CallbackIndex++)
    {
        /* manage enabled callbacks only */
        if(abCallbackEnabled[ui8CallbackIndex] == B_TRUE)
        {
            /* if counter is not yet expired */
            if(aui32CallbackCounters[ui8CallbackIndex] > UL_NULL)
            {
                /* decrement counter */
                aui32CallbackCounters[ui8CallbackIndex]--;
            }
            else
            {
                /* set the flag to call the related callback function */
                abCallbackExpired[ui8CallbackIndex] = B_TRUE;

                /* if timeout value is valid than re-arm the counter */
                if(aui32CallbackTimeout[ui8CallbackIndex] > UL_NULL)
                {
                    /* callback is still enabled: re-arm counter */
                    aui32CallbackCounters[ui8CallbackIndex] = aui32CallbackTimeout[ui8CallbackIndex];
                }
                else
                {
                    /* it was a single callback: the callback is disabled now */
                    abCallbackEnabled[ui8CallbackIndex] = B_FALSE;
                }
            }
        }
        /* else do nothing */
    }
    
    /* if tasks counter is elapsed set timeout flag */
    if(aui32TaskCounters > UL_NULL)
    {
        /* decrement tasks call counter */
        aui32TaskCounters--;
    }
    else
    {
        /* re-arm tasks call counter value */
        aui32TaskCounters = UL_TASK_COUNTER_TIMEOUT;

        /* indicate time base over */
        keTickTimerStatus = KE_TICK_TIMER_ELAPSED;
    }
}


/* Stop RTOS operation */
EXPORTED void RTOS_stopOperation( void )
{
    /* Stop tick timer */
    TMR_TickTimerStop();
}


/* Start RTOS operation: select required state if valid and start RTOS timer */
EXPORTED void RTOS_startOperation(RTOS_CFG_ke_states requiredState_e)
{
    /* arm tasks call counter value */
    aui32TaskCounters = UL_TASK_COUNTER_TIMEOUT;

    /* check required state validity */
    if((uint8)requiredState_e < RTOS_CFG_KE_STATE_MAX_NUM)
    {
        /* select the requested RTOS state */
        rtosActualState_u8 = (uint8)requiredState_e;

        /* Start tick timer */
        TMR_TickTimerStart();
    }
    else
    {
        /* invalid required state: do nothing */
    }
}


/* If tick is elapsed then execute all scheduled tasks and select new required state */
EXPORTED void RTOS_executeTask ( void )
{
    uint8 taskIndex_u8;
    uint8 rtosRequiredState_u8;
    uint8 ui8CallbackIndex;

    /* check if RTOS time base is elapsed */
    if(KE_TICK_TIMER_ELAPSED == keTickTimerStatus)
    {
        /* set tick timer not elapsed */
        keTickTimerStatus = KE_TICK_TIMER_NOT_ELAPSED;

        /* execute all tasks in actual selected RTOS state */
        for(taskIndex_u8 = U8_FIRST_TASK_INDEX_VALUE;
           RTOS_CFG_statesArray_at[rtosActualState_u8][taskIndex_u8] != NULL_PTR;
           taskIndex_u8++)
        {
            /* call actual selected task of actual RTOS state */
            (*RTOS_CFG_statesArray_at[rtosActualState_u8][taskIndex_u8])();
        }

        /* load new system state */
        rtosRequiredState_u8 = getNewStateToSwitch(rtosActualState_u8);

        /* new system state supported? */
        if(rtosRequiredState_u8 < RTOS_CFG_KE_STATE_MAX_NUM)
        {
            /* enter new system state */
            rtosActualState_u8 = rtosRequiredState_u8;
        }
        else
        {
            /* remain in actual system state */
        }
    }
    else
    {
        /* do nothing */
    }

    /* manage callback functions */
    for(ui8CallbackIndex = 0; ui8CallbackIndex < RTOS_CB_ID_CHECK; ui8CallbackIndex++)
    {
        /* if callback counter is expired  */
        if(abCallbackExpired[ui8CallbackIndex] == B_TRUE)
        {
            /* call callback function if valid pointer (so, single callbacks are called once) */
            if(apvCallbackFunctions[ui8CallbackIndex] != NULL_PTR)
            {
                /* call callback function */
                (*apvCallbackFunctions[ui8CallbackIndex])();

                /* after function call clear the function pointer if the callback is now disabled */
                if(abCallbackEnabled[ui8CallbackIndex] == B_FALSE)
                {
                    /* clear function pointer: stop callback */
                    apvCallbackFunctions[ui8CallbackIndex] = NULL_PTR;
                }
            }
            /* else callback is disabled */

            /* clear expired flag */
            abCallbackExpired[ui8CallbackIndex] = B_FALSE;
        }
        /* else leave it to expire */
    }
}


/* Get RTOS tick counter */
EXPORTED uint32 RTOS_tickCountGet ( void )
{
    uint32 tickCount;
    uint32 CurTmrVal;

    /* Get the current TMR value */
    CurTmrVal = (uint32)TMR_getTimerCounter();

    /* Calculate the tick count in us */
    tickCount = ( ( ui32TickCount * RTOS_UL_TICK_PERIOD_US ) +
                  ( CurTmrVal % RTOS_UL_TICK_PERIOD_US ) );

    /* Convert the tick count in ms */
    tickCount /= UL_1000;

    /* Returns the alarm count value */
    return ( tickCount );

}




/* -------------- Local functions implementation ----------------- */

/* This function determines the next RTOS mode */
LOCAL uint8 getNewStateToSwitch( uint8 actualState_u8 )
{
   uint8 requiredStateToReturn_u8;

   /* switch to system state */
   switch(actualState_u8)
   {
      /* RTOS_CFG_KE_INIT_STATE state? */
      case RTOS_CFG_KE_INIT_STATE:
      {
         /* enter RTOS_CFG_KE_NORMAL_STATE state */
        requiredStateToReturn_u8 = RTOS_CFG_KE_NORMAL_STATE;

         break;
      }
      /* RTOS_CFG_KE_NORMAL_STATE state? */
      case RTOS_CFG_KE_NORMAL_STATE:
      {
         /* remain in RTOS_CFG_KE_NORMAL_STATE state */
         requiredStateToReturn_u8 = RTOS_CFG_KE_NORMAL_STATE;

         break;
      }
      /* RTOS_CFG_KE_SLEEP_STATE state? */
      case RTOS_CFG_KE_SLEEP_STATE:
      {
         
         break;
      }
      /* default */
      default:
      {
         
         break;
      }
   }

   /*return new determined system state*/
   return requiredStateToReturn_u8;
}




/* End of file */



