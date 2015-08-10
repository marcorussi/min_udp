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
 * This file inch.c represents the source file of the input channels component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


/* Inclusions */
#include "../../fw_common.h"         /* general declarations */
#include "../../hal/adc.h"            /* component ADC header file */
#include "../../hal/port.h"           /* component PORT header file */
#include "../../hal/tmr.h"            /* component TMR header file */

#include "../rtos/rtos.h"           /* component RTOS header file */

#include "inch.h"           /* component INCH header file */


/* Debounce definitions */
/* Debounce period counter value */
#define US_DEB_TIME_COUNTER_VALUE       ((uint16)(INCH_DEB_TIME_VALUE_MS / RTOS_UL_TASKS_PERIOD_MS))
/* Debounce duty cycle counter value */
#define US_STUCK_TIME_COUNTER_VALUE     ((uint16)(INCH_STUCK_TIME_VALUE_MS / RTOS_UL_TASKS_PERIOD_MS))


/* Input channels port */
#define INCH_PORT_ID                    PORT_ID_D
/* Input channels port pin offset */
#define INCH_PORT_PIN_OFFSET            PORT_PIN_6
/* Input channels pin pressed status */
#define INCH_PRESSED_CH_VALUE           PORT_PIN_LOW


/* Local Constants */
/* Debounce buttons register flags masks.
    b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0
    |    |    |    |    |    |    |    |___ ACTUAL_STATUS_FLAG   :1 pressed - 0 released
    |    |    |    |    |    |    |________ TRANSIENT_STATUS_FLAG:1 pressed - 0 released
    |    |    |    |    |    |_____________ VALIDATED_STATUS_FLAG:1 pressed - 0 released
    |    |    |    |    |__________________ RISING_EDGE_FLAG     :1 occurs  - 0 not occurs
    |    |    |    |_______________________ FALLING_EDGE_FLAG    :1 occurs  - 0 not occurs
    |    |    |____________________________ STUCK_COND_FLAG      :1 present - 0 not present
*/
#define ACTUAL_STATUS_FLAG                (0x01)
/* see ACTUAL_STATUS_FLAG*/
#define TRANSIENT_STATUS_FLAG             (0x02)
/* see ACTUAL_STATUS_FLAG*/
#define VALIDATED_STATUS_FLAG             (0x04)
/* see ACTUAL_STATUS_FLAG*/
#define RISING_EDGE_FLAG                  (0x08)
/* see ACTUAL_STATUS_FLAG*/
#define FALLING_EDGE_FLAG                 (0x10)
/* see ACTUAL_STATUS_FLAG*/
#define STUCK_COND_FLAG                   (0x20)


/* Local Macros */
/* Macro to set, clear and check actual status */
/* Macro to set ACTUAL_STATUS_FLAG for each channel*/
#define SET_ACTUAL_STATUS_PRESSED(i)      (stChDebounceArray[(i)].ui8StatusRegister |= ACTUAL_STATUS_FLAG)
/* Macro to Clear ACTUAL_STATUS_FLAG for each channel*/
#define CLEAR_ACTUAL_STATUS_PRESSED(i)    (stChDebounceArray[(i)].ui8StatusRegister &= ~ACTUAL_STATUS_FLAG)
/* Macro to check ACTUAL_STATUS_FLAG for each channel*/
#define CHECK_ACTUAL_STATUS_PRESSED(i)    ((stChDebounceArray[(i)].ui8StatusRegister & ACTUAL_STATUS_FLAG) != UC_NULL)

/* Macro to set, clear and check transient status */
/* Macro to set TRANSIENT_STATUS_FLAG for each channel*/
#define SET_TRANS_STATUS_PRESSED(i)       (stChDebounceArray[(i)].ui8StatusRegister |= TRANSIENT_STATUS_FLAG)
/* Macro to Clear TRANSIENT_STATUS_FLAG for each channel*/
#define CLEAR_TRANS_STATUS_PRESSED(i)     (stChDebounceArray[(i)].ui8StatusRegister &= ~TRANSIENT_STATUS_FLAG)
/* Macro to check TRANSIENT_STATUS_FLAG for each channel*/
#define CHECK_TRANS_STATUS_PRESSED(i)     ((stChDebounceArray[(i)].ui8StatusRegister & TRANSIENT_STATUS_FLAG) != UC_NULL)

