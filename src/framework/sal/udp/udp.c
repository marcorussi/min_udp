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
 * This file udp.c represents the UDP layer of the UDP/IP stack.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 10/08/2015 - File created - Marco Russi
 *
*/


/*
TODO LIST:
    1)  implement data buffer point validity check in UDP_SendDataBuffer() function
    2)  implement some parameters checks in UDP_OpenUDPSocket() function
    3)  prepare TX packet and request transmission in a periodic task. See st_UDPSocketInfo structure.
        This is done in UDP_SendDataBuffer() API function at the moment.
        So, in the event of TX data buffer pointer not valid the request is discarded at the moment.
        See UDP_SendDataBuffer() API function.
    4)  implement header checksum validity check in UDP_unpackMessage() function
    5)  do not close sockets in case of pending RX or TX data. See UDP_CloseUDPSocket() function
*/




/* ------------- Inclusion files ----------------- */
#include "../../fw_common.h"
#include "udp.h"

#include "../../hal/ethmac.h"
#include "ipv4.h"




/* --------------- Local defines ------------------ */

#define UDP_HEADER_LENGTH               (2)
#define UDP_HEADER_BYTE_LENGTH          (UC_4 * UDP_HEADER_LENGTH)




/* --------------- Local macros definitions ------------------ */

/* Local macros bit positions definitions */
#define HDR_SRC_PORT_POS            UL_SHIFT_16
#define HDR_DST_PORT_POS            UL_SHIFT_0
#define HDR_LENGTH_POS              UL_SHIFT_16
#define HDR_CHECKSUM_POS            UL_SHIFT_0

/* UDP header fields set macros */
#define SET_HDR_SRC_PORT(x,y)       ((x) |= (((y) & 0xFFFF) << HDR_SRC_PORT_POS))
#define SET_HDR_DST_PORT(x,y)       ((x) |= (((y) & 0xFFFF) << HDR_DST_PORT_POS))
#define SET_HDR_LENGTH(x,y)         ((x) |= (((y) & 0xFFFF) << HDR_LENGTH_POS))
#define SET_HDR_CHECKSUM(x,y)       ((x) |= (((y) & 0xFFFF) << HDR_CHECKSUM_POS))

/* UDP header fields get macros */
#define GET_HDR_SRC_PORT(x)         ((((x) >> HDR_SRC_PORT_POS) & 0xFFFF))
#define GET_HDR_DST_PORT(x)         ((((x) >> HDR_DST_PORT_POS) & 0xFFFF))
#define GET_HDR_LENGTH(x)           ((((x) >> HDR_LENGTH_POS) & 0xFFFF))
#define GET_HDR_CHECKSUM(x)         ((((x) >> HDR_CHECKSUM_POS) & 0xFFFF))
/* update checksum field. TODO: optimize this operation */
#define UPDATE_HDR_CHECKSUM(x,y)    ((*(x)) = SWAP_BYTES_ORDER_32BIT_(SWAP_BYTES_ORDER_32BIT_(*(x)) | (((y) & 0xFFFF) << HDR_CHECKSUM_POS)))




/* --------------- Local types definitions ----------------- */

/* UDP connections info structure */
typedef struct
{
    boolean bSocketOpen;
    uint32 ui32IPSrcAddress;
    uint32 ui32IPDstAddress;
    uint16 ui16UDPSrcPort;
    uint16 ui16UDPDstPort;
    uint8 *pui8RXDataBufPtr;
    uint16 ui16RXDataLength;
    uint8 *pui8TXDataBufPtr;    /* not used at the moment. For future transmission in a periodic task */
    uint16 ui16TXDataLength;    /* not used at the moment. For future transmission in a periodic task */
    boolean bNewRXAvailData;
    boolean bNewTXAvailData;    /* not used at the moment. For future transmission in a periodic task */
} st_UDPSocketInfo;




/* --------------- Local variables declaration ----------------- */

/* Array to store connections info */
LOCAL st_UDPSocketInfo stUDPSocketInfo[UDP_SOCKET_MAX_NUM];




/* --------------- Local functions prototypes ----------------- */

LOCAL uint8     getSocketIndex      (uint32, uint32, uint16, uint16);
LOCAL uint16    calculateChecksum   (IPv4_st_PacketDescriptor *, uint16 *);




/* --------------- Exported functions declaration -------------- */

