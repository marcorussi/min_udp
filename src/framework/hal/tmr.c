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
 * This file tmr.c represents the source file of the timer component.
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

#include "tmr.h"
#include "int.h"

#include "../sal/dio/outch.h"
#include "../sal/rtos/rtos.h"




/* Tick timer prescaler ratio. This value is fixed and it depends by timer control register configuration */
#define TICK_TIMER_PRESCALER_RATIO      ((uint32)8)

/* Tick timer period default value */
#define TICK_TIMER_PERIOD_DEF_VALUE     ((uint32)((SYS_UL_FPB / (UL_1000000 / TMR_UL_TICK_PERIOD_US) / TICK_TIMER_PRESCALER_RATIO ) - 1))

/* T1 prescaler bits register position */
#define T1_PRESCALER_BITS_REG_POS       (4)


/* T1 interrupt macros */
#define T1E_POS                         4
#define T1F_POS                         4
#define T1_PRI_POS                      2
#define T1_SUBPRI_POS                   0
#define ENABLE_T1_INT()                 (IEC0SET = (1 << T1E_POS))
#define DISABLE_T1_INT()                (IEC0CLR = (1 << T1E_POS))
#define CLEAR_T1_INT_FLAG()             (IFS0CLR = (1 << T1F_POS))
#define SET_T1_PRIORITY(x)              (IPC1SET = (x << T1_PRI_POS))
#define SET_T1_SUBPRIORITY(x)           (IPC1SET = (x << T1_SUBPRI_POS))
#define CLEAR_T1_PRIORITY()             (IPC1CLR = ((7 << T1_PRI_POS) | (3 << T1_SUBPRI_POS)))




/* Exported functions declaration */

EXPORTED void TMR_TickTimerStart( void )
{
    T1CONCLR = 0x8000;      /* Disable Timer1 */

    /* Configure timer 1 */
    T1CON = (1 << T1_PRESCALER_BITS_REG_POS);   /* 1:8 prescale value */
    PR1 = TICK_TIMER_PERIOD_DEF_VALUE;          /* Set tick timer period */

    /* Configure interrupt */
    CLEAR_T1_INT_FLAG();        /* Clear the T1 interrupt flag */
    SET_T1_PRIORITY(6);         /* Set T1 interrupt priority to 6 */
    SET_T1_SUBPRIORITY(3);      /* Set Subpriority to 3, maximum */
    ENABLE_T1_INT();            /* Enable T1 interrupt */

    T1CONSET = 0x8000;          /* Enable Timer1 */
}


EXPORTED void TMR_TickTimerStop( void )
{
    T1CONCLR = 0x8000;      /* Disable Timer1 */
}


EXPORTED uint16 TMR_getTimerCounter( void )
{
    return TMR1;            /* return Timer1 counter value */
}




/* Local function declaration */

/* Interrupt service routine */
void __ISR(_TIMER_1_VECTOR, ipl6) TickTimer_IntHandler (void)
{
    /* Call RTOS callback function */
    RTOS_TickTimerCallback();

    /* ATTENTION: blinking should be manage by tick timer */
    OUTCH_ManageBlinking();

    CLEAR_T1_INT_FLAG();        /* Clear the T1 interrupt flag */
}
