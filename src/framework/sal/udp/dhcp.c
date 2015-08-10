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
 * This file dhcp.c represents the DHCP layer of the UDP/IP stack.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 10/08/2015 - File created - Marco Russi
 *
*/


/* ------------ Inclusion files --------------- */
#include "../../fw_common.h"
#include "dhcp.h"

#include "../../hal/ethmac.h"
#include "../rtos/rtos.h"
#include "ipv4.h"
#include "udp.h"




/* ------------ Local defines --------------- */

/* DHCP dst and src ports */
#define UC_DEST_PORT                        ((uint8)67)
#define UC_SRC_PORT                         ((uint8)68)
/* DHCP dst and src IP addresses */
#define UL_DEST_IP_ADD                      ((uint32)0xFFFFFFFF)    /* 255.255.255.255 */
#define UL_SRC_IP_ADD                       ((uint32)0x00000000)    /* 0.0.0.0 */
/* DHCP UDP socket number */
#define UC_UDP_SOCKET_NUM                   (UDP_SOCKET_3)


/* DHCP Magic Cookie value */
#define UL_DHCP_MAGIC_COOKIE                ((uint32)0x63825363)

/* DHCP Magic Cookie value bytes */
#define UC_DHCP_MAGIC_COOKIE_4              ((uint8)((UL_DHCP_MAGIC_COOKIE & 0xFF000000) >> UL_SHIFT_24))
#define UC_DHCP_MAGIC_COOKIE_3              ((uint8)((UL_DHCP_MAGIC_COOKIE & 0x00FF0000) >> UL_SHIFT_16))
#define UC_DHCP_MAGIC_COOKIE_2              ((uint8)((UL_DHCP_MAGIC_COOKIE & 0x0000FF00) >> UL_SHIFT_8))
#define UC_DHCP_MAGIC_COOKIE_1              ((uint8)(UL_DHCP_MAGIC_COOKIE & 0x000000FF))

/* default IP address of DHCP DISCOVERY message */
#define UC_IP_ADD_DISCOVERY_4               ((uint8)192)
#define UC_IP_ADD_DISCOVERY_3               ((uint8)168)
#define UC_IP_ADD_DISCOVERY_2               ((uint8)1)
#define UC_IP_ADD_DISCOVERY_1               ((uint8)100)


/* DHCP messages fields values */
/* BOOTP message DHCP DISCOVERY/REQUEST OPERATION value */
#define UC_BOOTP_DISC_REQ_OP                ((uint8)1)

/* BOOTP message DHCP OFFER/ACK OPERATION value */
#define UC_BOOTP_OFFER_ACK_OP               ((uint8)2)

/* BOOTP message HTYPE field value */
#define UC_BOOTP_DHCP_HTYPE                 ((uint8)1)

/* BOOTP message HLEN field value */
#define UC_BOOTP_DHCP_HLEN                  ((uint8)6)

/* BOOTP message HOPS field value */
#define UC_BOOTP_DHCP_HOPS                  ((uint8)0)


/* DHCP messages fields length and bit positions */
/* IP addresses length in bytes */
#define UC_IP_ADD_LENGTH_BYTES              ((uint8)4)

/* Length of client HW address field */
#define UC_HW_ADD_LENGTH_BYTES              ((uint8)16)

/* Length of server name field */
#define UC_S_NAME_LENGTH_BYTES              ((uint8)64)

/* Length of server name field */
#define UC_FILE_NAME_LENGTH_BYTES           ((uint8)128)

/* Length of DISCOVERY options list. This value must be the length of the aui8OptStrDiscovery array */
#define UC_DISCOVERY_OPT_LENGTH_BYTES       ((uint8)20)

/* Length of REQUEST options list. This value must be the length of the aui8OptStrRequest array */
#define UC_REQUEST_OPT_LENGTH_BYTES         ((uint8)20)

/* Maximum plausible received length of OFFER/ACK messages options list */
#define UC_RX_OPT_MAX_LENGTH_BYTES          ((uint8)50)

/* Length of Magic Cookie field */
#define UC_MAGIC_COOKIE_LENGTH_BYTES        ((uint8)4)

/* Bit position of client IP address field */
#define UC_CLIENT_IP_ADD_MSG_BIT_POS        ((uint8)12)

/* Bit position of your IP address field */
#define UC_YOUR_IP_ADD_MSG_BIT_POS          ((uint16)(UC_CLIENT_IP_ADD_MSG_BIT_POS + UC_IP_ADD_LENGTH_BYTES))

