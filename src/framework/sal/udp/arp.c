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
 * This file arp.c represents the ARP layer of the UDP/IP stack.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 10/08/2015 - File created - Marco Russi
 *
*/


/*
TODO LIST:
    1)  limit the number and the frequency of ARP requests
    2)  check IP address validity in ARP_getEthAddFromIPAdd function (0.0.0.X addresses should not be used... maybe)
    3)  the first value of the local IP add table is overwritten in case of table full. Change the behaviour.
        See ARP_setLocalIPAddress() function
    4)  implement counters to free IP addresses locations after a while (least used IP addresses are removed first)
    5)  check some packets fields in decodeARPPacket() function
    6)  implement a addresses validity check in ARP_setEthAddToIPAdd function
*/




/* -------------- Inclusions files --------------- */
#include <xc.h>
#include <sys/attribs.h>

#include "../../fw_common.h"
#include "../../hal/ethmac.h"

#include "arp.h"




/* ---------------- Local defines ------------------ */

/* Max num of local IP addresses */
#define UC_MAX_NUM_OF_LOCAL_IP_ADD      ((uint8)4)

/* Max num of IP addresses */
#define UC_MAX_NUM_OF_IP_ADD            ((uint8)8)

/* Max num of ETH addresses */
#define UC_MAX_NUM_OF_ETH_ADD           ((uint8)8)

/* Broadcast MAC address */
#define BROADCAST_MAC_ADDRESS           ((uint64)0x0000FFFFFFFFFFFF)

/* Null MAC address */
#define NULL_MAC_ADDRESS                ((uint64)0x0000000000000000)

/* Ethernet type header field ARP value */
#define US_ETH_TYPE_ARP                 ((uint16)0x0806)

/* ARP message length in bytes */
#define ARP_MESSAGE_BYTE_LENGTH         ((uint8)28)

/* ARP fields values */
#define ARP_HW_TYPE                     ((uint16)1)         /* Ethernet */
#define ARP_PROT_TYPE                   ((uint16)0x0800)    /* IP */
#define HW_ADD_BYTE_LENGTH              ((uint8)6)          /* 48 bit */
#define PROT_ADD_BYTE_LENGTH            ((uint8)4)          /* 32 bit */

#define ARP_OP_REQUEST                  ((uint16)1)
#define ARP_OP_REPLY                    ((uint16)2)




/* ----------------- Local macros definitions ------------------- */

/* ARP packet fields set macros */
#define SET_HW_TYPE(x,y)                ((x) |= SWAP_BYTES_ORDER_16BIT_(y))
#define SET_PROT_TYPE(x,y)              ((x) |= (SWAP_BYTES_ORDER_16BIT_(y) << UL_SHIFT_16))
#define SET_HW_ADD_LENGTH(x,y)          ((x) |= ((y) & 0xFF))
#define SET_PROT_ADD_LENGTH(x,y)        ((x) |= (((y) & 0xFF) << UL_SHIFT_8))
#define SET_OPERATION(x,y)              ((x) |= (SWAP_BYTES_ORDER_16BIT_(y) << UL_SHIFT_16))
#define SET_HIGH_16BIT(x,y)             ((x) |= SWAP_BYTES_ORDER_16BIT_(y))
#define SET_LOW_16BIT(x,y)              ((x) |= (SWAP_BYTES_ORDER_16BIT_(y) << UL_SHIFT_16))

/* ARP packet fields get macros */
#define GET_HW_TYPE(x)                  (SWAP_BYTES_ORDER_16BIT_((x & 0x0000FFFF)))
#define GET_PROT_TYPE(x)                (SWAP_BYTES_ORDER_16BIT_(((x & 0xFFFF0000) >> UL_SHIFT_16)))
#define GET_HW_ADD_LENGTH(x)            ((x) & 0xFF)
#define GET_PROT_ADD_LENGTH(x)          (((x) & 0x0000FF00) >> UL_SHIFT_8)
#define GET_OPERATION(x)                (SWAP_BYTES_ORDER_16BIT_(((x & 0xFFFF0000) >> UL_SHIFT_16)))
#define GET_HIGH_16BIT(x)               (SWAP_BYTES_ORDER_16BIT_((x & 0x0000FFFF)))
#define GET_LOW_16BIT(x)                (SWAP_BYTES_ORDER_16BIT_(((x & 0xFFFF0000) >> UL_SHIFT_16)))




