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
 * This file eep.h represents the header file of the EEP component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


/****** Definition for mono inclusion ******/
#ifdef I_EEP
#else
#define I_EEP 1

/**************************************************************************
 **                Declaration of exported constants                     **
 **************************************************************************/

/* EEPROM buffer length in bytes */
#define EEP_UC_EEPROM_BUFFER_LEN                 UC_32

/**************************************************************************
 **                Declaration of exported macros                        **
 **************************************************************************/


/**************************************************************************
 **                Declaration of exported types                         **
 **************************************************************************/

/* EEPROM state */
typedef enum
{
   EEP_KE_EEPROM_STATE_IDLE,
   EEP_KE_EEPROM_STATE_READ,
   EEP_KE_EEPROM_STATE_WRITE
} EEP_ke_eeprom_state;

/* EEPROM request status */
typedef enum
{
   EEP_KE_REQUEST_REJECTED,
   EEP_KE_REQUEST_ACCEPTED
} EEP_ke_req_status;

/**************************************************************************
 **                Declaration of exported variables                     **
 **************************************************************************/

/*************************************************************************
Syntax: extern EEP_ke_eeprom_state EEP_eEepromState
Object: store actual macro state of EEPROM state machine for external
        diagnostic request. Auxiliary state such as WAIT_WRITE, WAIT_READ
        and similar are not available, and are managed as main state.
        See typedef for details.
Unit :  enum
Range:  see EEP_ke_eeprom_state typedef
**************************************************************************/
extern EEP_ke_eeprom_state EEP_eEepromState;

/*************************************************************************
Syntax: extern uchar EEP_aucEepromTXBuffer[EEP_UC_EEPROM_BUFFER_LEN]
Object: buffer containing data to be written in eeprom or read from it
Unit :  array of uchar
Range:  0x00 - 0xFF for each element
**************************************************************************/
extern uchar EEP_aucEepromTXBuffer[EEP_UC_EEPROM_BUFFER_LEN];

/*************************************************************************
Syntax: extern uchar EEP_aucEepromRXBuffer[EEP_UC_EEPROM_BUFFER_LEN]
Object: buffer containing data to be written in eeprom or read from it
Unit :  array of uchar
Range:  0x00 - 0xFF for each element
**************************************************************************/
extern uchar EEP_aucEepromRXBuffer[EEP_UC_EEPROM_BUFFER_LEN];

/**************************************************************************
 **                Declaration of exported constant data                 **
 **************************************************************************/


/**************************************************************************
 **                Exported Functions, in alphabetic order               **
 **************************************************************************/
extern void                EEP_TK_PeriodicManagement (void);
extern void                EEP_Initialise            (void);
extern EEP_ke_req_status   EEP_eReadEeprom           (ushort, ushort);
extern EEP_ke_req_status   EEP_eWriteEeprom          (ushort, ushort);

/* End of conditional inclusion of component EEP */
#endif

/******************************* END OF FILE ******************************/
