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
 * This file port.c represents the source file of the I/O pins component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


#include "p32mx795f512l.h"
#include "../fw_common.h"

#include "port.h"




/* Function to read a port pin status */
EXPORTED PORT_ke_PinStatus PORT_ReadPortPin(PORT_ke_PortID ePortID, PORT_ke_PinNumber ePortPinNum)
{
    VOLATILE uint32 * pui32RegAddress;
    PORT_ke_PinStatus ePinStatus;

    /* check parameters values */
    if((ePortID < PORT_ID_MAX_CHECK)
    && (ePortPinNum < PORT_PIN_MAX_CHECK))
    {
        /* get port register address */
        switch(ePortID)
        {
            case PORT_ID_A:
            {
                pui32RegAddress = (VOLATILE uint32 *)&PORTA;
                break;
            }
            case PORT_ID_B:
            {
                pui32RegAddress = (VOLATILE uint32 *)&PORTB;
                break;
            }
            case PORT_ID_C:
            {
                pui32RegAddress = (VOLATILE uint32 *)&PORTC;
                break;
            }
            case PORT_ID_D:
            {
                pui32RegAddress = (VOLATILE uint32 *)&PORTD;
                break;
            }
            default:
            {
                /* never arrive here! */
            }
        }

        /* read pin status */
        if((*pui32RegAddress & (1 << ePortPinNum)) > 0)
        {
            /* pin is HIGH */
            ePinStatus = PORT_PIN_HIGH;
        }
        else
        {
            /* pin is LOW */
            ePinStatus = PORT_PIN_LOW;
        }
    }
    else
    {
        /* do nothing - discard request */
    }

    return ePinStatus;
}




/* Function to set a port pin direction
 * TODO: add a return value (SUCCESS, FAIL) */
EXPORTED void PORT_SetPortPinDirection(PORT_ke_PortID ePortID, PORT_ke_PinNumber ePortPinNum, PORT_ke_Direction eDirection)
{
    VOLATILE uint32 * pui32RegAddress;

    /* check parameters values */
    if((ePortID < PORT_ID_MAX_CHECK)
    && (ePortPinNum < PORT_PIN_MAX_CHECK)
    && (eDirection < PORT_DIR_MAX_CHECK))
    {
        /* get port register address */
        switch(ePortID)
        {
            case PORT_ID_A:
            {
                pui32RegAddress = (VOLATILE uint32 *)&TRISA;
                break;
            }
            case PORT_ID_B:
            {
                pui32RegAddress = (VOLATILE uint32 *)&TRISB;
                break;
            }
            case PORT_ID_C:
            {
                pui32RegAddress = (VOLATILE uint32 *)&TRISC;
                break;
            }
            case PORT_ID_D:
            {
                pui32RegAddress = (VOLATILE uint32 *)&TRISD;
                break;
            }
            default:
            {
                /* never arrive here */
            }
        }

        /* execute operation */
        if(PORT_DIR_IN == eDirection)
        {
            *pui32RegAddress |= (uint32)(1 << ePortPinNum);
        }
        else if(PORT_DIR_OUT == eDirection)
        {
            *pui32RegAddress &= (uint32)(~(1 << ePortPinNum));
        }
        else
        {
            /* never arrive here */
        }
    }
    else
    {
        /* do nothing - discard request */
    }
}


/* Function to set a port pin value to 1
 * TODO: add a return value (SUCCESS, FAIL) */
EXPORTED void PORT_SetPortPin(PORT_ke_PortID ePortID, PORT_ke_PinNumber ePortPinNum)
{
    VOLATILE uint32 * pui32RegAddress;

    /* check parameters values */
    if((ePortID < PORT_ID_MAX_CHECK)
    && (ePortPinNum < PORT_PIN_MAX_CHECK))
    {
        /* get port register address */
        switch(ePortID)
        {
            case PORT_ID_A:
            {
                pui32RegAddress = (VOLATILE uint32 *)&LATA;
                break;
            }
            case PORT_ID_B:
            {
                pui32RegAddress = (VOLATILE uint32 *)&LATB;
                break;
            }
            case PORT_ID_C:
            {
                pui32RegAddress = (VOLATILE uint32 *)&LATC;
                break;
            }
            case PORT_ID_D:
            {
                pui32RegAddress = (VOLATILE uint32 *)&LATD;
                break;
            }
            default:
            {
                /* never arrive here! */
            }
        }
        
        /* execute pin operation */
        *pui32RegAddress |= (uint32)(1 << ePortPinNum);
    }
    else
    {
        /* do nothing - discard request */
    }
}


/* Function to set a port pin value to 0
 * TODO: add a return value (SUCCESS, FAIL) */
EXPORTED void PORT_ClearPortPin(PORT_ke_PortID ePortID, PORT_ke_PinNumber ePortPinNum)
{
    VOLATILE uint32 * pui32RegAddress;

    /* check parameters values */
    if((ePortID < PORT_ID_MAX_CHECK)
    && (ePortPinNum < PORT_PIN_MAX_CHECK))
    {
        /* get port register address */
        switch(ePortID)
        {
            case PORT_ID_A:
            {
                pui32RegAddress = (VOLATILE uint32 *)&LATA;
                break;
            }
            case PORT_ID_B:
            {
                pui32RegAddress = (VOLATILE uint32 *)&LATB;
                break;
            }
            case PORT_ID_C:
            {
                pui32RegAddress = (VOLATILE uint32 *)&LATC;
                break;
            }
            case PORT_ID_D:
            {
                pui32RegAddress = (VOLATILE uint32 *)&LATD;
                break;
            }
            default:
            {
                /* never arrive here! */
            }
        }

        /* execute pin operation */
        *pui32RegAddress &= (uint32)(~(1 << ePortPinNum));
    }
    else
    {
        /* do nothing - discard request */
    }
}


/* Function to toggle a port pin value
 * TODO: add a return value (SUCCESS, FAIL) */
EXPORTED void PORT_TogglePortPin(PORT_ke_PortID ePortID, PORT_ke_PinNumber ePortPinNum)
{
    uint32 ui32PortValue;

    /* check parameters values */
    if((ePortID < PORT_ID_MAX_CHECK)
    && (ePortPinNum < PORT_PIN_MAX_CHECK))
    {
        /* read port value */
        switch(ePortID)
        {
            case PORT_ID_A:
            {
                ui32PortValue = PORTA;
                break;
            }
            case PORT_ID_B:
            {
                ui32PortValue = PORTB;
                break;
            }
            case PORT_ID_C:
            {
                ui32PortValue = PORTC;
                break;
            }
            case PORT_ID_D:
            {
                ui32PortValue = PORTD;
                break;
            }
            default:
            {
                /* never arrive here */
            }
        }

        /* mask the required bit */
        ui32PortValue &= (1 << ePortPinNum);

        /* call port function according to actual value */
        if(ui32PortValue > 0)
        {
            PORT_ClearPortPin(ePortID, ePortPinNum);
        }
        else
        {
            PORT_SetPortPin(ePortID, ePortPinNum);
        }
    }
    else
    {
        /* do nothing - discard request */
    }
}
