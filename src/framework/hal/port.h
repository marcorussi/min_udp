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
 * This file port.h represents the header file of the I/O pins component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


#include "p32mx795f512l.h"

#include "../fw_common.h"


/* Port pin number enum */
typedef enum
{
    PORT_PIN_0,
    PORT_PIN_1,
    PORT_PIN_2,
    PORT_PIN_3,
    PORT_PIN_4,
    PORT_PIN_5,
    PORT_PIN_6,
    PORT_PIN_7,
    PORT_PIN_8,
    PORT_PIN_9,
    PORT_PIN_10,
    PORT_PIN_11,
    PORT_PIN_12,
    PORT_PIN_13,
    PORT_PIN_14,
    PORT_PIN_15,
    PORT_PIN_MAX_CHECK
} PORT_ke_PinNumber;


/* Port ID enum */
typedef enum
{
    PORT_ID_A,
    PORT_ID_B,
    PORT_ID_C,
    PORT_ID_D,
    PORT_ID_MAX_CHECK
} PORT_ke_PortID;


/* Port pin direction enum */
/* Enum order is important! */
typedef enum
{
    PORT_DIR_OUT,
    PORT_DIR_IN,
    PORT_DIR_MAX_CHECK
} PORT_ke_Direction;


/* Port pin status enum */
typedef enum
{
    PORT_PIN_LOW,
    PORT_PIN_HIGH,
    PORT_PIN_STATUS_CHECK
} PORT_ke_PinStatus;


/* Exported function prototypes */
EXTERN void                 PORT_SetPortPinDirection(PORT_ke_PortID, PORT_ke_PinNumber, PORT_ke_Direction);

EXTERN void                 PORT_SetPortPin         (PORT_ke_PortID, PORT_ke_PinNumber);

EXTERN void                 PORT_ClearPortPin       (PORT_ke_PortID, PORT_ke_PinNumber);

EXTERN PORT_ke_PinStatus    PORT_ReadPortPin        (PORT_ke_PortID, PORT_ke_PinNumber);

EXTERN void                 PORT_TogglePortPin      (PORT_ke_PortID, PORT_ke_PinNumber);
