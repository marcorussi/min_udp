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
 * This file IPv4.c represents the IPv4 layer of the UDP/IP stack.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 10/08/2015 - File created - Marco Russi
 *
*/


/* ------------ Inclusion files ------------------- */
#include "../../fw_common.h"

#include "ipv4.h"
#include "../../hal/ethmac.h"
#include "arp.h"
#include "icmp.h"
#include "udp.h"


/* 
TODO LIST:
    1)  call ARP module to update ETH/IP addresses table. See manageReceivedPacket() function
    2)  set a proper value to ui8Dscp, ui8Dscp and ui8TimeToLive. See sendPendingIPv4Packet() function and GET_TIME_TO_LIVE() macro
    3)  ensure avoid data corruption of pui8DataBufferPtr: now it depends by IPV4_getDataBuffPtr() function
    4)  avoid to send data or return data pointer if module has been deinitialised. Refer to IPV4_Deinit function
*/




/* --------------- Local defines ------------------ */

/* Ethernet MAC address length in bytes */
#define UC_ETH_MAC_ADD_LENGTH           ((uint8)6)

/* Length of ETH type field in bytes */
#define UC_ETH_TYPE_LENGTH              ((uint8)2)

/* Position in 32-bits words of checksum field in an IPv4 header (the first one is 0) */
#define UC_IPV4_HDR_WORDS_CHK_POS       ((uint8)2)

/* Ethernet type header field IPv4 value */
#define US_ETH_TYPE_IPV4                ((uint16)0x0800)

/* Ethernet type header field ARP value */
#define US_ETH_TYPE_ARP                 ((uint16)0x0806)

/* IPv4 header fields */
#define IPV4_HEADER_VERSION             (4)
#define IPV4_HEADER_MIN_LENGTH          (5)     /* number of 32-bit words */
#define IPV4_HEADER_MIN_BYTE_LENGTH     (UC_4 * IPV4_HEADER_MIN_LENGTH) /* number of bytes */

/* Byte position of source IP address in IPv4 header */
#define UC_IPV4_SRC_IPADD_BYTE_POS      (12)

/* IPv4 maximum options length in bytes */
#define IPV4_HDR_OPT_MAX_BYTE_LENGTH    (40)

/* Max allowed data length in bytes to transmit (fragmentation) */
#define IPV4_US_MAX_TRANS_UNIT          ((uint16)IPV4_US_FRAG_MAX_LENGTH)

/* Num of octects for each NFB */
#define IPV4_UC_OCTECTS_EACH_NFB        ((uint8)8)      /* 8 octects */

/* don't fragment flags value */
#define IPV4_DO_NOT_FRAG_FLAGS          (2)
/* more fragments flags value */
#define IPV4_MORE_FRAG_FLAGS            (1)
/* no more fragments flags value */
#define IPV4_NO_MORE_FRAG_FLAGS         (0)

/* End of options list byte value */
#define UC_END_OF_OPTIONS_LIST          ((uint8)0x00)




/* ------------------ Local macros definition ----------------- */

/* Local macros bit positions definitions */
#define HDR_VERSION_POS             UL_SHIFT_28
#define HDR_IHL_POS                 UL_SHIFT_24
#define HDR_DSCP_POS                UL_SHIFT_18
#define HDR_ECN_POS                 UL_SHIFT_16
#define HDR_TOT_LENGTH_POS          UL_SHIFT_0
#define HDR_IDENTIF_POS             UL_SHIFT_16
#define HDR_FLAGS_POS               UL_SHIFT_13
#define HDR_FRAG_OFF_POS            UL_SHIFT_0
#define HDR_TIME_LIVE_POS           UL_SHIFT_24
#define HDR_PROTOCOL_POS            UL_SHIFT_16
#define HDR_CHECKSUM_POS            UL_SHIFT_0
#define HDR_SRC_ADD_POS             UL_SHIFT_0
#define HDR_DST_ADD_POS             UL_SHIFT_0