/* Macro to set, clear and check validated status */
/* Macro to set VALIDATED_STATUS_FLAG for each channel*/
#define SET_VALID_STATUS_PRESSED(i)       (stChDebounceArray[(i)].ui8StatusRegister |= VALIDATED_STATUS_FLAG)
/* Macro to Clear VALIDATED_STATUS_FLAG for each channel*/
#define CLEAR_VALID_STATUS_PRESSED(i)     (stChDebounceArray[(i)].ui8StatusRegister &= ~VALIDATED_STATUS_FLAG)
/* Macro to check VALIDATED_STATUS_FLAG for each channel*/
#define CHECK_VALID_STATUS_PRESSED(i)     ((stChDebounceArray[(i)].ui8StatusRegister & VALIDATED_STATUS_FLAG) != UC_NULL)

/* Macro to set, clear and check rising edge transition */
/* Macro to set RISING_EDGE_FLAG for each channel*/
#define SET_EDGE_EVENT_RISING(i)          (stChDebounceArray[(i)].ui8StatusRegister |= RISING_EDGE_FLAG)
/* Macro to Clear RISING_EDGE_FLAG for each channel*/
#define CLEAR_EDGE_EVENT_RISING(i)        (stChDebounceArray[(i)].ui8StatusRegister &= ~RISING_EDGE_FLAG)
/* Macro to check RISING_EDGE_FLAG for each channel*/
#define CHECK_EDGE_EVENT_RISING(i)        ((stChDebounceArray[(i)].ui8StatusRegister & RISING_EDGE_FLAG) != UC_NULL)

/* Macro to set, clear and check falling edge transition */
/* Macro to set FALLING_EDGE_FLAG for each channel*/
#define SET_EDGE_EVENT_FALLING(i)         (stChDebounceArray[(i)].ui8StatusRegister |= FALLING_EDGE_FLAG)
/* Macro to Clear FALLING_EDGE_FLAG for each channel*/
#define CLEAR_EDGE_EVENT_FALLING(i)       (stChDebounceArray[(i)].ui8StatusRegister &= ~FALLING_EDGE_FLAG)
/* Macro to check FALLING_EDGE_FLAG for each channel*/
#define CHECK_EDGE_EVENT_FALLING(i)       ((stChDebounceArray[(i)].ui8StatusRegister & FALLING_EDGE_FLAG) != UC_NULL)

/* Macro to set, clear and check stuck condition */
/* Macro to set STUCK_COND_FLAG for each channel*/
#define SET_STUCK_COND_PRESENT(i)         (stChDebounceArray[(i)].ui8StatusRegister |= STUCK_COND_FLAG)
/* Macro to Clear STUCK_COND_FLAG for each channel*/
#define CLEAR_STUCK_COND_PRESENT(i)       (stChDebounceArray[(i)].ui8StatusRegister &= ~STUCK_COND_FLAG)
/* Macro to check STUCK_COND_FLAG for each channel*/
#define CHECK_STUCK_COND_PRESENT(i)       ((stChDebounceArray[(i)].ui8StatusRegister & STUCK_COND_FLAG) != UC_NULL)


/* Local Types */
/* Definition for channels info */
typedef struct
{
   /* Status register flags */
   uint8 ui8StatusRegister;
   /* Debounce counter */
   int8 i8DebounceCounter;
   /* Stuck condition counter */
   uint16 ui16StuckCounter;
} st_buttons_info;


/* Local Variables */
/* Debounce info array */
LOCAL st_buttons_info stChDebounceArray[INCH_KE_CHANNELS_CHECK];

/* Debounce counter value */
LOCAL int8 i8DebounceCntValue;

/* Stuck condition counter value */
LOCAL uint16 ui16StuckCntValue;


/* Local Functions Prototypes */
LOCAL void     applyDebounceAlgo (uint8);


/* Exported Functions */

