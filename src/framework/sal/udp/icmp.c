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
 * This file icmp.c represents the ICMP layer of the UDP/IP stack.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 10/08/2015 - File created - Marco Russi
 *
*/


/*
TODO LIST:
	1) manage multiple ECHO REQUESTS at the same time
        2) share checksum calculation with IP layer (maybe)
*/




/* --------------- inclusion files ----------------- */
#include "../../fw_common.h"
#include "icmp.h"

#include "ipv4.h"
#include "../rtos/rtos.h"


/* --------------- local defines ----------------- */

#define UC_TYPE_ECHO_REQ		((uint8)8)
#define UC_TYPE_ECHO_REPLY		((uint8)0)
#define UC_CODE_ECHO			((uint8)0)

/* Fixed ECHO REQUEST length in bytes. Data payload is fixed */
#define UC_ECHO_REQ_HDR_LENGTH		((uint8)8)
#define UC_ECHO_REQ_DATA_LENGTH		((uint8)22)	/* length of aui8DataPayload array */
#define UC_ECHO_REQ_TOT_LENGTH		((uint8)(UC_ECHO_REQ_HDR_LENGTH + UC_ECHO_REQ_DATA_LENGTH))

/* ECHO request period counter  */
#define US_ECHO_REQ_PERIOD_MS		((uint16)500)      /* 500 ms */
#define US_ECHO_REQ_PERIOD_CNT		((uint16)(US_ECHO_REQ_PERIOD_MS / RTOS_UL_TASKS_PERIOD_MS))

/* ECHO request timeout counter  */
#define US_ECHO_REQ_TIMEOUT_MS		((uint16)2000)      /* 2 s */
#define US_ECHO_REQ_TIMEOUT_CNT		((uint16)(US_ECHO_REQ_TIMEOUT_MS / RTOS_UL_TASKS_PERIOD_MS))

/* ECHO reply message max length */
#define UC_ECHO_REPLY_MAX_TOT_LENGTH    ((uint8)64) /* this value means the length of header and data both */




/* --------------- local typedefs declaration ----------------- */

/* internal ECHO request state */
typedef enum
{
    ECHO_REQ_START,
    ECHO_REQ_PENDING,
    ECHO_REQ_AWAIT,
    ECHO_REQ_IDLE
} keEchoReqState;


/* Structure to store pending ECHO reply info */
typedef struct
{
    uint32 ui32SrcIPAdd;
    uint32 ui32DstIPAdd;
    uint8 aui8ReqMsgCpy[UC_ECHO_REPLY_MAX_TOT_LENGTH];
    uint16 ui16MsgLength;
    boolean bReplyIsPending;
} st_PendingEchoReply;


/* Structure to store pending ECHO request info */
typedef struct
{
    uint32 ui32SrcIPAdd;
    uint32 ui32DstIPAdd;
    uint16 ui16Identifier;
    uint16 ui16SequenceNum;
    uint8 aui8DataPayload[UC_ECHO_REQ_DATA_LENGTH];
    uint16 ui16MsgLength;
    keEchoReqState eEchoReqState;
    uint16 ui16TimeoutCounter;
    uint16 ui16PeriodCounter;
    uint8 ui8LostPackets;
    uint8 ui8NotValidReplyPackets;
    uint8 ui8SentPackets;
} st_PendingEchoReq;




/* --------------- local macros ----------------- */

#define UC_TYPE_BYTE_POS		((uint8)0)
#define UC_CODE_BYTE_POS		((uint8)1)
#define UC_CHECKSUM_BYTE_POS		((uint8)2)
#define UC_IDENTIF_BYTE_POS		((uint8)4)
#define UC_SEQ_NUM_BYTE_POS		((uint8)6)
#define UC_FIRST_DATA_BYTE_POS		((uint8)8)