/* IPv4 header fields set macros */
#define SET_HDR_VERSION(x,y)        ((x) |= (((y) & 0xF) << HDR_VERSION_POS))
#define SET_HDR_IHL(x,y)            ((x) |= (((y) & 0xF) << HDR_IHL_POS))
#define SET_HDR_DSCP(x,y)           ((x) |= (((y) & 0x3F) << HDR_DSCP_POS))
#define SET_HDR_ECN(x,y)            ((x) |= (((y) & 0x3) << HDR_ECN_POS))
#define SET_HDR_TOT_LENGTH(x,y)     ((x) |= (((y) & 0xFFFF) << HDR_TOT_LENGTH_POS))
#define SET_HDR_IDENTIF(x,y)        ((x) |= (((y) & 0xFFFF) << HDR_IDENTIF_POS))
#define SET_HDR_FLAGS(x,y)          ((x) |= (((y) & 0x7) << HDR_FLAGS_POS))
#define SET_HDR_FRAG_OFFSET(x,y)    ((x) |= (((y) & 0x1FFF) << HDR_FRAG_OFF_POS))
#define SET_HDR_TIME_LIVE(x,y)      ((x) |= (((y) & 0xFF) << HDR_TIME_LIVE_POS))
#define SET_HDR_PROTOCOL(x,y)       ((x) |= (((y) & 0xFF) << HDR_PROTOCOL_POS))
#define SET_HDR_CHECKSUM(x,y)       ((x) |= (((y) & 0xFFFF) << HDR_CHECKSUM_POS))
#define SET_HDR_SRC_ADD(x,y)        ((x) |= (((y) & 0xFFFFFFFF) << HDR_SRC_ADD_POS))
#define SET_HDR_DST_ADD(x,y)        ((x) |= (((y) & 0xFFFFFFFF) << HDR_DST_ADD_POS))
/* update checksum field. TODO: optimize this operation */
#define UPDATE_HDR_CHECKSUM(x,y)    ((*(x)) = SWAP_BYTES_ORDER_32BIT_(SWAP_BYTES_ORDER_32BIT_(*(x)) | (((y) & 0xFFFF) << HDR_CHECKSUM_POS)))

/* IPv4 header fields get macros */
#define GET_HDR_VERSION(x)          ((((x) >> HDR_VERSION_POS) & 0xF))
#define GET_HDR_IHL(x)              ((((x) >> HDR_IHL_POS) & 0xF))
#define GET_HDR_DSCP(x)             ((((x) >> HDR_DSCP_POS) & 0x3F))
#define GET_HDR_ECN(x)              ((((x) >> HDR_ECN_POS) & 0x3))
#define GET_HDR_TOT_LENGTH(x)       ((((x) >> HDR_TOT_LENGTH_POS) & 0xFFFF))
#define GET_HDR_IDENTIF(x)          ((((x) >> HDR_IDENTIF_POS) & 0xFFFF))
#define GET_HDR_FLAGS(x)            ((((x) >> HDR_FLAGS_POS) & 0x7))
#define GET_HDR_FRAG_OFFSET(x)      ((((x) >> HDR_FRAG_OFF_POS) & 0x1FFF))
#define GET_HDR_TIME_LIVE(x)        ((((x) >> HDR_TIME_LIVE_POS) & 0xFF))
#define GET_HDR_PROTOCOL(x)         ((((x) >> HDR_PROTOCOL_POS) & 0xFF))
#define GET_HDR_CHECKSUM(x)         ((((x) >> HDR_CHECKSUM_POS) & 0xFFFF))
#define GET_HDR_SRC_ADD(x)          ((((x) >> HDR_SRC_ADD_POS) & 0xFFFFFFFF))
#define GET_HDR_DST_ADD(x)          ((((x) >> HDR_DST_ADD_POS) & 0xFFFFFFFF))

/* Ethernet frame related get macros */
#define GET_ETHERTYPE(x)            (SWAP_BYTES_ORDER_16BIT_((x)))

/* Get source IP address from IPv4 header */
#define GET_IPV4_SRC_IP_ADD(x,y)    READ_32BIT((x),(y))

/* Get time to live value. TODO: fixed at the moment. Implement something more clever */
#define GET_TIME_TO_LIVE()          (0xFF)
/* Get identifier number value. Just the current value and increment at the moment */
#define GET_IDENTIF_NUM()           (ui16IdentifCounter++)




/* ----------------- Local structures declaration --------------------- */

/* IPv4 header parameters */
typedef struct
{
    uint8  ui8Dscp;
    uint8  ui8Ecn;
    uint8  ui8Protocol;
    uint8  ui8HdrLength;
    uint16 ui16TotLength;
    uint16 ui8Flags;
    uint16 ui16Identifier;
    uint16 ui16FragOffset;
    uint8  ui8TimeToLive;
    uint32 ui32IPSrcAddress;
    uint32 ui32IPDstAddress;
} st_HeaderParams;


/* RX pending fragmentation data struct */
typedef struct
{
    uint16 ui16Identif;
    uint32 ui32SrcIPAdd;
    uint32 ui32DstIPAdd;
    uint8 ui8Protocol;
    uint8 aui8OptionsPtr[IPV4_HDR_OPT_MAX_BYTE_LENGTH];
    uint8 ui8OptLength;
    boolean bOptReady;
    boolean bFragPending;
} st_PendingFrag;




/* ------------------- Local variables declaration ------------------- */

/* TX data buffer pointer where upper layers write data */
LOCAL uint8 *pui8TXDataBuffPtr;

/* RX data buffer pointer used for fragmented packets */
LOCAL uint8 *pui8RXDataBuffPtr;

/* store pending packet to send */
LOCAL IPv4_st_PacketDescriptor stPendingIPv4Packet;

/* signal a message ready to be sent */
LOCAL boolean bPendingPacket = B_FALSE;