/* Bit position of server IP address field */
#define UC_SERVER_IP_ADD_MSG_BIT_POS        ((uint16)(UC_YOUR_IP_ADD_MSG_BIT_POS + UC_IP_ADD_LENGTH_BYTES))

/* Bit position of gateway IP address field */
#define UC_GATWAY_ADD_MSG_BIT_POS           ((uint16)(UC_SERVER_IP_ADD_MSG_BIT_POS + UC_IP_ADD_LENGTH_BYTES))

/* Bit position of client HW address field */
#define UC_HW_ADD_MSG_BIT_POS               ((uint16)(UC_GATWAY_ADD_MSG_BIT_POS + UC_IP_ADD_LENGTH_BYTES))

/* Bit position of server name field */
#define UC_S_NAME_MSG_BIT_POS               ((uint16)(UC_HW_ADD_MSG_BIT_POS + UC_HW_ADD_LENGTH_BYTES))

/* Bit position of server name field */
#define UC_FILE_NAME_MSG_BIT_POS            ((uint16)(UC_S_NAME_MSG_BIT_POS + UC_S_NAME_LENGTH_BYTES))

/* Bit position of DHCP magic cookie or BOOTP options bit position */
#define UC_DHCP_OPT_MSG_BIT_POS             ((uint16)(UC_FILE_NAME_MSG_BIT_POS + UC_FILE_NAME_LENGTH_BYTES))

/* Header min length in bytes */
#define US_DHCP_HDR_MIN_LENGTH_BYTES        ((uint16)UC_DHCP_OPT_MSG_BIT_POS)

/* Length of DISCOVERY message */
#define US_DISCOVERY_MSG_LENGTH_BYTES       ((uint16)(US_DHCP_HDR_MIN_LENGTH_BYTES + UC_DISCOVERY_OPT_LENGTH_BYTES))

/* Length of REQUEST message */
#define US_REQUEST_MSG_LENGTH_BYTES         ((uint16)(US_DHCP_HDR_MIN_LENGTH_BYTES + UC_REQUEST_OPT_LENGTH_BYTES))


/* Specific option fields bit position */
/* Server IP address value bit position in REQUEST msg: magic cookie (4) + option type (3) + req IP add type and length (2) */
#define UC_REQ_IP_ADD_OPT_OFF               ((uint8)9)
/* Server IP address value bit position in REQUEST msg: UC_REQ_IP_ADD_OPT_OFF + req IP add value + server IP add type and length (2) */
#define UC_SERVER_IP_ADD_OPT_OFF            ((uint8)(UC_REQ_IP_ADD_OPT_OFF + 4 + 2))


/* BOOTP DHCP options fields length and bit positions */
/* DHCP option 1 */
#define DHCP_OPT_SUBNET_VALUE               ((uint8)1)
#define DHCP_OPT_SUBNET_LENGTH              ((uint8)4)

/* DHCP option 3 */
#define DHCP_OPT_ROUTER_VALUE               ((uint8)3)
#define DHCP_OPT_ROUTER_LENGTH              ((uint8)4)  /* ATTENTION: this value has been fixed at 4 (min value) */

/* DHCP option 6 */
#define DHCP_OPT_DNSERVER_VALUE             ((uint8)6)
#define DHCP_OPT_DNSERVER_LENGTH            ((uint8)4)  /* ATTENTION: this value has been fixed at 4 (min value) */

/* DHCP option 15 */
#define DHCP_OPT_DNAME_VALUE                ((uint8)15)
#define DHCP_OPT_DNAME_LENGTH               ((uint8)4)  /* ATTENTION: this value has been fixed at 4 (min value) */

/* DHCP option 28 */
#define DHCP_OPT_BROADCAST_VALUE            ((uint8)28)
#define DHCP_OPT_BROADCAST_LENGTH           ((uint8)4)

/* DHCP option 50 */
#define DHCP_OPT_REQ_IP_VALUE               ((uint8)50)
#define DHCP_OPT_REQ_IP_LENGTH              ((uint8)4)

/* DHCP option 51 */
#define DHCP_OPT_LEASE_T_VALUE              ((uint8)51)
#define DHCP_OPT_LEASE_T_LENGTH             ((uint8)4)

