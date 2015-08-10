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
 * This file pwm.c represents the source file of the PWM component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


#include <xc.h>
#include <sys/attribs.h>
#include "p32mx795f512l.h"
#include "../fw_common.h"
#include "../sal/sys/sys.h"
#include "pwm.h"

#include "port.h"


/* TEMPORARY DEFINES */
/* Default value of PWM frequency to set at PWM init function */
#define PWM_FREQ_DEFAULT_VALUE_HZ       ((uint32)1000)  /* 1 kHz */

/* PWM timer prescaler ratio. This value is fixed and it depends by timer control register configuration */
#define PWM_TIMER_PRESCALER_RATIO       ((uint32)8)

/* PWM timer period default value */
#define PWM_TIMER_PERIOD_DEF_VALUE      ((uint32)((SYS_UL_FPB / PWM_FREQ_DEFAULT_VALUE_HZ / PWM_TIMER_PRESCALER_RATIO ) - 1))

/* Prescaler bits register position */
#define T2_PRESCALER_BITS_REG_POS       (4)


/* OCx interrupt bits mask macros */
#define OC1E_POS                        6
#define OC1F_POS                        6
#define OC2E_POS                        10
#define OC2F_POS                        10
#define OC3E_POS                        14
#define OC3F_POS                        14
#define OC4E_POS                        18
#define OC4F_POS                        18
#define OCx_PRI_POS                     18
#define OCx_SUBPRI_POS                  16

/* OC1 interrupt macros */
#define ENABLE_OC1_INT()                (IEC0SET = (1 << OC1E_POS))
#define DISABLE_OC1_INT()               (IEC0CLR = (1 << OC1E_POS))
#define CLEAR_OC1_INT_FLAG()            (IFS0CLR = (1 << OC1F_POS))
#define SET_OC1_PRIORITY(x)             (IPC1SET = (x << OCx_PRI_POS))
#define SET_OC1_SUBPRIORITY(x)          (IPC1SET = (x << OCx_SUBPRI_POS))
#define CLEAR_OC1_PRIORITY()            (IPC1CLR = ((7 << OCx_PRI_POS) | (3 << OCx_SUBPRI_POS)))

/* OC2 interrupt macros */
#define ENABLE_OC2_INT()                (IEC0SET = (1 << OC2E_POS))
#define DISABLE_OC2_INT()               (IEC0CLR = (1 << OC2E_POS))
#define CLEAR_OC2_INT_FLAG()            (IFS0CLR = (1 << OC2F_POS))
#define SET_OC2_PRIORITY(x)             (IPC2SET = (x << OCx_PRI_POS))
#define SET_OC2_SUBPRIORITY(x)          (IPC2SET = (x << OCx_SUBPRI_POS))
#define CLEAR_OC2_PRIORITY()            (IPC2CLR = ((7 << OCx_PRI_POS) | (3 << OCx_SUBPRI_POS)))

/* OC3 interrupt macros */
#define ENABLE_OC3_INT()                (IEC0SET = (1 << OC3E_POS))
#define DISABLE_OC3_INT()               (IEC0CLR = (1 << OC3E_POS))
#define CLEAR_OC3_INT_FLAG()            (IFS0CLR = (1 << OC3F_POS))
#define SET_OC3_PRIORITY(x)             (IPC3SET = (x << OCx_PRI_POS))
#define SET_OC3_SUBPRIORITY(x)          (IPC3SET = (x << OCx_SUBPRI_POS))
#define CLEAR_OC3_PRIORITY()            (IPC3CLR = ((7 << OCx_PRI_POS) | (3 << OCx_SUBPRI_POS)))

/* OC4 interrupt macros */
#define ENABLE_OC4_INT()                (IEC0SET = (1 << OC4E_POS))
#define DISABLE_OC4_INT()               (IEC0CLR = (1 << OC4E_POS))
#define CLEAR_OC4_INT_FLAG()            (IFS0CLR = (1 << OC4F_POS))
#define SET_OC4_PRIORITY(x)             (IPC4SET = (x << OCx_PRI_POS))
#define SET_OC4_SUBPRIORITY(x)          (IPC4SET = (x << OCx_SUBPRI_POS))
#define CLEAR_OC4_PRIORITY()            (IPC4CLR = ((7 << OCx_PRI_POS) | (3 << OCx_SUBPRI_POS)))

