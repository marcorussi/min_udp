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
 * This file adc.c represents the source file of the ADC component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


/* --------------- Inclusions -------------- */

#include <xc.h>
#include <sys/attribs.h>
#include "p32mx795f512l.h"

#include "../fw_common.h"

#include "adc.h"
#include "port.h"

#include "../sal/sys/sys.h"




/* ADC TIMER 3 DEFINES */
/* Default value of PWM frequency to set at PWM init function */
#define ADC_FREQ_DEFAULT_VALUE_HZ       ((uint32)100)  /* 100 Hz */

/* PWM timer prescaler ratio. This value is fixed and it depends by timer control register configuration */
#define ADC_TIMER_PRESCALER_RATIO       ((uint32)8)

/* PWM timer period default value */
#define ADC_TIMER_PERIOD_DEF_VALUE      ((uint32)((SYS_UL_FPB / ADC_FREQ_DEFAULT_VALUE_HZ / ADC_TIMER_PRESCALER_RATIO ) - 1))

/* Prescaler bits register position */
#define T3_PRESCALER_BITS_REG_POS       (4)


/* ADC registers bits configuration in MANUAL MODE */
/* AD1CON1 */
#define AD1CON1_ON_MAN          0       /* ADC OFF */
#define AD1CON1_FRZ_MAN         0       /* Continue operation when CPU enters Debug Exception mode */
#define AD1CON1_SIDL_MAN        0       /* Continue module operation in IDLE mode */
#define AD1CON1_FORM_MAN        0       /* Integer 16-bit */
#define AD1CON1_SSRC_MAN        7       /* Internal counter ends sampling and starts conversion (auto convert) */
#define AD1CON1_CLRASAM_MAN     0       /* Normal operation, buffer contents will be overwritten by the next conversion sequence */
#define AD1CON1_ASAM_MAN        1       /* Sampling begins immediately after last conversion completes; SAMP bit is automatically set */
#define AD1CON1_SAMP_MAN        0       /* The ADC sample/hold amplifier is holding */
#define AD1CON1_DONE_MAN        0       /* read only - not used */

/* AD1CON2 */
#define AD1CON2_VCFG_MAN        0       /* VR+=AVdd / VR-=AVss */
#define AD1CON2_OFFCAL_MAN      0       /* Disable Offset Calibration mode */
#define AD1CON2_CSCNA_MAN       1       /* Scan inputs */
#define AD1CON2_BUFS_MAN        0       /* read only - not used */
#define AD1CON2_SMPI_MAN        0       /* Interrupt flag is polled by task - this value is set at every acquisition */
#define AD1CON2_BUFM_MAN        0       /* Buffer configured as one 16-word buffer ADC1BUF(15...0.) */
#define AD1CON2_ALTS_MAN        0       /* Always use MUX A input multiplexer settings */

/* AD1CON3 */
#define AD1CON3_ADRC_MAN        0       /* Clock derived from Peripheral Bus Clock (PBclock) */
#define AD1CON3_SAMC_MAN        25      /* Auto-Sample Time = 25 T_AD */
#define AD1CON3_ADCS_MAN        63      /* 128 * T_PB = T_AD */

/* AD1CHS */
#define AD1CHS_CH0NB_MAN        0       /* MUX B Channel 0 negative input is VR- */
#define AD1CHS_CH0SB_MAN        0       /* MUX B Channel 0 positive input is AN0 - not used in this configuration */
#define AD1CHS_CH0NA_MAN        0       /* MUX A Channel 0 negative input is VR- */
#define AD1CHS_CH0SA_MAN        0       /* MUX A Channel 0 positive input is AN0 */