/* DHCP option 53 */
#define DHCP_OPT_TYPE_VALUE                 ((uint8)53)
#define DHCP_OPT_TYPE_LENGTH                ((uint8)1)
#define DHCP_OPT_TYPE_DISCOVER              ((uint8)1)
#define DHCP_OPT_TYPE_OFFER                 ((uint8)2)
#define DHCP_OPT_TYPE_REQUEST               ((uint8)3)
#define DHCP_OPT_TYPE_DECLINE               ((uint8)4)
#define DHCP_OPT_TYPE_PACK                  ((uint8)5)
#define DHCP_OPT_TYPE_NACK                  ((uint8)6)
#define DHCP_OPT_TYPE_RELEASE               ((uint8)7)
#define DHCP_OPT_TYPE_INFORM                ((uint8)8)

/* DHCP option 54 */
#define DHCP_OPT_SERVER_ID_VALUE            ((uint8)54)
#define DHCP_OPT_SERVER_ID_LENGTH           ((uint8)4)

/* DHCP option 55 */
#define DHCP_OPT_REQ_PARAM_VALUE            ((uint8)55)

/* DHCP option 58 */
#define DHCP_OPT_T1_VALUE                   ((uint8)58)
#define DHCP_OPT_T1_LENGTH                  ((uint8)4)

/* DHCP option 59 */
#define DHCP_OPT_T2_VALUE                   ((uint8)59)
#define DHCP_OPT_T2_LENGTH                  ((uint8)4)

/* End of options list */
#define END_OF_OPTIONS_LIST                 (0xFF)


/* Negotiation timeout values */
/* Timeout in ms */
#define US_DHCP_TIMEOUT_MS                  ((uint16)7000)  /* 7 s */

/* Timeout counter value */
#define US_DHCP_TIMEOUT_CNT_VALUE           ((uint16)(US_DHCP_TIMEOUT_MS / RTOS_UL_TASKS_PERIOD_MS))




/* --------------- Local macros definition -------------- */

/* Local macros bit positions definitions */
#define HDR_OP_POS                          UL_SHIFT_24
#define HDR_HTYPE_POS                       UL_SHIFT_16
#define HDR_HLEN_POS                        UL_SHIFT_8
#define HDR_HOPS_POS                        UL_SHIFT_0

/* TCP header fields set macros */
#define SET_HDR_OP(x,y)                     ((x) |= (((y) & 0xF) << HDR_OP_POS))
#define SET_HDR_HTYPE(x,y)                  ((x) |= (((y) & 0xF) << HDR_HTYPE_POS))
#define SET_HDR_HLEN(x,y)                   ((x) |= (((y) & 0xF) << HDR_HLEN_POS))
#define SET_HDR_HOPS(x,y)                   ((x) |= (((y) & 0xF) << HDR_HOPS_POS))

/* TCP header fields get macros */
#define GET_HDR_OP(x)                       ((((x) >> HDR_OP_POS) & 0xF))
#define GET_HDR_HTYPE(x)                    ((((x) >> HDR_HTYPE_POS) & 0xF))
#define GET_HDR_HLEN(x)                     ((((x) >> HDR_HLEN_POS) & 0xF))
#define GET_HDR_HOPS(x)                     ((((x) >> HDR_HOPS_POS) & 0xF))

/* set transaction ID value. Initialised at 0 at the moment, incremented by 1 first */
#define SET_TRANSACTION_ID(x)               ((x) = ++ui8TransactionID)
/* get last transaction ID value */
#define GET_TRANSACTION_ID(x)               (ui8TransactionID)
/* check if transaction ID is the expected one */
#define CHECK_TRANSACTION_ID(x)             (ui8TransactionID == (x))

/* Get local message pointer */
#define GET_LOCAL_MSG_POINTER()             (pui8MessagePtr)




/* ------------ Local structures definitions -------------- */

/* struct to store message info */
typedef struct
{
    uint8 *pui8MsgPtr;
    uint16 ui16MsgLength;
} st_DhcpMsgInfo;

/* struct to store message info */
typedef struct
{
    uint32 ui32RequestedIPAdd;
    uint32 ui32ServerIPAdd;
    uint32 ui32SubnetIPAdd;
    uint32 ui32BroadcastIPAdd;
    uint32 ui32DNSIPAdd;
    uint32 ui32DNSName;
    uint32 ui32RouterIPAdd;
    uint32 ui32LeaseTime;
    uint32 ui32RenewalTime;
    uint32 ui32RebindingTime;
    boolean bReqPending;
} st_DhcpNetInfo;


/* DHCP states enum */
typedef enum
{
    KE_DEINIT_STATE,
    KE_INIT_STATE,
    KE_DISCOVERY_STATE,
    KE_REQUEST_STATE,
    KE_WAIT_TO_STATE,
    KE_CLOSE_STATE
} ke_DhcpState;




/* ------------ Local variables declaration -------------- */