/* unpack an eventual received data buffer */
EXPORTED void UDP_checkReceivedData(UDP_keSocketNum unSocketNum, uint8 **pui8DataPtr, uint16 *pui16DataLength )
{
    /* if new data are available */
    if(B_TRUE == stUDPSocketInfo[unSocketNum].bNewRXAvailData)
    {
        /* copy buffer pointer */
        *pui8DataPtr = stUDPSocketInfo[unSocketNum].pui8RXDataBufPtr;
        /* copy data length */
        *pui16DataLength = stUDPSocketInfo[unSocketNum].ui16RXDataLength;

        /* reset flag. ATTENTION: should be an atomic operation */
        stUDPSocketInfo[unSocketNum].bNewRXAvailData = B_FALSE;
    }
    else
    {
        /* set a NULL pointer */
        *pui8DataPtr = NULL_PTR;
        /* set data length at 0 */
        *pui16DataLength = US_NULL;
    }
}


/* request to unpack a data buffer */
EXPORTED void UDP_unpackMessage( uint32 ui32SrcIPAdd, uint32 ui32DstIPAdd, uint8 *ui8MessagePtr )
{
    uint32 *pui32HeaderPtr;
    uint32 ui32HdrWord;
    uint16 ui16Checksum;
    uint16 ui16Length;
    uint16 ui16SourcePort;
    uint16 ui16DestPort;
    uint8 ui8SocketIndex;

    /* get buffer pointer */
    pui32HeaderPtr = (uint32 *)ui8MessagePtr;

    READ_32BIT_AND_NEXT(pui32HeaderPtr, ui32HdrWord);
    /* get src and dst ports */
    ui16SourcePort = GET_HDR_SRC_PORT(ui32HdrWord);
    ui16DestPort = GET_HDR_DST_PORT(ui32HdrWord);
    READ_32BIT_AND_NEXT(pui32HeaderPtr, ui32HdrWord);
    /* get length */
    ui16Length = GET_HDR_LENGTH(ui32HdrWord);
    /* get checksum */
    ui16Checksum = GET_HDR_CHECKSUM(ui32HdrWord);
    /* checksum validity check */
    //...

    /* get socket id from src and dst addresses and ports */
    ui8SocketIndex = getSocketIndex(ui32SrcIPAdd, ui32DstIPAdd, ui16SourcePort, ui16DestPort);
    if(ui8SocketIndex < UDP_SOCKET_MAX_NUM)
    {
        /* calculate data length: remove header length from total length */
        ui16Length -= UDP_HEADER_BYTE_LENGTH;

        /* check length */
        if(ui16Length <= UDP_MAX_DATA_LENGTH_ALLOWED)
        {
            /* store data length */
            stUDPSocketInfo[ui8SocketIndex].ui16RXDataLength = ui16Length;

            /* copy received data */
            MEM_COPY(stUDPSocketInfo[ui8SocketIndex].pui8RXDataBufPtr,
                     pui32HeaderPtr,
                     ui16Length);

            /* set flag. ATTENTION: should be an atomic operation */
            stUDPSocketInfo[ui8SocketIndex].bNewRXAvailData = B_TRUE;
        }
        else
        {
            /* length is more than maximum available: discard data at the moment */
        }
    }
    else
    {
        /* ATTENTION */
        /* received data are not for an open socket */
        /* discard data */
    }
}


/* open an UDP socket */
EXPORTED UDP_keOpResult UDP_OpenUDPSocket(UDP_keSocketNum unSocketNum, uint32 ui32IPSrcAddress, uint32 ui32IPDstAddress, uint16 ui16SrcPort, uint16 ui16DstPort )
{
    UDP_keOpResult unOpResult;

    /* ATTENTION: some parameters checks are needed */

    /* if socket is not open */
    if(stUDPSocketInfo[unSocketNum].bSocketOpen != B_TRUE)
    {
        /* allocate RX data buffer */
        stUDPSocketInfo[unSocketNum].pui8RXDataBufPtr = (uint8 *)MEM_MALLOC(UDP_MAX_DATA_LENGTH_ALLOWED);
        if(stUDPSocketInfo[unSocketNum].pui8RXDataBufPtr != NULL)
        {
            /* set src and dst addresses and ports */
            stUDPSocketInfo[unSocketNum].ui32IPSrcAddress = ui32IPSrcAddress;
            stUDPSocketInfo[unSocketNum].ui32IPDstAddress = ui32IPDstAddress;
            stUDPSocketInfo[unSocketNum].ui16UDPSrcPort = ui16SrcPort;
            stUDPSocketInfo[unSocketNum].ui16UDPDstPort = ui16DstPort;

            /* socket open */
            stUDPSocketInfo[unSocketNum].bSocketOpen = B_TRUE;

            /* TODO: do not set a 0.0.0.0 src address */

            /* send the new local IP address to lower layers */
            IPV4_setLocalIPAddress(ui32IPSrcAddress);

            /* success */
            unOpResult = UDP_OP_OK;
        }
        else
        {
            /* fail to alloc RX data buffer */
            unOpResult = UDP_OP_FAIL;
        }
    }
    else
    {
        /* fail - socket already open */
        unOpResult = UDP_OP_FAIL;
    }
    
    return unOpResult;
}