/* ADC registers bits configuration in AUTO MODE */
/* AD1CON1 */
#define AD1CON1_ON_AUTO         0       /* ADC OFF */
#define AD1CON1_FRZ_AUTO        0       /* Continue operation when CPU enters Debug Exception mode */
#define AD1CON1_SIDL_AUTO       0       /* Continue module operation in IDLE mode */
#define AD1CON1_FORM_AUTO       0       /* Integer 16-bit */
#define AD1CON1_SSRC_AUTO       2       /* Timer 3 period match ends sampling and starts conversion */
#define AD1CON1_CLRASAM_AUTO    0       /* Normal operation, buffer contents will be overwritten by the next conversion sequence */
#define AD1CON1_ASAM_AUTO       1       /* Sampling begins immediately after last conversion completes; SAMP bit is automatically set */
#define AD1CON1_SAMP_AUTO       0       /* The ADC sample/hold amplifier is holding */
#define AD1CON1_DONE_AUTO       0       /* read only - not used */

/* AD1CON2 */
#define AD1CON2_VCFG_AUTO       0       /* VR+=AVdd / VR-=AVss */
#define AD1CON2_OFFCAL_AUTO     0       /* Disable Offset Calibration mode */
#define AD1CON2_CSCNA_AUTO      1       /* Scan inputs */
#define AD1CON2_BUFS_AUTO       0       /* read only - not used */
#define AD1CON2_SMPI_AUTO       3       /* Interrupts at the completion of conversion for each 4th sequence */
#define AD1CON2_BUFM_AUTO       0       /* Buffer configured as one 16-word buffer ADC1BUF(15...0.) */
#define AD1CON2_ALTS_AUTO       0       /* Always use MUX A input multiplexer settings */

/* AD1CON3 */
#define AD1CON3_ADRC_AUTO       0       /* Clock derived from Peripheral Bus Clock (PBclock) */
#define AD1CON3_SAMC_AUTO       0       /* Auto-Sample = 0 T_AD (Not allowed) - not used in this configuration */
#define AD1CON3_ADCS_AUTO       63      /* 128 * T_PB = T_AD */

/* AD1CHS */
#define AD1CHS_CH0NB_AUTO       0       /* MUX B Channel 0 negative input is VR- */
#define AD1CHS_CH0SB_AUTO       0       /* MUX B Channel 0 positive input is AN0 - not used in this configuration */
#define AD1CHS_CH0NA_AUTO       0       /* MUX A Channel 0 negative input is VR- */
#define AD1CHS_CH0SA_AUTO       0       /* MUX A Channel 0 positive input is AN0 */

/* AD1PCFG */
#define AD1PCFG_PCFG_AUTO       15      /* AN0/AN1/AN2/AN3 as analog input */

/* AD1CSSL */
#define AD1CSSL_CSSL_AUTO       15      /* select AN0/AN1/AN2/AN3 as scan inputs */


/* ADC registers bits position */
/* AD1CON1 POS */
#define AD1CON1_ON_POS      15
#define AD1CON1_FRZ_POS     14
#define AD1CON1_SIDL_POS    13
#define AD1CON1_FORM_POS    8
#define AD1CON1_SSRC_POS    5
#define AD1CON1_CLRASAM_POS 4
#define AD1CON1_ASAM_POS    2
#define AD1CON1_SAMP_POS    1
#define AD1CON1_DONE_POS    0
/* AD1CON2 POS */
#define AD1CON2_VCFG_POS    13
#define AD1CON2_OFFCAL_POS  12
#define AD1CON2_CSCNA_POS   10
#define AD1CON2_BUFS_POS    7
#define AD1CON2_SMPI_POS    2
#define AD1CON2_BUFM_POS    1
#define AD1CON2_ALTS_POS    0
/* AD1CON3 POS */
#define AD1CON3_ADRC_POS    15
#define AD1CON3_SAMC_POS    8
#define AD1CON3_ADCS_POS    0
/* AD1CHS POS */
#define AD1CHS_CH0NB_POS    31
#define AD1CHS_CH0SB_POS    24
#define AD1CHS_CH0NA_POS    23
#define AD1CHS_CH0SA_POS    16
/* AD1PCFG POS */
#define AD1PCFG_PCFG_POS    0
/* AD1CSSL POS */
#define AD1CSSL_CSSL_POS    0