/* T2 interrupt macros */
#define T2E_POS                         8
#define T2F_POS                         8
#define T2_PRI_POS                      2
#define T2_SUBPRI_POS                   0
#define ENABLE_T2_INT()                 (IEC0SET = (1 << T2E_POS))
#define DISABLE_T2_INT()                (IEC0CLR = (1 << T2E_POS))
#define CLEAR_T2_INT_FLAG()             (IFS0CLR = (1 << T2F_POS))
#define SET_T2_PRIORITY(x)              (IPC2SET = (x << T2_PRI_POS))
#define SET_T2_SUBPRIORITY(x)           (IPC2SET = (x << T2_SUBPRI_POS))
#define CLEAR_T2_PRIORITY()             (IPC2CLR = ((7 << T2_PRI_POS) | (3 << T2_SUBPRI_POS)))


/* variable to store last PWM period */
LOCAL uint32 ui32LastPeriod;


EXPORTED void PWM_Init( void )
{
    /* Turn off OCx while doing setup */
    OC1CON = 0x0000;
    OC2CON = 0x0000;
    OC3CON = 0x0000;
    OC4CON = 0x0000; 

    /* store default period value */
    ui32LastPeriod = PWM_TIMER_PERIOD_DEF_VALUE;

    /* Configure OCx as PWM mode and init DC to 0% */
    OC1R = 0;               
    OC1RS = 0;              
    OC1CON = 0x0006;
    OC2R = 0;
    OC2RS = 0;
    OC2CON = 0x0006;
    OC3R = 0;
    OC3RS = 0;
    OC3CON = 0x0006;
    OC4R = 0;
    OC4RS = 0;
    OC4CON = 0x0006;        

    /* Configure timer 2 for PWM mode */
    T2CON = (3 << T2_PRESCALER_BITS_REG_POS);   /* 1:8 prescale value */
    PR2 = PWM_TIMER_PERIOD_DEF_VALUE;           /* Set default period */

    /* Configure OC1 interrupt */
    CLEAR_OC1_INT_FLAG();           /* Clear the OC1 interrupt flag */
    SET_OC1_PRIORITY(4);            /* Set OC1 interrupt priority to 4 */
    SET_OC1_SUBPRIORITY(3);         /* Set Subpriority to 3, maximum */
//    ENABLE_OC1_INT();               /* Enable OC1 interrupt */

    /* Configure OC2 interrupt */
    CLEAR_OC2_INT_FLAG();           /* Clear the OC2 interrupt flag */
    SET_OC2_PRIORITY(4);            /* Set OC2 interrupt priority to 4 */
    SET_OC2_SUBPRIORITY(2);         /* Set Subpriority to 3, maximum */
//    ENABLE_OC2_INT();               /* Enable OC2 interrupt */

    /* Configure OC3 interrupt */
    CLEAR_OC3_INT_FLAG();           /* Clear the OC3 interrupt flag */
    SET_OC3_PRIORITY(4);            /* Set OC3 interrupt priority to 4 */
    SET_OC3_SUBPRIORITY(1);         /* Set Subpriority to 3, maximum */
//    ENABLE_OC3_INT();               /* Enable OC3 interrupt */

    /* Configure OC4 interrupt */
    CLEAR_OC4_INT_FLAG();           /* Clear the OC4 interrupt flag */
    SET_OC4_PRIORITY(4);            /* Set OC4 interrupt priority to 4 */
    SET_OC4_SUBPRIORITY(0);         /* Set Subpriority to 3, maximum */
//    ENABLE_OC4_INT();               /* Enable OC4 interrupt */

    /* Configure T2 interrupt */
    CLEAR_T2_INT_FLAG();            /* Clear the T2 interrupt flag */
    SET_T2_PRIORITY(2);             /* Set T2 interrupt priority to 2 */
    SET_T2_SUBPRIORITY(3);          /* Set T2 interrupt sub-priority to 3 */
//    ENABLE_T2_INT();                /* Enable T2 interrupt */

    T2CONSET = 0x8000;              /* Enable Timer2 */
    OC1CONSET = 0x8000;             /* Enable OC1 */
    OC2CONSET = 0x8000;             /* Enable OC2 */
    OC3CONSET = 0x8000;             /* Enable OC3 */
    OC4CONSET = 0x8000;             /* Enable OC4 */
}