/* ----------------- Local typedefs definition ------------------- */

/* struct to store correlation between IP address and ETH addresses array index */
typedef struct
{
    uint32 ui32IPAdd;       /* IP address value */
    uint8 ui8EthAddIndex;   /* Index of ETH addresses array */
} st_IPAddToEthAddIdx;


/* struct to store router IP address and subnet mask */
typedef struct
{
    uint32 ui32RouterIPAdd;     /* router IP address value */
    uint32 ui32SubnetMask;      /* IP subnet mask value */
} st_RouterInfo;




/* ----------------- Local variables declaration ------------------- */

/* store pointer of message ready to be decoded */
LOCAL uint8 *pui8MessagePtr;

/* Array of destination ETH addresses */
LOCAL uint64 aui64EthAddArray[UC_MAX_NUM_OF_ETH_ADD] = {0};

/* Array of destination IP address to ETH address elements */
LOCAL st_IPAddToEthAddIdx astIPAddToEthAddArray[UC_MAX_NUM_OF_IP_ADD] = {0, 0};

/* Array of IP addresses of this device */
LOCAL uint32 aui32LocalIPAddArray[UC_MAX_NUM_OF_LOCAL_IP_ADD] = {0};

/* Structure to store router informations */
LOCAL st_RouterInfo stRouterInfo =
{
    UL_NULL,
    UL_NULL
};




/* ----------------- Local functions prototypes --------------------- */

LOCAL void      decodeARPPacket         (uint8 *);
LOCAL void      prepareAndSendReply     (uint32, uint32, uint64);
LOCAL void      prepareAndSendRequest   (uint32, uint32);
LOCAL void      updateDstEthAddTable    (uint32, uint64);




/* ---------------- Exported functions declaration ------------------- */

/* update router info: IP address and subnet mask */
EXPORTED void ARP_setRouterInfo( uint32 ui32RouterIPAdd, uint32 ui32SubnetMask )
{
    stRouterInfo.ui32RouterIPAdd = ui32RouterIPAdd;
    stRouterInfo.ui32SubnetMask = ui32SubnetMask;
}


/* update local IP addresses table */
EXPORTED void ARP_setLocalIPAddress( uint32 ui32IPAdd )
{
    uint8 ui8Index;

    /* search in local IP addresses array */
    while((aui32LocalIPAddArray[ui8Index] != ui32IPAdd)
    &&    (aui32LocalIPAddArray[ui8Index] != UL_NULL)
    &&    (ui8Index < UC_MAX_NUM_OF_LOCAL_IP_ADD))
    {
        /* next IP address */
        ui8Index++;
    }

    if(ui8Index < UC_MAX_NUM_OF_LOCAL_IP_ADD)
    {
        /* save the new IP address if missing (aui32LocalIPAddArray[ui8Index] == UL_NULL) */
        /* In case of the IP address is already present this operation overwrites its value */
        aui32LocalIPAddArray[ui8Index] = ui32IPAdd;
    }
    else
    {
        /* ATTENTION: table is full - overwrite the first value */
        aui32LocalIPAddArray[UC_NULL] = ui32IPAdd;
    }
}


/* find IP address in our local IP addresses */
EXPORTED boolean ARP_checkLocalIPAdd( uint32 ui32IPAdd )
{
    uint8 ui8Index;
    boolean bIPAddFound;

    /* search in local IP addresses array */
    while((aui32LocalIPAddArray[ui8Index] != ui32IPAdd)
    &&    (ui8Index < UC_MAX_NUM_OF_LOCAL_IP_ADD))
    {
        /* next IP address */
        ui8Index++;
    }

    if(ui8Index < UC_MAX_NUM_OF_LOCAL_IP_ADD)
    {
        /* IP address found */
        bIPAddFound = B_TRUE;
    }
    else
    {
        /* IP address not found */
        bIPAddFound = B_FALSE;
    }

    return bIPAddFound;
}


