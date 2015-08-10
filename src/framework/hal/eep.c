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
 * This file epp.c represents the source file of the EEP component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


/* ----------- files inclusions ------------------- */
#include <plib.h>
#include "../fw_common.h"
#include "eep.h"

/*************************************************************************
 **                   Other files included                               **
 **************************************************************************/

/*************************************************************************
 **                   Declaration of local constants                     **
 **************************************************************************/

//#define I2C_CLK_FREQUENCY_400KHZ
#define I2C_CLK_FREQUENCY_100KHZ

/* EEPROM physical page size in bytes */
#define US_EEPROM_PHYS_PAGE_SIZE             US_16

/* I2C device addresses definitions */
#define UC_I2C_DEV_ADDR_EEP_CTRL_BLK         (uchar)(0x50 << UC_SHIFT_1)

#if I2C_CLK_FREQUENCY_400KHZ
/* Baud rate register value for 400 KHz */
#define US_I2C_BAUD_RATE_VALUE               ((ushort)(0x002C))
#else
/* Baud rate register value for 100 KHz or else */
#define US_I2C_BAUD_RATE_VALUE               ((ushort)(0x00C2))
#endif

/*************************************************************************
 **                   Declaration of local macros                        **
 **************************************************************************/
/* I2CxCON register bits position */
#define I2CxSTAT_ACKSTAT_BIT_POS            15

/* I2CxCON register bits position */
#define I2CxCON_ON_BIT_POS                  15
#define I2CxCON_FRZ_BIT_POS                 14
#define I2CxCON_SIDL_BIT_POS                13
#define I2CxCON_SCLREL_BIT_POS              12
#define I2CxCON_STRICT_BIT_POS              11
#define I2CxCON_A10M_BIT_POS                10
#define I2CxCON_DISSLW_BIT_POS              9
#define I2CxCON_SMEN_BIT_POS                8
#define I2CxCON_GCEN_BIT_POS                7
#define I2CxCON_STREN_BIT_POS               6
#define I2CxCON_ACKDT_BIT_POS               5
#define I2CxCON_ACKEN_BIT_POS               4
#define I2CxCON_RCEN_BIT_POS                3
#define I2CxCON_PEN_BIT_POS                 2
#define I2CxCON_RSEN_BIT_POS                1
#define I2CxCON_SEN_BIT_POS                 0

/* I2CxCON register bits values */
#define I2CxCON_ON_VALUE                    0
#define I2CxCON_FRZ_VALUE                   0
#define I2CxCON_SIDL_VALUE                  0
#define I2CxCON_SCLREL_VALUE                0
#define I2CxCON_STRICT_VALUE                0
#define I2CxCON_A10M_VALUE                  0
#define I2CxCON_DISSLW_VALUE                1
#define I2CxCON_SMEN_VALUE                  0
#define I2CxCON_GCEN_VALUE                  0
#define I2CxCON_STREN_VALUE                 0
#define I2CxCON_ACKDT_VALUE                 0
#define I2CxCON_ACKEN_VALUE                 0
#define I2CxCON_RCEN_VALUE                  0
#define I2CxCON_PEN_VALUE                   0
#define I2CxCON_RSEN_VALUE                  0
#define I2CxCON_SEN_VALUE                   0

/* I2CxCON register value */
#define I2CxCON_REG_VALUE                   ((I2CxCON_ON_VALUE << I2CxCON_ON_BIT_POS)           | \
                                             (I2CxCON_FRZ_VALUE << I2CxCON_FRZ_BIT_POS)         | \
                                             (I2CxCON_SIDL_VALUE << I2CxCON_SIDL_BIT_POS)	| \
                                             (I2CxCON_SCLREL_VALUE << I2CxCON_SCLREL_BIT_POS)	| \
                                             (I2CxCON_STRICT_VALUE << I2CxCON_STRICT_BIT_POS)	| \
                                             (I2CxCON_A10M_VALUE << I2CxCON_A10M_BIT_POS)	| \
                                             (I2CxCON_DISSLW_VALUE << I2CxCON_DISSLW_BIT_POS)	| \
                                             (I2CxCON_SMEN_VALUE << I2CxCON_SMEN_BIT_POS)	| \
                                             (I2CxCON_GCEN_VALUE << I2CxCON_GCEN_BIT_POS)	| \
                                             (I2CxCON_STREN_VALUE << I2CxCON_STREN_BIT_POS)	| \
                                             (I2CxCON_ACKDT_VALUE << I2CxCON_ACKDT_BIT_POS)	| \
                                             (I2CxCON_ACKEN_VALUE << I2CxCON_ACKEN_BIT_POS)	| \
                                             (I2CxCON_RCEN_VALUE << I2CxCON_RCEN_BIT_POS)	| \
                                             (I2CxCON_PEN_VALUE << I2CxCON_PEN_BIT_POS)         | \
                                             (I2CxCON_RSEN_VALUE << I2CxCON_RSEN_BIT_POS)	| \
                                             (I2CxCON_SEN_VALUE << I2CxCON_SEN_BIT_POS)           )