/* counter of identifier field. Incremented at every packet send */
LOCAL uint16 ui16IdentifCounter = 0x0500;

/* RX pending fragmentation data struct */
LOCAL st_PendingFrag stRXPendingFrag;

/* IP address obtained via DHCP. Init as 0.0.0.0 */
LOCAL uint32 ui32ObtainedIPAdd = UL_NULL;




/* ------------------ Local functions prototypes ------------------------ */

LOCAL void      manageReceivedPacket    (void);
LOCAL void      manageReceivedOptions   (uint8 *, uint8);
LOCAL void      sendPendingIPv4Packet   (IPv4_st_PacketDescriptor *);
LOCAL void      prepareIPv4Header       (uint8 *, st_HeaderParams *, st_HeaderOptions *);
LOCAL void      decodeIPv4Packet        (uint8 *);
LOCAL uint16    calcHeaderChecksum      (uint8 *, uint8);




/* ------------------ Exported functions declaration ---------------- */

/* get the obtained IP address via DHCP */
EXPORTED uint32 IPV4_getObtainedIPAdd( void )
{
    return ui32ObtainedIPAdd;
}


/* set a new local IP address */
EXPORTED void IPV4_setLocalIPAddress( uint32 ui32LocalIPAdd )
{
    /* store IP address in IPv4 module */
    ui32ObtainedIPAdd = ui32LocalIPAdd;

    /* update local IP addresses table of ARP module */
    ARP_setLocalIPAddress(ui32LocalIPAdd);
}


/* set a router info: router IP address and subnet mask */
EXPORTED void IPV4_setRouterInfo( uint32 ui32RouterIPAdd, uint32 ui32SubnetMask )
{
    /* update router info */
    ARP_setRouterInfo(ui32RouterIPAdd, ui32SubnetMask);
}


/* Init IPv4 module */
EXPORTED boolean IPV4_Init( void )
{
    boolean bInitSuccess;

    /* allocate TX data buffer */
    pui8TXDataBuffPtr = (uint8 *)MEM_MALLOC(IPV4_US_ACCEPTED_MIN_LENGTH);
    /* allocate RX data buffer */
    pui8RXDataBuffPtr = (uint8 *)MEM_MALLOC(IPV4_US_ACCEPTED_MIN_LENGTH);
    /* if both pointers are valid */
    if((pui8TXDataBuffPtr != NULL_PTR) && (pui8RXDataBuffPtr != NULL_PTR))
    {
        /* init success */
        bInitSuccess = B_TRUE;
    }
    else
    {
        /* init fail */
        bInitSuccess = B_FALSE;
    }

    return bInitSuccess;
}


/* De-init IPv4 module */
EXPORTED void IPV4_Deinit( void )
{
    /* free TX data buffer */
    MEM_FREE(pui8TXDataBuffPtr);
    /* free RX data buffer */
    MEM_FREE(pui8RXDataBuffPtr);
}


/* Function to get data pointer */
EXPORTED uint8 * IPV4_getDataBuffPtr( void )
{
    uint8 *pui8RetPtr;

    /* If a previous packet is pending then return NULL */
    /* The reason is to avoid data buffer corruption  */
    if(B_TRUE == bPendingPacket)
    {
        pui8RetPtr = NULL;
    }
    else
    {
        pui8RetPtr = pui8TXDataBuffPtr;
    }

    return pui8RetPtr;
}


/* Function to check if a IP address is a local one */
EXPORTED boolean IPV4_checkLocalIPAdd( uint32 ui32IPAddress )
{
    return ARP_checkLocalIPAdd(ui32IPAddress);
}


/* Periodic task. Send pending TX packets and unpack received packets */
EXPORTED void IPV4_PeriodicTask( void )
{
    uint64 ui64DstEthAdd;

    /* manage eventual received packets */
    manageReceivedPacket();

    /* if a packet is ready to be sent */
    if(B_TRUE == bPendingPacket)
    {
        /* update local IP addresses table */
        ARP_setLocalIPAddress(stPendingIPv4Packet.ui32IPSrcAddress);
        /* get ETH address from ARP module */
        ui64DstEthAdd = ARP_getEthAddFromIPAdd(stPendingIPv4Packet.ui32IPSrcAddress, stPendingIPv4Packet.ui32IPDstAddress);
//        ui64DstEthAdd = 0x000000262d904b96;

        if(ui64DstEthAdd != ULL_NULL)
        {
            /* update dst ETH address */
            stPendingIPv4Packet.ui64DstEthAdd = ui64DstEthAdd;

            /* prepare and send a packet */
            sendPendingIPv4Packet(&stPendingIPv4Packet);

            /* clear signal flag */
            bPendingPacket = B_FALSE;
        }
        else
        {
            /* ETH address was unknown, an ARP request has been sent. Try at next run */
        }
    }
    else
    {
        /* do nothing */
    }
}