/* get ETH address from IP address */
EXPORTED uint64 ARP_getEthAddFromIPAdd( uint32 ui32SrcIPAdd, uint32 ui32DstIPAdd )
{
    uint8 ui8Index = UC_NULL;
    uint64 ui64DstEthAdd;

    /* If destination IP address is not in the local network */
    if((ui32DstIPAdd & stRouterInfo.ui32SubnetMask) != (stRouterInfo.ui32RouterIPAdd & stRouterInfo.ui32SubnetMask))
    {
        /* drop the packet to the router */
        ui32DstIPAdd = stRouterInfo.ui32RouterIPAdd;
    }
    else
    {
        /* destination IP address is in the local network */
    }

    /* If destination address is a IP broadcast address */
    if(0xFFFFFFFF == ui32DstIPAdd)
    {
        /* set HW address to broadcast */
        ui64DstEthAdd = 0x0000FFFFFFFFFFFF;
    }
    else 
    {
        /* find IP address */
        while((astIPAddToEthAddArray[ui8Index].ui32IPAdd != ui32DstIPAdd)
        &&    (ui8Index < UC_MAX_NUM_OF_IP_ADD))
        {
            /* next array element */
            ui8Index++;
        }

        /* if IP address is present and found index is valid */
        if((ui8Index < UC_MAX_NUM_OF_IP_ADD)
        && (astIPAddToEthAddArray[ui8Index].ui8EthAddIndex < UC_MAX_NUM_OF_ETH_ADD))
        {
            /* get found dst ETH address */
            ui64DstEthAdd = aui64EthAddArray[(astIPAddToEthAddArray[ui8Index].ui8EthAddIndex)];
        }
        else
        {
            /* send a ARP request */
            prepareAndSendRequest(ui32SrcIPAdd, ui32DstIPAdd);

            /* dst ETH address not available */
            ui64DstEthAdd = ULL_NULL;
        }
    }
    
    return ui64DstEthAdd;
}


/* set ETH address related to an IP address */
EXPORTED void ARP_setEthAddToIPAdd( uint32 ui32IPAdd, uint64 ui64EthAdd )
{
    /* implement a addresses validity check */
    //...

    /* call function to update dst ETH and IP addresses tables */
    updateDstEthAddTable(ui32IPAdd, ui64EthAdd);
}


/* Periodic task */
EXPORTED void ARP_PeriodicTask( void )
{
    /* implement IP addresses table counters management */
    //...
}


/* Function to unpack an ARP packet */
EXPORTED void ARP_decodeARPPacket( uint8 *pui8BufferPtr )
{
    /* call function to decode the received ARP packet */
    decodeARPPacket(pui8BufferPtr);
}




/* ---------------- Local functions declaration ------------------ */

