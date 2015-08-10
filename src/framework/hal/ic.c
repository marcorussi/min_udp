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
 * This file ic.c represents the source file of the IC component.
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

#include "ic.h"


// TODO: The three ICs runs togheter always. Evaluate if it is necessary to separate each IC run.
//       Modify local functions in INLINE


/* Results buffer length */
#define U8_IC_RES_BUFFER_LENGTH         3   /* Rising - falling - rising */


/* IC TIMER DEFINES */
/* IC timer prescaler ratio. This value is fixed and it depends by timer control register configuration */
#define IC_TIMER_PRESCALER_RATIO        ((uint32)256)

/* IC timer source fequency value */
#define IC_TIMER_FREQUENCY_VALUE        ((uint32)((SYS_UL_FPB / IC_TIMER_PRESCALER_RATIO ) - 1))

/* Prescaler bits register position */
#define T3_PRESCALER_BITS_REG_POS       (4)


/* ICxCON bits configuration */
#define ICxCON_ON               0   /* disable module */
#define ICxCON_FRZ              0   /* Do not freeze module operation when in Debug mode */
#define ICxCON_SIDL             0   /* Continue to operate in CPU Idle mode */
#define ICxCON_ICFEDGE          1   /* Capture rising edge first */
#define ICxCON_ICC32            0   /* 16-bit timer resource capture */
#define ICxCON_ICTMR            0   /* Timer3 is the counter source for capture */
#define ICxCON_ICI              2   /* Interrupt on every third capture event */
#define ICxCON_ICOV             0   /* Overflow Status Flag bit (read-only) */
#define ICxCON_ICBNE            0   /* Buffer Not Empty Status bit (read-only) */
#define ICxCON_ICM              6   /* Simple Capture Event mode - every edge, specified edge first and every edge thereafter */

/* ICxCON bits position mask */
#define ICxCON_ON_POS           15
#define ICxCON_FRZ_POS          14
#define ICxCON_SIDL_POS         13
#define ICxCON_ICFEDGE_POS      9
#define ICxCON_ICC32_POS        8
#define ICxCON_ICTMR_POS        7
#define ICxCON_ICI_POS          5
#define ICxCON_ICOV_POS         4
#define ICxCON_ICBNE_POS        3
#define ICxCON_ICM_POS          0

/* IC1CON register value */
#define ICxCON1_ON_REG_VALUE   ((ICxCON_ON << ICxCON_ON_POS) |              \
                                (ICxCON_FRZ << ICxCON_ICBNE_POS) |          \
                                (ICxCON_SIDL << ICxCON_ICOV_POS) |          \
                                (ICxCON_ICFEDGE << ICxCON_ICFEDGE_POS) |    \
                                (ICxCON_ICC32 << ICxCON_ICC32_POS) |        \
                                (ICxCON_ICTMR << ICxCON_ICTMR_POS) |        \
                                (ICxCON_ICI << ICxCON_ICI_POS) |            \
                                (ICxCON_ICOV << ICxCON_ICOV_POS) |          \
                                (ICxCON_ICBNE << ICxCON_ICBNE_POS) |        \
                                (ICxCON_ICM << ICxCON_ICM_POS)              )

/* Macros to enable and disable ICx */
#define IC1_TURN_ON()           (IC1CONSET = (1 << ICxCON_ON_POS))
#define IC1_TURN_OFF()          (IC1CONCLR = (1 << ICxCON_ON_POS))
#define IC2_TURN_ON()           (IC2CONSET = (1 << ICxCON_ON_POS))
#define IC2_TURN_OFF()          (IC2CONCLR = (1 << ICxCON_ON_POS))
#define IC3_TURN_ON()           (IC3CONSET = (1 << ICxCON_ON_POS))
#define IC3_TURN_OFF()          (IC3CONCLR = (1 << ICxCON_ON_POS))


/* Macro to check ICx buffer empty */
#define IS_IC1_BUFFER_NOT_EMPTY()       ((IC1CON & (1 << ICxCON_ICBNE_POS)) > 0)
#define IS_IC2_BUFFER_NOT_EMPTY()       ((IC2CON & (1 << ICxCON_ICBNE_POS)) > 0)
#define IS_IC3_BUFFER_NOT_EMPTY()       ((IC3CON & (1 << ICxCON_ICBNE_POS)) > 0)


/* IC1 interrupt macros */
#define IC1E_POS                        5
#define IC1F_POS                        5
#define IC2E_POS                        9
#define IC2F_POS                        9
#define IC3E_POS                        13
#define IC3F_POS                        13
#define ICx_PRI_POS                     10
#define ICx_SUBPRI_POS                  8

