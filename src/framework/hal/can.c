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
 * This file can.c represents the source file of the CAN component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


#define FILE_CAN  "CAN"
/*************************************************************************
**                   system and library files included                  **
**************************************************************************/
#include <xc.h>
#include <sys/attribs.h>
#include <sys/kmem.h>
#include "../fw_common.h"
#include "can.h"

/*************************************************************************
**                   Other files included                               **
**************************************************************************/
//#include COMP_SYS

/*************************************************************************
**                   Declaration of local constants                     **
*************************************************************************/



// TEMP
#define SYS_UL_FOSC         ((uint32)80000000)               /* 80 MHz */
#define SYS_UL_FCY          ((uint32)(SYS_UL_FOSC / 1))      /* 80 MHz */
#define SYS_UL_FPB          ((uint32)(SYS_UL_FCY / 2))       /* 40 MHz */

/* Peripheral clock period in ns */
#define SYS_US_TPB_NS       ((uint16)(1000000000 / SYS_UL_FPB))





/* Each FIFO buffer is long 32 messages */
/* FIFO buffers are divided as follow
 * - 1 highest priority TX buffer
 * - 1 lowest priority TX buffer
 * - 1 RX buffer
 * - 1 RTR messages buffer */

/* Buffers length */
#define UC_NUM_OF_TX_FIFO_BUFFERS               UC_2
#define UC_NUM_OF_RX_FIFO_BUFFERS     		UC_1
#define UC_NUM_OF_RTR_FIFO_BUFFERS     		UC_1
#define UC_TOTAL_NUM_OF_FIFO_BUFFERS            (uint8)(UC_NUM_OF_TX_FIFO_BUFFERS + UC_NUM_OF_RX_FIFO_BUFFERS + UC_NUM_OF_RTR_FIFO_BUFFERS)

/* Message length */
#define UC_NUM_OF_MESSAGES_EACH_BUFFER          UC_32

/* Message length */
#define UC_NUM_OF_WORDS_EACH_MESSAGE            UC_4

/* Total number of words of FIFO buffers */
#define US_NUM_OF_WORDS_FIFO_BUFFERS            (uint16)(UC_TOTAL_NUM_OF_FIFO_BUFFERS * UC_NUM_OF_MESSAGES_EACH_BUFFER * UC_NUM_OF_WORDS_EACH_MESSAGE)

/* Maximum acceptance filters number */
#define UC_MAX_NUM_OF_ACCEPTANCE_FILTERS	UC_32

/* Maximum baud rate value in Kilobit/s */
#define US_MAX_BAUDRATE_KBIT_PER_SEC            CAN_US_MAX_BAUDRATE_KBIT_PER_SEC

/* Bit time = (sync jump + phase seg 1 + phase seg 2 + prog seg) * TQ */
#define UL_N_TQ_SYNC_JUMP                       ((uint32)1)
#define UL_N_TQ_PHASE_SEG_1                     ((uint32)8)
#define UL_N_TQ_PHASE_SEG_2                     ((uint32)8)
#define UL_N_TQ_PROG_SEG                        ((uint32)3)
#define UL_N_TQ_PER_BIT                         (UL_N_TQ_SYNC_JUMP + UL_N_TQ_PHASE_SEG_1 + UL_N_TQ_PHASE_SEG_2 + UL_N_TQ_PROG_SEG)

/*************************************************************************
**                   Declaration of local macros                        **
**************************************************************************/
                 
/*************************************************************************
**                   Declaration of local types                         **
**************************************************************************/
typedef struct
{
   boolean  bInitialised;
   uint8    ucTxErrorCounter;
   uint8    ucRxErrorCounter;
   uint8    ucWriteRequestIndex;
   uint8    ucWriteExecutionIndex;
   uint8    ucReadRequestIndex;
   uint8    ucReadExecutionIndex;
   uint8    ucWriteBufferCounter;
   uint8    ucReadBufferCounter;
} st_Info;

/* Message buffer CMSGSID data type. */
typedef struct
{
    unsigned SID :11;
    unsigned     :21;
} st_msgsid;

/* Message buffer CMSGEID data type. */
typedef struct
{
    unsigned DLC :4;
    unsigned RB0 :1;
    unsigned     :3;
    unsigned RB1 :1;
    unsigned RTR :1;
    unsigned EID :18;
    unsigned IDE :1;
    unsigned SRR :1;
    unsigned     :2;
} st_msgeid;

/* Message buffer */
typedef union
{
    struct
    {
        st_msgsid stMsgSID;
        st_msgeid stMsgEID;
        uint8 ucvData[CAN_UC_MESSAGE_LENGTH_BYTES];
    };
    uint32 messageWord[UC_4];
} CANMessageBuffer;

/*DC**********************************************************************
**                   Data Declaration                                   **
*************************************************************************/


          /**************************************
          * 1. Declaration of LOCAL variables   *
          **************************************/

/*************************************************************************
Syntax: uint32 CANFIFOBuffers[US_NUM_OF_WORDS_FIFO_BUFFERS]
Object: FIFO buffers allocation
Unit :  uint32
Range:  None
**************************************************************************/
LOCAL uint32 CANFIFOBuffers[US_NUM_OF_WORDS_FIFO_BUFFERS];