/* Get channel transition */
EXPORTED INCH_ke_ChannelTrans INCH_GetChannelTransition( INCH_ke_Channels eReqChannel )
{
   INCH_ke_ChannelTrans eChannelTransition;

   /* if rising transition */
   if(CHECK_EDGE_EVENT_RISING((uint8)eReqChannel))
   {
      eChannelTransition = INCH_KE_RISING_EDGE;
      
      /* clear transition event */
      CLEAR_EDGE_EVENT_RISING((uint8)eReqChannel);
   }
   /* if falling transition */
   else if(CHECK_EDGE_EVENT_FALLING((uint8)eReqChannel))
   {
      eChannelTransition = INCH_KE_FALLING_EDGE;

      /* clear transition event */
      CLEAR_EDGE_EVENT_FALLING((uint8)eReqChannel);
   }
   /* if no transition */
   else
   {
      eChannelTransition = INCH_KE_NO_TRANS;
   }

   return eChannelTransition;
}


/* Get channel status */
EXPORTED INCH_ke_ChannelStatus INCH_GetChannelStatus( INCH_ke_Channels eReqChannel )
{
   INCH_ke_ChannelStatus eChannelStatus;

   /* if stuck condition */
   if(CHECK_STUCK_COND_PRESENT((uint8)eReqChannel))
   {
      eChannelStatus = INCH_KE_STATUS_STUCK;
   }
   else
   {
      /* prepare button status to return */
      if(CHECK_VALID_STATUS_PRESSED((uint8)eReqChannel))
      {
         eChannelStatus = INCH_KE_STATUS_PRESSED;
      }
      else
      {
         eChannelStatus = INCH_KE_STATUS_NOT_PRESSED;
      }
   }

   return eChannelStatus;
}


/* Periodic task */
EXPORTED void INCH_PeriodicTask( void )
{
    uint8 ui8ChIndex;
    PORT_ke_PinStatus eChPinStatus;



//    PORT_SetPortPin(PORT_ID_D, PORT_PIN_0);



    /* apply debounce algo at all channels */
    for(ui8ChIndex = INCH_KE_FIRST_CHANNEL; ui8ChIndex <= INCH_KE_LAST_CHANNEL; ui8ChIndex++)
    {
        /* get input channel status */
        eChPinStatus = PORT_ReadPortPin(INCH_PORT_ID, (PORT_ke_PinNumber)(INCH_PORT_PIN_OFFSET + ui8ChIndex));

        /* update actual status */
        if(INCH_PRESSED_CH_VALUE == eChPinStatus)
        {
            SET_ACTUAL_STATUS_PRESSED(ui8ChIndex);
        }
        else
        {
            CLEAR_ACTUAL_STATUS_PRESSED(ui8ChIndex);
        }

        /* manage debounce */
        applyDebounceAlgo(ui8ChIndex);

        /* --------- Manage STUCK condition ----------- */
        /* if validated status is pressed */
        if(CHECK_VALID_STATUS_PRESSED(ui8ChIndex))
        {
            /* check if counter is elapsed */
            if(UC_NULL < stChDebounceArray[ui8ChIndex].ui16StuckCounter)
            {
                /* decrement counter */
                stChDebounceArray[ui8ChIndex].ui16StuckCounter--;
            }
            else
            {
                /* stuck condition is present */
                SET_STUCK_COND_PRESENT(ui8ChIndex);
            }
        }
        else
        {
            /* re-arm stuck condition counter */
            stChDebounceArray[ui8ChIndex].ui16StuckCounter = ui16StuckCntValue;

            /* stuck condition is not present */
            CLEAR_STUCK_COND_PRESENT(ui8ChIndex);
        }
    }
}


/*******************************************************************************
 * Function  : INCH_Init                                                       *
 * Parameters: None                                                            *
 * Return    : None                                                            *
 * Purpose   : Init module                                                     *
 ******************************************************************************/
EXPORTED void INCH_Init( void )
{
    uint8 ui8ChIndex;

    /* update debounce counter value */
    i8DebounceCntValue = US_DEB_TIME_COUNTER_VALUE;

    /* update stuck condition counter value */
    ui16StuckCntValue = US_STUCK_TIME_COUNTER_VALUE;

    /* arm stuck condition counter for all buttons */
    for(ui8ChIndex = INCH_KE_FIRST_CHANNEL; ui8ChIndex <= INCH_KE_LAST_CHANNEL; ui8ChIndex++)
    {
        /* set input channels direction */
        PORT_SetPortPinDirection(INCH_PORT_ID, (PORT_ke_PinNumber)(INCH_PORT_PIN_OFFSET + ui8ChIndex), PORT_DIR_IN);

        /* reset stuck counter value */
        stChDebounceArray[ui8ChIndex].ui16StuckCounter = ui16StuckCntValue;
    }
}