/* IC1 interrupts macros */
#define ENABLE_IC1_INT()                (IEC0SET = (1 << IC1E_POS))
#define DISABLE_IC1_INT()               (IEC0CLR = (1 << IC1E_POS))
#define CLEAR_IC1_INT_FLAG()            (IFS0CLR = (1 << IC1F_POS))
#define SET_IC1_PRIORITY(x)             (IPC1SET = (x << ICx_PRI_POS))
#define SET_IC1_SUBPRIORITY(x)          (IPC1SET = (x << ICx_SUBPRI_POS))
#define CLEAR_IC1_PRIORITY()            (IPC1CLR = ((7 << ICx_PRI_POS) | (3 << ICx_SUBPRI_POS)))

/* IC2 interrupts macros */
#define ENABLE_IC2_INT()                (IEC0SET = (1 << IC2E_POS))
#define DISABLE_IC2_INT()               (IEC0CLR = (1 << IC2E_POS))
#define CLEAR_IC2_INT_FLAG()            (IFS0CLR = (1 << IC2F_POS))
#define SET_IC2_PRIORITY(x)             (IPC2SET = (x << ICx_PRI_POS))
#define SET_IC2_SUBPRIORITY(x)          (IPC2SET = (x << ICx_SUBPRI_POS))
#define CLEAR_IC2_PRIORITY()            (IPC2CLR = ((7 << ICx_PRI_POS) | (3 << ICx_SUBPRI_POS)))

/* IC3 interrupts macros */
#define ENABLE_IC3_INT()                (IEC0SET = (1 << IC3E_POS))
#define DISABLE_IC3_INT()               (IEC0CLR = (1 << IC3E_POS))
#define CLEAR_IC3_INT_FLAG()            (IFS0CLR = (1 << IC3F_POS))
#define SET_IC3_PRIORITY(x)             (IPC3SET = (x << ICx_PRI_POS))
#define SET_IC3_SUBPRIORITY(x)          (IPC3SET = (x << ICx_SUBPRI_POS))
#define CLEAR_IC3_PRIORITY()            (IPC3CLR = ((7 << ICx_PRI_POS) | (3 << ICx_SUBPRI_POS)))


/* Macros to start and stop timer */
#define START_TIMER()                   (T3CONSET = 0x8000)
#define STOP_TIMER()                    (T3CONCLR = 0x8000)


/* task states enum */
typedef enum
{
    KE_IC_IDLE,
    KE_IC_CAP_IN_PROG,
    KE_IC_CALCULATE
} ke_ICTaskState;


/* ICx results buffer */
LOCAL uint16 aui16ICResultsBuffer[IC_KE_CHANNEL_CHECK][U8_IC_RES_BUFFER_LENGTH];

/* ICx task state */
LOCAL ke_ICTaskState eICTaskState[IC_KE_CHANNEL_CHECK];

/* required run commands for each IC */
LOCAL boolean bReqRunCommand[IC_KE_CHANNEL_CHECK];

/* calculated frequency value in Hz for each IC */
LOCAL uint32 ui32CalcFrequencyHz[IC_KE_CHANNEL_CHECK];

/* calculated duty cycle value in permillage for each channel */
LOCAL uint16 ui16DCPermillage[IC_KE_CHANNEL_CHECK];




/* local functions prototypes */
LOCAL void calculateFreqAndDC   ( uint8 );
LOCAL void flushBuffer          ( uint8 );
LOCAL void turnON               ( uint8 );
LOCAL void turnOFF              ( uint8 );
LOCAL void clearIntFlag         ( uint8 );




/* IC init function */
EXPORTED void IC_Init( void )
{
    DISABLE_IC1_INT();      /* disable interrupt */
    CLEAR_IC1_PRIORITY();   /* clear interrupt priority */

    IC1_TURN_OFF();         /* stop IC1 */
    IC2_TURN_OFF();         /* stop IC2 */
    IC3_TURN_OFF();         /* stop IC3 */

    /* empty buffers */
    flushBuffer(0);
    flushBuffer(1);
    flushBuffer(2);

    /* init task states */
    eICTaskState[0] = KE_IC_IDLE;
    eICTaskState[1] = KE_IC_IDLE;
    eICTaskState[2] = KE_IC_IDLE;

    /* set ICxCON registers */
    IC1CON = ICxCON1_ON_REG_VALUE;
    IC2CON = ICxCON1_ON_REG_VALUE;
    IC3CON = ICxCON1_ON_REG_VALUE;

    /* Configure timer 3 for IC - prescaler only */
    T3CON = (7 << T3_PRESCALER_BITS_REG_POS);       /* 1:256 prescale value */

    CLEAR_IC1_INT_FLAG();   /* clear interrupt flag */
    SET_IC1_PRIORITY(4);    /* set interrupt priority to 4 */
    SET_IC1_SUBPRIORITY(1); /* set interrupt sub-priority to 1 */
    ENABLE_IC1_INT();       /* enable interrupt */

    CLEAR_IC2_INT_FLAG();   /* clear interrupt flag */
    SET_IC2_PRIORITY(4);    /* set interrupt priority to 4 */
    SET_IC2_SUBPRIORITY(1); /* set interrupt sub-priority to 1 */
    ENABLE_IC2_INT();       /* enable interrupt */

    CLEAR_IC3_INT_FLAG();   /* clear interrupt flag */
    SET_IC3_PRIORITY(4);    /* set interrupt priority to 4 */
    SET_IC3_SUBPRIORITY(1); /* set interrupt sub-priority to 1 */
    ENABLE_IC3_INT();       /* enable interrupt */
}


