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
 * This file outch.c represents the source file of the output channels component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


//TODO: Modify the timer tick define dependency


/* Inclusions */
#include "../../fw_common.h"             /* general declarations */

#include "../../hal/tmr.h"                /* component TMR header file */
#include "../../hal/pwm.h"                /* component PWM header file */
#include "../../hal/port.h"               /* component PORT header file */

#include "../rtos/rtos.h"               /* component RTOS header file */

#include "outch.h"              /* component header file */


// TODO: decide if blinking period and DC should be dinamically configured or not. They are constant now.



/* Blinking period counter value */
#define US_BLINK_PERIOD_COUNTER_VALUE   ((uint16)((uint32)(OUTCH_BLINK_PERIOD_VALUE_MS * UL_1000) / TMR_UL_TICK_PERIOD_US))
/* Blinking duty cycle counter value */
#define US_BLINK_DC_COUNTER_VALUE       ((uint16)((US_BLINK_PERIOD_COUNTER_VALUE * OUTCH_BLINK_DC_VALUE_PERCENT) / US_100))


/* DIgital pins port */
#define DIGITAL_PINS_PORT               PORT_ID_D


/* Local Constants */
/* Module info flags */
/* b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0
    |    |    |    |    |    |    |    |___ BLINKING_STATUS_FLAG :1 turn ON - 0 turn OFF
    |    |    |    |    |    |    |________ RECOVERY_REQUEST_FLAG:1 required - 0 not required
    |    |    |    |    |    |_____________ RECOVERY_STATUS_FLAG :1 present - 0 not present  */
#define BLINKING_STATUS_FLAG            (0x01)
#define RECOVERY_REQUEST_FLAG           (0x02)
#define RECOVERY_STATUS_FLAG            (0x04)


/* Local Macros */
/* ---------------------- LEDs status request macros ------------------*/
/* Macro to set ON status request */
#define SET_TURN_ON_REQ(i)             (u8StatusReqStateChs |= (UC_1 << (i)))
/* Macro to set OFF status request */
#define SET_TURN_OFF_REQ(i)            (u8StatusReqStateChs &= ~(UC_1 << (i)))
/* Macro to check turn ON request */
#define CHECK_TURN_ON_REQ(i)           ((u8StatusReqStateChs & (UC_1 << (i))) != UC_NULL)
/* Macro to check if all LEDs are in turn OFF request */
#define CHECK_ALL_LEDS_TURN_OFF()      (u8StatusReqStateChs == UC_NULL)

/* ---------------------- Blinking macros ----------------------------*/
/* Macro to set blinking LED request */
#define SET_BLINKING_REQ(i)            (ui8BlinkingReqChs |= (US_1 << (i)))
/* Macro to clear blinking LED request */
#define CLEAR_BLINKING_REQ(i)          (ui8BlinkingReqChs &= ~(US_1 << (i)))
/* Macro to check blinking LED request */
#define CHECK_BLINKING_REQ(i)          ((ui8BlinkingReqChs & (US_1 << (i))) != US_NULL)
/* Macro to check blinking enable */
#define CHECK_BLINKING_ENABLED()       (ui8BlinkingReqChs != US_NULL)
/* Macro to set blinking status ON */
#define SET_BLINKING_STATUS_ON()       (ui8ModuleInfoFlagsReg |= BLINKING_STATUS_FLAG)
/* Macro to set blinking status OFF */
#define SET_BLINKING_STATUS_OFF()      (ui8ModuleInfoFlagsReg &= ~BLINKING_STATUS_FLAG)
/* Macro to check blinking status */
#define CHECK_BLINKING_STATUS_ON()     ((ui8ModuleInfoFlagsReg & BLINKING_STATUS_FLAG) != UC_NULL)

