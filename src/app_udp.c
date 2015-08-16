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
 * This file app_udp.c represents the UDP demo application source file
 * of the UDP/IP stack.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 10/08/2015 - File created - Marco Russi
 * 16/08/2015 - Minor changes and coding improvement for release - Marco Russi
 *
*/


/* ------------- Inclusion files -------------- */

#include "framework/fw_common.h"

#include "app_udp.h"

#include "framework/hal/port.h"
#include "framework/sal/rtos/rtos.h"
#include "framework/sal/udp/udp.h"
#include "framework/sal/dio/outch.h"




/* ---------------- Local definitions ---------------- */

/* Remote IP address for UDP sockets */
#define UL_UDP_SOCKET_REMOTE_IP_ADD             (0x0A2A0001)  /* 10.42.0.1 */

/* LEDs UDP sockets source /destination ports */
#define LED_1_UDP_SRC_PORT                      (2020)
#define LED_1_UDP_DST_PORT                      (1010)
#define LED_2_UDP_SRC_PORT                      (4040)
#define LED_2_UDP_DST_PORT                      (3030)

/* LEDs UDP sockets numbers */
#define LED_1_UDP_SOCKET_NUM                    (UDP_SOCKET_2)
#define LED_2_UDP_SOCKET_NUM                    (UDP_SOCKET_5)

/* LEDs UDP sockets numbers */
#define LED_1_OUT_CHANNEL                       (OUTCH_KE_CHANNEL_1)
#define LED_2_OUT_CHANNEL                       (OUTCH_KE_CHANNEL_2)




/* ---------------- Local typedef definitions ---------------- */

/* app LED indexes enum */
typedef enum
{
    KE_FIRST_LED
   ,KE_LED_1 = KE_FIRST_LED
   ,KE_LED_2
   ,KE_LAST_LED = KE_LED_2
   ,KE_LED_NAX_NUM
} ke_AppLEDIndexes;

/* connection states enum */
typedef enum
{
    KE_FIRST_STATE
   ,KE_INIT_STATE = KE_FIRST_STATE
   ,KE_RUN_STATE
   ,KE_WAIT_STATE
   ,KE_IDLE_STATE
   ,KE_LAST_STATE = KE_IDLE_STATE
} ke_ConnectionStatus;


/* UDP packets request types enum */
typedef enum
{
    KE_INVALID_REQ
   ,KE_LED_OFF_REQ
   ,KE_LED_ON_REQ
   ,KE_LED_BLINK_REQ
} ke_LEDStatusRequests;




/* ---------------- Local const variables declaration ------------------ */

/* Array association between LED index and output LED channels */
LOCAL const uint8 ui8LEDIndexToOutLEDCh[KE_LED_NAX_NUM] =
{
    LED_1_OUT_CHANNEL,      /* KE_LED_1 */
    LED_2_OUT_CHANNEL       /* KE_LED_2 */
};

/* Array association between LED index and UDP sockets */
LOCAL const uint8 ui8LEDIndexToUDPSocket[KE_LED_NAX_NUM] =
{
    LED_1_UDP_SOCKET_NUM,   /* KE_LED_1 */
    LED_2_UDP_SOCKET_NUM    /* KE_LED_2 */
};

/* Matrix association between LED index and source/destination UDP ports */
LOCAL const uint16 ui16LEDIndexToUDPPorts[KE_LED_NAX_NUM][UC_2] =
{
    {LED_1_UDP_SRC_PORT, LED_1_UDP_DST_PORT},   /* KE_LED_1 */
    {LED_2_UDP_SRC_PORT, LED_2_UDP_DST_PORT}    /* KE_LED_2 */
};

/* LED requests data strings */
LOCAL const uint8 kpui8LEDONcmdString[] = "led on";
LOCAL const uint8 kpui8LEDOFFcmdString[] = "led off";
LOCAL const uint8 kpui8LEDBlinkcmdString[] = "led blink";




/* ---------------- Local variables declaration ------------------ */

/* Current connection status */
LOCAL ke_ConnectionStatus enConnStatus = KE_IDLE_STATE;

/* UDP received data pointer */
LOCAL uint8 *pui8UDPRXDataPtr = NULL_PTR;

/* UDP received data length */
LOCAL uint16 ui16UDPRXDataLength = US_NULL;

/* Obtained IPv4 address */
LOCAL uint32 ui32IPAddress = UL_NULL;




/* -------------- Local functions prototypes --------------------- */

LOCAL ke_LEDStatusRequests  checkUDPData        (uint8 *, uint16);
LOCAL void                  manageReceivedData  (ke_AppLEDIndexes, uint8 *, uint16);




/* --------------- Exported functions declaration --------------- */

EXPORTED void APP_UDP_Init( void )
{
    boolean bInitSuccess = B_TRUE;

    /* init ETHMAC */
    bInitSuccess &= ETHMAC_Init();
    /* init IPv4 */
    bInitSuccess &= IPV4_Init();
    /* init DHCP */
    bInitSuccess &= DHCP_Init();

    if( B_TRUE == bInitSuccess )
    {
        /* start a IP address request via DHCP */
        DHCP_StartIPAddReq();

        /* init connection status */
        enConnStatus = KE_INIT_STATE;
    }
    else
    {
        /* fail, do not go to init state */
    }
}