/* BOOTP options list for DISCOVERY message. Length must be equal to UC_DISCOVERY_OPT_LENGTH_BYTES define */
LOCAL const uint8 aui8OptStrDiscovery[] =
{
    UC_DHCP_MAGIC_COOKIE_4,
    UC_DHCP_MAGIC_COOKIE_3,
    UC_DHCP_MAGIC_COOKIE_2,
    UC_DHCP_MAGIC_COOKIE_1,
    DHCP_OPT_TYPE_VALUE,
    DHCP_OPT_TYPE_LENGTH,
    DHCP_OPT_TYPE_DISCOVER,
    DHCP_OPT_REQ_IP_VALUE,
    DHCP_OPT_REQ_IP_LENGTH,
    UC_IP_ADD_DISCOVERY_4,
    UC_IP_ADD_DISCOVERY_3,
    UC_IP_ADD_DISCOVERY_2,
    UC_IP_ADD_DISCOVERY_1,
    DHCP_OPT_REQ_PARAM_VALUE,
    UC_4,
    DHCP_OPT_SUBNET_VALUE,
    DHCP_OPT_ROUTER_VALUE,
    DHCP_OPT_DNAME_VALUE,
    DHCP_OPT_DNSERVER_VALUE,
    END_OF_OPTIONS_LIST
};

/* BOOTP options list for REQUEST message. Length must be equal to UC_REQUEST_OPT_LENGTH_BYTES define */
/* IP address are set later */
LOCAL uint8 aui8OptStrRequest[] =
{
    UC_DHCP_MAGIC_COOKIE_4,
    UC_DHCP_MAGIC_COOKIE_3,
    UC_DHCP_MAGIC_COOKIE_2,
    UC_DHCP_MAGIC_COOKIE_1,
    DHCP_OPT_TYPE_VALUE,
    DHCP_OPT_TYPE_LENGTH,
    DHCP_OPT_TYPE_REQUEST,
    DHCP_OPT_REQ_IP_VALUE,
    DHCP_OPT_REQ_IP_LENGTH,
    UC_NULL,
    UC_NULL,
    UC_NULL,
    UC_NULL,
    DHCP_OPT_SERVER_ID_VALUE,
    DHCP_OPT_SERVER_ID_LENGTH,
    UC_NULL,
    UC_NULL,
    UC_NULL,
    UC_NULL,
    END_OF_OPTIONS_LIST
};

/* DHCP negotiation timeout counter */
LOCAL uint16 ui16TimeoutCounter = US_DHCP_TIMEOUT_CNT_VALUE;

/* DHCP state. De-initialised by default */
LOCAL ke_DhcpState eDhcpState = KE_DEINIT_STATE;

/* Transaction ID */
LOCAL uint8 ui8TransactionID = UC_NULL;

/* pointer to UDP RX data */
LOCAL uint8 *pui8UDPRXDataPtr = NULL_PTR;

/* UDP RX data length in bytes */
LOCAL uint16 ui16UDPRXDataLength = US_NULL;

/* Network DHCP info */
LOCAL st_DhcpNetInfo stDhcpNetInfo =
{
    UL_NULL,
    UL_NULL,
    UL_NULL,
    UL_NULL,
    UL_NULL,
    UL_NULL,
    UL_NULL,
    UL_NULL,
    UL_NULL,
    UL_NULL,
    B_FALSE
};

/* structure to store pending message info */
st_DhcpMsgInfo stMsgInfo =
{
    NULL_PTR,
    US_NULL
};

/* Local pointer to use for preparing DHCP messages */
LOCAL uint8 * pui8MessagePtr = NULL_PTR;




/* ------------ Local functions prototypes -------------- */

LOCAL uint8 unpackReceivedMsg   (st_DhcpNetInfo *, uint8 *, uint16);
LOCAL void  getOfferInfo        (st_DhcpNetInfo *, uint8 *, uint16);
LOCAL void  prepareRequestMsg   (st_DhcpMsgInfo *, st_DhcpNetInfo *);
LOCAL void  prepareDiscoveryMsg (st_DhcpMsgInfo *);




/* ------------ Exported functions prototypes -------------- */