/* TODO: implement error check */
LOCAL void decodeARPPacket( uint8 *pui8BufferPtr )
{
    uint32 *pui32BufPtr;
    uint16 ui16CheckWord;
    uint16 ui16Operation;
    uint64 ui64TargetEthAdd;
    uint64 ui64SenderEthAdd;
    uint32 ui32TargetProtAdd;
    uint32 ui32SenderProtAdd;
    
    /* set the 32-bit pointer */
    pui32BufPtr = (uint32 *)pui8BufferPtr;

    ui16CheckWord = GET_HW_TYPE(*pui32BufPtr);
//    if(ARP_HW_TYPE == ui16CheckWord) 
    ui16CheckWord = GET_PROT_TYPE(*pui32BufPtr);
//    if(ARP_PROT_TYPE == ui16CheckWord) 
    pui32BufPtr++;
    ui16CheckWord = GET_HW_ADD_LENGTH(*pui32BufPtr);
//    if(HW_ADD_BYTE_LENGTH == ui16CheckWord) 
    ui16CheckWord = GET_PROT_ADD_LENGTH(*pui32BufPtr);
//    if(PROT_ADD_BYTE_LENGTH == ui16CheckWord) 

    /* get operation code */
    ui16Operation = GET_OPERATION(*pui32BufPtr);

    pui32BufPtr++;
    /* get sender MAC address */
    ui64SenderEthAdd = ((uint64)GET_HIGH_16BIT(*pui32BufPtr) << ULL_SHIFT_32) & 0x0000FFFF00000000;
    ui64SenderEthAdd |= ((uint64)GET_LOW_16BIT(*pui32BufPtr) << ULL_SHIFT_16) & 0x00000000FFFF0000;
    pui32BufPtr++;
    ui64SenderEthAdd |= ((uint64)GET_HIGH_16BIT(*pui32BufPtr) & 0x000000000000FFFF);

    /* get sender protocol address */
    ui32SenderProtAdd = (GET_LOW_16BIT(*pui32BufPtr) << UL_SHIFT_16) & 0xFFFF0000;
    pui32BufPtr++;
    ui32SenderProtAdd |= (GET_HIGH_16BIT(*pui32BufPtr) & 0x0000FFFF);

    /* get target MAC address */
    ui64TargetEthAdd = ((uint64)GET_LOW_16BIT(*pui32BufPtr) << ULL_SHIFT_32) & 0x0000FFFF00000000;
    pui32BufPtr++;
    ui64TargetEthAdd |= ((uint64)GET_HIGH_16BIT(*pui32BufPtr) << ULL_SHIFT_16) & 0x00000000FFFF0000;
    ui64TargetEthAdd |= ((uint64)GET_LOW_16BIT(*pui32BufPtr) & 0x000000000000FFFF);

    pui32BufPtr++;
    /* get target protocol address */
    ui32TargetProtAdd = (GET_HIGH_16BIT(*pui32BufPtr) << UL_SHIFT_16) & 0xFFFF0000;
    ui32TargetProtAdd |= (GET_LOW_16BIT(*pui32BufPtr) & 0x0000FFFF);

    /* manage operation request */
    switch(ui16Operation)
    {
        /* request */
        case ARP_OP_REQUEST:
        {
            /* update destination ETH addresses table */
            /* not needed - called early from IPv4 layer at every received packet */
            updateDstEthAddTable(ui32SenderProtAdd, ui64SenderEthAdd);

            /* search target IP add in our local IP add */
            if(B_TRUE == ARP_checkLocalIPAdd(ui32TargetProtAdd))
            {
                /* found it - prepare frame and send it */
                /* - set this Protocol address as sender */
                /* - our Protocol target is the Protocol sender now */
                /* - our HW target is the HW sender now */
                prepareAndSendReply(ui32TargetProtAdd, ui32SenderProtAdd, ui64SenderEthAdd);
            }
            else
            {
                /* not found - do not reply */
            }

            break;
        }
        /* reply */
        case ARP_OP_REPLY:
        {
            /* update destination ETH addresses table */
            updateDstEthAddTable(ui32SenderProtAdd, ui64SenderEthAdd);

            break;
        }
        default:
        {
            /* do nothing */
        }
    }
}