#define SET_FIELD_TYPE(x,y)		(*((uint8 *)((x) + UC_TYPE_BYTE_POS)) = ((y) & 0xFF))
#define SET_FIELD_CODE(x,y)		(*((uint8 *)((x) + UC_CODE_BYTE_POS)) = ((y) & 0xFF))
#define SET_FIELD_CHECKSUM(x,y)		WRITE_16BIT(((uint16 *)((x) + UC_CHECKSUM_BYTE_POS)), (y))
#define SET_FIELD_IDENTIF(x,y)		WRITE_16BIT(((uint16 *)((x) + UC_IDENTIF_BYTE_POS)), (y))
#define SET_FIELD_SEQ_NUM(x,y)		WRITE_16BIT(((uint16 *)((x) + UC_SEQ_NUM_BYTE_POS)), (y))

#define GET_FIELD_TYPE(x,y)		((y) = *((uint8 *)((x) + UC_TYPE_BYTE_POS)))
#define GET_FIELD_CODE(x,y)		((y) = *((uint8 *)((x) + UC_CODE_BYTE_POS)))
#define GET_FIELD_CHECKSUM(x,y)		READ_16BIT(((uint16 *)((x) + UC_CHECKSUM_BYTE_POS)), (y))
#define GET_FIELD_IDENTIF(x,y)		READ_16BIT(((uint16 *)((x) + UC_IDENTIF_BYTE_POS)), (y))
#define GET_FIELD_SEQ_NUM(x,y)		READ_16BIT(((uint16 *)((x) + UC_SEQ_NUM_BYTE_POS)), (y))

/*
#define SET_FIELD_TYPE(x,y)		(*((uint8 *)((x) + UC_TYPE_BYTE_POS)) = ((y) & 0xFF))
#define SET_FIELD_CODE(x,y)		(*((uint8 *)((x) + UC_CODE_BYTE_POS)) = ((y) & 0xFF))
#define SET_FIELD_CHECKSUM(x,y)		(*((uint16 *)((x) + UC_CHECKSUM_BYTE_POS)) = ((y) & 0xFFFF))
#define SET_FIELD_IDENTIF(x,y)		(*((uint16 *)((x) + UC_IDENTIF_BYTE_POS)) = ((y) & 0xFFFF))
#define SET_FIELD_SEQ_NUM(x,y)		(*((uint16 *)((x) + UC_SEQ_NUM_BYTE_POS)) = ((y) & 0xFFFF))

#define GET_FIELD_TYPE(x)		(*((uint8 *)((x) + UC_TYPE_BYTE_POS)))
#define GET_FIELD_CODE(x)		(*((uint8 *)((x) + UC_CODE_BYTE_POS)))
#define GET_FIELD_CHECKSUM(x)		(*((uint16 *)((x) + UC_CHECKSUM_BYTE_POS)))
#define GET_FIELD_IDENTIF(x)		(*((uint16 *)((x) + UC_IDENTIF_BYTE_POS)))
#define GET_FIELD_SEQ_NUM(x)		(*((uint16 *)((x) + UC_SEQ_NUM_BYTE_POS)))
*/

/* ECHO request identifier update macro */
#define UPDATE_ECHO_REQ_IDENTIF(x)	((x)++)		/* just increment by 1 */

/* ECHO request sequence number update macro */
#define UPDATE_ECHO_REQ_SEQ_NUM(x)	((x)++)		/* just increment by 1 */