/* DHCP module init function */
EXPORTED boolean DHCP_Init( void )
{
    boolean bInitResult;

    /* init the module if de-initialised only */
    if(KE_DEINIT_STATE == eDhcpState)
    {
        /* Alloc message buffer, DISCOVERY is the largest one */
        pui8MessagePtr = (uint8 *)MEM_MALLOC(US_DISCOVERY_MSG_LENGTH_BYTES);
        /* check if pointer is not NULL */
        if(pui8MessagePtr != NULL_PTR)
        {
            /* perform a 32-bit word alignment */
            ALIGN_32BIT_OF_8BIT_PTR(pui8MessagePtr);

            /* open a UDP socket for DHCP the message via UDP */
            UDP_OpenUDPSocket(UC_UDP_SOCKET_NUM, UL_SRC_IP_ADD, UL_DEST_IP_ADD, UC_SRC_PORT, UC_DEST_PORT);

            /* go to INIT */
            eDhcpState = KE_INIT_STATE;

            /* init success */
            bInitResult = B_TRUE;
        }
        else
        {
            /* init fail */
            bInitResult = B_FALSE;
        }
    }
    else
    {
        /* init fail */
        bInitResult = B_FALSE;
    }

    return bInitResult;
}


/* DHCP module deinit function */
EXPORTED void DHCP_Deinit( void )
{
    /* go to CLOSE */
    eDhcpState = KE_CLOSE_STATE;
}


/* DHCP module init function */
EXPORTED boolean DHCP_StartIPAddReq( void )
{
    boolean bSuccess;

    /* start a new request only if the module is in INIT state */
    if(KE_INIT_STATE == eDhcpState)
    {
        /* go to DISCOVERY */
        eDhcpState = KE_DISCOVERY_STATE;
        
        /* success */
        bSuccess = B_TRUE;
    }
    else
    {
        /* fail */
        bSuccess = B_FALSE;
    }

    return bSuccess;
}


/* DHCP module periodic task */
EXPORTED void DHCP_PeriodicTask( void )
{
    uint8 ui8OptType;

    /* manage actual state */
    switch(eDhcpState)
    {
        case KE_DISCOVERY_STATE:
        {
            /* prepare a DISCOVERY message */
            prepareDiscoveryMsg(&stMsgInfo);

            /* fall through */
        }
        case KE_REQUEST_STATE:
        {
            /* send the message via UDP */
            UDP_SendDataBuffer(UC_UDP_SOCKET_NUM, stMsgInfo.pui8MsgPtr, stMsgInfo.ui16MsgLength);
                    
            /* re-arm timeout counter */
            ui16TimeoutCounter = US_DHCP_TIMEOUT_CNT_VALUE;

            /* go to WAIT timeout */
            eDhcpState = KE_WAIT_TO_STATE;

            break;
        }
        case KE_WAIT_TO_STATE:
        {
            /* check if a DHCP offer has been received */
            UDP_checkReceivedData(UC_UDP_SOCKET_NUM, &pui8UDPRXDataPtr, &ui16UDPRXDataLength);
            if(pui8UDPRXDataPtr != NULL_PTR)
            {
                /* unpack received DCHP message */
                ui8OptType = unpackReceivedMsg(&stDhcpNetInfo, pui8UDPRXDataPtr, ui16UDPRXDataLength);
                if(DHCP_OPT_TYPE_OFFER == ui8OptType)
                {
                    /* prepare REQUEST message */
                    prepareRequestMsg(&stMsgInfo, &stDhcpNetInfo);

                    /* a request message is pending */
                    stDhcpNetInfo.bReqPending = B_TRUE;
                    
                    /* go to REQUEST state */
                    eDhcpState = KE_REQUEST_STATE;
                }
                else if(DHCP_OPT_TYPE_PACK == ui8OptType)
                {
                    /* a request message is not pending anymore */
                    stDhcpNetInfo.bReqPending = B_FALSE;

                    /* set local IP address */
                    IPV4_setLocalIPAddress(stDhcpNetInfo.ui32RequestedIPAdd);
                    /* set router IP address and subnet mask */
                    IPV4_setRouterInfo(stDhcpNetInfo.ui32RouterIPAdd, stDhcpNetInfo.ui32SubnetIPAdd);

                    /* go to INIT */
                    eDhcpState = KE_INIT_STATE;
                }
                else if(DHCP_OPT_TYPE_NACK == ui8OptType)
                {
                    /* a request message is not pending anymore */
                    stDhcpNetInfo.bReqPending = B_FALSE;

                    /* go to INIT */
                    eDhcpState = KE_INIT_STATE;

                    /* do nothing else at the moment */
                }
                else
                {
                    /* do nothing, remain in this state */
                }
            }
            else
            {
                /* do nothing */
            }

            /* decrement timeout counter */
            ui16TimeoutCounter--;
            /* if timeout is expired */
            if(US_NULL == ui16TimeoutCounter)
            {
                /* re-arm timeout counter */
                ui16TimeoutCounter = US_DHCP_TIMEOUT_CNT_VALUE;

                /* a request message is not pending anymore */
                if(B_TRUE == stDhcpNetInfo.bReqPending)
                {
                    /* send the request message again: go to REQUEST state */
                    eDhcpState = KE_REQUEST_STATE;
                }
                else
                {
                    /* do not send the discovery message again: go to INIT state */
                    eDhcpState = KE_INIT_STATE;
                }
            }
            else
            {
                /* leave timeout counter to expire */
            }
            
            break;
        }
        case KE_CLOSE_STATE:
        {
            /* re-arm timeout counter */
            ui16TimeoutCounter = US_DHCP_TIMEOUT_CNT_VALUE;

            /* if the message buffer was allocated */
            if(pui8MessagePtr != NULL_PTR)
            {
                /* free the message buffer */
                MEM_FREE(pui8MessagePtr);
            }
            else
            {
                /* do nothing */
            }

            /* go to DEINIT */
            eDhcpState = KE_DEINIT_STATE;

            break;
        }
        case KE_DEINIT_STATE:
        case KE_INIT_STATE:
        default:
            /* do nothing */
            break;
    }
}




