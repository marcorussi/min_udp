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
 * This file ipv4.h represents the IPv4 layer inclusion file of the UDP/IP stack.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 10/08/2015 - File created - Marco Russi
 *
*/


/* ----------------- Inclusion files --------------------- */
#include "../../fw_common.h"
#include "../../hal/ethmac.h"




/* -------------- Exported defines ------------------- */

/* Maximum length in octects of datagrams */
#define IPV4_US_MAX_DATAGRAM_LENGTH         ((uint16)65535)

/* Minimum length in octects of datagrams to receive */
#define IPV4_US_ACCEPTED_MIN_LENGTH         ((uint16)576)

/* Maximum data length for each fragment */
#define IPV4_US_FRAG_MAX_LENGTH             ((uint16)576)




/* ----------------- Exported enums definitions ----------------- */

/* IPV4 operations result */
typedef enum
{
    IPV4_OP_OK
   ,IPV4_OP_FAIL
} IPV4_keOpResult;

/* IPV4 supported protocols */
typedef enum
{
    IPV4_PROT_ICMP = 1
   ,IPV4_PROT_TCP = 6
   ,IPV4_PROT_UDP = 17
   ,IPV4_PROT_CHECK_VALUE
} IPV4_keSuppProtocols;




/* ------------------ Exported structs definitions --------------- */

/* IPv4 header options */
typedef struct
{
    union un_OptionType
    {
        struct st_OptionType
        {
            uint8 optionNumber  :5;
            uint8 optionClass   :2;
            uint8 copiedFlag    :1;
        } stOptionType;
        uint8 ui8OptionType;
    } unOptionType;
    uint8 ui8OptionLength;
    uint8 *pui8OptionDataPtr;
    uint8 ui8TotOptLength;
    boolean bSendOptions;
} st_HeaderOptions;


/* IPv4 packet descriptor */
typedef struct
{
    uint32 ui32IPSrcAddress;
    uint32 ui32IPDstAddress;
    uint16 ui16DataLength;
    uint64 ui64DstEthAdd;
    IPV4_keSuppProtocols enProtocol;
    boolean bDoNotFragment;
    st_HeaderOptions stOptions;
} IPv4_st_PacketDescriptor;




/* ----------------- Exported functions declaration --------------- */

EXTERN uint32           IPV4_getObtainedIPAdd   (void);
EXTERN void             IPV4_setLocalIPAddress  (uint32);
EXTERN boolean          IPV4_checkLocalIPAdd    (uint32);
EXTERN void             IPV4_setRouterInfo      (uint32, uint32);
EXTERN boolean          IPV4_Init               (void);
EXTERN void             IPV4_Deinit             (void);
EXTERN void             IPV4_PeriodicTask       (void);
EXTERN uint8 *          IPV4_getDataBuffPtr     (void);
EXTERN IPV4_keOpResult  IPV4_SendPacket         (IPv4_st_PacketDescriptor);




/* End of file */