/* Get calculated values function */
EXPORTED boolean IC_GetICFreqAndDC(IC_ke_Channels eChannel, uint32 * pui32FrequencyHz, uint16 * pui16DCPermillage)
{
    boolean bValidValues;

    /* return valid data if calculated */
    if(KE_IC_CALCULATE == eICTaskState[(uint8)eChannel])
    {
        bValidValues = B_TRUE;

        /* copy values */
        *pui32FrequencyHz = ui32CalcFrequencyHz[(uint8)eChannel];
        *pui16DCPermillage = ui16DCPermillage[(uint8)eChannel];

        /* go in KE_IC_IDLE state */
        eICTaskState[(uint8)eChannel] = KE_IC_IDLE;
    }
    else
    {
        bValidValues = B_FALSE;
    }

    return bValidValues;
}


/* IC run command function */
EXPORTED void IC_RunInputCapture(IC_ke_Channels eChannel, boolean bRunCommand )
{
    /* copy required run command */
    bReqRunCommand[(uint8)eChannel] = bRunCommand;
}


/* IC periodic task function */
EXPORTED void IC_PeriodicTask( void )
{
    uint8 ui8ICIndex;

    /* manage state of each IC */
    for(ui8ICIndex = IC_KE_FIRST_CHANNEL; ui8ICIndex < IC_KE_FIRST_CHANNEL; ui8ICIndex++)
    {
        /* manage IC state */
        switch(eICTaskState[ui8ICIndex])
        {
            case KE_IC_IDLE:
            {
                /* if run is required */
                if(B_TRUE == bReqRunCommand[ui8ICIndex])
                {
                    /* start IC timer */
                    START_TIMER();
                    /* start IC */
                    turnON(ui8ICIndex);

                    /* go in KE_IC_CAP_IN_PROG state */
                    eICTaskState[ui8ICIndex] = KE_IC_CAP_IN_PROG;
                }
                else
                {
                    /* remain in this state */
                }

                break;
            }
            case KE_IC_CAP_IN_PROG:
            {
                /* if stop is required */
                if(B_FALSE == bReqRunCommand[ui8ICIndex])
                {
                    /* stop IC timer */
                    STOP_TIMER();
                    /* stop IC */
                    turnOFF(ui8ICIndex);

                    /* empty buffer */
                    flushBuffer(ui8ICIndex);

                    /* clear an eventual not served IC1 interrupt flag */
                    clearIntFlag(ui8ICIndex);
                }
                else
                {
                    /* remain in this state - wait */
                }

                break;
            }
            case KE_IC_CALCULATE:
            {
                /* calculate freq and DC */
                calculateFreqAndDC(ui8ICIndex);

                /* get values function call will change state */

                break;
            }
            default:
            {

            }
        }
    }
}


/* turn ON */
LOCAL void turnON( uint8 ui8ICIndex )
{
    switch(ui8ICIndex)
    {
        case IC_KE_CHANNEL_1:
        {
            IC1_TURN_ON();
            break;
        }
        case IC_KE_CHANNEL_2:
        {
            IC2_TURN_ON();
            break;
        }
        case IC_KE_CHANNEL_3:
        {
            IC3_TURN_ON();
            break;
        }
        default:
        {
            break;
        }
    }
}


/* turn OFF */
LOCAL void turnOFF( uint8 ui8ICIndex )
{
    switch(ui8ICIndex)
    {
        case IC_KE_CHANNEL_1:
        {
            IC1_TURN_OFF();
            break;
        }
        case IC_KE_CHANNEL_2:
        {
            IC2_TURN_OFF();
            break;
        }
        case IC_KE_CHANNEL_3:
        {
            IC3_TURN_OFF();
            break;
        }
        default:
        {
            break;
        }
    }
}