/* ------------------- Local functions declaration ---------------- */

/* unpack DHCP received message */
LOCAL uint8 unpackReceivedMsg(st_DhcpNetInfo *pstNetInfo, uint8 *pui8BuffPtr, uint16 ui16BufLength )
{
    uint8 ui8OptTypeReturn;
    uint32 ui32WordData;
    uint32 *pui32WordPtr;

    /* reset type value to return */
    ui8OptTypeReturn = UC_NULL;
    
    /* set word pointer */
    pui32WordPtr = (uint32 *)pui8BuffPtr;
    /* get 32-bit word */
    READ_32BIT_AND_NEXT(pui32WordPtr, ui32WordData);
    /* get and check  OPERATION field */
    if(UC_BOOTP_OFFER_ACK_OP == GET_HDR_OP(ui32WordData))
    {
//        GET_HDR_HTYPE(ui32WordData);
//        GET_HDR_HLEN(ui32WordData);
//        GET_HDR_HOPS(ui32WordData);
        /* get transaction ID */
        READ_32BIT_AND_NEXT(pui32WordPtr, ui32WordData);
        /* check if transaction ID is the expected one */
        if(CHECK_TRANSACTION_ID(ui32WordData))
        {
            /* get YOUR IP address */
            pui32WordPtr = (uint32 *)(pui8BuffPtr + UC_YOUR_IP_ADD_MSG_BIT_POS);
            READ_32BIT_AND_NEXT(pui32WordPtr, ui32WordData);
            /* save offered IP address. This value will be the requested one */
            pstNetInfo->ui32RequestedIPAdd = ui32WordData;
            /* get SERVER IP address */
            READ_32BIT_AND_NEXT(pui32WordPtr, ui32WordData);
            /* get received options, get and check magic cookie */
//            GET_MAGIC_COOKIE(aui8RXOpt);
            /* move pointer to option type position */
            pui8BuffPtr = (uint8 *)(pui8BuffPtr + UC_DHCP_OPT_MSG_BIT_POS + UC_MAGIC_COOKIE_LENGTH_BYTES + UC_2);
            /* check option type. It should be the value of first option */
            ui8OptTypeReturn = (uint8)(*pui8BuffPtr);
            if(DHCP_OPT_TYPE_OFFER == ui8OptTypeReturn)
            {
                /* manage OFFER options */
                getOfferInfo(&stDhcpNetInfo, (uint8 *)(pui8BuffPtr + UC_1), (ui16BufLength - US_DHCP_HDR_MIN_LENGTH_BYTES));
            }
            else if(DHCP_OPT_TYPE_PACK == ui8OptTypeReturn)
            {
                /* do not check PACK options */
            }
            else if(DHCP_OPT_TYPE_NACK == ui8OptTypeReturn)
            {
                /* do not check NACK options */
            }
            else
            {
                /* no other types of DHCP message are managed: discard the message */
            }
        }
        else
        {
            /* transaction ID is wrong */
        }
    }
    else
    {
        /* OPERATION field is wrong */
    }

    /* return the option type field */
    return ui8OptTypeReturn;
}