/* I2C macros */
#define ENABLE_I2C_MODULE()                  (I2C1CONSET = (1 << I2CxCON_ON_BIT_POS))
#define SEND_START_CONDITION()               (I2C1CONSET = (1 << I2CxCON_SEN_BIT_POS))
#define SEND_RESTART_CONDITION()             (I2C1CONSET = (1 << I2CxCON_RSEN_BIT_POS))
#define SEND_STOP_CONDITION()                (I2C1CONSET = (1 << I2CxCON_PEN_BIT_POS))
#define ACK_FROM_SLAVE_RECEIVED()            (I2C1STAT & (1 << I2CxSTAT_ACKSTAT_BIT_POS) == 0)
#define ENABLE_I2C_RECEPTION()               (I2C1CONSET = (1 << I2CxCON_RCEN_BIT_POS))
#define PREPARE_NACK_TO_SLAVE()              (I2C1CONSET = (1 << I2CxCON_ACKDT_BIT_POS))
#define PREPARE_ACK_TO_SLAVE()               (I2C1CONCLR = (1 << I2CxCON_ACKDT_BIT_POS))
#define SEND_ACK_TO_SLAVE()                  (I2C1CONSET = (1 << I2CxCON_ACKEN_BIT_POS))


/* I2C1 Master interrupt macros */
#define I2C1E_POS                       31
#define I2C1F_POS                       31
#define I2C1_PRI_POS                    10
#define I2C1_SUBPRI_POS                 8
#define ENABLE_I2C1_INT()               (IEC0SET = (1 << I2C1E_POS))
#define DISABLE_I2C1_INT()              (IEC0CLR = (1 << I2C1E_POS))
#define CLEAR_I2C1_INT_FLAG()           (IFS0CLR = (1 << I2C1F_POS))
#define SET_I2C1_PRIORITY(x)            (IPC6SET = (x << I2C1_PRI_POS))
#define SET_I2C1_SUBPRIORITY(x)         (IPC6SET = (x << I2C1_SUBPRI_POS))
#define CLEAR_I2C1_PRIORITY()           (IPC6CLR = ((7 << I2C1_PRI_POS) | (3 << I2C1_SUBPRI_POS)))

/* I2C1 Bus Collision interrupt macros */
#define I2C1BCE_POS                       29
#define I2C1BCF_POS                       29
#define ENABLE_I2C1BC_INT()               (IEC0SET = (1 << I2C1BCE_POS))
#define DISABLE_I2C1BC_INT()              (IEC0CLR = (1 << I2C1BCE_POS))
#define CLEAR_I2C1BC_INT_FLAG()           (IFS0CLR = (1 << I2C1BCF_POS))


/*************************************************************************
 **                   Declaration of local types                         **
 **************************************************************************/

/* EEPROM state */
typedef enum
{
   KE_EEPROM_STATE_IDLE
  ,KE_EEPROM_STATE_READ
  ,KE_EEPROM_STATE_WAIT_READ
  ,KE_EEPROM_STATE_WRITE
  ,KE_EEPROM_STATE_WAIT_WRITE
  ,KE_EEPROM_STATE_WAIT_STATUS
} ke_eeprom_state;

/* EEPROM operation */
typedef enum
{
   KE_EEP_OP_REQUIRED_NONE
  ,KE_EEP_OP_REQUIRED_READ
  ,KE_EEP_OP_REQUIRED_WRITE
} ke_eep_operation;

/* Physical EEPROM status */
typedef enum
{
   KE_PHYS_EEPROM_STATUS_NOT_UPDATED
  ,KE_PHYS_EEPROM_STATUS_FREE
} ke_phys_eep_status;

/* I2C operation status */
typedef enum
{
   I2C_KE_OP_UNDEFINED           /* Only at Init state */
  ,I2C_KE_OP_IN_PROGRESS
  ,I2C_KE_OP_FINISHED_SUCCESS
  ,I2C_KE_OP_ERROR_GENERIC
  ,I2C_KE_OP_ERROR_TIMEOUT
  ,I2C_KE_OP_ERROR_MISSING_ACK
  ,I2C_KE_OP_INTERNAL_ERROR
} ke_i2c_op_status;

/* I2C FSM status */
typedef enum
{
   KE_EEP_FIRST_STATUS
  ,KE_EEP_IDLE_STATUS = KE_EEP_FIRST_STATUS
  ,KE_EEP_START_CONDITION_EXECUTED
  ,KE_EEP_ADDR_SENT_1
  ,KE_EEP_ADDR_SENT_2
  ,KE_EEP_WORD_ADDR_SENT
  ,KE_EEP_RESTART_SENT
  ,KE_EEP_RECEIVING_DATA
  ,KE_EEP_TRANSMITTING_DATA
  ,KE_EEP_ACK_TO_SLAVE_SENT
  ,KE_EEP_DATA_RECEIVED
  ,KE_EEP_STOP_CONDITION_EXECUTED
  ,KE_EEP_LAST_STATUS
} ke_i2c_fsm_status;

/* I2C operation type */
typedef enum
{
   KE_OPERATION_READ    /* request to read from device --> device shall write to bus  */
  ,KE_OPERATION_WRITE   /* request to write to device -->  device shall read from bus */
} ke_op_type;