/* ADC registers init values in MANUAL MODE */
/* AD1CON1 register init value */
#define AD1CON1_MAN_REG_VALUE   ((AD1CON1_ON_MAN << AD1CON1_ON_POS) |      \
                                (AD1CON1_FRZ_MAN << AD1CON1_FRZ_POS) |     \
                                (AD1CON1_SIDL_MAN << AD1CON1_SIDL_POS) |   \
                                (AD1CON1_FORM_MAN << AD1CON1_FORM_POS) |   \
                                (AD1CON1_SSRC_MAN << AD1CON1_SSRC_POS) |   \
                                (AD1CON1_CLRASAM_MAN << AD1CON1_CLRASAM_POS) |\
                                (AD1CON1_ASAM_MAN << AD1CON1_ASAM_POS) |   \
                                (AD1CON1_SAMP_MAN << AD1CON1_SAMP_POS)     )

/* AD1CON2 register init value */
#define AD1CON2_MAN_REG_VALUE  ((AD1CON2_VCFG_MAN << AD1CON2_VCFG_POS) |   \
                                (AD1CON2_OFFCAL_MAN << AD1CON2_OFFCAL_POS) |\
                                (AD1CON2_CSCNA_MAN << AD1CON2_CSCNA_POS) | \
                                (AD1CON2_SMPI_MAN << AD1CON2_SMPI_POS) |   \
                                (AD1CON2_BUFM_MAN << AD1CON2_BUFM_POS) |   \
                                (AD1CON2_ALTS_MAN << AD1CON2_ALTS_POS)     )

/* AD1CON3 register init value */
#define AD1CON3_MAN_REG_VALUE  ((AD1CON3_ADRC_MAN << AD1CON3_ADRC_POS) |   \
                                (AD1CON3_SAMC_MAN << AD1CON3_SAMC_POS) |   \
                                (AD1CON3_ADCS_MAN << AD1CON3_ADCS_POS)     )

/* AD1CHS register init value */
#define AD1CHS_MAN_REG_VALUE   ((AD1CHS_CH0NB_MAN << AD1CHS_CH0NB_POS) |   \
                                (AD1CHS_CH0SB_MAN << AD1CHS_CH0SB_POS) |   \
                                (AD1CHS_CH0NA_MAN << AD1CHS_CH0NA_POS) |   \
                                (AD1CHS_CH0SA_MAN << AD1CHS_CH0SA_POS)     )


/* ADC registers init values in AUTO MODE */
/* AD1CON1 register init value */
#define AD1CON1_AUT_REG_VALUE   ((AD1CON1_ON_AUTO << AD1CON1_ON_POS) |      \
                                (AD1CON1_FRZ_AUTO << AD1CON1_FRZ_POS) |     \
                                (AD1CON1_SIDL_AUTO << AD1CON1_SIDL_POS) |   \
                                (AD1CON1_FORM_AUTO << AD1CON1_FORM_POS) |   \
                                (AD1CON1_SSRC_AUTO << AD1CON1_SSRC_POS) |   \
                                (AD1CON1_CLRASAM_AUTO << AD1CON1_CLRASAM_POS) |\
                                (AD1CON1_ASAM_AUTO << AD1CON1_ASAM_POS) |   \
                                (AD1CON1_SAMP_AUTO << AD1CON1_SAMP_POS)     )

/* AD1CON2 register init value */
#define AD1CON2_AUT_REG_VALUE  ((AD1CON2_VCFG_AUTO << AD1CON2_VCFG_POS) |   \
                                (AD1CON2_OFFCAL_AUTO << AD1CON2_OFFCAL_POS) |\
                                (AD1CON2_CSCNA_AUTO << AD1CON2_CSCNA_POS) | \
                                (AD1CON2_SMPI_AUTO << AD1CON2_SMPI_POS) |   \
                                (AD1CON2_BUFM_AUTO << AD1CON2_BUFM_POS) |   \
                                (AD1CON2_ALTS_AUTO << AD1CON2_ALTS_POS)     )