/* ---------------------- Recovery macros ----------------------------*/
/* Macro to set recovery request */
#define SET_RECOVERY_REQUEST()         (ui8ModuleInfoFlagsReg |= RECOVERY_REQUEST_FLAG)
/* Macro to clear recovery request */
#define CLEAR_RECOVERY_REQUEST()       (ui8ModuleInfoFlagsReg &= ~RECOVERY_REQUEST_FLAG)
/* Macro to check recovery request */
#define CHECK_RECOVERY_REQUEST()       ((ui8ModuleInfoFlagsReg & RECOVERY_REQUEST_FLAG) != UC_NULL)
/* Macro to set recovery status */
#define SET_RECOVERY_STATUS()          (ui8ModuleInfoFlagsReg |= RECOVERY_STATUS_FLAG)
/* Macro to clear recovery status */
#define CLEAR_RECOVERY_STATUS()        (ui8ModuleInfoFlagsReg &= ~RECOVERY_STATUS_FLAG)
/* Macro to check recovery status */
#define CHECK_RECOVERY_STATUS()        ((ui8ModuleInfoFlagsReg & RECOVERY_STATUS_FLAG) != UC_NULL)


/* Local Variables */
/* Module info flags register */
LOCAL boolean ui8ModuleInfoFlagsReg;

/* Channels blinking request bits order as ILL_ke_Channels enum: 1 required - 0 not required. 8 channels */
LOCAL uint8 ui8BlinkingReqChs;

/* Channels state to apply bits order as ILL_ke_ChState enum: 1 ON - 0 OFF. 8 channels*/
LOCAL uint8 u8StatusReqStateChs;

/* ------------------ Blinking ----------------- */
/* Blinking period timeout */
LOCAL uint16 ui16BlinkPeriodCounter;

/* Blinking TON timeout */
LOCAL uint16 ui16BlinkTONCounter;

/* Blinking counter */
LOCAL uint16 ui16BlinkingCounter;

/* ------------------ Required outch illumination level ----------------- */
/* Permillage PWM to ill level association array */
LOCAL uint16 aui16IllLevelPWMalues[OUTCH_KE_ILL_LEVEL_CHECK] =
{
    US_200,
    US_400,
    US_500,
    US_600,
    US_1000
};

/* Permillage PWM to apply for each channel. Set an average value as default */
LOCAL uint16 aui16ChannelsPWMalues[OUTCH_KE_ILL_CH_CHECK] =
{
   US_500,
   US_500,
   US_500
};


/*==============================================================================
    Local Functions Prototypes
==============================================================================*/

LOCAL void setChannelOutput     ( OUTCH_ke_Channels );
LOCAL void resetChannelOutput   ( OUTCH_ke_Channels );




/*==============================================================================
    Exported Functions
==============================================================================*/


/* Init ILL module */
EXPORTED void OUTCH_Init(void)
{
    uint8 ui8ChIndex;

    /* init module info flags */
    ui8ModuleInfoFlagsReg = UC_NULL;

    /* init blinking counter */
    ui16BlinkingCounter = US_NULL;

    /* init blinking period counter value */
    ui16BlinkPeriodCounter = US_BLINK_PERIOD_COUNTER_VALUE;
      
    /* init blinking TON counter value */
    ui16BlinkTONCounter = US_BLINK_DC_COUNTER_VALUE;

    /* configure channels as output */
    /* ATTENTION: this enum should be aligned with PWM channels enum and PORT pin number enum */
    for( ui8ChIndex = UC_NULL; ui8ChIndex < OUTCH_KE_CH_CHECK; ui8ChIndex++ )
    {
        /* set port pin direction as output */
        PORT_SetPortPinDirection(PORT_ID_D, (PORT_ke_PinNumber)ui8ChIndex, PORT_DIR_OUT);
        /* clear port pin output */
        PORT_ClearPortPin(PORT_ID_D, PORT_PIN_0);
    }

    /* init PWM channels */
    PWM_Init();
}