/*
#define UL_TYPE_POS			(UL_SHIFT_24)
#define UL_CODE_POS			(UL_SHIFT_16)
#define UL_CHECKSUM_POS			(UL_SHIFT_0)
#define UL_IDENTIF_POS			(UL_SHIFT_16)
#define UL_SEQ_NUM_POS			(UL_SHIFT_0)

#define SET_FIELD_TYPE(x,y)		((x) |= (((y) & 0xFF) << UL_TYPE_POS))
#define SET_FIELD_CODE(x,y)		((x) |= (((y) & 0xFF) << UL_CODE_POS))
#define SET_FIELD_CHECKSUM(x,y)		((x) |= (((y) & 0xFFFF) << UL_CHECKSUM_POS))
#define SET_FIELD_IDENTIF(x,y)		((x) |= (((y) & 0xFFFF) << UL_IDENTIF_POS))
#define SET_FIELD_SEQ_NUM(x,y)		((x) |= (((y) & 0xFFFF) << UL_SEQ_NUM_POS))

#define GET_FIELD_TYPE(x)		(((x) >> UL_TYPE_POS) & 0x000000FF)
#define GET_FIELD_CODE(x)		(((x) >> UL_CODE_POS) & 0x000000FF)
#define GET_FIELD_CHECKSUM(x)		(((x) >> UL_CHECKSUM_POS) & 0x0000FFFF)
#define GET_FIELD_IDENTIF(x)		(((x) >> UL_IDENTIF_POS) & 0x0000FFFF)
#define GET_FIELD_SEQ_NUM(x)		(((x) >> UL_SEQ_NUM_POS) & 0x0000FFFF)
*/




/* --------------- local variables declaration ----------------- */

LOCAL st_PendingEchoReply stPendingEchoReply;

LOCAL st_PendingEchoReq stPendingEchoReq = 
{
    UL_NULL,
    UL_NULL,
    US_NULL,
    US_NULL,
    "MY PING! SEE YOU SOON!",
    UC_ECHO_REQ_TOT_LENGTH,
    ECHO_REQ_IDLE,
    US_ECHO_REQ_TIMEOUT_CNT,
    US_ECHO_REQ_PERIOD_CNT,
    UC_NULL,
    UC_NULL,
    UC_NULL
};




/* --------------- local functions prototypes ----------------- */

LOCAL uint8 * 	prepareEchoRequestMsg	(st_PendingEchoReq *);
LOCAL uint8 *	prepareEchoReplyMsg	(st_PendingEchoReply *);
LOCAL void 	checkReceivedEchoReply	(uint8 *, uint16);
LOCAL uint16    calculateChecksum       (uint8 *, uint8);




/* --------------- exported functions ----------------- */

EXPORTED ICMP_st_EchoResult ICMP_getEchoReqResult( uint32 ui32SrcIPAdd, uint32 ui32DstIPAdd )
{
    // TODO: implement IP addresses check
    // TODO: get result accordng to src and dst IP addresses

    ICMP_st_EchoResult stEchoResult;

    /* return valid data if not in IDLE state */
    if(stPendingEchoReq.eEchoReqState == ECHO_REQ_IDLE)
    {
        stEchoResult.ui8LostPackets = stPendingEchoReq.ui8LostPackets;
        stEchoResult.ui8NotValidReplyPackets = stPendingEchoReq.ui8NotValidReplyPackets;
        stEchoResult.ui8SentPackets = stPendingEchoReq.ui8SentPackets;
        stEchoResult.ui8ValidReplyRatio = (uint8)(UC_100 - (((uint16)(stPendingEchoReq.ui8LostPackets + stPendingEchoReq.ui8NotValidReplyPackets) * UC_100) / stPendingEchoReq.ui8SentPackets));
    }
    else
    {
        /* idle state returns all zeros */
        stEchoResult.ui8LostPackets = UC_NULL;
        stEchoResult.ui8NotValidReplyPackets = UC_NULL;
        stEchoResult.ui8SentPackets = UC_NULL;
        stEchoResult.ui8ValidReplyRatio = UC_NULL;
    }

    return stEchoResult;
}


EXPORTED void ICMP_StopEchoRequest( uint32 ui32SrcIPAdd, uint32 ui32DstIPAdd )
{
    // TODO: implement IP addresses check
    // TODO: stop request accordng to src and dst IP addresses

    /* set state to idle only */
    stPendingEchoReq.eEchoReqState = ECHO_REQ_IDLE;
}