/*************************************************************************
Syntax: stInfo stModuleInfo
Object: Module informations
Unit :  None
Range:  None
**************************************************************************/ 
LOCAL st_Info stModuleInfo;

          /****************************************
          * 2. Declaration of LOCAL constant data *
          *****************************************/

          /*****************************************
          * 3. Declaration of EXPORTED variables   *
          *****************************************/

          /*********************************************
          * 4. Declaration of EXPORTED constant data   *
          **********************************************/
          
/**************************************************************************
         Functions prototypes
**************************************************************************/
LOCAL void      setCanConfigurationMode ( void );
LOCAL void      setCanNormalMode        ( void );

/**************************************************************************
          Exported Functions
**************************************************************************/

/*DC***********************************************************************
  Detailed Conception for the function CAN_Initialise()
***************************************************************************
  Syntax   : <void> CAN_Initialise ( CAN_stCanInitParameters stParameters )
  Object   : Initialise can module
  Parameters: CAN_stCanInitParameters stParameters
  Return  : None
  Calls   : None
***************************************************************************
 Pseudo-code:
 BEGIN
 |
 END
**********************************************************************EDC*/
EXPORTED void CAN_Initialise ( CAN_stCanInitParameters stParameters )
{
   uint8 ucTimeQuantaN;
   uint16 usTimeQuantaFreqKbs;
   uint8 ucBRPValue;

   /* Reset module status */
   stModuleInfo.bInitialised = B_TRUE;
   stModuleInfo.ucTxErrorCounter = UC_NULL;
   stModuleInfo.ucRxErrorCounter = UC_NULL;
   stModuleInfo.ucWriteRequestIndex = UC_NULL;
   stModuleInfo.ucWriteExecutionIndex = UC_NULL;
   stModuleInfo.ucReadRequestIndex = UC_NULL;
   stModuleInfo.ucReadExecutionIndex = UC_NULL;
   stModuleInfo.ucWriteBufferCounter = UC_NULL;
   stModuleInfo.ucReadBufferCounter = UC_NULL;

   /* Enable CAN module */
   C1CONbits.ON = 1;

   setCanConfigurationMode();

   /* Reset status flags */
   C1FSTAT = 0;
   /* Disable all module interrupts and clear related flags */
   C1INT = 0;

   /* Enable or disable wake-up filter */
   C1CFGbits.WAKFIL = stParameters.eWakeUpFilter;
   
   /* Set configuration */
   C1CFGbits.SEG2PHTS = 1; /* always freely programmable */
   C1CFGbits.SEG2PH = stParameters.ePhaseSeg2;
   C1CFGbits.SEG1PH = stParameters.ePhaseSeg1;
   C1CFGbits.PRSEG = stParameters.ePropagation;
   C1CFGbits.SJW = stParameters.eSynchJumpWidth;
   C1CFGbits.SAM = stParameters.eBusSample;
   
   /* N from 8 to 25 */
   /* N = Sync + Prop + Phase Seg1 + Phase Seg2 */
   /* Sync is fixed to 1 */
   ucTimeQuantaN = UC_1 + stParameters.ePropagation + stParameters.ePhaseSeg1 + stParameters.ePhaseSeg2;
   /* calculate time quanta frequency: Ftq = N * Fbaud */
   usTimeQuantaFreqKbs = (uint32)((uint32)ucTimeQuantaN * (uint32)stParameters.usBaudRateKbs);
   /* calculate BRP value: BRP = (Fsys / (2 * Ftq)) - 1 */
   ucBRPValue = (uint8)(((SYS_UL_FOSC / usTimeQuantaFreqKbs) / US_1000) / UL_2) - UC_1;
   /* Set BRP value */
   C1CFGbits.BRP = ucBRPValue;

   /* Disable CAN capture */
   C1CONbits.CANCAP = 0;

   /* Set FIFO base address */
   C1FIFOBA = KVA_TO_PA(CANFIFOBuffers);

   /* Configure FIFO0 as highest TX buffer */
   C1FIFOCON0bits.FSIZE = 31;
   C1FIFOCON0bits.TXEN = 1;
   C1FIFOCON0bits.TXPRI = 3;

   /* Configure FIFO1 as lowest TX buffer */
   C1FIFOCON1bits.FSIZE = 31;
   C1FIFOCON1bits.TXEN = 1;
   C1FIFOCON1bits.TXPRI = 0;

   /* Configure FIFO2 as RX buffer */
   C1FIFOCON2bits.FSIZE = 31;
   C1FIFOCON2bits.TXEN = 0;

   /* Configure FIFO3 as intermediate highest TX RTR buffer */
   C1FIFOCON3bits.FSIZE = 31;
   C1FIFOCON3bits.TXEN = 1;
   C1FIFOCON3bits.TXPRI = 2;
   C1FIFOCON3bits.RTREN = 1;

   setCanNormalMode();

   /* all filters are matched with mask0 according to EXID and for all bits */
   C1RXM0bits.SID = 0x7FF; /* match all bits */
   C1RXM0bits.MIDE = 1; /* match according to EXID bit in filter */

   /* Enable reception interrupt */
   C1INTbits.RBIF = 0;
   C1INTbits.RBIE = 0;

   /* Enable CAN interrupt */
   IPC11bits.CAN1IP = 2;
   IFS1bits.CAN1IF = 0;
   IEC1bits.CAN1IE = 0;
}