/* Set channel status */
EXPORTED void OUTCH_SetChannelStatus(OUTCH_ke_Channels eRequiredCh, OUTCH_ke_ChState eRequiredState)
{
   /* check parameters */
   if((eRequiredCh < OUTCH_KE_CH_CHECK)
   && (eRequiredState < OUTCH_KE_CH_STATE_CHECK))
   {
      /* select status. In event of unknown status LED is turned off */
      switch(eRequiredState)
      {
         /* turn ON */
         case OUTCH_KE_CH_TURN_ON:
         {
            /* blinking is not required */
            CLEAR_BLINKING_REQ((uint8)eRequiredCh);
            /* set to ON */
            SET_TURN_ON_REQ((uint8)eRequiredCh);
            /* set channel immediately */
            setChannelOutput(eRequiredCh);

            break;
         }
         /* blinking */
         case OUTCH_KE_CH_BLINKING:
         {
            /* blinking is required */
            SET_BLINKING_REQ((uint8)eRequiredCh);

            /* blinking status is set synchronously to other blinking LEDs */

            break;
         }
         /* toggle */
         case OUTCH_KE_CH_TOGGLE:
         {
            /* toggle status */
            if(CHECK_TURN_ON_REQ((uint8)eRequiredCh))
            {
                SET_TURN_OFF_REQ((uint8)eRequiredCh);
                /* reset channel immediately */
                resetChannelOutput(eRequiredCh);
            }
            else
            {
                SET_TURN_ON_REQ((uint8)eRequiredCh);
                /* set channel immediately */
                setChannelOutput(eRequiredCh);
            }

            /* ATTENTION: evaluate if it is necessary to update the
             * required channel immediately or at the next task run */

            break;
         }
         /* turn OFF and default */
         case OUTCH_KE_CH_TURN_OFF:
         {
            /* set to OFF */
            SET_TURN_OFF_REQ((uint8)eRequiredCh);

            /* LED is turned off immediately if it is not blinking only.
               If LED is blinking the turn off status shall be updated
               with other blinking LEDs */
            if(CHECK_BLINKING_REQ((uint8)eRequiredCh))
            {
               /* clear blinking request */
               CLEAR_BLINKING_REQ((uint8)eRequiredCh);
            }
            else
            {
                /* reset channel immediately */
                resetChannelOutput(eRequiredCh);
            }

            break;
         }
         default:
         {
            /* invalid status: do nothing */
         }
      }
   }
   else
   {
      /* invalid LED index: do nothing */
   }
}


/* Set illumination level - for ill channels only */
EXPORTED void OUTCH_SetIlluminationLevel(OUTCH_ke_Channels eChannel, OUTCH_ke_IllLevel eIllLevel)
{
    /* check required level */
    if((eIllLevel < OUTCH_KE_ILL_LEVEL_CHECK)
    && (eChannel <= OUTCH_KE_LAST_ILL_CH))
    {
        /* set required PWM value for required channel */
        aui16ChannelsPWMalues[(uint8)eChannel] = aui16IllLevelPWMalues[(uint8)eIllLevel];
    }
    else
    {
        /* do nothing - discard request */
    }
}


/* Manage blinking */
EXPORTED void OUTCH_ManageBlinking(void)
{
    /* if almost one LED is required to blink */
    if(CHECK_BLINKING_ENABLED())
    {
        /* increment blinking counter */
        ui16BlinkingCounter++;

        /* check if TOFF is elapsed. The >= is to avoid uncorrect overflow conditions */
        if(ui16BlinkingCounter >= ui16BlinkPeriodCounter)
        {
            /* TOFF is elapsed: turn ON */
            SET_BLINKING_STATUS_ON();

            /* New period and TON values are constants value.
            Do not update at every period. */

            /* reset blinking counter */
            ui16BlinkingCounter = US_NULL;
        }
        else if(ui16BlinkingCounter == ui16BlinkTONCounter)
        {
            /* TON is elapsed: turn OFF */
            SET_BLINKING_STATUS_OFF();
        }
        else
        {
            /* leave counter to reach TON or TOFF */
        }
    }
    else
    {
        /* reset blinking counter */
        ui16BlinkingCounter = US_NULL;
    }
}