EXPORTED boolean ICMP_StartEchoRequest( uint32 ui32SrcIPAdd, uint32 ui32DstIPAdd )
{
    boolean bResult = B_FALSE;

    // TODO: implement IP addresses check
    // TODO: manage multiple requestes according to src and dst IP addresses

    /* require a new ECHO request if idle only */
    if(stPendingEchoReq.eEchoReqState == ECHO_REQ_IDLE)
    {
        /* set src and dst IP addresses */
        stPendingEchoReq.ui32SrcIPAdd = ui32SrcIPAdd;
        stPendingEchoReq.ui32DstIPAdd = ui32DstIPAdd;
        /* message length already set. it is fixed */
        //stPendingEchoReq.ui16MsgLength = UC_ECHO_REQUEST_LENGTH;
        /* update identifier value */
        UPDATE_ECHO_REQ_IDENTIF(stPendingEchoReq.ui16Identifier);
        /* reset sequence number */
        stPendingEchoReq.ui16SequenceNum = US_NULL;
        /* payload data is fixed */
        //aui8DataPayload[UC_ECHO_REQ_DATA_LENGTH];
        /* arm timeout counter */
        stPendingEchoReq.ui16TimeoutCounter = US_ECHO_REQ_TIMEOUT_CNT;
        /* reset lost packets counter at every new request */
        stPendingEchoReq.ui8LostPackets = UC_NULL;
        /* reset not valid REPLY received packets counter at every new request */
        stPendingEchoReq.ui8NotValidReplyPackets = UC_NULL;
        /* reset sent packets at every new request */
        stPendingEchoReq.ui8SentPackets = UC_NULL;
        /* set state as pending */
        stPendingEchoReq.eEchoReqState = ECHO_REQ_START;

        bResult = B_TRUE;
    }
    else
    {
        /* not idle: discard request! */
    }

    return bResult;
}


EXPORTED void ICMP_PeriodicTask( void )
{
    uint8 *pui8MsgPtr;

    // TODO: manage multiple requestes

    /* manage eventual pending echo request */
    switch(stPendingEchoReq.eEchoReqState)
    {
        case ECHO_REQ_START:
        {
            /* prepare ECHO REQUEST message */
            pui8MsgPtr = prepareEchoRequestMsg(&stPendingEchoReq);
            if(pui8MsgPtr != NULL_PTR)
            {
                /* packet has been sent */

                /* increment sent packets counter */
                stPendingEchoReq.ui8SentPackets++;

                /* set state as await */
                stPendingEchoReq.eEchoReqState = ECHO_REQ_AWAIT;
            }
            else
            {
                /* IP buffer not free: try to send later */
            }

            break;
        }
        case ECHO_REQ_AWAIT:
        {
            /* if timeout counter is expired */
            if(US_NULL == stPendingEchoReq.ui16TimeoutCounter)
            {
                /* timeout elapsed: increment lost packets */
                stPendingEchoReq.ui8LostPackets++;

                /* arm timeout counter */
                stPendingEchoReq.ui16TimeoutCounter = US_ECHO_REQ_TIMEOUT_CNT;

                /* set state as START: timeout should be greater than period so,
                   start immediately if timeout is expired */
                stPendingEchoReq.eEchoReqState = ECHO_REQ_START;
            }
            else
            {
                /* leave counter expiring */
                stPendingEchoReq.ui16TimeoutCounter--;
            }

            /* do not break: count period in AWAIT and PENDING states both. */
        }
        case ECHO_REQ_PENDING:
        {
            /* if period counter is expired */
            if(US_NULL == stPendingEchoReq.ui16PeriodCounter)
            {
                /* re-arm period counter */
                stPendingEchoReq.ui16PeriodCounter = US_ECHO_REQ_PERIOD_CNT;

                /* set state as START */
                stPendingEchoReq.eEchoReqState = ECHO_REQ_START;
            }
            else
            {
                /* leave counter expiring */
                stPendingEchoReq.ui16PeriodCounter--;
            }

            break;
        }
        case ECHO_REQ_IDLE:
        default:
        {
            /* do nothing */

            break;
        }
    }




    /* manage eventual pending ECHO replies */
    if(stPendingEchoReply.bReplyIsPending == B_TRUE)
    {
        /* prepare ECHO reply message */
        pui8MsgPtr = prepareEchoReplyMsg(&stPendingEchoReply);
        /* if ECHO reply message has been sent */
        if(pui8MsgPtr != NULL_PTR)
        {
            /* clear flag */
            stPendingEchoReply.bReplyIsPending = B_FALSE;
        }
        else
        {
            /* ECHO reply message has not been sent: try again later */
        }
    }
    else
    {
        /* do nothing */
    }
}


