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
 *
*/


/* ------------- Inclusion files -------------- */

#include "framework/fw_common.h"

#include "app_udp.h"

#include "framework/hal/port.h"
#include "framework/sal/rtos/rtos.h"
#include "framework/sal/udp/udp.h"




/* ---------------- Local typedef definitions ---------------- */

/* connection states enum */
typedef enum
{
    KE_FIRST_STATE
   ,KE_INIT_STATE = KE_FIRST_STATE
   ,KE_RUN_STATE
   ,KE_REPLY_STATE
   ,KE_WAIT_STATE
   ,KE_IDLE_STATE
   ,KE_LAST_STATE = KE_IDLE_STATE
} ke_ConnectionStatus;


/* TEST requests enum */
typedef enum
{
    KE_INVALID_REQ
   ,KE_LED_OFF_REQ
   ,KE_LED_ON_REQ
   ,KE_REPLY_NOW_REQ
} ke_TestRequests;




/* ---------------- Local variables declaration ------------------ */

LOCAL ke_ConnectionStatus enConnStatus = KE_IDLE_STATE;

LOCAL uint8 *pui8UDPRXDataPtr = NULL_PTR;

LOCAL uint16 ui16UDPRXDataLength = US_NULL;

LOCAL uint32 ui32IPAddress = UL_NULL;




/* -------------- Local functions prototypes --------------------- */

LOCAL uint8 checkTestUDPData    (uint8 *, uint16);
LOCAL void  manageTestData      (uint8, uint8 *, uint16);
LOCAL void  pingReqCallback     (void);




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


// TEST!!!
uint8 ui8REPLYAString[] = "rispondo A: ti puzza il culo!";
uint8 ui8REPLYBString[] = "ciao rispondo! adesso proviamo la frammentazione quindi devo scrivere parecchio in questa stringa";
uint8 ui8REPLYXString[] = "rispondo X: cazzo mandi";
uint8 ui8RUNString[] = "ciao sono vivo! adesso proviamo la frammentazione quindi devo scrivere parecchio in questa stringa";
uint8 *pui8StringToSendPtr;