/* Function to require a packet transmission from upper layers */
EXPORTED IPV4_keOpResult IPV4_SendPacket(IPv4_st_PacketDescriptor stPacketDescriptor)
{
    IPV4_keOpResult unOpResult;

    /* check data length */
    if((stPacketDescriptor.ui16DataLength <= IPV4_US_MAX_DATAGRAM_LENGTH)
    && (stPacketDescriptor.enProtocol < IPV4_PROT_CHECK_VALUE))
    {
        /* copy requested packet to send */
        stPendingIPv4Packet = stPacketDescriptor;
        /* clear destination ethernet address */
//        stPendingIPv4Packet.ui64DstEthAdd = ULL_NULL;

        /* set pending packet flag */
        bPendingPacket = B_TRUE;

        /* success */
        unOpResult = IPV4_OP_OK;
    }
    else
    {
        /* fail */
        unOpResult = IPV4_OP_FAIL;
    }

    return unOpResult;
}




/* ---------------- Local functions declaration ------------------- */

/* unpack received packets from ETHMAC module */
LOCAL void manageReceivedPacket( void )
{
    uint8 *pui8BufPtr;
    uint16 ui16EthType = US_NULL;
    uint32 ui32SrcIPAdd = UL_NULL;
    uint64 ui64EthAddress = ULL_NULL;

    /* get first buffer pointer */
    pui8BufPtr = ETHMAC_getNextRXDataBuffer();
    /* loop */
    while(pui8BufPtr != NULL)
    {
        /* set pointer to src ETH address */
        pui8BufPtr = (uint8 *)(pui8BufPtr + UC_ETH_MAC_ADD_LENGTH);

        /* get src ETH address */
        ui64EthAddress |= ((uint64)*pui8BufPtr++ << ULL_SHIFT_40);
        ui64EthAddress |= ((uint64)*pui8BufPtr++ << ULL_SHIFT_32);
        ui64EthAddress |= ((uint64)*pui8BufPtr++ << ULL_SHIFT_24);
        ui64EthAddress |= ((uint64)*pui8BufPtr++ << ULL_SHIFT_16);
        ui64EthAddress |= ((uint64)*pui8BufPtr++ << ULL_SHIFT_8);
        ui64EthAddress |= ((uint64)*pui8BufPtr++);

        /* get eth type */
        ui16EthType = GET_ETHERTYPE(*((uint16 *)pui8BufPtr));

        /* call related protocol function */
        switch(ui16EthType)
        {
            case US_ETH_TYPE_IPV4:
            {
                /* get src IP address */
                GET_IPV4_SRC_IP_ADD(((uint32 *)(pui8BufPtr + UC_IPV4_SRC_IPADD_BYTE_POS + UC_ETH_TYPE_LENGTH)), ui32SrcIPAdd);

                /* call ARP module to update ETH/IP addresses table */
                ARP_setEthAddToIPAdd(ui32SrcIPAdd, ui64EthAddress);
        
                /* signal to IP layer that watermark has been reached */
                decodeIPv4Packet((uint8 *)(pui8BufPtr + UC_ETH_TYPE_LENGTH));

                break;
            }
            case US_ETH_TYPE_ARP:
            {
                /* call ARP */
                ARP_decodeARPPacket((uint8 *)(pui8BufPtr + UC_ETH_TYPE_LENGTH));
            }
            default:
            {
                /* do nothing at the moment */
            }
        }

        /* get next buffer pointer */
        pui8BufPtr = ETHMAC_getNextRXDataBuffer();
    }
}