EXPORTED void ICMP_manageICMPMsg( uint32 ui32SrcIPAdd, uint32 ui32DstIPAdd, uint8 *pui8BufPtr, uint16 ui16MsgLength )
{
    uint8 *pui8MsgPtr;
    uint8 ui8Code;
    uint8 ui8Type;

    /* if the destination is one of our IP addresses */
    if(B_TRUE == IPV4_checkLocalIPAdd(ui32DstIPAdd))
    {
        /* if checksum is valid */
        if(US_NULL == calculateChecksum(pui8BufPtr, ui16MsgLength))
        {
            /* get CODE */
            GET_FIELD_CODE(pui8BufPtr, ui8Code);

            /* if CODE is ECHO */
            if(UC_CODE_ECHO == ui8Code)
            {
                /* get TYPE */
                GET_FIELD_TYPE(pui8BufPtr, ui8Type);

                switch(ui8Type)
                {
                    case UC_TYPE_ECHO_REQ:
                    {
                        /* store src and dst IP addresses */
                        stPendingEchoReply.ui32SrcIPAdd = ui32SrcIPAdd;
                        stPendingEchoReply.ui32DstIPAdd = ui32DstIPAdd;
                        /* store REQUEST message */
                        if(ui16MsgLength > UC_ECHO_REPLY_MAX_TOT_LENGTH)
                        {
                            /* limit message length */
                            ui16MsgLength = UC_ECHO_REPLY_MAX_TOT_LENGTH;
                        }
                        MEM_COPY(stPendingEchoReply.aui8ReqMsgCpy, pui8BufPtr, ui16MsgLength);
                        /* store message length: REPLY has same length of REQUEST */
                        stPendingEchoReply.ui16MsgLength = ui16MsgLength;

                        /* prepare and try to send ECHO REPLY message */
                        pui8MsgPtr = prepareEchoReplyMsg(&stPendingEchoReply);
                        /* if message has been sent */
                        if(pui8MsgPtr != NULL_PTR)
                        {
                            /* ECHO reply has been sent immediately: good */
                        }
                        else
                        {
                            /* IP buffer is full: try again later */
                            stPendingEchoReply.bReplyIsPending = B_TRUE;
                        }

                        break;
                    }
                    case UC_TYPE_ECHO_REPLY:
                    {
                        /* if an ECHO REQUEST has been awaiting a REPLY */
                        if(stPendingEchoReq.eEchoReqState == ECHO_REQ_AWAIT)
                        {
                            /* manage ECHO REPLY message */
                            checkReceivedEchoReply(pui8BufPtr, ui16MsgLength);
                        }
                        else
                        {
                            /* discard the ECHO REPLY! */
                        }

                        break;
                    }
                    default:
                        break;
                }
            }
            else
            {
                /* CODE is not ECHO, discard the message at the moment! */
            }
        }
        else
        {
            /* checksum wrong: discard the message! */
        }
    }
    else
    {
        /* destination is not for us: discard the message */
    }
}




/* --------------- local functions ----------------- */

