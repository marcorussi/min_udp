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
 * This file icmp.h represents the ICMP layer inclusion file of the UDP/IP stack.
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


/* --------------- Exported structures definitions ---------------- */

/* ECHO request result info structure */
typedef struct
{
	uint8 ui8LostPackets;
	uint8 ui8NotValidReplyPackets;
	uint8 ui8SentPackets;
	uint8 ui8ValidReplyRatio;
} ICMP_st_EchoResult;




/* -------------- Exported functions prototypes -------------- */

EXTERN ICMP_st_EchoResult   ICMP_getEchoReqResult   (uint32, uint32);
EXTERN void                 ICMP_StopEchoRequest    (uint32, uint32);
EXTERN boolean              ICMP_StartEchoRequest   (uint32, uint32);
EXTERN void                 ICMP_PeriodicTask       (void);
EXTERN void                 ICMP_manageICMPMsg      (uint32, uint32, uint8 *, uint16);




/* End of file */