/* clear interrupt flag */
LOCAL void clearIntFlag( uint8 ui8ICIndex )
{
    switch(ui8ICIndex)
    {
        case IC_KE_CHANNEL_1:
        {
            CLEAR_IC1_INT_FLAG();
            break;
        }
        case IC_KE_CHANNEL_2:
        {
            CLEAR_IC2_INT_FLAG();
            break;
        }
        case IC_KE_CHANNEL_3:
        {
            CLEAR_IC3_INT_FLAG();
            break;
        }
        default:
        {
            break;
        }
    }
}


/* calculate freq and DC */
LOCAL void calculateFreqAndDC( uint8 ui8ICIndex )
{
    /* calculate frequency */
    ui32CalcFrequencyHz[ui8ICIndex] = (uint32)(IC_TIMER_FREQUENCY_VALUE / (aui16ICResultsBuffer[ui8ICIndex][2] - aui16ICResultsBuffer[ui8ICIndex][0]));

    /* calculate duty cycle */
    ui16DCPermillage[ui8ICIndex] = (uint16)((aui16ICResultsBuffer[ui8ICIndex][1] - aui16ICResultsBuffer[ui8ICIndex][0]) / (aui16ICResultsBuffer[ui8ICIndex][2] - aui16ICResultsBuffer[ui8ICIndex][0]));
}


/* flush buffer */
LOCAL void flushBuffer( uint8 ui8ICIndex )
{
    uint16 ui16Dummy;

    switch(ui8ICIndex)
    {
        case IC_KE_CHANNEL_1:
        {
            /* read data up to buffer is empty */
            while(IS_IC1_BUFFER_NOT_EMPTY())
            {
                ui16Dummy = IC1BUF;
            }
            break;
        }
        case IC_KE_CHANNEL_2:
        {
            /* read data up to buffer is empty */
            while(IS_IC2_BUFFER_NOT_EMPTY())
            {
                ui16Dummy = IC2BUF;
            }
            break;
        }
        case IC_KE_CHANNEL_3:
        {
            /* read data up to buffer is empty */
            while(IS_IC3_BUFFER_NOT_EMPTY())
            {
                ui16Dummy = IC3BUF;
            }
            break;
        }
        default:
        {
            break;
        }
    }
}


/* IC1 Interrupt service routine */
void __ISR(_INPUT_CAPTURE_1_VECTOR, ipl4) IC1_IntHandler (void)
{
    /* copy expected results */
    aui16ICResultsBuffer[0][0] = IC1BUF;
    aui16ICResultsBuffer[0][1] = IC1BUF;
    aui16ICResultsBuffer[0][2] = IC1BUF;

    /* empty buffer */
    flushBuffer((uint8)IC_KE_CHANNEL_1);

    /* go in KE_IC_CALCULATE state */
    eICTaskState[(uint8)IC_KE_CHANNEL_1] = KE_IC_CALCULATE;

    CLEAR_IC1_INT_FLAG();   /* Clear the IC1 interrupt flag */
}


/* IC2 Interrupt service routine */
void __ISR(_INPUT_CAPTURE_2_VECTOR, ipl4) IC2_IntHandler (void)
{
    /* copy expected results */
    aui16ICResultsBuffer[1][0] = IC2BUF;
    aui16ICResultsBuffer[1][1] = IC2BUF;
    aui16ICResultsBuffer[1][2] = IC2BUF;

    /* empty buffer */
    flushBuffer((uint8)IC_KE_CHANNEL_2);

    /* go in KE_IC_CALCULATE state */
    eICTaskState[(uint8)IC_KE_CHANNEL_2] = KE_IC_CALCULATE;

    CLEAR_IC2_INT_FLAG();   /* Clear the IC2 interrupt flag */
}


/* IC3 Interrupt service routine */
void __ISR(_INPUT_CAPTURE_3_VECTOR, ipl4) IC3_IntHandler (void)
{
    /* copy expected results */
    aui16ICResultsBuffer[2][0] = IC3BUF;
    aui16ICResultsBuffer[2][1] = IC3BUF;
    aui16ICResultsBuffer[2][2] = IC3BUF;

    /* empty buffer */
    flushBuffer((uint8)IC_KE_CHANNEL_3);

    /* go in KE_IC_CALCULATE state */
    eICTaskState[(uint8)IC_KE_CHANNEL_3] = KE_IC_CALCULATE;

    CLEAR_IC3_INT_FLAG();   /* Clear the IC3 interrupt flag */
}