/* EEPROM request info */
typedef struct
{
   ushort            usAddress;        /* EEPROM address for required operation */
   ushort            usByteNum;        /* Number of byte to read/write          */
   ushort            usWrittenBytes;   /* Number of bytes already written in current operation */
   ke_eep_operation  eEepOperation;    /* operation type: read/write/none       */
} st_eeprom_request_info;

/* I2C request info */
typedef struct
{
   ke_op_type  eOperationType;   /* read or write from device        */
   uchar       ucDeviceRegPtr;   /* device register pointer          */
   ushort      usEepAddress;     /* target address (only for EEPROM) */
   ushort      usByteNum;        /* number of byte to read/write     */
   ushort      usProcessedBytes; /* number of bytes currently R/W    */
} st_i2c_request_info;

/*DC**********************************************************************
**                   Data Declaration                                   **
*************************************************************************/


/**************************************
 * 1. Declaration of LOCAL variables  *
 **************************************/

/*************************************************************************
Syntax: LOCAL ke_eeprom_state eEepromState
Object: store actual state of EEPROM state machine.
        Do NOT use this variable to inform other modules about EEPROM
        status. Instead use EEP_eEepromState for this task.
Unit :  enum
Range:  see ke_eeprom_state typedef
**************************************************************************/
LOCAL ke_eeprom_state eEepromState;

/*************************************************************************
Syntax: LOCAL ke_phys_eep_status ePhysEepromStatus
Object: store information about EEPROM physical device status.
        It shall be set to NOT_UPDATED before being tested, and tested
        against values different from NOT_UPDATED later on.
Unit :  enum
Range:  see ke_phys_eep_status typedef
**************************************************************************/
LOCAL ke_phys_eep_status ePhysEepromStatus;

/*************************************************************************
Syntax: LOCAL st_EepromRequestInfo stEepromRequestInfo
Object: This variable contains information on last request forwarded to
        EEPROM module.
Unit :  None
Range:  see st_eeprom_request_info
**************************************************************************/
LOCAL st_eeprom_request_info stEepromRequestInfo;

/*************************************************************************
Syntax:  LOCAL st_i2c_request_info stRequestInfo
Object:  This variable contains information on last request forwarded to
         I2C module.
Fields:  see st_i2c_request_info typedef
**************************************************************************/
LOCAL st_i2c_request_info stI2CRequestInfo;

/*************************************************************************
Syntax:  ke_i2c_fsm_status eI2CFSMStatus
Object:  This variable contains I2C FSM status.
Fields:  KE_EEP_FIRST_STATUS - KE_EEP_LAST_STATUS
**************************************************************************/
LOCAL ke_i2c_fsm_status eI2CFSMStatus;

/*************************************************************************
Syntax:  I2C_ke_op_status aeOperationStatus
Object:  This variable represents the actual operation status
Fields:
**************************************************************************/
LOCAL ke_i2c_op_status aeI2COperationStatus;

/*************************************************************************
Syntax:  ushort usEepromBufferPointer
Object:  This variable represents the index of buffer of EEPROM caller
Fields:
**************************************************************************/
LOCAL ushort usEepromBufferPointer = US_NULL;


/****************************************
 * 2. Declaration of LOCAL constant data *
 *****************************************/


/*****************************************
 * 3. Declaration of EXPORTED variables   *
 *****************************************/

/*************************************************************************
Syntax: EXPORTED EEP_ke_eeprom_state EEP_eEepromState
Object: store actual macro state of EEPROM state machine for external
        diagnostic request. Auxiliary state such as WAIT_WRITE, WAIT_READ
        and similar are not available, and are managed as main state.
        See typedef for details.
Unit :  enum
Range:  see EEP_ke_eeprom_state typedef
**************************************************************************/
EXPORTED EEP_ke_eeprom_state EEP_eEepromState;

/*************************************************************************
Syntax: EXPORTED uchar EEP_aucEepromTXBuffer[EEP_UC_EEPROM_BUFFER_LEN]
Object: buffer containing data to be written in eeprom
Unit :  array of uchar
Range:  0x00 - 0xFF for each element
**************************************************************************/
EXPORTED uchar EEP_aucEepromTXBuffer[EEP_UC_EEPROM_BUFFER_LEN];

/*************************************************************************
Syntax: EXPORTED uchar EEP_aucEepromRXBuffer[EEP_UC_EEPROM_BUFFER_LEN]
Object: buffer containing data to be read from eeprom
Unit :  array of uchar
Range:  0x00 - 0xFF for each element
**************************************************************************/
EXPORTED uchar EEP_aucEepromRXBuffer[EEP_UC_EEPROM_BUFFER_LEN];


/*********************************************
 * 4. Declaration of EXPORTED constant data   *
 **********************************************/


/**************************************************************************
         Local functions prototypes
**************************************************************************/
LOCAL void I2CInitialise   (void);
LOCAL void I2CReadData     (ushort, ushort);
LOCAL void I2CWriteData    (ushort, ushort);

/**************************************************************************
        Exported Functions
**************************************************************************/