LOCAL uint8 * prepareEchoReplyMsg( st_PendingEchoReply *pstPendEchoReply )
{
    IPV4_keOpResult unIPOpResult;
    IPv4_st_PacketDescriptor stIPv4PacketDscpt;
    uint16 ui16Checksum;
    uint8 *pui8MsgPtr = NULL_PTR;

    /* get IPV4 buffer data pointer */
    pui8MsgPtr = IPV4_getDataBuffPtr();
    if(pui8MsgPtr != NULL_PTR)
    {
        ALIGN_32BIT_OF_8BIT_PTR(pui8MsgPtr);
        /* copy the entire REQUEST message as REPLY message (same identifier, seq num and data) */
        MEM_COPY(pui8MsgPtr,
                 (uint8 *)pstPendEchoReply->aui8ReqMsgCpy,
                 pstPendEchoReply->ui16MsgLength);
        /* set echo type as REPLY */
        SET_FIELD_TYPE(pui8MsgPtr, UC_TYPE_ECHO_REPLY);
        /* for checksum calculation the checksum should be at 0 */
        SET_FIELD_CHECKSUM(pui8MsgPtr, US_NULL);
        /* calculate message checksum and update it */
        ui16Checksum = calculateChecksum(pui8MsgPtr, pstPendEchoReply->ui16MsgLength);
        SET_FIELD_CHECKSUM(pui8MsgPtr, ui16Checksum);

        /* set IPv4 descriptor */
        stIPv4PacketDscpt.enProtocol = IPV4_PROT_ICMP;
        stIPv4PacketDscpt.bDoNotFragment = B_FALSE; /* ATTENTION: this value can change according to application request */
        stIPv4PacketDscpt.ui16DataLength = pstPendEchoReply->ui16MsgLength;
        stIPv4PacketDscpt.ui32IPDstAddress = pstPendEchoReply->ui32SrcIPAdd;
        stIPv4PacketDscpt.ui32IPSrcAddress = pstPendEchoReply->ui32DstIPAdd;

        /* send ICMP packet through IP */
        unIPOpResult = IPV4_SendPacket(stIPv4PacketDscpt);

        /* check IP operation result */
        if(unIPOpResult != IPV4_OP_OK)
        {
            /* fail */
            // TODO: do something...
        }
    }
    else
    {
        /* IP buffer not free: return NULL pointer */
    }

    return pui8MsgPtr;
}


LOCAL uint8 * prepareEchoRequestMsg( st_PendingEchoReq * pstPendEchoReq )
{
    IPV4_keOpResult unIPOpResult;
    IPv4_st_PacketDescriptor stIPv4PacketDscpt;
    uint16 ui16Checksum;
    uint8 *pui8MsgPtr = NULL_PTR;

    /* get IPV4 buffer data pointer */
    pui8MsgPtr = IPV4_getDataBuffPtr();
    if(pui8MsgPtr != NULL_PTR)
    {
        ALIGN_32BIT_OF_8BIT_PTR(pui8MsgPtr);

        /* set echo type as REPLY */
        SET_FIELD_TYPE(pui8MsgPtr, UC_TYPE_ECHO_REQ);
        /* set echo type as REPLY */
        SET_FIELD_CODE(pui8MsgPtr, UC_CODE_ECHO);
        /* for checksum calculation the checksum should be at 0 */
        SET_FIELD_CHECKSUM(pui8MsgPtr, US_NULL);

        /* do not update identifier: at every new request only */
        //UPDATE_ECHO_REQ_IDENTIF(pstPendEchoReq->ui16Identifier);
        /* update ECHO request sequence number */
        UPDATE_ECHO_REQ_SEQ_NUM(pstPendEchoReq->ui16SequenceNum);

        /* set indentifier field */
        SET_FIELD_IDENTIF(pui8MsgPtr, pstPendEchoReq->ui16Identifier);
        /* set sequence number field */
        SET_FIELD_SEQ_NUM(pui8MsgPtr, pstPendEchoReq->ui16SequenceNum);

        /* set payload data */
        MEM_COPY((uint8 *)(pui8MsgPtr + UC_FIRST_DATA_BYTE_POS), (uint8 *)(pstPendEchoReq->aui8DataPayload), (pstPendEchoReq->ui16MsgLength - UC_ECHO_REQ_HDR_LENGTH));

        /* calculate message checksum and update it */
        ui16Checksum = calculateChecksum(pui8MsgPtr, pstPendEchoReq->ui16MsgLength);
        SET_FIELD_CHECKSUM(pui8MsgPtr, ui16Checksum);

        /* set IPv4 descriptor */
        stIPv4PacketDscpt.enProtocol = IPV4_PROT_ICMP;
        stIPv4PacketDscpt.bDoNotFragment = B_FALSE; /* ATTENTION: this value can change according to application request */
        stIPv4PacketDscpt.ui16DataLength = pstPendEchoReq->ui16MsgLength;
        stIPv4PacketDscpt.ui32IPDstAddress = pstPendEchoReq->ui32DstIPAdd;
        stIPv4PacketDscpt.ui32IPSrcAddress = pstPendEchoReq->ui32SrcIPAdd;

        /* send ICMP packet through IP */
        unIPOpResult = IPV4_SendPacket(stIPv4PacketDscpt);

        /* check IP operation result */
        if(unIPOpResult != IPV4_OP_OK)
        {
            /* fail */
            // TODO: do something...
        }
    }
    else
    {
        /* IP buffer not free: return NULL pointer */
    }

    return pui8MsgPtr;
}