/* decode received frame and call related upper layer */
LOCAL void decodeIPv4Packet(uint8 *pui8FramePtr)
{
    uint32 ui32HdrLength;
    uint32 ui32TotLength;
    uint8 ui8Protocol;
    uint8 ui8Flags;
    uint16 ui16Identif;
    uint16 ui16FragOffset;
    uint16 ui16Checksum;
    uint32 ui32SrcIPAdd;
    uint32 ui32DstIPAdd;
    uint32 *pui32HeaderPtr;
    uint32 ui32HdrWord;
    uint8 *pui8DataPtr;
    uint8 *pui8OptionsPtr;
    uint8 ui8OptLength;
    boolean bOptReady = B_FALSE;
    boolean bSendDataUp = B_FALSE;

    /* decode header with pointer to uint32 */
    pui32HeaderPtr = (uint32 *)pui8FramePtr;
    READ_32BIT_AND_NEXT(pui32HeaderPtr, ui32HdrWord);
//    GET_HDR_VERSION(ui32HdrWord);
    /* ATTENTION: ui32HdrLength field is in 32-bit words */
    ui32HdrLength = GET_HDR_IHL(ui32HdrWord);
//    GET_HDR_DSCP(ui32HdrWord);
//    GET_HDR_ECN(ui32HdrWord);
    ui32TotLength = GET_HDR_TOT_LENGTH(ui32HdrWord);
    READ_32BIT_AND_NEXT(pui32HeaderPtr, ui32HdrWord);
    ui8Flags = GET_HDR_FLAGS(ui32HdrWord);
    ui16Identif = GET_HDR_IDENTIF(ui32HdrWord);
    ui16FragOffset = GET_HDR_FRAG_OFFSET(ui32HdrWord);
    READ_32BIT_AND_NEXT(pui32HeaderPtr, ui32HdrWord);
//    GET_HDR_TIME_LIVE(ui32HdrWord);
    ui8Protocol = GET_HDR_PROTOCOL(ui32HdrWord);
    ui16Checksum = GET_HDR_CHECKSUM(ui32HdrWord);
    READ_32BIT_AND_NEXT(pui32HeaderPtr, ui32HdrWord);
    ui32SrcIPAdd = GET_HDR_SRC_ADD(ui32HdrWord);
    READ_32BIT_AND_NEXT(pui32HeaderPtr, ui32HdrWord);
    ui32DstIPAdd = GET_HDR_DST_ADD(ui32HdrWord);

    /* if checkcum is valid */
    if(US_NULL == calcHeaderChecksum((uint8 *)pui8FramePtr, (ui32HdrLength * UC_4)))
    {
        /* if there is a pending fragmented packet */
        if(B_TRUE == stRXPendingFrag.bFragPending)
        {
            /* check if it is a fragment */
            if((stRXPendingFrag.ui32DstIPAdd == ui32DstIPAdd)
            && (stRXPendingFrag.ui32SrcIPAdd == ui32SrcIPAdd)
            && (stRXPendingFrag.ui16Identif == ui16Identif)
            && (stRXPendingFrag.ui8Protocol == ui8Protocol))
            {
                /* copy data in RX buffer according to frag offset - discard eventual copied options */
                MEM_COPY(((uint8 *)(pui8RXDataBuffPtr + (ui16FragOffset * IPV4_UC_OCTECTS_EACH_NFB))),
                         ((uint32 *)(pui8FramePtr + (ui32HdrLength * UC_4))),
                         (ui32TotLength - (ui32HdrLength * UC_4)));

                /* if it is the last fragment */
                if((ui8Flags & IPV4_MORE_FRAG_FLAGS) == 0)
                {
                    /* update options length. it depends by bOptReady flag, do it anyway */
                    ui8OptLength = stRXPendingFrag.ui8OptLength;
                    /* update options pointer. it depends by bOptReady flag, do it anyway */
                    pui8OptionsPtr = stRXPendingFrag.aui8OptionsPtr;
                    /* update option ready flag */
                    bOptReady = stRXPendingFrag.bOptReady;
                    /* update protocol field */
                    ui8Protocol = stRXPendingFrag.ui8Protocol;
                    /* set src IP address */
                    ui32SrcIPAdd = stRXPendingFrag.ui32SrcIPAdd;
                    /* set dst IP address */
                    ui32DstIPAdd = stRXPendingFrag.ui32DstIPAdd;
                    /* set data pointer */
                    pui8DataPtr = (uint8 *)pui8RXDataBuffPtr;

                    /* packet re-assembled: manage data */
                    bSendDataUp = B_TRUE;
                }
                else
                {
                    /* wait for other fragments */
                }
            }
            else
            {
                /* ATTENTION: other fragments than expected ones are discarded! */
            }
        }
        else    /* no pending fragmented packet */
        {
            /* if there are more fragments and this is the first one */
            if(((ui8Flags & IPV4_MORE_FRAG_FLAGS) == 1)
            && (ui16FragOffset == US_NULL))
            {
                /* copy all fragmentation related fields */
                stRXPendingFrag.ui32DstIPAdd = ui32DstIPAdd;
                stRXPendingFrag.ui32SrcIPAdd = ui32SrcIPAdd;
                stRXPendingFrag.ui16Identif = ui16Identif;
                stRXPendingFrag.ui8Protocol = ui8Protocol;

                /* copy data in RX buffer according to frag offset - discard eventual copied options */
                MEM_COPY((uint8 *)pui8RXDataBuffPtr,
                         ((uint32 *)(pui8FramePtr + (ui32HdrLength * UC_4))),
                         (ui32TotLength - (ui32HdrLength * UC_4)));

                /* if options are present */
                if(ui32HdrLength > IPV4_HEADER_MIN_LENGTH)
                {
                    /* update options length */
                    stRXPendingFrag.ui8OptLength = ((ui32HdrLength - IPV4_HEADER_MIN_LENGTH) * UC_4);

                    /* copy options to manage later */
                    MEM_COPY(stRXPendingFrag.aui8OptionsPtr,
                           (uint8 *)(pui8FramePtr + IPV4_HEADER_MIN_BYTE_LENGTH),
                           stRXPendingFrag.ui8OptLength);

                    /* options are valid and are pending to be managed */
                    stRXPendingFrag.bOptReady = B_TRUE;
                }
                else
                {
                    /* no options are present */
                    stRXPendingFrag.bOptReady = B_FALSE;
                }

                /* fragmentation pending */
                stRXPendingFrag.bFragPending = B_TRUE;
            }
            else
            {
                /* no fragmentation */

                /* if options are present */
                if(ui32HdrLength > IPV4_HEADER_MIN_LENGTH)
                {
                    /* update options length */
                    ui8OptLength = ((ui32HdrLength - IPV4_HEADER_MIN_LENGTH) * UC_4);
                    /* update options pointer */
                    pui8OptionsPtr = (uint8 *)(pui8FramePtr + IPV4_HEADER_MIN_BYTE_LENGTH);
                    /* options are raady to be managed */
                    bOptReady = B_TRUE;
                }
                else
                {
                    /* no options are present */
                    bOptReady = B_FALSE;
                }

                /* ui8Protocol, ui32SrcIPAdd and ui32DstIPAdd fields are already set */
                /* set data pointer */
                pui8DataPtr = (uint8 *)(pui8FramePtr + (ui32HdrLength * UC_4));

                /* data ready to be managed */
                bSendDataUp = B_TRUE;
            }
        }

        /* if data are ready to be manged */
        if(B_TRUE == bSendDataUp)
        {
            /* if options are present */
            if(B_TRUE == bOptReady)
            {
                /* call function to manage options */
                manageReceivedOptions(pui8OptionsPtr, ui8OptLength);
            }
            else
            {
                /* no options are present */
            }

            /* check protocol */
            switch(ui8Protocol)
            {
                case IPV4_PROT_UDP:
                {
                    /* call UDP */
                    UDP_unpackMessage(ui32SrcIPAdd, ui32DstIPAdd, (uint8 *)pui8DataPtr);
                    /* source and destination IP addresses are passed to upper layer */

                    break;
                }
                case IPV4_PROT_TCP:
                {
                    /* call TCP */
                    /* not present at the moment */

                    break;
                }
                case IPV4_PROT_ICMP:
                {
                    /* call ICMP */
                    ICMP_manageICMPMsg(ui32SrcIPAdd, ui32DstIPAdd, (uint8 *)pui8DataPtr, (uint16)(ui32TotLength - (ui32HdrLength * UC_4)));
                    /* source and destination IP addresses are passed to upper layer */

                    break;
                }
                default:
                {
                    /* do nothing at the moment: discard message */
                }
            }
        }
    }
    else
    {
        /* checksum is wrong: discard the packet! */
    }
}