/*DC***********************************************************************
 ** Detailed Conception for the function EEP_Initialise                  **
 **************************************************************************
 Syntax    : EXPORTED void EEP_Initialise (void)
 Object    : Initialise all EEPROM module variables
 Parameters: None
 Return    : None
 Calls     : None
 **********************************************************************EDC*/
EXPORTED void EEP_Initialise(void)
{
    /* init I2C */
    I2CInitialise();

    /* initialise EEPROM STM */
    eEepromState = KE_EEPROM_STATE_IDLE;

    /* initialise EEPROM state for SMB usage */
    EEP_eEepromState = EEP_KE_EEPROM_STATE_IDLE;

    /* initialise physical EEPROM (device) status */
    ePhysEepromStatus = KE_PHYS_EEPROM_STATUS_NOT_UPDATED;

    /* reset Eeprom buffer pointer living in I2C module */
    usEepromBufferPointer = US_NULL;

    /* initialise EEPROM operation request info */
    stEepromRequestInfo.eEepOperation = KE_EEP_OP_REQUIRED_NONE;
    stEepromRequestInfo.usAddress = US_NULL;
    stEepromRequestInfo.usByteNum = US_NULL;
    stEepromRequestInfo.usWrittenBytes = US_NULL;
}


/*DC***********************************************************************
 ** Detailed Conception for the function EEP_eReadEeprom                 **
 **************************************************************************
 Syntax    : EXPORTED EEP_ke_req_status EEP_ReadEeprom (ushort usAddress,
                                                        ushort usByteNum)
 Object    : Interface for reading from EEPROM. Function checks if EEPROM
             can accept the request according to its actual state.
             If yes, stores request data in a dedicated structure and start
             the EEPROM state machine which will manage the read operation to
             the end.
 Parameters: usAddress: EEPROM address from where start reading
             usByteNum: number of bytes to read
 Return    : see EEP_ke_req_status type
 Calls     : I2C_ResetBufferPtr
 **********************************************************************EDC*/
EXPORTED EEP_ke_req_status EEP_eReadEeprom(ushort usAddress, ushort usByteNum)
{
    EEP_ke_req_status eResult = EEP_KE_REQUEST_REJECTED;

    /* check if EEPROM is in idle state !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    stEepromRequestInfo.usAddress = usAddress;
    stEepromRequestInfo.usByteNum = usByteNum;
    stEepromRequestInfo.eEepOperation = KE_EEP_OP_REQUIRED_READ;

    /* reset Eeprom buffer pointer living in I2C module */
    usEepromBufferPointer = US_NULL;

    /* prepare positive response */
    eResult = EEP_KE_REQUEST_ACCEPTED;

    return eResult;
}


/*DC***********************************************************************
 ** Detailed Conception for the function EEP_eWriteEeprom                **
 **************************************************************************
 Syntax    : EXPORTED EEP_ke_req_status EEP_WriteEeprom (ushort usAddress,
                                                         ushort usByteNum)
 Object    : Interface for writing to EEPROM. Function checks if EEPROM
             can accept the request according to its actual state.
             If yes, stores request data in a dedicated structure and start
             the EEPROM state machine which will manage the write operation
             to the end.
 Parameters: usAddress: EEPROM address to start writing to
             usByteNum: number of bytes to write
 Return    : see EEP_ke_req_status type
 Calls     : I2C_ResetBufferPtr
 **********************************************************************EDC*/
EXPORTED EEP_ke_req_status EEP_eWriteEeprom(ushort usAddress, ushort usByteNum)
{
    EEP_ke_req_status eResult = EEP_KE_REQUEST_REJECTED;

    /* check if EEPROM is in idle state !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    stEepromRequestInfo.usAddress = usAddress;
    stEepromRequestInfo.usByteNum = usByteNum;
    stEepromRequestInfo.eEepOperation = KE_EEP_OP_REQUIRED_WRITE;

    /* reset Eeprom buffer pointer living in I2C module */
    usEepromBufferPointer = US_NULL;

    /* prepare positive response */
    eResult = EEP_KE_REQUEST_ACCEPTED;

    return eResult;
}


/*DC***********************************************************************
 ** Detailed Conception for the function EEP_TK_PeriodicManagement       **
 **************************************************************************
 Syntax    : EXPORTED void EEP_TK_PeriodicManagement (void)
 Object    : Manages EEPROM state machine. See software design for details.
 Parameters: None
 Return    : None
 Calls     : TBC
 **********************************************************************EDC*/