/* Local Functions */

/* apply debounce algo */
LOCAL void applyDebounceAlgo(uint8 ui8ChIndex)
{
   /* check channel index */
   if(ui8ChIndex < INCH_KE_CHANNELS_CHECK)
   {
#if 0
      if(CHECK_ACTUAL_STATUS_PRESSED(ui8ChIndex) ==
         CHECK_VALID_STATUS_PRESSED(ui8ChIndex))
      {
         if(CHECK_ACTUAL_STATUS_PRESSED(ui8ChIndex) ==
            CHECK_TRANS_STATUS_PRESSED(ui8ChIndex))
         {
            /* do nothing */
         }
         else
         {
            /* decrement debounce counter */
            stChDebounceArray[ui8ChIndex].i8DebounceCounter--;

            /* if counter is elapsed */
            if(UC_NULL > stChDebounceArray[ui8ChIndex].i8DebounceCounter)
            {
               /* copy valid status to transient status */
               if(CHECK_VALID_STATUS_PRESSED(ui8ChIndex))
               {
                  SET_TRANS_STATUS_PRESSED(ui8ChIndex);
               }
               else
               {
                  CLEAR_TRANS_STATUS_PRESSED(ui8ChIndex);
               }
               
               /* reset debounce counter */
               stChDebounceArray[ui8ChIndex].i8DebounceCounter = UC_NULL;
            }
            else
            {
               /* do nothing */
            }
         }
      }
      else
      {
#endif
#if 1
         if(CHECK_ACTUAL_STATUS_PRESSED(ui8ChIndex) ==
            CHECK_TRANS_STATUS_PRESSED(ui8ChIndex))
         {
            /* increment debounce counter */
            stChDebounceArray[ui8ChIndex].i8DebounceCounter++;
#if 1
            /* counter >= TH ? */
            if(stChDebounceArray[ui8ChIndex].i8DebounceCounter >= i8DebounceCntValue)
            {
#if 1
               /* if transient value is pressed */
               if(CHECK_TRANS_STATUS_PRESSED(ui8ChIndex))
               {
#if 1
                  /* if validated value is pressed */
                  if(CHECK_VALID_STATUS_PRESSED(ui8ChIndex))
                  {
                     /* do not update validated status: its value is just pressed */
                     /* no transitions occurs */
                  }
                  /* else validated value is not pressed */
                  else
                  {
                     /* update validated status to pressed */
                     SET_VALID_STATUS_PRESSED(ui8ChIndex);

                     /* a falling edge transition occurs */
                     SET_EDGE_EVENT_FALLING(ui8ChIndex);
                     CLEAR_EDGE_EVENT_RISING(ui8ChIndex);
                  }
#endif
               }
               /* else transient value is not pressed */
               else
               {
#if 1
                  /* if validated value is pressed */
                  if(CHECK_VALID_STATUS_PRESSED(ui8ChIndex))
                  {
                     /* update validated status to not pressed */
                     CLEAR_VALID_STATUS_PRESSED(ui8ChIndex);

                     /* a rising edge transition occurs */
                     SET_EDGE_EVENT_RISING(ui8ChIndex);
                     CLEAR_EDGE_EVENT_FALLING(ui8ChIndex);
                  }
                  else
                  {
                     /* do not update validated status: its value is just not pressed */
                     /* no transitions occurs */
                  }
#endif
               }
#endif
               /* reset debounce counter */
               stChDebounceArray[ui8ChIndex].i8DebounceCounter = UC_NULL;
            }
            else
            {
                 /* do nothing */
            }
#endif
         }
         else
         {
            /* copy actual status to transient status */
            if(CHECK_ACTUAL_STATUS_PRESSED(ui8ChIndex))
            {
               SET_TRANS_STATUS_PRESSED(ui8ChIndex);
            }
            else
            {
               CLEAR_TRANS_STATUS_PRESSED(ui8ChIndex);
            }

            /* reset debounce counter */
            stChDebounceArray[ui8ChIndex].i8DebounceCounter = UC_NULL;
         }
#endif
#if 0
      }
#endif
   }
   else
   {
      /* channel index can not be managed */
      /* do nothing */
   }
}


/* END OF FILE inch.c */