EXPORTED void PWM_SetFrequency( uint32 ui32RequiredFrequencyHz )
{
    uint32 ui32CalculatedPeriod;

    /* PWM Period = (PR + 1) * T_PB * TMR Prescale Value */
    /* PR = (PWM Period / T_PB / TMR Prescale Value) - 1 */
    /* PR = (PWM Period[s] * f_PB[Hz] / TMR Prescale Value) - 1 */
    /* PR = (f_PB[Hz] / f_PWM[Hz] / TMR Prescale Value) - 1 */

    /* calculate timer period value */
    ui32CalculatedPeriod = (uint32)((SYS_UL_FPB / ui32RequiredFrequencyHz) / PWM_TIMER_PRESCALER_RATIO ) - (uint32)1;

    /* store last updated period */
    ui32LastPeriod = ui32CalculatedPeriod;

    /* update timer period register */
    PR2 = ui32CalculatedPeriod; /* Set default period */
}




EXPORTED void PWM_SetDutyCycle( PWM_ke_Channels eChannel, uint16 ui16DCPerMillage )
{
    uint32 ui32TimerValue;

    /* check parameters */
    if((eChannel < PWM_KE_CHANNEL_CHECK)
    && (ui16DCPerMillage <= US_1000))
    {
        /* PWM Period = (PR + 1) ? T_PB ? TMR Prescale Value */
        /* PR = (PWM Period / T_PB / TMR Prescale Value) - 1 */
        /* PR = (PWM Period[s] * f_PB[Hz] / TMR Prescale Value) - 1 */
        /* PR = (f_PB[Hz] / f_PWM[Hz] / TMR Prescale Value) - 1 */
        /* Calculate timer period value */
        ui32TimerValue = (ui32LastPeriod * ui16DCPerMillage) / (uint32)1000;

        /* IMPORTANT: the duty cycle is update immediatly by OCxR register,
         * To avoid this, update OCxRS only */
        /* Update duty cycle */
        switch(eChannel)
        {
            case PWM_KE_CHANNEL_1:
            {
                OC1R = ui32TimerValue;
                OC1RS = ui32TimerValue;

                break;
            }
            case PWM_KE_CHANNEL_2:
            {
                OC2R = ui32TimerValue;
                OC2RS = ui32TimerValue;

                break;
            }
            case PWM_KE_CHANNEL_3:
            {
                OC3R = ui32TimerValue;
                OC3RS = ui32TimerValue;

                break;
            }
            case PWM_KE_CHANNEL_4:
            {
                OC4R = ui32TimerValue;
                OC4RS = ui32TimerValue;

                break;
            }
            default:
            {
                /* should never arrive here */
            }
        }
    }
    else
    {
        /* do nothing - discard request */
    }
}




/* Interrupt service routine */
void __ISR(_OUTPUT_COMPARE_1_VECTOR, ipl4) OC1_IntHandler (void)
{
    CLEAR_OC1_INT_FLAG();     /* Clear the OC1 interrupt flag */
}


/* Interrupt service routine */
void __ISR(_OUTPUT_COMPARE_2_VECTOR, ipl4) OC2_IntHandler (void)
{
    CLEAR_OC2_INT_FLAG();     /* Clear the OC2 interrupt flag */
}


/* Interrupt service routine */
void __ISR(_OUTPUT_COMPARE_3_VECTOR, ipl4) OC3_IntHandler (void)
{
    CLEAR_OC3_INT_FLAG();     /* Clear the OC3 interrupt flag */
}


/* Interrupt service routine */
void __ISR(_OUTPUT_COMPARE_4_VECTOR, ipl4) OC4_IntHandler (void)
{
    CLEAR_OC4_INT_FLAG();     /* Clear the OC4 interrupt flag */
}


/* Interrupt service routine */
void __ISR(_TIMER_2_VECTOR, ipl2) T2_IntHandler (void)
{
    CLEAR_T2_INT_FLAG();        /* Clear the T2 interrupt flag */
}