/* manage received options */
LOCAL void manageReceivedOptions( uint8 * pui8OptionsPtr, uint8 ui8OptLength )
{
    //TODO
    //...
}


/* send IPv4 packet through ETHMAC layer. Fragment packet if necessary */
LOCAL void sendPendingIPv4Packet( IPv4_st_PacketDescriptor *stPacketDscpt )
{
    uint8 *pui8BuffPtr;
    st_HeaderParams stHeaderParams;
    st_HeaderOptions stHdrOptions;
    uint16 ui16TotalLength;
    uint16 ui16DataLength;
    uint8 ui8NumOfFragPackets = UC_NULL;
    uint8 ui8NumOfNFB = UC_NULL;

    /* copy option structure and examine it */
    stHdrOptions = stPacketDscpt->stOptions;

    /* ATTENTION: if End of options list or No operation */
    if(stHdrOptions.unOptionType.ui8OptionType < UC_2)
    {
        /* do not send options */
        stHdrOptions.bSendOptions = B_FALSE;
    }
    else
    {
        /* send options */
    }
    
    /* if options are required to be sent */
    if(B_TRUE == stHdrOptions.bSendOptions)
    {
        /* init total option length with option data length plus option type and length octects */
        stHdrOptions.ui8TotOptLength = stHdrOptions.ui8OptionLength + UC_2;

        /* if option length is not multiple of 32-bit */
        if((stHdrOptions.ui8TotOptLength % UC_4) > 0)
        {
            /* increment total option length by 1. Header will be filled later */
            stHdrOptions.ui8TotOptLength += (UC_4 - (stHdrOptions.ui8TotOptLength % UC_4));
        }
        else
        {
            /* already 32-bit aligned */
        }
    }
    else
    {
        /* be sure that total option length is 0 */
        stHdrOptions.ui8TotOptLength = UC_NULL;
    }

    /* prepare all other common header fields for all packets */
    stHeaderParams.ui8Protocol = (uint8)stPacketDscpt->enProtocol;          /* protocol */
    stHeaderParams.ui32IPDstAddress = stPacketDscpt->ui32IPDstAddress;      /* destination IP address */
    stHeaderParams.ui32IPSrcAddress = stPacketDscpt->ui32IPSrcAddress;      /* source IP address */
    stHeaderParams.ui8Dscp = 0;                                             /* TODO: fixed at 0 at the moment */
    stHeaderParams.ui8Ecn = 0;                                              /* TODO: fixed at 0 at the moment */
    stHeaderParams.ui8TimeToLive = GET_TIME_TO_LIVE();                      /* set time to live */
    stHeaderParams.ui16Identifier = GET_IDENTIF_NUM();                      /* set identifier number */
    stHeaderParams.ui8Flags = UC_NULL;                                      /* reset flags */
    /* set flags field */
    if(B_TRUE == stPacketDscpt->bDoNotFragment)
    {
        /* do not fragment */
        stHeaderParams.ui8Flags = IPV4_DO_NOT_FRAG_FLAGS;
    }
    else
    {
        /* do nothing */
    }

    /* ATTENTION: stHeaderParams.ui8HdrLength field is in bytes */
    /* init header length to the minimum value */
    stHeaderParams.ui8HdrLength = IPV4_HEADER_MIN_BYTE_LENGTH;
    /* calculate total length */
    ui16TotalLength = stHeaderParams.ui8HdrLength + stHdrOptions.ui8TotOptLength + stPacketDscpt->ui16DataLength;

    /* calculate num of NFB units once */
    ui8NumOfNFB = (uint8)((IPV4_US_MAX_TRANS_UNIT - stHeaderParams.ui8HdrLength) / IPV4_UC_OCTECTS_EACH_NFB);

    /* fragmentation loop */
    do
    {
        /* set header length considering options */
        if(stHdrOptions.bSendOptions == B_TRUE)
        {
            stHeaderParams.ui8HdrLength = IPV4_HEADER_MIN_BYTE_LENGTH + stHdrOptions.ui8TotOptLength;
        }
        else
        {
            /* minimum header length */
            stHeaderParams.ui8HdrLength = IPV4_HEADER_MIN_BYTE_LENGTH;
        }

        /* check if fragmentation is necessary and if yes then implement it */
        if((ui16TotalLength > IPV4_US_MAX_TRANS_UNIT)
        && (B_FALSE == stPacketDscpt->bDoNotFragment))
        {
            /* update total length */
            stHeaderParams.ui16TotLength = (ui8NumOfNFB * IPV4_UC_OCTECTS_EACH_NFB) + stHeaderParams.ui8HdrLength;
        
            /* set more fragments flag */
            stHeaderParams.ui8Flags = IPV4_MORE_FRAG_FLAGS;

            /* update data length */
            ui16DataLength = (ui8NumOfNFB * IPV4_UC_OCTECTS_EACH_NFB);

            /* decrement remaining total length */
            ui16TotalLength -= ui16DataLength;
        }
        /* ATTENTION: if do not fragment then send the datagram anyway without considering IPV4_US_MAX_TRANS_UNIT */
        else
        {
            /* update total length */
            stHeaderParams.ui16TotLength = ui16TotalLength;

            /* set no more fragments flag */
            stHeaderParams.ui8Flags = IPV4_NO_MORE_FRAG_FLAGS;

            /* update data length */
            ui16DataLength = (ui16TotalLength - stHeaderParams.ui8HdrLength);

            /* update remaining total length */
            ui16TotalLength = US_NULL;
        }

        /* update fragmentation offset field */
        stHeaderParams.ui16FragOffset = (ui8NumOfFragPackets * ui8NumOfNFB);

        /* get next buffer pointer */
        pui8BuffPtr = (uint8 *)ETHMAC_getTXBufferPointer(stHeaderParams.ui16TotLength);
        /* perform a 32-bit word alignment */
        ALIGN_32BIT_OF_8BIT_PTR(pui8BuffPtr);

        /* update header */
        prepareIPv4Header(pui8BuffPtr, &stHeaderParams, &stHdrOptions);

        /* attach data */
        MEM_COPY((uint8 *)(pui8BuffPtr + stHeaderParams.ui8HdrLength),
                 (pui8TXDataBuffPtr + (ui8NumOfFragPackets * (ui8NumOfNFB * IPV4_UC_OCTECTS_EACH_NFB))),
                 ui16DataLength);

        /* increment num of fragmentation packets */
        ui8NumOfFragPackets++;

        /* request TX packet transmission */
        ETHMAC_sendPacket(pui8BuffPtr, stHeaderParams.ui16TotLength, ETHMAC_ui64MACAddress, stPacketDscpt->ui64DstEthAdd, US_ETH_TYPE_IPV4);

    } while(ui16TotalLength > UC_NULL);
}