/* get info from DHCP OFFER message */
LOCAL void getOfferInfo( st_DhcpNetInfo *pstNetInfo, uint8 *pui8OptListPtr, uint16 ui16OptListLength )
{
    uint8 ui8OptType;
    uint8 ui8OptLength;

    /* parse all the options list */
    while(ui16OptListLength > US_NULL)
    {
        /* get next option type */
        ui8OptType = *pui8OptListPtr;
        /* get option length */
        pui8OptListPtr++;
        ui8OptLength = *pui8OptListPtr;
        /* set pointer to first option byte */
        pui8OptListPtr++;
        /* get option data and move pointer */
        switch(ui8OptType)
        {
            case DHCP_OPT_SUBNET_VALUE:
            {
                /* copy always a fixed minimum length */
                READ_SWAP_4_BYTES(pui8OptListPtr, pstNetInfo->ui32SubnetIPAdd);

                break;
            }
            case DHCP_OPT_ROUTER_VALUE:
            {
                /* copy always a fixed minimum length */
                READ_SWAP_4_BYTES(pui8OptListPtr, pstNetInfo->ui32RouterIPAdd);
                
                break;
            }
            case DHCP_OPT_DNSERVER_VALUE:
            {
                /* copy always a fixed minimum length */
                READ_SWAP_4_BYTES(pui8OptListPtr, pstNetInfo->ui32DNSIPAdd);

                break;
            }
            case DHCP_OPT_DNAME_VALUE:
            {
                /* copy always a fixed minimum length */
                READ_SWAP_4_BYTES(pui8OptListPtr, pstNetInfo->ui32DNSName);

                break;
            }
            case DHCP_OPT_BROADCAST_VALUE:
            {
                /* copy always a fixed minimum length */
                READ_SWAP_4_BYTES(pui8OptListPtr, pstNetInfo->ui32BroadcastIPAdd);
      
                break;
            }
            case DHCP_OPT_LEASE_T_VALUE:
            {
                /* copy always a fixed minimum length */
                READ_SWAP_4_BYTES(pui8OptListPtr, pstNetInfo->ui32LeaseTime);

                break;
            }
            case DHCP_OPT_SERVER_ID_VALUE:
            {
                READ_SWAP_4_BYTES(pui8OptListPtr, pstNetInfo->ui32ServerIPAdd);

                break;
            }
            case DHCP_OPT_T1_VALUE:
            {
                /* copy always a fixed minimum length */
                READ_SWAP_4_BYTES(pui8OptListPtr, pstNetInfo->ui32RenewalTime);

                break;
            }
            case DHCP_OPT_T2_VALUE:
            {
                /* copy always a fixed minimum length */
                READ_SWAP_4_BYTES(pui8OptListPtr, pstNetInfo->ui32RebindingTime);

                break;
            }
            /* following options should not be received */
            case DHCP_OPT_TYPE_VALUE:
            case DHCP_OPT_REQ_IP_VALUE:
            case DHCP_OPT_REQ_PARAM_VALUE:
            default:
            {
                break;
            }
        }

        /* decrement length by effective option length. Be carefull: do not underflow */
        if(ui16OptListLength < ui8OptLength)
        {
            ui16OptListLength = US_NULL;
        }
        else
        {
            ui16OptListLength -= ui8OptLength;
        }
    }
}