EXPORTED void APP_UDP_PeriodicTask( void )
{
    /* manage connection */
    switch(enConnStatus)
    {
        case KE_INIT_STATE:
        {
            /* get obtained IP address via DHCP */
            ui32IPAddress = IPV4_getObtainedIPAdd();
            if(ui32IPAddress != UL_NULL)
            {
                /* open UDP socket for LED 1 */
                UDP_OpenUDPSocket(ui8LEDIndexToUDPSocket[KE_LED_1],     /* UDP socket number */
                                  ui32IPAddress,                        /* local IP address */
                                  UL_UDP_SOCKET_REMOTE_IP_ADD,          /* remote IP address */
                                  ui16LEDIndexToUDPPorts[KE_LED_1][0],  /* source port */
                                  ui16LEDIndexToUDPPorts[KE_LED_1][1]); /* destination port */
                /* open UDP socket for LED 2 */
                UDP_OpenUDPSocket(ui8LEDIndexToUDPSocket[KE_LED_2],     /* UDP socket number */
                                  ui32IPAddress,                        /* local IP address */
                                  UL_UDP_SOCKET_REMOTE_IP_ADD,          /* remote IP address */
                                  ui16LEDIndexToUDPPorts[KE_LED_2][0],  /* source port */
                                  ui16LEDIndexToUDPPorts[KE_LED_2][1]); /* destination port */
                /* go into RUN state */
                enConnStatus = KE_RUN_STATE;
            }
            else
            {
                /* remain in this state and wait for a valid IP address */
            }

            break;
        }
        case KE_RUN_STATE:
        {
            /* ATTENTION: send an eventual message to the remote IP address if needed */

            /* go into WAIT state */
            enConnStatus = KE_WAIT_STATE;

            break;
        }
        case KE_WAIT_STATE:
        {
            /* check if UDP data have been received for LED 1 socket */
            UDP_checkReceivedData(ui8LEDIndexToUDPSocket[KE_LED_1], &pui8UDPRXDataPtr, &ui16UDPRXDataLength);
            /* manage eventual received data */
            manageReceivedData(KE_LED_1, pui8UDPRXDataPtr, ui16UDPRXDataLength);

            /* check if UDP data have been received for LED 1 socket */
            UDP_checkReceivedData(ui8LEDIndexToUDPSocket[KE_LED_2], &pui8UDPRXDataPtr, &ui16UDPRXDataLength);
            /* manage eventual received data */
            manageReceivedData(KE_LED_2, pui8UDPRXDataPtr, ui16UDPRXDataLength);

            /* remain in this state */

            break;
        }
        case KE_IDLE_STATE:
        default:
        {
            /* do nothing */
        }
    }
}




/* -------------- Local functions declaration ------------------ */

/* get LED status request from received data */
LOCAL ke_LEDStatusRequests checkUDPData( uint8 *pui8PtrToData, uint16 ui16UDPRXDataLength)
{
    ke_LEDStatusRequests eReturnRequest;

    /* ATTENTION: TEST: check data */
    if(!MEM_COMPARE(pui8PtrToData, kpui8LEDONcmdString, ui16UDPRXDataLength))
    {
        eReturnRequest = KE_LED_ON_REQ;
    }
    else if(!MEM_COMPARE(pui8PtrToData, kpui8LEDOFFcmdString, ui16UDPRXDataLength))
    {
        eReturnRequest = KE_LED_OFF_REQ;
    }
    else if(!MEM_COMPARE(pui8PtrToData, kpui8LEDBlinkcmdString, ui16UDPRXDataLength))
    {
        eReturnRequest = KE_LED_BLINK_REQ;
    }
    else
    {
        eReturnRequest = KE_INVALID_REQ;
    }

    return eReturnRequest;
}


/* perform LED status change if a valid request has been received */
LOCAL void manageReceivedData( ke_AppLEDIndexes eLedIndex, uint8 *pui8UDPRXDataPtr, uint16 ui16UDPRXDataLength )
{
    uint8 aui8StringToSend[30]; /* ATTENTION: this array should contain the longest string */
    ke_LEDStatusRequests eLedStatusRequest;

    /* ATTENTION: It is supposed that eLedIndex is in the valid range */

    /* if data are valid */
    if((pui8UDPRXDataPtr != NULL_PTR)
    && (ui16UDPRXDataLength > US_NULL))
    {
        /* get LED status request from received data */
        eLedStatusRequest = checkUDPData(pui8UDPRXDataPtr, ui16UDPRXDataLength);

        /* manage request */
        switch((eLedStatusRequest))
        {
            case KE_LED_ON_REQ:
            {
                /* turn required LED ON */
                OUTCH_SetChannelStatus(ui8LEDIndexToOutLEDCh[eLedIndex], OUTCH_KE_CH_TURN_ON);
                
                /* prepare ACK data string */
                sprintf(aui8StringToSend, "LED %d state is ON", (uint8)(eLedIndex + UC_1));

                break;
            }
            case KE_LED_OFF_REQ:
            {
                /* turn required LED OFF */
                OUTCH_SetChannelStatus(ui8LEDIndexToOutLEDCh[eLedIndex], OUTCH_KE_CH_TURN_OFF);

                /* prepare ACK data string */
                sprintf(aui8StringToSend, "LED %d state is OFF", (uint8)(eLedIndex + UC_1));

                break;
            }
            case KE_LED_BLINK_REQ:
            {
                /* blink required LED */
                OUTCH_SetChannelStatus(ui8LEDIndexToOutLEDCh[eLedIndex], OUTCH_KE_CH_BLINKING);

                /* prepare ACK data string */
                sprintf(aui8StringToSend, "LED %d state is blinking", (uint8)(eLedIndex + UC_1));

                break;
            }
            case KE_INVALID_REQ:
            default:
            {
                /* send a negative ACK string */
                sprintf(aui8StringToSend, "Invalid LED state request");
            }
        }

        /* send UDP ACK packet */
        UDP_SendDataBuffer(ui8LEDIndexToUDPSocket[eLedIndex], aui8StringToSend, MEM_GET_LENGTH(aui8StringToSend));
    }
    else
    {
        /* do nothing */
    }
}




/* End of file */