/* prepare a REPLY packet and request transmission */
LOCAL void prepareAndSendReply(uint32 ui32SrcIPAdd, uint32 ui32DstIPAdd, uint64 ui64DstEthAdd)
{
    uint8 *pui8BufPtr;
    uint32 *pui32HdrWords;

    /* ATTENTION: set datagram pointer (use the first one at the moment) */
    pui8BufPtr = (uint8 *)ETHMAC_getTXBufferPointer((uint16)ARP_MESSAGE_BYTE_LENGTH);
    /* perform a 32-bit word alignment */
    ALIGN_32BIT_OF_8BIT_PTR(pui8BufPtr);

    /* update shared buffer pointer */
    pui32HdrWords = (uint32 *)pui8BufPtr;

    /* clear the buffer */
    memset(pui32HdrWords, UC_NULL, ARP_MESSAGE_BYTE_LENGTH);

    /* prepare fields */
    SET_HW_TYPE(*pui32HdrWords, ARP_HW_TYPE);
    SET_PROT_TYPE(*pui32HdrWords, ARP_PROT_TYPE);
    pui32HdrWords++;
    SET_HW_ADD_LENGTH(*pui32HdrWords, HW_ADD_BYTE_LENGTH);
    SET_PROT_ADD_LENGTH(*pui32HdrWords, PROT_ADD_BYTE_LENGTH);
    SET_OPERATION(*pui32HdrWords, ARP_OP_REPLY);
    pui32HdrWords++;

    /* sender MAC address */
    SET_HIGH_16BIT(*pui32HdrWords, ((ETHMAC_ui64MACAddress & 0x0000FFFF00000000) >> ULL_SHIFT_32));
    SET_LOW_16BIT(*pui32HdrWords, ((ETHMAC_ui64MACAddress & 0x00000000FFFF0000) >> ULL_SHIFT_16));
    pui32HdrWords++;
    SET_HIGH_16BIT(*pui32HdrWords, (ETHMAC_ui64MACAddress & 0x000000000000FFFF));

    /* sender protocol address */
    SET_LOW_16BIT(*pui32HdrWords, ((ui32SrcIPAdd & 0xFFFF0000) >> UL_SHIFT_16));
    pui32HdrWords++;
    SET_HIGH_16BIT(*pui32HdrWords, (ui32SrcIPAdd & 0x0000FFFF));

    /* target MAC address */
    SET_LOW_16BIT(*pui32HdrWords, ((ui64DstEthAdd & 0x0000FFFF00000000) >> ULL_SHIFT_32));
    pui32HdrWords++;
    SET_HIGH_16BIT(*pui32HdrWords, ((ui64DstEthAdd & 0x00000000FFFF0000) >> ULL_SHIFT_16));
    SET_LOW_16BIT(*pui32HdrWords, (ui64DstEthAdd & 0x000000000000FFFF));

    pui32HdrWords++;
    /* target protocol address */
    SET_HIGH_16BIT(*pui32HdrWords, ((ui32DstIPAdd & 0xFFFF0000) >> UL_SHIFT_16));
    SET_LOW_16BIT(*pui32HdrWords, (ui32DstIPAdd & 0x0000FFFF));

    /* request ETH packet transmission */
    ETHMAC_sendPacket(pui8BufPtr, ARP_MESSAGE_BYTE_LENGTH, ETHMAC_ui64MACAddress, ui64DstEthAdd, US_ETH_TYPE_ARP);
}