EXPORTED void EEP_TK_PeriodicManagement(void)
{
    ke_i2c_op_status eI2COpStatus = I2C_KE_OP_UNDEFINED;
    ushort usStartingPage = US_NULL;
    ushort usEndingPage = US_NULL;
    ushort usEndingAddress = US_NULL;
    ushort usCurrentAddress = US_NULL;
    ushort usRemainingBytes = US_NULL;
    ushort usTemp = US_NULL;

    switch(eEepromState)
    {
        case KE_EEPROM_STATE_IDLE:
        {
            /* check for pending READ request */
            if (stEepromRequestInfo.eEepOperation == KE_EEP_OP_REQUIRED_READ)
            {
                /* set EEPROM FSM state to READ */
                eEepromState = KE_EEPROM_STATE_READ;

                /* set EEPROM status to READ */
                EEP_eEepromState = EEP_KE_EEPROM_STATE_READ;
            }
            else
            {
                /* check for pending WRITE request */
                if (stEepromRequestInfo.eEepOperation == KE_EEP_OP_REQUIRED_WRITE)
                {
                    /* set EEPROM FSM state to WRITE */
                    eEepromState = KE_EEPROM_STATE_WRITE;

                    /* set EEPROM status to WRITE */
                    EEP_eEepromState = EEP_KE_EEPROM_STATE_WRITE;
                }
                else
                {
                    /* no pending request; nothing to do */
                }
            }
            break;
        }
        case KE_EEPROM_STATE_READ:
        {
            /* check if an I2C operation is not in progress */
            if(aeI2COperationStatus != I2C_KE_OP_IN_PROGRESS)
            {
                /* send read request to low level */
                I2CReadData(stEepromRequestInfo.usAddress, stEepromRequestInfo.usByteNum);

                /* set EEPROM FSM state to WAIT_READ */
                eEepromState = KE_EEPROM_STATE_WAIT_READ;
            }
            else
            {
                /* wait */
            }
         
            break;
        }
        case KE_EEPROM_STATE_WRITE:
        {
            /* check if an I2C operation is not in progress */
            if(aeI2COperationStatus != I2C_KE_OP_IN_PROGRESS)
            {
                /* update starting address considering bytes already written */
                usCurrentAddress = stEepromRequestInfo.usAddress + stEepromRequestInfo.usWrittenBytes;

                /* update remaining bytes counter */
                usRemainingBytes = stEepromRequestInfo.usByteNum - stEepromRequestInfo.usWrittenBytes;

                /* check if bytes to be written cross EEPROM page boundaries */
                usEndingAddress = usCurrentAddress + usRemainingBytes - US_1;
                usStartingPage = usCurrentAddress / US_EEPROM_PHYS_PAGE_SIZE;
                usEndingPage   = usEndingAddress / US_EEPROM_PHYS_PAGE_SIZE;
                if (usStartingPage == usEndingPage)
                {
                    /* no page boundaries crossing detected */

                    /* send whole EEPROM write request      */
                    I2CWriteData(usCurrentAddress, usRemainingBytes);

                    /* update number of written bytes */
                    stEepromRequestInfo.usWrittenBytes += usRemainingBytes;
                }
                else
                {
                    /* page boundaries crossing detected */
                    /* calculate number of bytes up to page bound */
                    usTemp = ((usStartingPage + US_1) * US_EEPROM_PHYS_PAGE_SIZE) - usCurrentAddress;

                    /* send partial EEPROM write request */
                    I2CWriteData(usCurrentAddress, usTemp);

                    /* update number of written bytes */
                    stEepromRequestInfo.usWrittenBytes += usTemp;
                }

                /* set EEPROM FSM state to WAIT_WRITE */
                eEepromState = KE_EEPROM_STATE_WAIT_WRITE;
            }
            else
            {
                /* wait */
            }
        
            break;
        }
        case KE_EEPROM_STATE_WAIT_READ:
        {
            /* check I2C bus status */
            if(aeI2COperationStatus == I2C_KE_OP_IN_PROGRESS)
            {
                /* wait  */
            }
            else if (aeI2COperationStatus == I2C_KE_OP_FINISHED_SUCCESS)
            {
                /* set EEPROM FSM state to IDLE */
                eEepromState = KE_EEPROM_STATE_IDLE;

                /* set EEPROM status to IDLE */
                EEP_eEepromState = EEP_KE_EEPROM_STATE_IDLE;

                /* manage data here if necessary !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

                /* DO NOT INVERT FOLLOWING TWO INSTRUCTIONS     */

                /* notify operation end here if necessary !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

                /* reset required operation */
                stEepromRequestInfo.eEepOperation = KE_EEP_OP_REQUIRED_NONE;

            }
            else if(eI2COpStatus == I2C_KE_OP_ERROR_TIMEOUT)
            {

            }
            else if(eI2COpStatus == I2C_KE_OP_ERROR_MISSING_ACK)
            {

            }
            else if(eI2COpStatus == I2C_KE_OP_ERROR_GENERIC)
            {

            }
            else
            {
                /* do nothing */
            }
         
            break;
        }
        case KE_EEPROM_STATE_WAIT_WRITE:
        {
            /* set physical EEPROM status to NOT UPDATED */
            ePhysEepromStatus = KE_PHYS_EEPROM_STATUS_NOT_UPDATED;

            /* set EEPROM FSM state to WAIT STATUS */
            eEepromState = KE_EEPROM_STATE_WAIT_STATUS;

            break;
        }
        case KE_EEPROM_STATE_WAIT_STATUS:
        {
            /* updat physical EEPROM status and check for FREE */
            ePhysEepromStatus = KE_PHYS_EEPROM_STATUS_FREE;

            if (ePhysEepromStatus == KE_PHYS_EEPROM_STATUS_FREE)
            {
                /* check for other bytes to write */
                if (stEepromRequestInfo.usWrittenBytes < stEepromRequestInfo.usByteNum)
                {
                    /* set EEPROM FSM state to WRITE */
                    eEepromState = KE_EEPROM_STATE_WRITE;
                }
                else
                {
                    /* no further bytes to write; return to IDLE state */
                    eEepromState = KE_EEPROM_STATE_IDLE;

                    /* set EEPROM status to IDLE */
                    EEP_eEepromState = EEP_KE_EEPROM_STATE_IDLE;

                    /* DO NOT INVERT FOLLOWING TWO INSTRUCTIONS     */

                    /* notify operation end here if necessary !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

                    /* reset required operation */
                    stEepromRequestInfo.eEepOperation = KE_EEP_OP_REQUIRED_NONE;
                }
            }
            else
            {
                /* nothing to do, wait for status update */
            }
            break;
        }
        default:
        {
            /* should never arrive here */
            break;
        }
    }
}