/* AD1CON3 register init value */
#define AD1CON3_AUT_REG_VALUE  ((AD1CON3_ADRC_AUTO << AD1CON3_ADRC_POS) |   \
                                (AD1CON3_SAMC_AUTO << AD1CON3_SAMC_POS) |   \
                                (AD1CON3_ADCS_AUTO << AD1CON3_ADCS_POS)     )

/* AD1CHS register init value */
#define AD1CHS_AUT_REG_VALUE   ((AD1CHS_CH0NB_AUTO << AD1CHS_CH0NB_POS) |   \
                                (AD1CHS_CH0SB_AUTO << AD1CHS_CH0SB_POS) |   \
                                (AD1CHS_CH0NA_AUTO << AD1CHS_CH0NA_POS) |   \
                                (AD1CHS_CH0SA_AUTO << AD1CHS_CH0SA_POS)     )

/* AD1PCFG register init value */
#define AD1PCFG_AUT_REG_VALUE  ((AD1PCFG_PCFG_AUTO << AD1PCFG_PCFG_POS)     )

/* AD1CSSL register init value */
#define AD1CSSL_AUT_REG_VALUE  ((AD1CSSL_CSSL_AUTO << AD1CSSL_CSSL_POS)     )


/* Macros to set register bits */
/* Turn ON ADC */
#define ADC_TURN_ON()       (AD1CON1SET = (1 << AD1CON1_ON_POS))
/* Turn OFF ADC */
#define ADC_TURN_OFF()      (AD1CON1CLR = (1 << AD1CON1_ON_POS))
/* Start sampling */
#define ADC_START_SAMP()    (AD1CON1SET = (1 << AD1CON1_SAMP_POS))


/* ADC1 interrupt macros */
#define ADC1E_POS                       1
#define ADC1F_POS                       1
#define ADC1_PRI_POS                    26
#define ADC1_SUBPRI_POS                 24
#define ENABLE_ADC1_INT()               (IEC1SET = (1 << ADC1E_POS))
#define DISABLE_ADC1_INT()              (IEC1CLR = (1 << ADC1E_POS))
#define CLEAR_ADC1_INT_FLAG()           (IFS1CLR = (1 << ADC1F_POS))
#define SET_ADC1_PRIORITY(x)            (IPC6SET = (x << ADC1_PRI_POS))
#define SET_ADC1_SUBPRIORITY(x)         (IPC6SET = (x << ADC1_SUBPRI_POS))
#define CLEAR_ADC1_PRIORITY()           (IPC6CLR = ((7 << ADC1_PRI_POS) | (3 << ADC1_SUBPRI_POS)))


/* T3 interrupt macros */
#define T3E_POS                         12
#define T3F_POS                         12
#define T3_PRI_POS                      2
#define T3_SUBPRI_POS                   0
#define ENABLE_T3_INT()                 (IEC0SET = (1 << T3E_POS))
#define DISABLE_T3_INT()                (IEC0CLR = (1 << T3E_POS))
#define CLEAR_T3_INT_FLAG()             (IFS0CLR = (1 << T3F_POS))
#define SET_T3_PRIORITY(x)              (IPC3SET = (x << T3_PRI_POS))
#define SET_T3_SUBPRIORITY(x)           (IPC3SET = (x << T3_SUBPRI_POS))
#define CLEAR_T3_PRIORITY()             (IPC3CLR = ((7 << T3_PRI_POS) | (3 << T3_SUBPRI_POS)))


/* Macro to check acquisition end */
#define IS_ACQ_COMPLETE()               ((IFS1 & (1 << ADC1F_POS)) > 0)