/* prepare and send DHCP REQUEST message */
LOCAL void prepareRequestMsg( st_DhcpMsgInfo *pstMsgInfo, st_DhcpNetInfo *pstNetInfo )
{
    uint32 *pui32MsgPtr;
    uint8 *pui8MsgPtr;
    uint32 ui32MsgWord = UL_NULL;

    /* get local message pointer */
    pui8MsgPtr = GET_LOCAL_MSG_POINTER();
    pui32MsgPtr = (uint32 *)pui8MsgPtr;

    /* write the first 32-bit word */
    SET_HDR_OP(ui32MsgWord, UC_BOOTP_DISC_REQ_OP);
    SET_HDR_HTYPE(ui32MsgWord, UC_BOOTP_DHCP_HTYPE);
    SET_HDR_HLEN(ui32MsgWord, UC_BOOTP_DHCP_HLEN);
    SET_HDR_HOPS(ui32MsgWord, UC_BOOTP_DHCP_HOPS);
    WRITE_32BIT_AND_NEXT(pui32MsgPtr, ui32MsgWord);

    /* XID value (transaction ID) */
    ui32MsgWord = GET_TRANSACTION_ID();
    WRITE_32BIT_AND_NEXT(pui32MsgPtr, ui32MsgWord);

    /* set the server IP address */
    ui32MsgWord = pstNetInfo->ui32ServerIPAdd;
    pui32MsgPtr = (uint32 *)(pui8MsgPtr + UC_SERVER_IP_ADD_MSG_BIT_POS);
    WRITE_32BIT_AND_NEXT(pui32MsgPtr, ui32MsgWord);

    /* write client HW address - TODO: get ETHMAC_ui64MACAddress value in a more clever way... */
    /* set the address */
    pui32MsgPtr = (uint32 *)(pui8MsgPtr + UC_HW_ADD_MSG_BIT_POS);
    /* set the value */
    ui32MsgWord = (uint32)((ETHMAC_ui64MACAddress & 0x0000FFFFFFFF0000) >> ULL_SHIFT_16);
    WRITE_32BIT_AND_NEXT(pui32MsgPtr, ui32MsgWord);
    ui32MsgWord = (uint32)((ETHMAC_ui64MACAddress & 0x000000000000FFFF) << ULL_SHIFT_16);
    WRITE_32BIT_AND_NEXT(pui32MsgPtr, ui32MsgWord);

    /* update DHCP options list */
    /* set requested IP address */
    WRITE_SWAP_4_BYTES((uint8 *)&aui8OptStrRequest[UC_REQ_IP_ADD_OPT_OFF], pstNetInfo->ui32RequestedIPAdd);
    /* set DHCP server IP address */
    WRITE_SWAP_4_BYTES((uint8 *)&aui8OptStrRequest[UC_SERVER_IP_ADD_OPT_OFF], pstNetInfo->ui32ServerIPAdd);
    
    /* set DHCP options list */
    pui32MsgPtr = (uint32 *)(pui8MsgPtr + UC_DHCP_OPT_MSG_BIT_POS);
    MEM_COPY((uint8 *)pui32MsgPtr, aui8OptStrRequest, UC_REQUEST_OPT_LENGTH_BYTES);

    /* update message info structure */
    pstMsgInfo->pui8MsgPtr = pui8MsgPtr;
    pstMsgInfo->ui16MsgLength = US_REQUEST_MSG_LENGTH_BYTES;
}


/* prepare and send DHCP DISCOVERY message */
LOCAL void prepareDiscoveryMsg( st_DhcpMsgInfo *pstMsgInfo )
{
    uint32 *pui32MsgPtr;
    uint8 *pui8MsgPtr;
    uint32 ui32MsgWord = UL_NULL;

    /* get local message pointer */
    pui8MsgPtr = GET_LOCAL_MSG_POINTER();
    pui32MsgPtr = (uint32 *)pui8MsgPtr;

    /* write the first 32-bit word */
    SET_HDR_OP(ui32MsgWord, UC_BOOTP_DISC_REQ_OP);
    SET_HDR_HTYPE(ui32MsgWord, UC_BOOTP_DHCP_HTYPE);
    SET_HDR_HLEN(ui32MsgWord, UC_BOOTP_DHCP_HLEN);
    SET_HDR_HOPS(ui32MsgWord, UC_BOOTP_DHCP_HOPS);
    WRITE_32BIT_AND_NEXT(pui32MsgPtr, ui32MsgWord);

    /* XID value (transaction ID) */
    SET_TRANSACTION_ID(ui32MsgWord);
    WRITE_32BIT_AND_NEXT(pui32MsgPtr, ui32MsgWord);

    /* write client HW address - TODO: get ETHMAC_ui64MACAddress value in a more clever way... */
    /* set the address */
    pui32MsgPtr = (uint32 *)(pui8MsgPtr + UC_HW_ADD_MSG_BIT_POS);
    /* set the value */
    ui32MsgWord = (uint32)((ETHMAC_ui64MACAddress & 0x0000FFFFFFFF0000) >> ULL_SHIFT_16);
    WRITE_32BIT_AND_NEXT(pui32MsgPtr, ui32MsgWord);
    ui32MsgWord = (uint32)((ETHMAC_ui64MACAddress & 0x000000000000FFFF) << ULL_SHIFT_16);
    WRITE_32BIT_AND_NEXT(pui32MsgPtr, ui32MsgWord);

    /* set DHCP options list */
    pui32MsgPtr = (uint32 *)(pui8MsgPtr + UC_DHCP_OPT_MSG_BIT_POS);
    MEM_COPY((uint8 *)pui32MsgPtr, aui8OptStrDiscovery, UC_DISCOVERY_OPT_LENGTH_BYTES);

    /* update message info structure */
    pstMsgInfo->pui8MsgPtr = pui8MsgPtr;
    pstMsgInfo->ui16MsgLength = US_DISCOVERY_MSG_LENGTH_BYTES;
}




/* End of file */