/**************************************************************************
         Local Functions
**************************************************************************/

/*DC***********************************************************************
** Detailed Conception for the function I2CInitialise                    **
**************************************************************************
Syntax    :  void I2CInitialise(void)
Object    :  This function initialises I2C.
Parameters:  None
Return    :  None
Calls     :  None
**********************************************************************EDC*/
LOCAL void I2CInitialise (void)
{
    DISABLE_I2C1_INT();
    DISABLE_I2C1BC_INT();

    /* init I2C1CON register */
    I2C1CON = I2CxCON_REG_VALUE;

    /* set baud rate */
    I2C1BRG = US_I2C_BAUD_RATE_VALUE;

    /* enable I2C module */
    ENABLE_I2C_MODULE();

    /* clear int flags */
    CLEAR_I2C1_INT_FLAG();
    CLEAR_I2C1BC_INT_FLAG();

    /* set priority */
    CLEAR_I2C1_PRIORITY();
    SET_I2C1_PRIORITY(3);
    SET_I2C1_SUBPRIORITY(0);

    /* enable master and bus collision int */
    ENABLE_I2C1_INT();
    ENABLE_I2C1BC_INT();

    /* init FSMs status */
    eI2CFSMStatus = KE_EEP_IDLE_STATUS;

    /* init operation status */
    aeI2COperationStatus = I2C_KE_OP_UNDEFINED;
}


/*DC***********************************************************************
** Detailed Conception for the function I2CReadData                      **
**************************************************************************
Syntax    : void I2CReadData (ushort usAddress, ushort usByteNum)
Object    : Start an EEPROM read operation on I2C bus. The operation end
            will be notified through I2C ISR.
            Before calling this function, calling module shall have
            previously conquered the bus through the interface
            I2C_ConquerBus, as stRequestInfo structure is updated by this
            interface without further checks
Parameters: usAddress: starting eeprom address to read from
            usByteNum: number of bytes to read
Return    : None
Calls     : SEND_START_CONDITION() macro
**********************************************************************EDC*/
LOCAL void I2CReadData(ushort usAddress, ushort usByteNum)
{
    /* save request params and complete them adding implicit params */
    /* save implicit params */
    stI2CRequestInfo.eOperationType = KE_OPERATION_READ;

    /* save explicit params */
    stI2CRequestInfo.usByteNum = usByteNum;
    stI2CRequestInfo.usEepAddress = usAddress;

    /* Start condition executed */
    eI2CFSMStatus = KE_EEP_START_CONDITION_EXECUTED;

    /* update operation status */
    aeI2COperationStatus = I2C_KE_OP_IN_PROGRESS;

    /* start I2C operations */
    SEND_START_CONDITION();
}


/*DC***********************************************************************
** Detailed Conception for the function I2CWriteData                     **
**************************************************************************
Syntax    : void I2CWriteData (ushort usAddress, ushort usByteNum)
Object    : Start an EEPROM write operation on I2C bus. The operation end
            will be notified through I2C ISR.
Parameters: usAddress: starting eeprom address to write to
            usByteNum: number of bytes to write
Return    : None
Calls     : SEND_START_CONDITION() macro
**********************************************************************EDC*/
LOCAL void I2CWriteData(ushort usAddress, ushort usByteNum)
{
    /* save request params and complete them adding implicit params */
    /* save implicit params */
    stI2CRequestInfo.eOperationType = KE_OPERATION_WRITE;

    /* save explicit params */
    stI2CRequestInfo.usByteNum = usByteNum;
    stI2CRequestInfo.usEepAddress = usAddress;

    /* clear index of processed bytes */
    stI2CRequestInfo.usProcessedBytes = US_NULL;

    /* start condition executed */
    eI2CFSMStatus = KE_EEP_START_CONDITION_EXECUTED;

    /* update operation status */
    aeI2COperationStatus = I2C_KE_OP_IN_PROGRESS;

    /* start I2C operations */
    SEND_START_CONDITION();
}