/* set header fields values */
LOCAL void prepareIPv4Header(uint8 *pui8HdrPtr, st_HeaderParams *stHdrParams, st_HeaderOptions *stHdrOptions)
{
    uint8 ui8FillBytes;
    uint32 *pui32HdrWordsPtr;
    uint16 ui16HdrChecksum;
    uint32 ui32HdrWord = UL_NULL;

    /* set options first: move pointer to options position */
    pui32HdrWordsPtr = (uint32 *)(pui8HdrPtr + IPV4_HEADER_MIN_BYTE_LENGTH);

    /* set options if required */
    if(B_TRUE == stHdrOptions->bSendOptions)
    {
        /* set the option type octect */
        WRITE_8BIT_AND_NEXT(pui32HdrWordsPtr, stHdrOptions->unOptionType.ui8OptionType);

        /* if option number is greater than 2 then data length is not 0 */
        if((stHdrOptions->unOptionType.stOptionType.optionNumber > UC_2)
        && (stHdrOptions->ui8OptionLength > UC_NULL))
        {
            /* send option length */
            WRITE_8BIT_AND_NEXT(pui32HdrWordsPtr, stHdrOptions->ui8OptionLength);

            /* copy option data */
            MEM_COPY((uint8 *)pui32HdrWordsPtr, stHdrOptions->pui8OptionDataPtr, stHdrOptions->ui8OptionLength);
            /* set pointer to end of data options */
            pui32HdrWordsPtr = (uint32 *)((uint8 *)pui32HdrWordsPtr + stHdrOptions->ui8OptionLength);
            /* calculate num of bytes to fill */
            ui8FillBytes = (stHdrOptions->ui8TotOptLength - UC_2 - stHdrOptions->ui8OptionLength);
            /* fill */
            WRITE_N_BYTES(pui32HdrWordsPtr, UC_END_OF_OPTIONS_LIST, ui8FillBytes);
        }
        else
        {
            /* do not add data options */
        }

        /* if flags can be copied then keep bSendOptions as B_TRUE */
        if(UC_1 == stHdrOptions->unOptionType.stOptionType.copiedFlag)
        {
            /* keep bSendOptions as B_TRUE */
        }
        else
        {
            /* do not send options */
            stHdrOptions->bSendOptions = B_FALSE;
        }
    }
    else
    {
        /* do not send options */
    }

    /* set all other header fields: move pointer to header pointer */
    pui32HdrWordsPtr = (uint32 *)(pui8HdrPtr);

    SET_HDR_VERSION(ui32HdrWord, IPV4_HEADER_VERSION);              /* set IP version 4 */
    SET_HDR_IHL(ui32HdrWord, (stHdrParams->ui8HdrLength / UC_4));   /* set header length */
    SET_HDR_DSCP(ui32HdrWord, stHdrParams->ui8Dscp);                /* set DSCP field */
    SET_HDR_ECN(ui32HdrWord, stHdrParams->ui8Ecn);                  /* set ECN field */
    /* set total length */
    SET_HDR_TOT_LENGTH(ui32HdrWord, stHdrParams->ui16TotLength);
    WRITE_32BIT_AND_NEXT(pui32HdrWordsPtr, ui32HdrWord);

    SET_HDR_IDENTIF(ui32HdrWord, stHdrParams->ui16Identifier);      /* set identifier */
    SET_HDR_FLAGS(ui32HdrWord, stHdrParams->ui8Flags);              /* set flags */
    SET_HDR_FRAG_OFFSET(ui32HdrWord, stHdrParams->ui16FragOffset);  /* set fragmentation offset */
    WRITE_32BIT_AND_NEXT(pui32HdrWordsPtr, ui32HdrWord);

    SET_HDR_TIME_LIVE(ui32HdrWord, stHdrParams->ui8TimeToLive);     /* set time to live */
    SET_HDR_PROTOCOL(ui32HdrWord, stHdrParams->ui8Protocol);        /* set required protocol */
    SET_HDR_CHECKSUM(ui32HdrWord, US_NULL);                         /* set as 0 for checksum calculation */
    WRITE_32BIT_AND_NEXT(pui32HdrWordsPtr, ui32HdrWord);

    SET_HDR_SRC_ADD(ui32HdrWord, stHdrParams->ui32IPSrcAddress);    /* set IP source address */
    WRITE_32BIT_AND_NEXT(pui32HdrWordsPtr, ui32HdrWord);

    SET_HDR_DST_ADD(ui32HdrWord, stHdrParams->ui32IPDstAddress);    /* set IP destination address */
    WRITE_32BIT_AND_NEXT(pui32HdrWordsPtr, ui32HdrWord);

    /* re-store header pointer to the beginning of header */
    pui32HdrWordsPtr -= IPV4_HEADER_MIN_LENGTH;
    /* calculate header checksum */
    ui16HdrChecksum = calcHeaderChecksum((uint8 *)pui32HdrWordsPtr, stHdrParams->ui8HdrLength);
    /* set header pointer to the checksum word position */
    pui32HdrWordsPtr += UC_IPV4_HDR_WORDS_CHK_POS;
    /* update header checksum field value */
    UPDATE_HDR_CHECKSUM(pui32HdrWordsPtr, ui16HdrChecksum);
}


/* calculate header checksum */
LOCAL uint16 calcHeaderChecksum(uint8 *pui8Header, uint8 ui8HdrLength)
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