/* prepare a REQUEST packet and request transmission */
LOCAL void prepareAndSendRequest( uint32 ui32SrcIPAdd, uint32 ui32DstIPAdd )
{
    uint8 *pui8BufPtr;
    uint32 *pui32HdrWords;

    /* ATTENTION: set datagram pointer (use the first one at the moment) */
    pui8BufPtr = (uint8 *)ETHMAC_getTXBufferPointer((uint16)ARP_MESSAGE_BYTE_LENGTH);
    /* perform a 32-bit word alignment */
    ALIGN_32BIT_OF_8BIT_PTR(pui8BufPtr);

    /* update shared buffer pointer */
    pui32HdrWords = (uint32 *)pui8BufPtr;

    /* clear the buffer */
    memset(pui32HdrWords, UC_NULL, ARP_MESSAGE_BYTE_LENGTH);

    /* prepare fields */
    SET_HW_TYPE(*pui32HdrWords, ARP_HW_TYPE);
    SET_PROT_TYPE(*pui32HdrWords, ARP_PROT_TYPE);
    pui32HdrWords++;
    SET_HW_ADD_LENGTH(*pui32HdrWords, HW_ADD_BYTE_LENGTH);
    SET_PROT_ADD_LENGTH(*pui32HdrWords, PROT_ADD_BYTE_LENGTH);
    SET_OPERATION(*pui32HdrWords, ARP_OP_REQUEST);
    pui32HdrWords++;

    /* sender MAC address */
    SET_HIGH_16BIT(*pui32HdrWords, ((ETHMAC_ui64MACAddress & 0x0000FFFF00000000) >> ULL_SHIFT_32));
    SET_LOW_16BIT(*pui32HdrWords, ((ETHMAC_ui64MACAddress & 0x00000000FFFF0000) >> ULL_SHIFT_16));
    pui32HdrWords++;
    SET_HIGH_16BIT(*pui32HdrWords, (ETHMAC_ui64MACAddress & 0x000000000000FFFF));

    /* sender protocol address */
    SET_LOW_16BIT(*pui32HdrWords, ((ui32SrcIPAdd & 0xFFFF0000) >> UL_SHIFT_16));
    pui32HdrWords++;
    SET_HIGH_16BIT(*pui32HdrWords, (ui32SrcIPAdd & 0x0000FFFF));

    /* target MAC address */
    SET_LOW_16BIT(*pui32HdrWords, ((NULL_MAC_ADDRESS & 0x0000FFFF00000000) >> ULL_SHIFT_32));
    pui32HdrWords++;
    SET_HIGH_16BIT(*pui32HdrWords, ((NULL_MAC_ADDRESS & 0x00000000FFFF0000) >> ULL_SHIFT_16));
    SET_LOW_16BIT(*pui32HdrWords, (NULL_MAC_ADDRESS & 0x000000000000FFFF));

    pui32HdrWords++;
    /* target protocol address */
    SET_HIGH_16BIT(*pui32HdrWords, ((ui32DstIPAdd & 0xFFFF0000) >> UL_SHIFT_16));
    SET_LOW_16BIT(*pui32HdrWords, (ui32DstIPAdd & 0x0000FFFF));

    /* request ETH packet transmission */
    ETHMAC_sendPacket(pui8BufPtr, ARP_MESSAGE_BYTE_LENGTH, ETHMAC_ui64MACAddress, BROADCAST_MAC_ADDRESS, US_ETH_TYPE_ARP);
}


/* update destination ETH addresses table */
LOCAL void updateDstEthAddTable(uint32 ui32IPAdd, uint64 ui64EthAdd)
{
    uint8 ui8EthIndex = UC_NULL;
    uint8 ui8IPIndex = UC_NULL;

    /* find if ETH address is already present */
    while((aui64EthAddArray[ui8EthIndex] != ui64EthAdd)
    &&    (aui64EthAddArray[ui8EthIndex] != UL_NULL)
    &&    (ui8EthIndex < UC_MAX_NUM_OF_ETH_ADD))
    {
        /* next array element */
        ui8EthIndex++;
    }

    /* if array is not full */
    if(ui8EthIndex < UC_MAX_NUM_OF_ETH_ADD)
    {
        /* if ETH address is missing then add it */
        /* if ETH address is already present then overwrite its value anyway */
        aui64EthAddArray[ui8EthIndex] = ui64EthAdd;

        /* find if IP address is already present */
        while((astIPAddToEthAddArray[ui8IPIndex].ui32IPAdd != ui32IPAdd)
        &&    (astIPAddToEthAddArray[ui8IPIndex].ui32IPAdd != UL_NULL)
        &&    (ui8IPIndex < UC_MAX_NUM_OF_IP_ADD))
        {
            /* next array element */
            ui8IPIndex++;
        }

        /* if array is not full */
        if(ui8IPIndex < UC_MAX_NUM_OF_IP_ADD)
        {
            /* add the IP address value in case of it is not present (ui32IPAdd == UL_NULL) */
            /* if the IP address is already present then this operation overwrites the IP address value */
            astIPAddToEthAddArray[ui8IPIndex].ui32IPAdd = ui32IPAdd;
            /* and update Eth address index */
            astIPAddToEthAddArray[ui8IPIndex].ui8EthAddIndex = ui8EthIndex;
        }
    }
    else
    {
        /* array is full - discard both addresses at the moment */
    }
}




/* End of file */