/* task states enum */
typedef enum
{
    KE_AUTO_MODE_INIT,
    KE_MANUAL_MODE_INIT,
    KE_ACQ_IN_PROG,
    KE_ACQ_FINISHED,
    KE_ACQ_WAIT,
    KE_TASK_STATE_CHECK
} ke_ADCTaskState;


/* ADC MANUAL MODE required acquisition info structure */
typedef struct
{
    uint16 ui16ReqChannelsMask; /* requested channels mask */
    uint8 ui8ReqChannelsNum;    /* requested channels number */
} st_ManualAcq;


/* ADC result buffer pointer */
LOCAL VOLATILE uint32 *pui32ResultBufPtrValue;

/* ADC result buffer pointer */
LOCAL VOLATILE uint32 *pui32ResultBufPtr;

/* ADC actual acquisition mode */
LOCAL ke_ADCTaskState eActualTaskState;

/* ADC MANUAL MODE acquisition data structure */
LOCAL st_ManualAcq stManualAcqData;


/* ADC samples buffer */
EXPORTED uint32 ADC_aui32ResultsBuffer[U32_SAMPLES_BUFFER_LENGTH];


LOCAL void startManualAcquisition   ( uint16, uint8 );
LOCAL void stopManualAcquisition    ( void );
LOCAL void startAutoAcquisition     ( void );
LOCAL void stopAutoAcquisition      ( void );
LOCAL void readAcqResults           ( void );




/* Init ADC function */
EXPORTED void ADC_Init( ADC_ke_AcquisitionMode eReqAcqMode )
{
    /* get result buffer pointer */
    pui32ResultBufPtrValue = (VOLATILE uint32 *)&ADC1BUF0;
    pui32ResultBufPtr = pui32ResultBufPtrValue;

    /* Turn OFF ADC */
    ADC_TURN_OFF();

    /* AUTO acquisition mode */
    if(ADC_MODE_AUTO_ACQ == eReqAcqMode)
    {
        /* ADC module in AUTO MODE */
        eActualTaskState = KE_AUTO_MODE_INIT;

        /* Set ADC registers */
        AD1CON1 = AD1CON1_AUT_REG_VALUE;
        AD1CON2 = AD1CON2_AUT_REG_VALUE;
        AD1CON3 = AD1CON3_AUT_REG_VALUE;
        AD1CHS = AD1CHS_AUT_REG_VALUE;
        AD1PCFG = AD1PCFG_AUT_REG_VALUE;
        AD1CSSL = AD1CSSL_AUT_REG_VALUE;

        /* Configure timer 3 for ADC conversion trigger */
        T3CON = (3 << T3_PRESCALER_BITS_REG_POS);       /* 1:8 prescale value */
        PR3 = ADC_TIMER_PERIOD_DEF_VALUE;

        /* Clear ADC interrupt flag */
        CLEAR_ADC1_INT_FLAG();
        /* Set ADC interrupt priority */
        SET_ADC1_PRIORITY(5);
        /* Set ADC interrupt sub-priority */
        SET_ADC1_SUBPRIORITY(1);
        /* Enable ADC interrupt */
        ENABLE_ADC1_INT();

        /* Clear T3 interrupt flag */
        CLEAR_T3_INT_FLAG();
        /* Set T3 priority interrupt */
        SET_T3_PRIORITY(6);
        /* Set T3 sub-priority interrupt */
        SET_T3_SUBPRIORITY(1);
        /* Enable T3 interrupt */
//        ENABLE_T3_INT();
    }
    /* any other value considered as MANUAL acquisition mode */
    else
    {
        /* ADC module in MANUAL MODE */
        eActualTaskState = KE_MANUAL_MODE_INIT;

        /* Set ADC registers */
        AD1CON1 = AD1CON1_MAN_REG_VALUE;
        AD1CON2 = AD1CON2_MAN_REG_VALUE;
        AD1CON3 = AD1CON3_MAN_REG_VALUE;
        AD1CHS = AD1CHS_MAN_REG_VALUE;

        /* Interrupt not used in MANUAL MODE */
        /* Clear ADC interrupt flag */
        CLEAR_ADC1_INT_FLAG();
        /* Clear ADC interrupt priority */
        CLEAR_ADC1_PRIORITY();
        /* Disable ADC interrupt */
        DISABLE_ADC1_INT();
    }

    /* Turn ON ADC */
    ADC_TURN_ON();
}