/* request to send a data buffer through an already open UDP socket */
EXPORTED UDP_keOpResult UDP_SendDataBuffer(UDP_keSocketNum unSocketNum, uint8 *pui8BuffPtr, uint16 ui16BuffLength )
{
    UDP_keOpResult unOpResult;
    IPV4_keOpResult unIPOpResult;
    IPv4_st_PacketDescriptor stIPv4PacketDscpt;
    uint16 ui16Checksum;
    uint8 *pui8BufferPtr;
    uint32 *pui32HdrWords;
    uint32 ui32HdrWord = UL_NULL;

    /* check required socket number and if the socket is open */
    if((unSocketNum < UDP_SOCKET_MAX_NUM)
    && (stUDPSocketInfo[unSocketNum].bSocketOpen == B_TRUE))
    {
        /* ATTENTION: a pointer availability check is needed */
        pui8BufferPtr = (uint8 *)IPV4_getDataBuffPtr();
        if(pui8BufferPtr != NULL)
        {
            /* perform a 32-bit word alignment */
            ALIGN_32BIT_OF_8BIT_PTR(pui8BufferPtr);
            /* set 32-bit header pointer */
            pui32HdrWords = (uint32 *)pui8BufferPtr;

            /* set source port */
            SET_HDR_SRC_PORT(ui32HdrWord, stUDPSocketInfo[unSocketNum].ui16UDPSrcPort);
            /* set destination port */
            SET_HDR_DST_PORT(ui32HdrWord, stUDPSocketInfo[unSocketNum].ui16UDPDstPort);
            WRITE_32BIT_AND_NEXT(pui32HdrWords, ui32HdrWord);
            /* set UDP length as data length plus header length */
            SET_HDR_LENGTH(ui32HdrWord, (ui16BuffLength + UDP_HEADER_BYTE_LENGTH));
            /* ATTENTION: checksum is fixed at 0 (not mandatory) */
            SET_HDR_CHECKSUM(ui32HdrWord, 0x0000);
            WRITE_32BIT_AND_NEXT(pui32HdrWords, ui32HdrWord);

            /* attach data */
            MEM_COPY((uint8 *)pui32HdrWords, pui8BuffPtr, ui16BuffLength);

            /* set IPv4 descriptor */
            stIPv4PacketDscpt.enProtocol = IPV4_PROT_UDP;
            stIPv4PacketDscpt.bDoNotFragment = B_FALSE; /* ATTENTION: this value can change according to application request */
            stIPv4PacketDscpt.ui16DataLength = (ui16BuffLength + UDP_HEADER_BYTE_LENGTH);
            stIPv4PacketDscpt.ui32IPDstAddress = stUDPSocketInfo[unSocketNum].ui32IPDstAddress;
            stIPv4PacketDscpt.ui32IPSrcAddress = stUDPSocketInfo[unSocketNum].ui32IPSrcAddress;

            /* calculate and update checksum field */
            pui32HdrWords = (uint32 *)pui8BufferPtr;
            ui16Checksum = calculateChecksum(&stIPv4PacketDscpt, (uint16 *)pui32HdrWords);
            pui32HdrWords += 1;
            UPDATE_HDR_CHECKSUM(pui32HdrWords, ui16Checksum);
/*
            // example of options
            uint8 pippo[] = "fakeoptions";
            stIPv4PacketDscpt.stOptions.bSendOptions = B_TRUE;
            stIPv4PacketDscpt.stOptions.pui8OptionDataPtr = pippo;
            stIPv4PacketDscpt.stOptions.ui8OptionLength = 11;
            stIPv4PacketDscpt.stOptions.unOptionType.stOptionType.copiedFlag = 1;
            stIPv4PacketDscpt.stOptions.unOptionType.stOptionType.optionClass = 2;
            stIPv4PacketDscpt.stOptions.unOptionType.stOptionType.optionNumber = 4;
*/
            /* send UDP packet through IP */
            unIPOpResult = IPV4_SendPacket(stIPv4PacketDscpt);
        
            /* check IP operation result */
            if(IPV4_OP_OK == unIPOpResult)
            {
                /* success */
                unOpResult = UDP_OP_OK;
            }
            else
            {
                /* fail to send the packet through IPv4 module */
                unOpResult = UDP_OP_FAIL;
            }
        }
        else
        {
            /* ATTENTION */
            /* TX buffer pointer is still busy with a previous TX request or it is corrupted */
            /* if this operation is performed in a periodic task then the request would not be discarded
             * but tried again at the next run */
            unOpResult = UDP_OP_FAIL;
        }
    }
    else
    {
        /* fail - invalid socket number or socket is not open */
        unOpResult = UDP_OP_FAIL;
    }

    return unOpResult;
}


