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
 * This file pwm.h represents the header file of the PWM component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


#include "../fw_common.h"


/* PWM channels enum */
typedef enum
{
    PWM_KE_FIRST_CHANNEL,
    PWM_KE_CHANNEL_1 = PWM_KE_FIRST_CHANNEL,
    PWM_KE_CHANNEL_2,
    PWM_KE_CHANNEL_3,
    PWM_KE_CHANNEL_4,
    PWM_KE_LAST_CHANNEL = PWM_KE_CHANNEL_4,
    PWM_KE_CHANNEL_CHECK
} PWM_ke_Channels;




EXTERN void PWM_Init( void );

EXTERN void PWM_SetFrequency( uint32 );

EXTERN void PWM_SetDutyCycle( PWM_ke_Channels, uint16 );