/* Start analog channels acquisition */
EXPORTED void ADC_StartChnAcq( uint16 ui16ChannelsMask, uint8 ui8ChannelsNum)
{
    /* check number of channel <= than result buffer length */
    if(ui8ChannelsNum <= U32_SAMPLES_BUFFER_LENGTH)
    {
        /* store required channels mask to acquire */
        stManualAcqData.ui16ReqChannelsMask = ui16ChannelsMask;

        /* store required channels number to acquire */
        stManualAcqData.ui8ReqChannelsNum = ui8ChannelsNum;
    }
    else
    {
        /* too many channels - ignore request */
    }
}


/* Check acquistion end */
EXPORTED boolean ADC_CheckIsAcqFinished( void )
{
    boolean bAcqIsFinished = B_FALSE;

    /* if acquisition is in progress */
    if(KE_ACQ_IN_PROG == eActualTaskState)
    {
        /* return B_FALSE */
    }
    /* any other state is considered as request reset */
    else
    {
        /* acquisition is finished */
        bAcqIsFinished = B_TRUE;

        /* request is considered served - clear it */
        stManualAcqData.ui16ReqChannelsMask = 0;
        stManualAcqData.ui8ReqChannelsNum = 0;

        /* ADC module in MANUAL MODE */
        eActualTaskState = KE_MANUAL_MODE_INIT;
    }

    return bAcqIsFinished;
}


/* Manage periodic ADC task */
EXPORTED void ADC_PeriodicTask( void )
{
    /* manage actual ADC state */
    switch(eActualTaskState)
    {
        case KE_AUTO_MODE_INIT:
        {
            /* start acquisition in AUTO MODE */
            startAutoAcquisition();

            /* got to wait indefinitely */
            eActualTaskState = KE_ACQ_WAIT;

            break;
        }
        case KE_MANUAL_MODE_INIT:
        {
            /* execute an eventual requested manual command */
            if((stManualAcqData.ui16ReqChannelsMask > 0)
            && (stManualAcqData.ui8ReqChannelsNum > 0))
            {
                /* start MANUAL acquisition */
                startManualAcquisition(stManualAcqData.ui16ReqChannelsMask, stManualAcqData.ui8ReqChannelsNum);

                /* got to acquisition in progress */
                eActualTaskState = KE_ACQ_IN_PROG;
            }
            else
            {
                /* remain in this state */
            }

            break;
        }
        case KE_ACQ_WAIT:
        {
            /* do nothing */
            
            break;
        }
        case KE_ACQ_IN_PROG:
        {
            /* if the last requested command is still valid */
            if((stManualAcqData.ui16ReqChannelsMask > 0)
            && (stManualAcqData.ui8ReqChannelsNum > 0))
            {
                /* wait and check acquisition end */
                if(IS_ACQ_COMPLETE())
                {
                    /* read acquisition results */
                    readAcqResults();

                    /* got to acq complete state */
                    eActualTaskState = KE_ACQ_FINISHED;
                    

                    PORT_TogglePortPin(PORT_ID_D, PORT_PIN_1);
                }
                else
                {
                    /* wait... */
                }
            }
            else
            {
                /* a stop command is received - stop MANUAL acquisition */
                stopManualAcquisition();

                /* got back to init state */
                eActualTaskState = KE_MANUAL_MODE_INIT;
            }

            break;
        }
        case KE_ACQ_FINISHED:
        {
            /* wait results are read */

            break;
        }
        default:
        {
            /* INTERNAL ERROR: go to wait indefinitely */
            eActualTaskState = KE_ACQ_WAIT;
        }
    }
}