LOCAL void checkReceivedEchoReply( uint8 *pui8BufPtr, uint16 ui16MsgLength )
{
    uint16 ui16Identifier;
    uint16 ui16SeqNum;

    /* get identifier and sequence number */
    GET_FIELD_IDENTIF(pui8BufPtr, ui16Identifier);
    GET_FIELD_SEQ_NUM(pui8BufPtr, ui16SeqNum);

    /* if REPLY is the expected one */
    if((stPendingEchoReq.ui16Identifier == ui16Identifier)
    && (stPendingEchoReq.ui16SequenceNum == ui16SeqNum)
    && (0 == MEM_COMPARE((uint8 *)(pui8BufPtr + UC_FIRST_DATA_BYTE_POS), (uint8 *)(stPendingEchoReq.aui8DataPayload), (ui16MsgLength - UC_ECHO_REQ_HDR_LENGTH))))
    {
        /* rearm timeout counter */
        stPendingEchoReq.ui16TimeoutCounter = US_ECHO_REQ_TIMEOUT_CNT;

        /* REPLY is valid: set state as PENDING */
        stPendingEchoReq.eEchoReqState = ECHO_REQ_PENDING;
    }
    else
    {
        /* REPLY is not valid: increment related counter */
        stPendingEchoReq.ui8NotValidReplyPackets++;
    }
}


/* calculate checksum */
LOCAL uint16 calculateChecksum(uint8 *pui8Header, uint8 ui8HdrLength)
{
    uint16 *ui16HdrPointer;
    uint32 ui32Checksum = 0;

    ui16HdrPointer = (uint16 *)pui8Header;

    while(ui8HdrLength > UC_1)
    {
        ui32Checksum += (*ui16HdrPointer);

        ui16HdrPointer++;

        /* if high order bit set, fold */
        if(ui32Checksum & 0x80000000)
        {
            ui32Checksum = (ui32Checksum & 0xFFFF) + (ui32Checksum >> UL_SHIFT_16);
        }

        ui8HdrLength -= 2;
    }

    /* take care of left over byte */
    if(ui8HdrLength)
    {
        ui32Checksum += (uint16)(*((uint8 *)pui8Header));
    }

    while(ui32Checksum >> UL_SHIFT_16)
    {
        ui32Checksum = (ui32Checksum & 0xFFFF) + (ui32Checksum >> UL_SHIFT_16);
    }

    /* invert it */
    ui32Checksum = (~ui32Checksum);

    /* swap bytes order */
    ui32Checksum = SWAP_BYTES_ORDER_16BIT_((uint16)ui32Checksum);

    return (uint16)ui32Checksum;
}




/* End of file */