EXPORTED void APP_UDP_PeriodicTask( void )
{
    /* manage periodic tasks */
    ARP_PeriodicTask();
    IPV4_PeriodicTask();
    ICMP_PeriodicTask();
    DHCP_PeriodicTask();

    /* manage connection */
    switch(enConnStatus)
    {
        case KE_INIT_STATE:
        {
            /* TEST!!! */
            PORT_SetPortPinDirection(PORT_ID_D, PORT_PIN_0, PORT_DIR_OUT);
            PORT_SetPortPinDirection(PORT_ID_D, PORT_PIN_1, PORT_DIR_OUT);
            PORT_SetPortPinDirection(PORT_ID_D, PORT_PIN_2, PORT_DIR_OUT);

            /* get obtained IP address via DHCP */
            ui32IPAddress = IPV4_getObtainedIPAdd();
            if(ui32IPAddress != UL_NULL)
            {
//                RTOS_SetCallback (RTOS_CB_ID_1, RTOS_CB_TYPE_SINGLE, 5000, &pingReqCallback);
//                TCPIP_StartPingReq(ui32IPAddress, 0x0A2A0001);
//                TCPIP_StartPingReq(ui32IPAddress, 0x3E958C17);0x40E9A000 0x3E958CA0

                /* TEST: open a UDP socket */
//                UDP_OpenUDPSocket(UDP_SOCKET_5, ui32IPAddress, 0x3E958C17, 2020, 1010);
                UDP_OpenUDPSocket(UDP_SOCKET_5, ui32IPAddress, 0x0A2A0001, 2020, 1010);
                UDP_OpenUDPSocket(UDP_SOCKET_2, ui32IPAddress, 0x0A2A0001, 3030, 4040);

//                enConnStatus = KE_RUN_STATE;
                enConnStatus = KE_IDLE_STATE;
            }
            else
            {
                /* remain in this state and wait for a valid IP address */
            }

            break;
        }
        case KE_RUN_STATE:
        {
            /* PING TEST */
//            RTOS_SetCallback (RTOS_CB_ID_1, RTOS_CB_TYPE_SINGLE, 5000, &pingReqCallback);
//            TCPIP_StartPingReq(ui32IPAddress, 0x0A2A0001);
//            TCPIP_StartPingReq(ui32IPAddress, 0x3E958C17);

//            UDP_SendDataBuffer(UDP_SOCKET_5, ui8RUNString, MEM_GET_LENGTH(ui8RUNString));

            /* go to WAIT state */
            enConnStatus = KE_WAIT_STATE;

            break;
        }
        case KE_REPLY_STATE:
        {
            /* send UDP reply */
            UDP_SendDataBuffer(UDP_SOCKET_5, pui8StringToSendPtr, MEM_GET_LENGTH(pui8StringToSendPtr));

            /* go to WAIT state */
            enConnStatus = KE_WAIT_STATE;

            break;
        }
        case KE_WAIT_STATE:
        {
            /* do test */
            UDP_checkReceivedData(UDP_SOCKET_5, &pui8UDPRXDataPtr, &ui16UDPRXDataLength);
            manageTestData(0, pui8UDPRXDataPtr, ui16UDPRXDataLength);

            UDP_checkReceivedData(UDP_SOCKET_2, &pui8UDPRXDataPtr, &ui16UDPRXDataLength);
            manageTestData(1, pui8UDPRXDataPtr, ui16UDPRXDataLength);

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

// TEST!!!
const uint8 REPLYcmdString[] = "feply";
const uint8 LEDONcmdString[] = "led on";
const uint8 LEDOFFcmdString[] = "led off";

LOCAL uint8 checkTestUDPData( uint8 *pui8PtrToData, uint16 ui16UDPRXDataLength)
{
    uint8 ui8RetCode;

    /* ATTENTION: TEST: check data */
    if(!MEM_COMPARE(pui8PtrToData, LEDONcmdString, ui16UDPRXDataLength))
    {
        ui8RetCode = (uint8)KE_LED_ON_REQ;
    }
    else if(!MEM_COMPARE(pui8PtrToData, LEDOFFcmdString, ui16UDPRXDataLength))
    {
        ui8RetCode = (uint8)KE_LED_OFF_REQ;
    }
    else if(!MEM_COMPARE(pui8PtrToData, REPLYcmdString, 5))
    {
        if(*(pui8PtrToData + 6) == 'A')
        {
            pui8StringToSendPtr = ui8REPLYAString;
        }
        else if(*(pui8PtrToData + 6) == 'B')
        {
            pui8StringToSendPtr = ui8REPLYBString;
        }
        else
        {
            pui8StringToSendPtr = ui8REPLYXString;
        }

        ui8RetCode = (uint8)KE_REPLY_NOW_REQ;
    }
    else
    {
        ui8RetCode = (uint8)KE_INVALID_REQ;
    }

    return ui8RetCode;
}


LOCAL void manageTestData(uint8 ui8LEDNum, uint8 *pui8UDPRXDataPtr, uint16 ui16UDPRXDataLength )
{
    /* if data are valid */
    if((pui8UDPRXDataPtr != NULL_PTR)
    && (ui16UDPRXDataLength > US_NULL))
    {
        /* manage data */
        switch((ke_TestRequests)checkTestUDPData(pui8UDPRXDataPtr, ui16UDPRXDataLength))
        {
            case KE_LED_ON_REQ:
            {
                PORT_SetPortPin(PORT_ID_D, (PORT_ke_PinNumber)ui8LEDNum);

                break;
            }
            case KE_LED_OFF_REQ:
            {
                PORT_ClearPortPin(PORT_ID_D, (PORT_ke_PinNumber)ui8LEDNum);

                break;
            }
            case KE_REPLY_NOW_REQ:
            {
                /* go to REPLY state */
                enConnStatus = KE_REPLY_STATE;

                break;
            }
            default:
            {
                /* discard request */
            }
        }
    }
    else
    {
        /* do nothing */
    }
}


LOCAL void pingReqCallback( void )
{
    //TCPIP_StopPingReq(ui32IPAddress, 0x3E958C17);
}




/* End of file */