/*DC***********************************************************************
  Detailed Conception for the function CAN_StopReceiveMsgID ()
***************************************************************************
  Syntax   : <CAN_eCanResponseCodes> CAN_StopReceiveMsgID ( uint8 ucNumberOfID )
  Object   : Stop to receive ID
  Parameters: uint8 ucNumberOfID
  Return  : CAN_eCanResponseCodes
  Calls   : None
***************************************************************************
 Pseudo-code:
 BEGIN
 |
 END
**********************************************************************EDC*/
EXPORTED CAN_eCanResponseCodes CAN_StopReceiveMsgID (uint8 ucNumberOfID)
{
   CAN_eCanResponseCodes eResponseCode;

   /* check acceptance filter number */
   if(ucNumberOfID <= UC_MAX_NUM_OF_ACCEPTANCE_FILTERS)
   {
      switch(ucNumberOfID)
      {
         case UC_NULL:
         {
            C1FLTCON0bits.FLTEN0 = 0;  /* disable the filter */
         }
         case UC_1:
         {
            C1FLTCON0bits.FLTEN1 = 0;  /* disable the filter */
         }
         case UC_2:
         {
            C1FLTCON0bits.FLTEN2 = 0;  /* disable the filter */
         }
         case UC_3:
         {
            C1FLTCON0bits.FLTEN3 = 0;  /* disable the filter */
         }
         case UC_4:
         {
            C1FLTCON1bits.FLTEN4 = 0;  /* disable the filter */
         }
         case UC_5:
         {
            C1FLTCON1bits.FLTEN5 = 0;  /* disable the filter */
         }
         case UC_6:
         {
            C1FLTCON1bits.FLTEN6 = 0;  /* disable the filter */
         }
         case UC_7:
         {
            C1FLTCON1bits.FLTEN7 = 0;  /* disable the filter */
         }
         case UC_8:
         {
            C1FLTCON2bits.FLTEN8 = 0;  /* disable the filter */
         }
         case UC_9:
         {
            C1FLTCON2bits.FLTEN9 = 0;  /* disable the filter */
         }
         case UC_10:
         {
            C1FLTCON2bits.FLTEN10 = 0;  /* disable the filter */
         }
         case UC_11:
         {
            C1FLTCON2bits.FLTEN11 = 0;  /* disable the filter */
         }
         case UC_12:
         {
            C1FLTCON3bits.FLTEN12 = 0;  /* disable the filter */
         }
         case UC_13:
         {
            C1FLTCON3bits.FLTEN13 = 0;  /* disable the filter */
         }
         case UC_14:
         {
            C1FLTCON3bits.FLTEN14 = 0;  /* disable the filter */
         }
         case UC_15:
         {
            C1FLTCON3bits.FLTEN15 = 0;  /* disable the filter */
         }
         case UC_16:
         {
            C1FLTCON4bits.FLTEN16 = 0;  /* disable the filter */
         }
         case UC_17:
         {
            C1FLTCON4bits.FLTEN17 = 0;  /* disable the filter */
         }
         case UC_18:
         {
            C1FLTCON4bits.FLTEN18 = 0;  /* disable the filter */
         }
         case UC_19:
         {
            C1FLTCON4bits.FLTEN19 = 0;  /* disable the filter */
         }
         case UC_20:
         {
            C1FLTCON5bits.FLTEN20 = 0;  /* disable the filter */
         }
         case UC_21:
         {
            C1FLTCON5bits.FLTEN21 = 0;  /* disable the filter */
         }
         case UC_22:
         {
            C1FLTCON5bits.FLTEN22 = 0;  /* disable the filter */
         }
         case UC_23:
         {
            C1FLTCON5bits.FLTEN23 = 0;  /* disable the filter */
         }
         case UC_24:
         {
            C1FLTCON6bits.FLTEN24 = 0;  /* disable the filter */
         }
         case UC_25:
         {
            C1FLTCON6bits.FLTEN25 = 0;  /* disable the filter */
         }
         case UC_26:
         {
            C1FLTCON6bits.FLTEN26 = 0;  /* disable the filter */
         }
         case UC_27:
         {
            C1FLTCON6bits.FLTEN27 = 0;  /* disable the filter */
         }
         case UC_28:
         {
            C1FLTCON7bits.FLTEN28 = 0;  /* disable the filter */
         }
         case UC_29:
         {
            C1FLTCON7bits.FLTEN29 = 0;  /* disable the filter */
         }
         case UC_30:
         {
            C1FLTCON7bits.FLTEN30 = 0;  /* disable the filter */
         }
         case UC_31:
         {
            C1FLTCON7bits.FLTEN31 = 0;  /* disable the filter */
         }
         default:
         {
            break;
         }
      }

      /* no errors */
      eResponseCode = CAN_NO_ERRORS;
   }
   else
   {
      /* unvalid acceptance filter number */
      eResponseCode = CAN_ERRORS;
   }

   return eResponseCode;
}