/* close a previously open socket */
EXPORTED UDP_keOpResult UDP_CloseUDPSocket(UDP_keSocketNum unSocketNum)
{
    UDP_keOpResult opResult;

    /* if the socket is open */
    /* ATTENTION: it is possible to leave the socket open in case of pending RX or TX data */
    if(stUDPSocketInfo[unSocketNum].bSocketOpen == B_TRUE)
    {
        /* free RX data buffer */
        MEM_FREE(stUDPSocketInfo[unSocketNum].pui8RXDataBufPtr);

        /* socket is now closed */
        stUDPSocketInfo[unSocketNum].bSocketOpen = B_FALSE;

        /* success */
        opResult = UDP_OP_OK;
    }
    else
    {
        /* fail - socket is already closed */
        opResult = UDP_OP_FAIL;
    }

    return opResult;
}




/* ----------------- Local functions declaration ----------------- */

/* get socket index from src and dst addresses and ports */
LOCAL uint8 getSocketIndex(uint32 ui32SourceAdd, uint32 ui32DestAdd, uint16 ui16SourcePort, uint16 ui16DestPort)
{
    uint8 ui8SktIdx = UC_NULL;

    /* search socket */
    while(  (   (stUDPSocketInfo[ui8SktIdx].bSocketOpen != B_TRUE)              /* socket is still open */
            ||  ((stUDPSocketInfo[ui8SktIdx].ui32IPSrcAddress != ui32DestAdd) && (stUDPSocketInfo[ui8SktIdx].ui32IPSrcAddress != 0x00000000))   /* this device is the destination or source address is not 0.0.0.0 */
            ||  ((stUDPSocketInfo[ui8SktIdx].ui32IPDstAddress != ui32SourceAdd) && (stUDPSocketInfo[ui8SktIdx].ui32IPDstAddress != 0xFFFFFFFF)) /* the sender is the expected one or destination address is not a IP broadcast address */
            ||  (stUDPSocketInfo[ui8SktIdx].ui16UDPSrcPort != ui16DestPort)     /* destination port is this one */
            ||  (stUDPSocketInfo[ui8SktIdx].ui16UDPDstPort != ui16SourcePort))  /* source port is the expected one */
    &&      (ui8SktIdx < UDP_SOCKET_MAX_NUM))

    {
        /* next socket */
        ui8SktIdx++;
    }
    
    return ui8SktIdx;
}


/* Function to calculate checksum */
LOCAL uint16 calculateChecksum(IPv4_st_PacketDescriptor *stIPv4Header, uint16 *pui16UDPSegment)
{
    uint32 ui32Sum = 0;
    uint16 ui16Length = stIPv4Header->ui16DataLength;

    ui32Sum += ((SWAP_BYTES_ORDER_32BIT_(stIPv4Header->ui32IPSrcAddress) >> UL_SHIFT_16) & 0xFFFF);
    ui32Sum += (SWAP_BYTES_ORDER_32BIT_(stIPv4Header->ui32IPSrcAddress) & 0xFFFF);

    ui32Sum += ((SWAP_BYTES_ORDER_32BIT_(stIPv4Header->ui32IPDstAddress) >> UL_SHIFT_16) & 0xFFFF);
    ui32Sum += (SWAP_BYTES_ORDER_32BIT_(stIPv4Header->ui32IPDstAddress) & 0xFFFF);

    ui32Sum += SWAP_BYTES_ORDER_16BIT_(stIPv4Header->enProtocol);

    ui32Sum += SWAP_BYTES_ORDER_16BIT_(stIPv4Header->ui16DataLength);

    while( ui16Length > 1 )
    {
        ui32Sum += *pui16UDPSegment;
        pui16UDPSegment++;
        ui16Length -= 2;
    }

    if( ui16Length > 0 )
    {
        ui32Sum += ((*pui16UDPSegment) & SWAP_BYTES_ORDER_16BIT_(0xFF00));
    }

    /* Fold 32-bit sum to 16 bits: add carrier to result */
    while( ui32Sum >> 16 )
    {
        ui32Sum = (ui32Sum & 0xFFFF) + (ui32Sum >> 16);
    }
    ui32Sum = ~ui32Sum;

    /* swap bytes order */
    ui32Sum = SWAP_BYTES_ORDER_16BIT_(ui32Sum);

    return (uint16)ui32Sum;
}




/* end of file */
