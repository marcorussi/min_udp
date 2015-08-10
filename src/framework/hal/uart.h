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
 * This file uart.h represents the header file of the UART component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


/****** definition for mono-inclusion ******/
#ifdef I_UART
#else
#define I_UART 1

/*************************************************************************
**                   Inclusion                 			        **
*************************************************************************/
#include "../fw_common.h"

/*************************************************************************
**                   Declaration of exported constants                  **
**************************************************************************/

/* UART bits rate. Bits/s */
#define UART_UL_BAUD_RATE_BIT_PER_S         ((uint32)9600)

/*************************************************************************
**                   Declaration of exported macros                     **
**************************************************************************/

/**************************************************************************
**                   Declaration of exported types                       **
***************************************************************************/

/**************************************************************************
**                   Declaration of exported variables                   **
***************************************************************************/

/*************************************************************************
**                   Declaration of exported constant data              **
**************************************************************************/

/*************************************************************************
**                   Exported Functions (in alphabetic order)           **
**************************************************************************/

EXTERN void     UART_Init                       ( void );
EXTERN void     UART_EnableUARTReception        ( boolean );
EXTERN void     UART_EnableUARTTransmission     ( boolean );
EXTERN boolean  UART_bGetLastReceivedMessage    ( uint8 * );
EXTERN boolean  UART_bPutMessageToSend          ( uint8 * );


#endif   /* end of conditional inclusion of uart.h */