/*DC***********************************************************************
  Detailed Conception for the function CAN_SetReceiveMsgID ()
***************************************************************************
  Syntax   : <CAN_eCanResponseCodes> CAN_SetReceiveMsgID ( uint8 ucNumberOfID, uint32 ulReceiveID, boolean bExide )
  Object   : Set a receive ID
  Parameters: uint8 ucNumberOfID, uint32 ulReceiveID, boolean bExide
  Return  : CAN_eCanResponseCodes
  Calls   : None
***************************************************************************
 Pseudo-code:
 BEGIN
 |
 END
**********************************************************************EDC*/
EXPORTED CAN_eCanResponseCodes CAN_SetReceiveMsgID (uint8 ucNumberOfID, uint32 ulReceiveID, boolean bExide)
{
   CAN_eCanResponseCodes eResponseCode;

   /* check acceptance filter number */
   if(ucNumberOfID <= UC_MAX_NUM_OF_ACCEPTANCE_FILTERS)
   {
      switch(ucNumberOfID)
      {
         case UC_NULL:
         {
            C1FLTCON0bits.FLTEN0 = 0;  /* disable the filter */
            C1FLTCON0bits.FSEL0 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON0bits.MSEL0 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF0bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF0bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF0bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF0bits.SID = ulReceiveID; /* copy SID */
               C1RXF0bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON0bits.FLTEN0 = 1;  /* enable the filter */

            break;
         }
         case UC_1:
         {
            C1FLTCON0bits.FLTEN1 = 0;  /* disable the filter */
            C1FLTCON0bits.FSEL1 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON0bits.MSEL1 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF1bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF1bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF1bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF1bits.SID = ulReceiveID; /* copy SID */
               C1RXF1bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON0bits.FLTEN1 = 1;  /* enable the filter */

            break;
         }
         case UC_2:
         {
            C1FLTCON0bits.FLTEN2 = 0;  /* disable the filter */
            C1FLTCON0bits.FSEL2 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON0bits.MSEL2 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF2bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF2bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF2bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF2bits.SID = ulReceiveID; /* copy SID */
               C1RXF2bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON0bits.FLTEN2 = 1;  /* enable the filter */

            break;
         }
         case UC_3:
         {
            C1FLTCON0bits.FLTEN3 = 0;  /* disable the filter */
            C1FLTCON0bits.FSEL3 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON0bits.MSEL3 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF3bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF3bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF3bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF3bits.SID = ulReceiveID; /* copy SID */
               C1RXF3bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON0bits.FLTEN3 = 1;  /* enable the filter */

            break;
         }
         case UC_4:
         {
            C1FLTCON1bits.FLTEN4 = 0;  /* disable the filter */
            C1FLTCON1bits.FSEL4 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON1bits.MSEL4 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF4bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF4bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF4bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF4bits.SID = ulReceiveID; /* copy SID */
               C1RXF4bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON1bits.FLTEN4 = 1;  /* enable the filter */

            break;
         }
         case UC_5:
         {
            C1FLTCON1bits.FLTEN5 = 0;  /* disable the filter */
            C1FLTCON1bits.FSEL5 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON1bits.MSEL5 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF5bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF5bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF5bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF5bits.SID = ulReceiveID; /* copy SID */
               C1RXF5bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON1bits.FLTEN5 = 1;  /* enable the filter */

            break;
         }
         case UC_6:
         {
            C1FLTCON1bits.FLTEN6 = 0;  /* disable the filter */
            C1FLTCON1bits.FSEL6 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON1bits.MSEL6 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF6bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF6bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF6bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF6bits.SID = ulReceiveID; /* copy SID */
               C1RXF6bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON1bits.FLTEN6 = 1;  /* enable the filter */

            break;
         }
         case UC_7:
         {
            C1FLTCON1bits.FLTEN7 = 0;  /* disable the filter */
            C1FLTCON1bits.FSEL7 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON1bits.MSEL7 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF7bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF7bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF7bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF7bits.SID = ulReceiveID; /* copy SID */
               C1RXF7bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON1bits.FLTEN7 = 1;  /* enable the filter */

            break;
         }
         case UC_8:
         {
            C1FLTCON2bits.FLTEN8 = 0;  /* disable the filter */
            C1FLTCON2bits.FSEL8 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON2bits.MSEL8 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF8bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF8bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF8bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF8bits.SID = ulReceiveID; /* copy SID */
               C1RXF8bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON2bits.FLTEN8 = 1;  /* enable the filter */

            break;
         }
         case UC_9:
         {
            C1FLTCON2bits.FLTEN9 = 0;  /* disable the filter */
            C1FLTCON2bits.FSEL9 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON2bits.MSEL9 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF9bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF9bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF9bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF9bits.SID = ulReceiveID; /* copy SID */
               C1RXF9bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON2bits.FLTEN9 = 1;  /* enable the filter */

            break;
         }
         case UC_10:
         {
            C1FLTCON2bits.FLTEN10 = 0;  /* disable the filter */
            C1FLTCON2bits.FSEL10 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON2bits.MSEL10 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF10bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF10bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF10bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF10bits.SID = ulReceiveID; /* copy SID */
               C1RXF10bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON2bits.FLTEN10 = 1;  /* enable the filter */

            break;
         }
         case UC_11:
         {
            C1FLTCON2bits.FLTEN11 = 0;  /* disable the filter */
            C1FLTCON2bits.FSEL11 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON2bits.MSEL11 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF11bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF11bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF11bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF11bits.SID = ulReceiveID; /* copy SID */
               C1RXF11bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON2bits.FLTEN11 = 1;  /* enable the filter */

            break;
         }
         case UC_12:
         {
            C1FLTCON3bits.FLTEN12 = 0;  /* disable the filter */
            C1FLTCON3bits.FSEL12 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON3bits.MSEL12 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF12bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF12bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF12bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF12bits.SID = ulReceiveID; /* copy SID */
               C1RXF12bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON3bits.FLTEN12 = 1;  /* enable the filter */

            break;
         }
         case UC_13:
         {
            C1FLTCON3bits.FLTEN13 = 0;  /* disable the filter */
            C1FLTCON3bits.FSEL13 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON3bits.MSEL13 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF13bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF13bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF13bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF13bits.SID = ulReceiveID; /* copy SID */
               C1RXF13bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON3bits.FLTEN13 = 1;  /* enable the filter */

            break;
         }
         case UC_14:
         {
            C1FLTCON3bits.FLTEN14 = 0;  /* disable the filter */
            C1FLTCON3bits.FSEL14 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON3bits.MSEL14 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF14bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF14bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF14bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF14bits.SID = ulReceiveID; /* copy SID */
               C1RXF14bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON3bits.FLTEN14 = 1;  /* enable the filter */

            break;
         }
         case UC_15:
         {
            C1FLTCON3bits.FLTEN15 = 0;  /* disable the filter */
            C1FLTCON3bits.FSEL15 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON3bits.MSEL15 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF15bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF15bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF15bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF15bits.SID = ulReceiveID; /* copy SID */
               C1RXF15bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON3bits.FLTEN15 = 1;  /* enable the filter */

            break;
         }
         case UC_16:
         {
            C1FLTCON4bits.FLTEN16 = 0;  /* disable the filter */
            C1FLTCON4bits.FSEL16 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON4bits.MSEL16 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF16bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF16bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF16bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF16bits.SID = ulReceiveID; /* copy SID */
               C1RXF16bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON4bits.FLTEN16 = 1;  /* enable the filter */

            break;
         }
         case UC_17:
         {
            C1FLTCON4bits.FLTEN17 = 0;  /* disable the filter */
            C1FLTCON4bits.FSEL17 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON4bits.MSEL17 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF17bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF17bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF17bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF17bits.SID = ulReceiveID; /* copy SID */
               C1RXF17bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON4bits.FLTEN17 = 1;  /* enable the filter */

            break;
         }
         case UC_18:
         {
            C1FLTCON4bits.FLTEN18 = 0;  /* disable the filter */
            C1FLTCON4bits.FSEL18 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON4bits.MSEL18 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF18bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF18bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF18bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF18bits.SID = ulReceiveID; /* copy SID */
               C1RXF18bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON4bits.FLTEN18 = 1;  /* enable the filter */

            break;
         }
         case UC_19:
         {
            C1FLTCON4bits.FLTEN19 = 0;  /* disable the filter */
            C1FLTCON4bits.FSEL19 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON4bits.MSEL19 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF19bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF19bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF19bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF19bits.SID = ulReceiveID; /* copy SID */
               C1RXF19bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON4bits.FLTEN19 = 1;  /* enable the filter */

            break;
         }
         case UC_20:
         {
            C1FLTCON5bits.FLTEN20 = 0;  /* disable the filter */
            C1FLTCON5bits.FSEL20 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON5bits.MSEL20 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF20bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF20bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF20bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF20bits.SID = ulReceiveID; /* copy SID */
               C1RXF20bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON5bits.FLTEN20 = 1;  /* enable the filter */

            break;
         }
         case UC_21:
         {
            C1FLTCON5bits.FLTEN21 = 0;  /* disable the filter */
            C1FLTCON5bits.FSEL21 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON5bits.MSEL21 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF21bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF21bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF21bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF21bits.SID = ulReceiveID; /* copy SID */
               C1RXF21bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON5bits.FLTEN21 = 1;  /* enable the filter */

            break;
         }
         case UC_22:
         {
            C1FLTCON5bits.FLTEN22 = 0;  /* disable the filter */
            C1FLTCON5bits.FSEL22 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON5bits.MSEL22 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF22bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF22bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF22bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF22bits.SID = ulReceiveID; /* copy SID */
               C1RXF22bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON5bits.FLTEN22 = 1;  /* enable the filter */

            break;
         }
         case UC_23:
         {
            C1FLTCON5bits.FLTEN23 = 0;  /* disable the filter */
            C1FLTCON5bits.FSEL23 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON5bits.MSEL23 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF23bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF23bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF23bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF23bits.SID = ulReceiveID; /* copy SID */
               C1RXF23bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON5bits.FLTEN23 = 1;  /* enable the filter */

            break;
         }
         case UC_24:
         {
            C1FLTCON6bits.FLTEN24 = 0;  /* disable the filter */
            C1FLTCON6bits.FSEL24 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON6bits.MSEL24 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF24bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF24bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF24bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF24bits.SID = ulReceiveID; /* copy SID */
               C1RXF24bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON6bits.FLTEN24 = 1;  /* enable the filter */

            break;
         }
         case UC_25:
         {
            C1FLTCON6bits.FLTEN25 = 0;  /* disable the filter */
            C1FLTCON6bits.FSEL25 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON6bits.MSEL25 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF25bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF25bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF25bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF25bits.SID = ulReceiveID; /* copy SID */
               C1RXF25bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON6bits.FLTEN25 = 1;  /* enable the filter */

            break;
         }
         case UC_26:
         {
            C1FLTCON6bits.FLTEN26 = 0;  /* disable the filter */
            C1FLTCON6bits.FSEL26 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON6bits.MSEL26 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF26bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF26bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF26bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF26bits.SID = ulReceiveID; /* copy SID */
               C1RXF26bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON6bits.FLTEN26 = 1;  /* enable the filter */

            break;
         }
         case UC_27:
         {
            C1FLTCON6bits.FLTEN27 = 0;  /* disable the filter */
            C1FLTCON6bits.FSEL27 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON6bits.MSEL27 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF27bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF27bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF27bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF27bits.SID = ulReceiveID; /* copy SID */
               C1RXF27bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON6bits.FLTEN27 = 1;  /* enable the filter */

            break;
         }
         case UC_28:
         {
            C1FLTCON7bits.FLTEN28 = 0;  /* disable the filter */
            C1FLTCON7bits.FSEL28 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON7bits.MSEL28 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF28bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF28bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF28bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF28bits.SID = ulReceiveID; /* copy SID */
               C1RXF28bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON7bits.FLTEN28 = 1;  /* enable the filter */

            break;
         }
         case UC_29:
         {
            C1FLTCON7bits.FLTEN29 = 0;  /* disable the filter */
            C1FLTCON7bits.FSEL29 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON7bits.MSEL29 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF29bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF29bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF29bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF29bits.SID = ulReceiveID; /* copy SID */
               C1RXF29bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON7bits.FLTEN29 = 1;  /* enable the filter */

            break;
         }
         case UC_30:
         {
            C1FLTCON7bits.FLTEN30 = 0;  /* disable the filter */
            C1FLTCON7bits.FSEL30 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON7bits.MSEL30 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF30bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF30bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF30bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF30bits.SID = ulReceiveID; /* copy SID */
               C1RXF30bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON7bits.FLTEN30 = 1;  /* enable the filter */

            break;
         }
         case UC_31:
         {
            C1FLTCON7bits.FLTEN31 = 0;  /* disable the filter */
            C1FLTCON7bits.FSEL31 = 2;   /* Store messages in RX buffer FIFO2 */
            C1FLTCON7bits.MSEL31 = 0;   /* use Mask 0 */
            if(bExide == B_TRUE)
            {
               C1RXF31bits.SID = (ulReceiveID & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               C1RXF31bits.EID = (ulReceiveID & 0x0003FFFF); /* copy EID part */
               C1RXF31bits.EXID = 1;          /* filter only EID messages */
            }
            else
            {
               C1RXF31bits.SID = ulReceiveID; /* copy SID */
               C1RXF31bits.EXID = 0;          /* Filter only SID messages */
            }
            C1FLTCON7bits.FLTEN31 = 1;  /* enable the filter */

            break;
         }
         default:
         {
            break;
         }
      }

      /* no errors */
      eResponseCode = CAN_NO_ERRORS;
   }
   else
   {
      /* unvalid acceptance filter number */
      eResponseCode = CAN_ERRORS;
   }

   return eResponseCode;
}


/*DC***********************************************************************
  Detailed Conception for the function CAN_eCheckCANTxStatus ()
***************************************************************************
  Syntax   : <CAN_eCanResponseCodes> CAN_eCheckCANTxStatus ( void )
  Object   : Check transmission status
  Parameters: None
  Return  : CAN_eCanResponseCodes
  Calls   : None
***************************************************************************
 Pseudo-code:
 BEGIN
 | 
 END
**********************************************************************EDC*/
EXPORTED CAN_eCanResponseCodes CAN_eCheckCANTxStatus ( void )
{
   CAN_eCanResponseCodes eResponseCode;

   /* TODO */
   
   return eResponseCode;
}

/*DC***********************************************************************
  Detailed Conception for the function CAN_eCheckCANRxStatus ()
***************************************************************************
  Syntax   : <CAN_eCanResponseCodes> CAN_eCheckCANRxStatus ( void )
  Object   : Check reception status
  Parameters: None
  Return  : CAN_eCanResponseCodes
  Calls   : None
***************************************************************************
 Pseudo-code:
 BEGIN
 |
 END
**********************************************************************EDC*/
EXPORTED CAN_eCanResponseCodes CAN_eCheckCANRxStatus ( void )
{
   CAN_eCanResponseCodes eResponseCode;

   /* TODO */

   return eResponseCode;
}


/*DC***********************************************************************
  Detailed Conception for the function CAN_ePrepareRTRReply ()
***************************************************************************
  Syntax   : <CAN_eCanResponseCodes> CAN_ePrepareRTRReply ( CAN_stCanMessage *pstMessage )
  Object   : Prepare a RTR message
  Parameters: CAN_stCanMessage *pstMessage
  Return  : CAN_eCanResponseCodes
  Calls   : None
***************************************************************************
 Pseudo-code:
 BEGIN
 | 
 END
**********************************************************************EDC*/
EXPORTED CAN_eCanResponseCodes CAN_ePrepareRTRReply ( CAN_stCanMessage *pstMessage )
{
   CAN_eCanResponseCodes eResponseCode;

   /* TODO */

   return eResponseCode;
}


/*DC***********************************************************************
  Detailed Conception for the function CAN_eRequireMsgTransmission ()
***************************************************************************
  Syntax   : <CAN_eCanResponseCodes> CAN_eRequireMsgTransmission ( CAN_stCanMessage *pstMessage
                                                                   CAN_eMessagePriority ePriority )
  Object   : Send a message
  Parameters: CAN_stCanMessage *pstMessage, CAN_eMessagePriority ePriority
  Return  : CAN_eCanResponseCodes
  Calls   : None
***************************************************************************
 Pseudo-code:
 BEGIN
 | 
 END
**********************************************************************EDC*/
EXPORTED CAN_eCanResponseCodes CAN_eRequireMsgTransmission ( CAN_stCanMessage *pstMessage, CAN_eMessagePriority ePriority )
{
   CAN_eCanResponseCodes eResponseCode;
   CANMessageBuffer * transmitMessage;

   /* Manage selected priority FIFO */
   switch(ePriority)
   {
      case CAN_HIGH_PRIORITY:
      {
         /* If buffer is not full */
         if(C1FIFOINT0bits.TXNFULLIF == 1)
         {
            /* Get pointer of next free message of TX buffer */
            transmitMessage = (CANMessageBuffer *)(PA_TO_KVA1(C1FIFOUA0));

            /* Copy message data */
            if(pstMessage->fgExide == B_TRUE)
            {
               transmitMessage->stMsgSID.SID = (pstMessage->ulIdentifier & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               transmitMessage->stMsgEID.EID = (pstMessage->ulIdentifier & 0x0003FFFF);   /* copy EID part */
               transmitMessage->stMsgEID.IDE = 1;
            }
            else
            {
               transmitMessage->stMsgSID.SID = pstMessage->ulIdentifier;
               transmitMessage->stMsgEID.IDE = 0;
            }
    
            transmitMessage->stMsgEID.DLC = pstMessage->ucDataLength;
            transmitMessage->ucvData[0] = pstMessage->ucvData[0];
            transmitMessage->ucvData[1] = pstMessage->ucvData[1];
            transmitMessage->ucvData[2] = pstMessage->ucvData[2];
            transmitMessage->ucvData[3] = pstMessage->ucvData[3];
            transmitMessage->ucvData[4] = pstMessage->ucvData[4];
            transmitMessage->ucvData[5] = pstMessage->ucvData[5];
            transmitMessage->ucvData[6] = pstMessage->ucvData[6];
            transmitMessage->ucvData[7] = pstMessage->ucvData[7];

            /* Increment FIFO address */
            C1FIFOCON0SET = 0x00002000;
            
            /* Require transmission */
            C1FIFOCON0SET = 0x00000008;

            /* Request completed  */
            eResponseCode = CAN_NO_ERRORS;
         }
         else
         {
            /* FIFO buffer is full  */
            eResponseCode = CAN_TX_BUFFER_FULL;
         }

         break;
      }
      case CAN_LOW_PRIORITY:
      {
         /* If buffer is not full */
         if(C1FIFOINT1bits.TXNFULLIF == 1)
         {
            /* Get pointer of next free message of TX buffer */
            transmitMessage = (CANMessageBuffer *)(PA_TO_KVA1(C1FIFOUA1));

            /* Copy message data */
            if(pstMessage->fgExide == B_TRUE)
            {
               transmitMessage->stMsgSID.SID = (pstMessage->ulIdentifier & 0x1FFC0000) >> UL_SHIFT_18; /* copy SID part */
               transmitMessage->stMsgEID.EID = (pstMessage->ulIdentifier & 0x0003FFFF);   /* copy EID part */
               transmitMessage->stMsgEID.IDE = 1;
            }
            else
            {
               transmitMessage->stMsgSID.SID = pstMessage->ulIdentifier;
               transmitMessage->stMsgEID.IDE = 0;
            }

            transmitMessage->stMsgEID.DLC = pstMessage->ucDataLength;
            transmitMessage->ucvData[0] = pstMessage->ucvData[0];
            transmitMessage->ucvData[1] = pstMessage->ucvData[1];
            transmitMessage->ucvData[2] = pstMessage->ucvData[2];
            transmitMessage->ucvData[3] = pstMessage->ucvData[3];
            transmitMessage->ucvData[4] = pstMessage->ucvData[4];
            transmitMessage->ucvData[5] = pstMessage->ucvData[5];
            transmitMessage->ucvData[6] = pstMessage->ucvData[6];
            transmitMessage->ucvData[7] = pstMessage->ucvData[7];

            /* Increment FIFO address */
            C1FIFOCON1SET = 0x00002000;

            /* Require transmission */
            C1FIFOCON1SET = 0x00000008;

            /* Request completed  */
            eResponseCode = CAN_NO_ERRORS;
         }
         else
         {
            /* FIFO buffer is full  */
            eResponseCode = CAN_TX_BUFFER_FULL;
         }

         break;
      }
      default:
      {

      }
   }

   return eResponseCode;
}

/*DC***********************************************************************
  Detailed Conception for the function CAN_eReadReceivedMsg()
***************************************************************************
  Syntax   : <CAN_eCanResponseCodes> CAN_eReadReceivedMsg( CAN_stCanMessage *pstMessage )
  Object   : Read a message
  Parameters: CAN_stCanMessage *pstMessage
  Return  : CAN_eCanResponseCodes
  Calls   : None
***************************************************************************
 Pseudo-code:
 BEGIN
 | 
 END
**********************************************************************EDC*/
EXPORTED CAN_eCanResponseCodes CAN_eReadReceivedMsg ( CAN_stCanMessage *pstMessage )
{
   CAN_eCanResponseCodes eResponseCode;
   CANMessageBuffer * receiveMessage;

   /* If buffer is not empty */
   if(C1FIFOINT2bits.RXNEMPTYIF == 1)
   {
      /* Get pointer of next free message of TX buffer */
      receiveMessage = (CANMessageBuffer *)(PA_TO_KVA1(C1FIFOUA2));

      /* Copy message data */
      if(receiveMessage->stMsgEID.IDE == 1)
      {
         pstMessage->ulIdentifier = ((uint32)(receiveMessage->stMsgSID.SID) << UL_SHIFT_18);
         pstMessage->ulIdentifier |= receiveMessage->stMsgEID.EID;
         pstMessage->fgExide = B_TRUE;
      }
      else
      {
         pstMessage->ulIdentifier = receiveMessage->stMsgSID.SID;
         pstMessage->fgExide = B_FALSE;
      }
      if(receiveMessage->stMsgEID.RTR == 1)
      {
         pstMessage->fgRemote = B_TRUE;
      }
      else
      {
         pstMessage->fgRemote = B_FALSE;
      }
      pstMessage->ucDataLength = receiveMessage->stMsgEID.DLC;
      pstMessage->ucvData[0] = receiveMessage->ucvData[0];
      pstMessage->ucvData[1] = receiveMessage->ucvData[1];
      pstMessage->ucvData[2] = receiveMessage->ucvData[2];
      pstMessage->ucvData[3] = receiveMessage->ucvData[3];
      pstMessage->ucvData[4] = receiveMessage->ucvData[4];
      pstMessage->ucvData[5] = receiveMessage->ucvData[5];
      pstMessage->ucvData[6] = receiveMessage->ucvData[6];
      pstMessage->ucvData[7] = receiveMessage->ucvData[7];

      /* Increment FIFO address */
      C1FIFOCON2SET = 0x00002000;

      /* Request completed  */
      eResponseCode = CAN_NO_ERRORS;
   }
   else
   {
      /* FIFO buffer is empty  */
      eResponseCode = CAN_RX_BUFFER_EMPTY;
   }
   
   return eResponseCode;
}


/*DC***********************************************************************
  Detailed Conception for the function CAN_TK_PeriodicDiagnosis()
***************************************************************************
  Syntax   : <void> CAN_TK_PeriodicDiagnosis( void )
  Object   : Manage CAN module diagnosis periodically
  Parameters: None
  Return  : None
  Calls   : None
***************************************************************************
 Pseudo-code:
 BEGIN
 | 
 END
**********************************************************************EDC*/
EXPORTED void CAN_TK_PeriodicDiagnosis ( void )
{
   /* do nothing at the moment */
}


/**************************************************************************
          Local Functions
**************************************************************************/

/*DC***********************************************************************
  Detailed Conception for the function setCanConfigurationMode()
***************************************************************************
  Syntax   : <void> setCanConfigurationMode( void )
  Object   : Set CAN registers in configuration mode
  Parameters: None
  Return  : None
  Calls   : None
***************************************************************************
 Pseudo-code:
 BEGIN
 | 
 END
**********************************************************************EDC*/
LOCAL void setCanConfigurationMode( void )
{
   /* Configuration mode request */
   C1CONbits.REQOP = 4;

   /* wait configuration mode request */
   while(C1CONbits.OPMOD != 4);
}


/*DC***********************************************************************
  Detailed Conception for the function setCanNormalMode()
***************************************************************************
  Syntax   : <void> setCanNormalMode( void )
  Object   : Set CAN registers in normal mode
  Parameters: None
  Return  : None
  Calls   : None
***************************************************************************
 Pseudo-code:
 BEGIN
 | 
 END
**********************************************************************EDC*/
LOCAL void setCanNormalMode( void )
{
   /* Normal mode request */
   C1CONbits.REQOP = 0;
   
   /* wait normal mode request */
   while(C1CONbits.OPMOD != 0);
}


/******************************* END OF FILE ********************************/