/*DC***********************************************************************
** Detailed Conception for the function I2C1Handler                     **
**************************************************************************
Syntax    :  void I2C1Handler(void)
Object    :  This function is the ISR of the I2C master mode events.
Parameters:  None
Return    :  None
Calls     :  None
**********************************************************************EDC*/
void __ISR(_I2C_1_VECTOR, ipl3) I2C1Handler(void)
{
    // TODO: check bus collision error

    /* clear interrupt flag */
    CLEAR_I2C1_INT_FLAG();

    uchar ucTempByte;

    switch (eI2CFSMStatus)
    {
        /* Interrupt after start condition */
        case KE_EEP_START_CONDITION_EXECUTED:
        {
            /* EEPROM page write and random read, used in this project,     */
            /* have the same start of the request, so no difference is made */
            /* until after sending word address                             */
            /* refer to datasheet for details       */

            /* first 4 MSB are fixed (control block) */
            ucTempByte = UC_I2C_DEV_ADDR_EEP_CTRL_BLK;

            /* next 3 bits shall be taken from the target address (MSB) */
            ucTempByte |= (uchar)((stI2CRequestInfo.usEepAddress >> US_SHIFT_7) & 0x0E);

            /* R/W bit shall be 0 for the first address sending */
            /* it has already been reset by previous operation, (assuming it was clear) */

            /* set state for next interrupt  */
            eI2CFSMStatus = KE_EEP_ADDR_SENT_1;

            /* send prepared byte */
            I2C1ATRN = ucTempByte;

            break;
        }

        /* Interrupt after address transmission, eeprom read or write, first sending */
        case KE_EEP_ADDR_SENT_1:
        {
            /* Check ACK from slave */
            if (ACK_FROM_SLAVE_RECEIVED())
            {
                /* send word address (last 8-bit of the address) */
                ucTempByte = (uchar)stI2CRequestInfo.usEepAddress;

                /* set state for next interrupt  */
                eI2CFSMStatus = KE_EEP_WORD_ADDR_SENT;

                /* send prepared byte */
                I2C1ATRN = ucTempByte;
            }
            else
            {
                aeI2COperationStatus = I2C_KE_OP_ERROR_MISSING_ACK;

                /* ack not received. Not yet managed. Abort communication */
                eI2CFSMStatus = KE_EEP_STOP_CONDITION_EXECUTED;

                SEND_STOP_CONDITION();
            }

            break;
        }

        /* Interrupt after word address transmission in an EEPROM read or write operation */
        case KE_EEP_WORD_ADDR_SENT:
        {
            /* this state is only for EEPROM as caller, so eCaller switch/case is not needed */
            /* Check ACK from slave */
            if (ACK_FROM_SLAVE_RECEIVED())
            {
                /* now it's time to check if requested operation is a READ or a WRITE */
                if (stI2CRequestInfo.eOperationType == KE_OPERATION_READ)
                {
                    /* set state for next interrupt  */
                    eI2CFSMStatus = KE_EEP_RESTART_SENT;

                    /* send restart condition */
                    SEND_RESTART_CONDITION();
                }
                else if (stI2CRequestInfo.eOperationType == KE_OPERATION_WRITE)
                {
                    /* set state for next interrupt  */
                    eI2CFSMStatus = KE_EEP_TRANSMITTING_DATA;

                    /* send first byte to write */
                    I2C1ATRN = EEP_aucEepromTXBuffer[usEepromBufferPointer];
                    usEepromBufferPointer++;
                    stI2CRequestInfo.usProcessedBytes++;
                }
                else
                {
                    /* unknown operation type; should never arrive here */
                    /* abort communication */
                    eI2CFSMStatus = KE_EEP_STOP_CONDITION_EXECUTED;

                    SEND_STOP_CONDITION();
                }
            }
            else
            {
                aeI2COperationStatus = I2C_KE_OP_ERROR_MISSING_ACK;

                /* abort communication */
                eI2CFSMStatus = KE_EEP_STOP_CONDITION_EXECUTED;

                SEND_STOP_CONDITION();
            }

            break;
        }

        /* Interrupt after restart transmission in an EEPROM read operation */
        case KE_EEP_RESTART_SENT:
        {
            /* EEPROM restart is only for reading, so operation switch/case is not needed */
            /* send control byte for the second time */

            /* first 4 MSB are fixed (control block) */
            ucTempByte = UC_I2C_DEV_ADDR_EEP_CTRL_BLK;

            /* next 3 bits shall be taken from the target address (MSB) */
            ucTempByte |= (uchar)((stI2CRequestInfo.usEepAddress >> US_SHIFT_7) & 0x0E);

            /* R/W bit shall be 1 for the second address sending */
            ucTempByte |= 0x01;

            /* set state for next interrupt  */
            eI2CFSMStatus = KE_EEP_ADDR_SENT_2;

            /* send prepared byte */
            I2C1ATRN = ucTempByte;

            break;
        }

        /* Interrupt after second-time address transmission in an EEPROM read operation */
        case KE_EEP_ADDR_SENT_2:
        {
            /* this state is necessary to check ack from slave before start receiving read data */
            if (ACK_FROM_SLAVE_RECEIVED())
            {
                /* check for length of data to read */
                if (stI2CRequestInfo.usByteNum > US_NULL)
                {
                    /* start clock to receive first data byte */
                    ENABLE_I2C_RECEPTION();

                    /* set state for next interrupt */
                    eI2CFSMStatus = KE_EEP_RECEIVING_DATA;
                }
                else
                {
                    /* read finished (but never started!) Anyway, issue stop condition */
                    SEND_STOP_CONDITION();

                    eI2CFSMStatus = KE_EEP_STOP_CONDITION_EXECUTED;
                }
            }
            else
            {
                aeI2COperationStatus = I2C_KE_OP_ERROR_MISSING_ACK;

                /* abort communication */
                eI2CFSMStatus = KE_EEP_STOP_CONDITION_EXECUTED;

                SEND_STOP_CONDITION();
            }

            break;
        }


        /* Interrupt after byte reception in an EEPROM read operation */
        case KE_EEP_RECEIVING_DATA:
        {
            /* copy temperature sensor register data to dedicated buffer */
            EEP_aucEepromRXBuffer[usEepromBufferPointer] = I2C1ARCV;

            /* prepare next location for possible further receiving byte */
            usEepromBufferPointer++;

            if (usEepromBufferPointer == stI2CRequestInfo.usByteNum)
            {
                /* last byte, prepare a NACK to slave */
                PREPARE_NACK_TO_SLAVE();

                /* All data bytes are received */
                eI2CFSMStatus = KE_EEP_DATA_RECEIVED;
            }
            else
            {
                /* there are other bytes to read, prepare ACK to slave */
                PREPARE_ACK_TO_SLAVE();

                /* set state for next interrupt */
                eI2CFSMStatus = KE_EEP_ACK_TO_SLAVE_SENT;
            }

            SEND_ACK_TO_SLAVE();

            break;
        }

        /* Interrupt after byte transmission */
        case KE_EEP_TRANSMITTING_DATA:
        {
            /* Check ACK from slave */
            if (ACK_FROM_SLAVE_RECEIVED())
            {
                /* Transmit data byte */
                if (stI2CRequestInfo.usProcessedBytes < stI2CRequestInfo.usByteNum)
                {
                    /* there are other bytes to send */
                    I2C1ATRN = EEP_aucEepromTXBuffer[usEepromBufferPointer];
                    usEepromBufferPointer++;
                    stI2CRequestInfo.usProcessedBytes++;

                    /* Continue to send data bytes */
                    eI2CFSMStatus = KE_EEP_TRANSMITTING_DATA;
                }
                else
                {
                    /* All data bytes has been transmitted */
                    SEND_STOP_CONDITION();

                    eI2CFSMStatus = KE_EEP_STOP_CONDITION_EXECUTED;
                }
            }
            else
            {
                aeI2COperationStatus = I2C_KE_OP_ERROR_MISSING_ACK;

                /* ACK not received from slave; communication error; stop communication */
                SEND_STOP_CONDITION();

                eI2CFSMStatus = KE_EEP_STOP_CONDITION_EXECUTED;
            }

            break;
        }

        /* Interrupt after ACK sent to slave in an EEPROM operation */
        case KE_EEP_ACK_TO_SLAVE_SENT:
        {
            /* Enable i2c reception */
            ENABLE_I2C_RECEPTION();

            /* data reception */
            eI2CFSMStatus = KE_EEP_RECEIVING_DATA;

            break;
        }

        /* Interrupt after last byte reception */
        case KE_EEP_DATA_RECEIVED:
        {
             eI2CFSMStatus = KE_EEP_STOP_CONDITION_EXECUTED;

             /* if notification to the caller is needed, here is the right place to do it  */

             /* Stop communication MUST be sent before exit to avoid next interrupt loss   */
             SEND_STOP_CONDITION();

             break;
        }

        /* Interrupt after stop condition */
        case KE_EEP_STOP_CONDITION_EXECUTED:
        {
            if(aeI2COperationStatus == I2C_KE_OP_IN_PROGRESS)
            {
                aeI2COperationStatus = I2C_KE_OP_FINISHED_SUCCESS;
            }
            else
            {
                /* Operation failed, do nothing;                       */
                /* Operation Status already updated in previous states */
            }

            /* End of communication. Go in IDLE status */
            eI2CFSMStatus = KE_EEP_IDLE_STATUS;

            break;
        }

        /* Unknown interrupt */
        case KE_EEP_IDLE_STATUS:
        {
            /* If here caller didn't set any status */
            aeI2COperationStatus = I2C_KE_OP_INTERNAL_ERROR;

            eI2CFSMStatus = KE_EEP_STOP_CONDITION_EXECUTED;

            /* stop an eventual communication */
            SEND_STOP_CONDITION();

            break;
        }

        /* If FSM is in an unknown status */
        default:
        {
            aeI2COperationStatus = I2C_KE_OP_ERROR_GENERIC;

            eI2CFSMStatus = KE_EEP_STOP_CONDITION_EXECUTED;

            /* stop an eventual communication */
            SEND_STOP_CONDITION();

            break;
        }
    }
}




/******************************* END OF FILE ******************************/
