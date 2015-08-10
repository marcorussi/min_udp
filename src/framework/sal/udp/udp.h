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
 * This file udp.h represents the UDP layer inclusion file of the UDP/IP stack.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 10/08/2015 - File created - Marco Russi
 *
*/


/* ------------ Inclusion files --------------- */
#include "../../fw_common.h"




/* --------------- Exported defines ----------------- */

/* Max received data length allowed */
#define UDP_MAX_DATA_LENGTH_ALLOWED         (400)   /* ATTENTION: do not change it... be careful,
                                                    it is used by DHCP that needs big BOOT packets */




/* --------------- Exported enums definitions ---------------- */

/* UDP operations result enum */
typedef enum
{
    UDP_OP_OK
   ,UDP_OP_FAIL
} UDP_keOpResult;


/* sockets numbers enum */
typedef enum
{
    UDP_SOCKET_1
   ,UDP_SOCKET_2
   ,UDP_SOCKET_3
   ,UDP_SOCKET_4
   ,UDP_SOCKET_5
   ,UDP_SOCKET_6
   ,UDP_SOCKET_7
   ,UDP_SOCKET_8
   ,UDP_SOCKET_MAX_NUM
} UDP_keSocketNum;




/* -------------- Exported functions prototypes -------------- */

EXTERN UDP_keOpResult   UDP_OpenUDPSocket       (UDP_keSocketNum, uint32, uint32, uint16, uint16);
EXTERN UDP_keOpResult   UDP_SendDataBuffer      (UDP_keSocketNum, uint8 *, uint16);
EXTERN void             UDP_checkReceivedData   (UDP_keSocketNum, uint8 **, uint16 *);
EXTERN void             UDP_unpackMessage       (uint32, uint32, uint8 *);
EXTERN UDP_keOpResult   UDP_CloseUDPSocket      (UDP_keSocketNum);




/* End of file */