/* store acquisition results */
LOCAL void readAcqResults( void )
{
    uint8 ui8Index;

    /* copy results */
    for(ui8Index = 0; ui8Index < U32_SAMPLES_BUFFER_LENGTH; ui8Index++)
    {
        /* store samples */
        ADC_aui32ResultsBuffer[ui8Index] = *pui32ResultBufPtr;
        /* increment result buffer pointer */
        pui32ResultBufPtr += 4;
    }
    /* re-init result buffer pointer */
    pui32ResultBufPtr = pui32ResultBufPtrValue;
}


/* Start ADC MANUAL acquisition */
LOCAL void startManualAcquisition( uint16 ui16ChannelsMask, uint8 ui8ChannelsNum )
{
    /* clear registers */
    AD1PCFGSET = 0xFFFF;    /* all input as digital */
    AD1CSSL = 0;            /* no scan */

    /* set analog channels */
    AD1PCFGCLR = (ui16ChannelsMask << AD1PCFG_PCFG_POS);

    /* input scan selection */
    AD1CSSLSET = (ui16ChannelsMask << AD1CSSL_CSSL_POS);

    /* set int flag after channels number */
    AD1CON2SET = ((ui8ChannelsNum - 1) << AD1CON2_SMPI_POS);

    /* Start sampling */
    ADC_START_SAMP();
}


/* Stop ADC MANUAL acquisition */
LOCAL void stopManualAcquisition( void )
{
    /* Turn OFF ADC */
    ADC_TURN_OFF();

    /* Clear ADC registers */
    AD1CON1 = 0;
    AD1CON2 = 0;
    AD1CON3 = 0;
    AD1CHS = 0;
    AD1PCFG = 0;
    AD1CSSL = 0;
}


/* Start ADC AUTO acquisition */
LOCAL void startAutoAcquisition( void )
{
    /* Start sampling */
    ADC_START_SAMP();

    /* Enable Timer3 */
    T3CONSET = 0x8000;
}


/* Stop ADC AUTO acquisition */
LOCAL void stopAutoAcquisition( void )
{
    /* Disable Timer3 */
    T3CONCLR = 0x8000;

    /* Turn OFF ADC */
    ADC_TURN_OFF();

    /* Clear ADC registers */
    AD1CON1 = 0;
    AD1CON2 = 0;
    AD1CON3 = 0;
    AD1CHS = 0;
    AD1PCFG = 0;
    AD1CSSL = 0;

    /* Clear ADC interrupt flag */
    CLEAR_ADC1_INT_FLAG();
    /* Clear ADC interrupt priority */
    CLEAR_ADC1_PRIORITY();
    /* Disable ADC interrupt */
    DISABLE_ADC1_INT();

    /* Clear T3 interrupt flag */
    CLEAR_T3_INT_FLAG();
    /* Clear T3 interrupt priority */
    CLEAR_T3_PRIORITY();
    /* Disable T3 interrupt */
    DISABLE_T3_INT();
}


/* Interrupt service routine */
void __ISR(_TIMER_3_VECTOR, ipl6) T3_IntHandler (void)
{
//    PORT_TogglePortPin(PORT_ID_D, PORT_PIN_2);

    
    /* Clear T3 interrupt flag */
    CLEAR_T3_INT_FLAG();
}


/* Interrupt service routine */
void __ISR(_ADC_VECTOR, ipl5) ADC_IntHandler (void)
{
    PORT_TogglePortPin(PORT_ID_D, PORT_PIN_1);


    /* read acquisition results */
    readAcqResults();

    /* Clear ADC interrupt flag */
    CLEAR_ADC1_INT_FLAG();
}