/* ILL periodic task */
EXPORTED void OUTCH_PeriodicTask(void)
{
    uint8 ChIndex_u8;
//    uint16_t supplyVoltageMv_u16;

   /* TODO: recovery */
#if 0
    /* get last acquired supply voltage value from ADC module */
    supplyVoltageMv_u16 = ADC_getAcquiredValue(ADC_KE_SUPPLY_V_CH);

    /* check supply voltage to set or clear recovery request */
    if(supplyVoltageMv_u16 >= SUPPLY_VOLT_TH_H_HIGH_MV)
    {
        /* require recovery */
        SET_RECOVERY_REQUEST();
    }
    else if(supplyVoltageMv_u16 <= SUPPLY_VOLT_TH_H_LOW_MV)
    {
        /* do not require recovery */
        CLEAR_RECOVERY_REQUEST();
    }
    else
    {
        /* hysteresys: do nothing */
    }
#endif

    /* if recovery is required */
    if(CHECK_RECOVERY_REQUEST())
    {
        /* turn OFF all LEDs */
        // TODO - call function to turn off all channels

        /* recovery is present */
        SET_RECOVERY_STATUS();
    }
    else
    {
        /* disable recovery if present */
        if(CHECK_RECOVERY_STATUS())
        {
            /* do not turn OFF all LEDs */
            // TODO - call function to revert from turn off all channels

            /* recovery is not present */
            CLEAR_RECOVERY_STATUS();
        }
        else
        {
        /* do nothing */
        }

        /* toggle blinking channels status if required and apply it */
        for(ChIndex_u8 = OUTCH_KE_FIRST_CH; ChIndex_u8 <= OUTCH_KE_LAST_CH; ChIndex_u8++)
        {
            /* if blink channel required */
            if(CHECK_BLINKING_REQ(ChIndex_u8))
            {
                /* update channel status info and apply it */
                if(CHECK_BLINKING_STATUS_ON())
                {
                    SET_TURN_ON_REQ(ChIndex_u8);
                    /* ATTENTION: this operation should be done after for loop
                     * in order to update all channels simultaneously */
                    /* set channel output */
                    setChannelOutput(ChIndex_u8);
                }
                else
                {
                    SET_TURN_OFF_REQ(ChIndex_u8);
                    /* ATTENTION: this operation should be done after for loop
                     * in order to update all channels simultaneously */
                    /* reset channel output */
                    resetChannelOutput(ChIndex_u8);
                }
            }
        }
    }
}




/*==============================================================================
   Local Functions
==============================================================================*/


/* set a channel output status according to its channel number */
LOCAL void setChannelOutput( OUTCH_ke_Channels eChannelNumber )
{
    /* set channel status */
    if(eChannelNumber <= OUTCH_KE_LAST_ILL_CH)
    {
        /* update related PWM channel */
        PWM_SetDutyCycle((PWM_ke_Channels)eChannelNumber, aui16ChannelsPWMalues[(uint8)eChannelNumber]);
    }
    else
    {
        /* update normal out pin to 1 */
        PORT_SetPortPin(OUTCH_KE_FIRST_DIG_CH, (PORT_ke_PinNumber)(OUTCH_KE_FIRST_DIG_CH + eChannelNumber));
    }
}


/* set a channel output status according to its channel number */
LOCAL void resetChannelOutput( OUTCH_ke_Channels eChannelNumber )
{
    /* reset channel status */
    if(eChannelNumber <= OUTCH_KE_LAST_ILL_CH)
    {
        /* update related PWM channel */
        PWM_SetDutyCycle((PWM_ke_Channels)eChannelNumber, US_NULL);
    }
    else
    {
        /* update normal out pin to 0 */
        PORT_ClearPortPin(OUTCH_KE_FIRST_DIG_CH, (PORT_ke_PinNumber)(OUTCH_KE_FIRST_DIG_CH + eChannelNumber));
    }
}




/* END OF FILE outch.c */
