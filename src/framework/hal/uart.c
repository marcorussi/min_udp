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
 * This file uart.c represents the source file of the UART component.
 *
 * Author : Marco Russi
 *
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 *
*/


#include <xc.h>
#include <sys/attribs.h>
#include "p32mx795f512l.h"
#include "../fw_common.h"
#include "../sal/sys/sys.h"

#include "uart.h"


// TODO: verify if it is necessary to clear the RX flag at every UART1 byte reception
// TODO: verify if it is necessary to clear the TX flag at every UART1 byte transmission


/* UART data buffer number of bytes */
#define UC_MAX_UART_BUFF_BYTES		((uint8)64)
#define UC_MAX_UART_BUFF_MSG		((uint8)8)


/* UART1 RX and TX interrupt numbers */
#define UART1_RX_IRQ_NUM		(27)
#define UART1_TX_IRQ_NUM		(28)


/* UxMODE register bits positions */
#define UxMODE_ON_BIT_POS		15
#define UxMODE_SIDL_BIT_POS		13
#define UxMODE_IREN_BIT_POS		12
#define UxMODE_RTSMD_BIT_POS		11
#define UxMODE_UEN_BIT_POS		8
#define UxMODE_WAKE_BIT_POS		7
#define UxMODE_LPBACK_BIT_POS		6
#define UxMODE_ABAUD_BIT_POS		5
#define UxMODE_RXINV_BIT_POS		4
#define UxMODE_BRGH_BIT_POS		3
#define UxMODE_PDSEL_BIT_POS		1
#define UxMODE_STSEL_BIT_POS		0

/* UxMODE register bits value */
#define UxMODE_ON_VALUE			0	/* init as disabled */
#define UxMODE_SIDL_VALUE		0
#define UxMODE_IREN_VALUE		0
#define UxMODE_RTSMD_VALUE		0
#define UxMODE_UEN_VALUE		0
#define UxMODE_WAKE_VALUE		0
#define UxMODE_LPBACK_VALUE		0
#define UxMODE_ABAUD_VALUE		0
#define UxMODE_RXINV_VALUE		0
#define UxMODE_BRGH_VALUE		0
#define UxMODE_PDSEL_VALUE		1	/* 8-bit data, even parity */
#define UxMODE_STSEL_VALUE		0

/* UxMODE register value */
#define UxMODE_REG_VALUE		((UxMODE_ON_VALUE << UxMODE_ON_BIT_POS)	| \
					 (UxMODE_SIDL_VALUE << UxMODE_SIDL_BIT_POS)	| \
					 (UxMODE_IREN_VALUE << UxMODE_IREN_BIT_POS)	| \
					 (UxMODE_RTSMD_VALUE << UxMODE_RTSMD_BIT_POS)	| \
					 (UxMODE_UEN_VALUE << UxMODE_UEN_BIT_POS)	| \
					 (UxMODE_WAKE_VALUE << UxMODE_WAKE_BIT_POS)	| \
					 (UxMODE_LPBACK_VALUE << UxMODE_LPBACK_BIT_POS)	| \
					 (UxMODE_ABAUD_VALUE << UxMODE_ABAUD_BIT_POS)	| \
					 (UxMODE_RXINV_VALUE << UxMODE_RXINV_BIT_POS)	| \
				         (UxMODE_BRGH_VALUE << UxMODE_BRGH_BIT_POS)	| \
					 (UxMODE_PDSEL_VALUE << UxMODE_PDSEL_BIT_POS)	| \
					 (UxMODE_STSEL_VALUE << UxMODE_STSEL_BIT_POS)	  )





/* UxSTA register bits positions */
#define UxSTA_ADMEN_BIT_POS		24
#define UxSTA_ADDR_BIT_POS		16
#define UxSTA_UTXISEL_BIT_POS		14
#define UxSTA_UTXINV_BIT_POS		13
#define UxSTA_URXEN_BIT_POS		12
#define UxSTA_UTXBRK_BIT_POS		11
#define UxSTA_UTXEN_BIT_POS		10
#define UxSTA_UTXBF_BIT_POS		9
#define UxSTA_TRMT_BIT_POS		8
#define UxSTA_URXISEL_BIT_POS		6
#define UxSTA_ADDEN_BIT_POS		5
#define UxSTA_RIDLE_BIT_POS		4
#define UxSTA_PERR_BIT_POS		3
#define UxSTA_FERR_BIT_POS		2
#define UxSTA_OERR_BIT_POS		1
#define UxSTA_URXDA_BIT_POS		0

/* UxSTA register bits value */
#define UxSTA_ADMEN_VALUE		0
#define UxSTA_ADDR_VALUE		0
#define UxSTA_UTXISEL_VALUE		0	/* Interrupt is generated when the transmit buffer contains at least one empty space */
#define UxSTA_UTXINV_VALUE		0
#define UxSTA_URXEN_VALUE		0	/* init as disabled */
#define UxSTA_UTXBRK_VALUE		0
#define UxSTA_UTXEN_VALUE		0	/* init as disabled */
#define UxSTA_UTXBF_VALUE		0	/* read-only */
#define UxSTA_TRMT_VALUE		0	/* read-only */
#define UxSTA_URXISEL_VALUE		0	/* Interrupt flag bit is set when a character is received */
#define UxSTA_ADDEN_VALUE		0
#define UxSTA_RIDLE_VALUE		0	/* read-only */
#define UxSTA_PERR_VALUE		0	/* read-only */
#define UxSTA_FERR_VALUE		0	/* read-only */
#define UxSTA_OERR_VALUE		0	/* read-only/clear */
#define UxSTA_URXDA_VALUE		0	/* read-only */

/* UxSTA register value */
#define UxSTA_REG_VALUE			((UxSTA_ADMEN_VALUE << UxSTA_ADMEN_BIT_POS)	| \
					 (UxSTA_ADDR_VALUE << UxSTA_ADDR_BIT_POS)	| \
					 (UxSTA_UTXISEL_VALUE << UxSTA_UTXISEL_BIT_POS)	| \
					 (UxSTA_UTXINV_VALUE << UxSTA_UTXINV_BIT_POS)	| \
					 (UxSTA_URXEN_VALUE << UxSTA_URXEN_BIT_POS)	| \
					 (UxSTA_UTXBRK_VALUE << UxSTA_UTXBRK_BIT_POS)	| \
					 (UxSTA_UTXEN_VALUE << UxSTA_UTXEN_BIT_POS)	| \
				         (UxSTA_URXISEL_VALUE << UxSTA_URXISEL_BIT_POS)	| \
					 (UxSTA_ADDEN_VALUE << UxSTA_ADDEN_BIT_POS)	  )




/* UxBRG register value */
#define UxBRG_REG_VALUE         ((uint32)((SYS_UL_FPB / (UL_16 * UART_UL_BAUD_RATE_BIT_PER_S)) - UL_1))




/* Macros to enable/disable module, TX and RX */
#define ENABLE_UART1()		(U1MODESET = (1 << UxMODE_ON_BIT_POS))
#define ENABLE_UART1_TX()	(U1STASET = (1 << UxSTA_UTXEN_BIT_POS))
#define ENABLE_UART1_RX()	(U1STASET = (1 << UxSTA_URXEN_BIT_POS))
#define DISABLE_UART1()		(U1MODECLR = (1 << UxMODE_ON_BIT_POS))
#define DISABLE_UART1_TX()	(U1STACLR = (1 << UxSTA_UTXEN_BIT_POS))
#define DISABLE_UART1_RX()	(U1STACLR = (1 << UxSTA_URXEN_BIT_POS))




/* DMA controller and  channel x registers bits positions */
#define DMACON_ON_BIT_POS		15
#define DCHxCON_CHPRI_BIT_POS		0
#define DCHxCON_CHEN_BIT_POS		7
#define DCHxECON_SIRQEN_POS_BIT		4
#define DCHxECON_PATEN_POS_BIT		5
#define DCHxECON_CHSIRQ_POS_BIT		8
#define DCHxINT_FLAGS_POS_BIT		0
#define DCHxINT_ENS_POS_BIT		16
#define DCHxINT_CHERIE_POS_BIT		16
#define DCHxINT_CHBCIE_POS_BIT		19

/* Macros for DMA controller */
#define ENABLE_DMA_MODULE()		(DMACONSET = (1 << DMACON_ON_BIT_POS))
#define DISABLE_DMA_MODULE()		(DMACONCLR = (1 << DMACON_ON_BIT_POS))

/* Macros for DMA channel 1 */
#define ENABLE_DMA_CH1()		(DCH1CONSET = (1 << DCHxCON_CHEN_BIT_POS))
#define DISABLE_DMA_CH1()		(DCH1CONCLR = (1 << DCHxCON_CHEN_BIT_POS))
#define SET_DMA_CH1_PRI(x)		(DCH1CONSET = ((x) << DCHxCON_CHPRI_BIT_POS))
#define SET_DMA_CH1_START_IRQ(x)	(DCH1ECONSET = ((x) << DCHxECON_CHSIRQ_POS_BIT))
#define CLEAR_DMA_CH1_INT_FLAGS()	(DCH1INTCLR = (0xFF << DCHxINT_FLAGS_POS_BIT))
#define DISABLE_DMA_CH1_INT()		(DCH1INTCLR = (0xFF << DCHxINT_ENS_POS_BIT))
#define ENABLE_DMA_CH1_ERR_INT()	(DCH1INTSET = (1 << DCHxINT_CHERIE_POS_BIT))
#define ENABLE_DMA_CH1_BLOCK_INT()	(DCH1INTSET = (1 << DCHxINT_CHBCIE_POS_BIT))
#define DISABLE_DMA_CH1_ERR_INT()	(DCH1INTCLR = (1 << DCHxINT_CHERIE_POS_BIT))
#define DISABLE_DMA_CH1_BLOCK_INT()	(DCH1INTCLR = (1 << DCHxINT_CHBCIE_POS_BIT))

/* Macros for DMA channel 2 */
#define ENABLE_DMA_CH2()		(DCH2CONSET = (1 << DCHxCON_CHEN_BIT_POS))
#define DISABLE_DMA_CH2()		(DCH2CONCLR = (1 << DCHxCON_CHEN_BIT_POS))
#define SET_DMA_CH2_PRI(x)		(DCH2CONSET = ((x) << DCHxCON_CHPRI_BIT_POS))
#define SET_DMA_CH2_START_IRQ(x)	(DCH2ECONSET = ((x) << DCHxECON_CHSIRQ_POS_BIT))
#define CLEAR_DMA_CH2_INT_FLAGS()	(DCH2INTCLR = (0xFF << DCHxINT_FLAGS_POS_BIT))
#define DISABLE_DMA_CH2_INT()		(DCH2INTCLR = (0xFF << DCHxINT_ENS_POS_BIT))
#define ENABLE_DMA_CH2_ERR_INT()	(DCH2INTSET = (1 << DCHxINT_CHERIE_POS_BIT))
#define ENABLE_DMA_CH2_BLOCK_INT()	(DCH2INTSET = (1 << DCHxINT_CHBCIE_POS_BIT))
#define DISABLE_DMA_CH2_ERR_INT()	(DCH2INTCLR = (1 << DCHxINT_CHERIE_POS_BIT))
#define DISABLE_DMA_CH2_BLOCK_INT()	(DCH2INTCLR = (1 << DCHxINT_CHBCIE_POS_BIT))




/* UART TX, RX, and ERROR interrupts definitions */
#define U1TX_INT_FLAG_BIT_POS		28
#define U1TX_INT_EN_BIT_POS		28
#define U1RX_INT_FLAG_BIT_POS		27
#define U1RX_INT_EN_BIT_POS		27
#define U1ERR_INT_FLAG_BIT_POS		26
#define U1ERR_INT_EN_BIT_POS		26
#define U1_INT_PRI_BIT_POS		2
#define U1_INT_SUBPRI_BIT_POS		0
/* TX */
#define ENABLE_U1TX_INT()	(IEC0SET = (1 << U1TX_INT_EN_BIT_POS))
#define DISABLE_U1TX_INT()	(IEC0CLR = (1 << U1TX_INT_EN_BIT_POS))
#define CLEAR_U1TX_INT()	(IFS0CLR = (1 << U1TX_INT_FLAG_BIT_POS))
/* RX */
#define ENABLE_U1RX_INT()	(IEC0SET = (1 << U1RX_INT_EN_BIT_POS))
#define DISABLE_U1RX_INT()	(IEC0CLR = (1 << U1RX_INT_EN_BIT_POS))
#define CLEAR_U1RX_INT()	(IFS0CLR = (1 << U1RX_INT_FLAG_BIT_POS))
/* ERROR */
#define ENABLE_U1ERR_INT()	(IEC0SET = (1 << U1ERR_INT_EN_BIT_POS))
#define DISABLE_U1ERR_INT()	(IEC0CLR = (1 << U1ERR_INT_EN_BIT_POS))
#define CLEAR_U1ERRX_INT()	(IFS0CLR = (1 << U1ERR_INT_FLAG_BIT_POS))
/* Interrupt priority */
#define SET_U1_INT_PRI(x)	(IPC6SET = ((x) << U1_INT_PRI_BIT_POS))
#define SET_U1_INT_SUBPRI(x)	(IPC6SET = ((x) << U1_INT_SUBPRI_BIT_POS))
#define CLEAR_U1_INT_PRI()	(IPC6CLR = ((7 << U1_INT_PRI_BIT_POS) | (3 << U1_INT_SUBPRI_BIT_POS)))




/* DMA channel 1 interrupts definitions */
#define DMA1_INT_FLAG_BIT_POS		17
#define DMA1_INT_EN_BIT_POS		17
#define DMA1_INT_PRI_BIT_POS		10
#define DMA1_INT_SUBPRI_BIT_POS		8

#define ENABLE_DMA1_INT()	(IEC1SET = (1 << DMA1_INT_EN_BIT_POS))
#define DISABLE_DMA1_INT()	(IEC1CLR = (1 << DMA1_INT_EN_BIT_POS))
#define CLEAR_DMA1_INT()	(IFS1CLR = (1 << DMA1_INT_FLAG_BIT_POS))
#define SET_DMA1_INT_PRI(x)	(IPC9SET = ((x) << DMA1_INT_PRI_BIT_POS))
#define SET_DMA1_INT_SUBPRI(x)	(IPC9SET = ((x) << DMA1_INT_SUBPRI_BIT_POS))
#define CLEAR_DMA1_INT_PRI()	(IPC9CLR = ((7 << DMA1_INT_PRI_BIT_POS) | (3 << DMA1_INT_SUBPRI_BIT_POS)))

/* DMA channel 2 interrupts definitions */
#define DMA2_INT_FLAG_BIT_POS		18
#define DMA2_INT_EN_BIT_POS		18
#define DMA2_INT_PRI_BIT_POS		18
#define DMA2_INT_SUBPRI_BIT_POS		16

#define ENABLE_DMA2_INT()	(IEC1SET = (1 << DMA2_INT_EN_BIT_POS))
#define DISABLE_DMA2_INT()	(IEC1CLR = (1 << DMA2_INT_EN_BIT_POS))
#define CLEAR_DMA2_INT()	(IFS1CLR = (1 << DMA2_INT_FLAG_BIT_POS))
#define SET_DMA2_INT_PRI(x)	(IPC9SET = ((x) << DMA2_INT_PRI_BIT_POS))
#define SET_DMA2_INT_SUBPRI(x)	(IPC9SET = ((x) << DMA2_INT_SUBPRI_BIT_POS))
#define CLEAR_DMA2_INT_PRI()	(IPC9CLR = ((7 << DMA2_INT_PRI_BIT_POS) | (3 << DMA2_INT_SUBPRI_BIT_POS)))




/* RX FIFO buffer macros */
#define IS_RX_BUFFER_NOT_FULL()		(ui8RxFIFOBuffMsgCounter < UC_MAX_UART_BUFF_MSG)
#define IS_RX_BUFFER_NOT_EMPTY()	(ui8RxFIFOBuffMsgCounter > UC_NULL)
// TODO: verify that decrement operation is atomic. This operation should not be preempted
//#define DECREMENT_RX_MSG_COUNTER()	(DISABLE_DMA_CH1_BLOCK_INT(); 	\
//				 	 ui8RxFIFOBuffMsgCounter--;	\
//				 	 ENABLE_DMA_CH1_BLOCK_INT())
#define DECREMENT_RX_MSG_COUNTER()	(ui8RxFIFOBuffMsgCounter--)
#define INCREMENT_RX_MSG_COUNTER()	(ui8RxFIFOBuffMsgCounter++)
/* TX FIFO buffer macros */
#define IS_TX_BUFFER_NOT_FULL()		(ui8TxFIFOBuffMsgCounter < UC_MAX_UART_BUFF_MSG)
#define IS_TX_BUFFER_NOT_EMPTY()	(ui8TxFIFOBuffMsgCounter > UC_NULL)
#define DECREMENT_TX_MSG_COUNTER()	(ui8TxFIFOBuffMsgCounter--)
// TODO: verify that increment operation is atomic. This operation should not be preempted
//#define INCREMENT_TX_MSG_COUNTER()	(DISABLE_DMA_CH2_BLOCK_INT(); 	\
//				 	 ui8TxFIFOBuffMsgCounter++;	\
//				 	 ENABLE_DMA_CH12_BLOCK_INT())
#define INCREMENT_TX_MSG_COUNTER()	(ui8TxFIFOBuffMsgCounter++)




/* UART RX and TX FIFO buffers declaration */
EXPORTED uint8 UART_RxFIFOBuffer[UC_MAX_UART_BUFF_MSG][UC_MAX_UART_BUFF_BYTES];
EXPORTED uint8 UART_TxFIFOBuffer[UC_MAX_UART_BUFF_MSG][UC_MAX_UART_BUFF_BYTES];


/* UART RX FIFO buffer next free location index */
LOCAL uint8 ui8RxFIFOBuffNextFreeIndex;
/* UART RX FIFO buffer last busy location index */
LOCAL uint8 ui8RxFIFOBuffLastBusyIndex;

/* UART TX FIFO buffer next free location index */
LOCAL uint8 ui8TxFIFOBuffNextFreeIndex;
/* UART TX FIFO buffer last busy location index */
LOCAL uint8 ui8TxFIFOBuffLastBusyIndex;

/* UART RX FIFO buffer message counter */
LOCAL uint8 ui8RxFIFOBuffMsgCounter;
/* UART TX FIFO buffer message counter */
LOCAL uint8 ui8TxFIFOBuffMsgCounter;


/* Local functions prototypes */
LOCAL void configDMAChannelRX( void );
LOCAL void configDMAChannelTX( void );




/* Init UART1 module */
EXPORTED void UART_Init( void )
{
    /* reset RX next free location index */
    ui8RxFIFOBuffNextFreeIndex = UC_NULL;
    /* reset RX last busy location index */
    ui8RxFIFOBuffLastBusyIndex = UC_NULL;
    /* reset TX next free location index */
    ui8TxFIFOBuffNextFreeIndex = UC_NULL;
    /* reset TX last busy location index */
    ui8TxFIFOBuffLastBusyIndex = UC_NULL;

    /* configure a DMA channel for UART1 RX purpose */
    configDMAChannelRX();
    /* configure a DMA channel for UART1 TX purpose */
    configDMAChannelTX();

    /* configure UART1 registers */
    U1BRG = UxBRG_REG_VALUE;
    U1MODE = UxMODE_REG_VALUE;
    U1STA = UxSTA_REG_VALUE;

    /* clear int flags */
    CLEAR_U1TX_INT();
    CLEAR_U1RX_INT();
    CLEAR_U1ERRX_INT();

    /* clear and set int priority */
    CLEAR_U1_INT_PRI();
    SET_U1_INT_PRI(1);
    SET_U1_INT_SUBPRI(1);

    /* enable UART1 ERROR int */
    ENABLE_U1ERR_INT();
    /* disable RX and TX int - not used */
//    ENABLE_U1RX_INT();
//    ENABLE_U1TX_INT();

    /* enable module and then TX and RX */
    // ATTENTION: RX and TX are enabled at intialisation!
    ENABLE_UART1();
    ENABLE_UART1_TX();
    ENABLE_UART1_RX();
}


/* function to enable or disable UART reception */
EXPORTED void UART_EnableUARTReception( boolean bEnableRx)
{
    /* only B_TRUE value enables RX */
    if(B_TRUE == bEnableRx)
    {
        /* clear RX int flag */
        CLEAR_U1RX_INT();
        /* enable RX int */
//        ENABLE_U1RX_INT();
        /* enable RX */
        ENABLE_UART1_RX();
    }
    /* B_FALSE and any other wrong values disable RX */
    else
    {
        /* disable RX */
        DISABLE_UART1_RX();
        /* disable an eventual RX int */
//        DISABLE_U1RX_INT();
        /* clear RX int flag */
        CLEAR_U1RX_INT();
    }
}


/* function to enable or disable UART transmission */
EXPORTED void UART_EnableUARTTransmission( boolean bEnableTx)
{
    /* only B_TRUE value enables TX */
    if(B_TRUE == bEnableTx)
    {
        /* clear TX int flag */
        CLEAR_U1TX_INT();
        /* enable TX int */
//        ENABLE_U1TX_INT();
        /* enable TX */
        ENABLE_UART1_TX();
    }
    /* B_FALSE and any other wrong values disable TX */
    else
    {
        /* disable TX */
        DISABLE_UART1_TX();
        /* disable an eventual TX int */
//        DISABLE_U1TX_INT();
        /* clear TX int flag */
        CLEAR_U1TX_INT();
    }
}


/* function to get the last received message from the RX FIFO buffer */
EXPORTED boolean UART_bGetLastReceivedMessage( uint8 * pui8LastMsg )
{
    boolean bLastMsgValid;

    /* if buffer is not empty than get last message */
    if(IS_RX_BUFFER_NOT_EMPTY())
    {
        /* copy last busy message pointer */
        pui8LastMsg = &(UART_RxFIFOBuffer[ui8RxFIFOBuffLastBusyIndex][UC_NULL]);

        /* increment last busy location index  */
        ui8RxFIFOBuffLastBusyIndex++;
        if(UC_MAX_UART_BUFF_MSG == ui8RxFIFOBuffLastBusyIndex)
        {
            ui8RxFIFOBuffLastBusyIndex = UC_NULL;
        }

        /* decrement FIFO buffer message counter */
        DECREMENT_RX_MSG_COUNTER();

        bLastMsgValid = B_TRUE;
    }
    /* else if buffer is empty */
    else
    {
        /* no messages to read */
        pui8LastMsg = NULL;

        bLastMsgValid = B_FALSE;
    }

    return bLastMsgValid;
}


/* function to put a message to send into the TX FIFO buffer */
EXPORTED boolean UART_bPutMessageToSend( uint8 * pui8MsgToSend )
{
    boolean bTxBufferFull;

    /* if buffer is not full than put the message to send */
    if(IS_TX_BUFFER_NOT_FULL())
    {
        /* copy message in the next free location */
        UART_TxFIFOBuffer[ui8TxFIFOBuffNextFreeIndex][UC_NULL] = *pui8MsgToSend;

        /* increment next free location index  */
        ui8TxFIFOBuffNextFreeIndex++;
        if(UC_MAX_UART_BUFF_MSG == ui8TxFIFOBuffNextFreeIndex)
        {
            ui8TxFIFOBuffNextFreeIndex = UC_NULL;
        }

        /* increment FIFO buffer message counter */
        INCREMENT_TX_MSG_COUNTER();

        bTxBufferFull = B_FALSE;
    }
    /* else if buffer is empty */
    else
    {
        /* buffer is full - discard request */

        bTxBufferFull = B_TRUE;
    }

    return bTxBufferFull;
}




/* function to configure a DMA channel 1 for UART1 RX purpose */
LOCAL void configDMAChannelRX( void )
{
    /* disable DMA channel 1 interrupt */
    DISABLE_DMA1_INT();
    /* clear any existing DMA channel 1 interrupt flag */
    CLEAR_DMA1_INT();

    /* configure channel 1 as no chaining - default */

    /* enable the DMA controller */
    ENABLE_DMA_MODULE();

    /* set DMA channel 1 priority */
    SET_DMA_CH1_PRI(3);

    /* start on irq enabled, abort transfer on pattern match */
//    DCH1ECONSET = ((1 << DCH1ECON_SIRQEN_POS_BIT) | (1 << DCH1ECON_PATEN_POS_BIT));
    /* start on irq enabled*/
    DCH1ECONSET = (1 << DCHxECON_SIRQEN_POS_BIT);
    /* set UART1 RX as start irq */
    SET_DMA_CH1_START_IRQ(UART1_RX_IRQ_NUM);
    /* set abort pattern match */
//    DCH1DAT = '\r';		/* carriage return */
    /* set source start address as UART1 RX register */
    DCH1SSA = (uint32 volatile)&U1RXREG;
    /* set destination start address as UART RX FIFO buffer */
    DCH1DSA = (uint32 volatile)&(UART_RxFIFOBuffer[ui8RxFIFOBuffNextFreeIndex][UC_NULL]);
    /* set source size */
    DCH1SSIZ = 1;
    /* set destination size */
    DCH1DSIZ = UC_MAX_UART_BUFF_BYTES;
    /* set cell size */
    DCH1CSIZ = 1;

    /* Specific DMA channel 1 interrupts */
    /* reset channel 1 int */
    DISABLE_DMA_CH1_INT();
    /* clear channel 1 int flags */
    CLEAR_DMA_CH1_INT_FLAGS();
    /* enable Block Complete and error interrupts */
    ENABLE_DMA_CH1_ERR_INT();
    ENABLE_DMA_CH1_BLOCK_INT();

    /* General DMA channel 1 interrupts */
    /* clear the DMA channel 1 priority and sub-priority */
    CLEAR_DMA1_INT_PRI();
    /* set the DMA channel 1 priority and sub-priority */
    SET_DMA1_INT_PRI(5);
    SET_DMA1_INT_SUBPRI(1);
    /* enable DMA channel 1 interrupt */
    ENABLE_DMA1_INT();

    /* turn DMA channel 1 on */
    ENABLE_DMA_CH1();
}


/* function to configure a DMA channel 2 for UART1 TX purpose */
LOCAL void configDMAChannelTX( void )
{
    /* disable DMA channel 2 interrupt */
    DISABLE_DMA2_INT();
    /* clear any existing DMA channel 2 interrupt flag */
    CLEAR_DMA2_INT();

    /* configure channel 2 as no chaining - default */

    /* enable the DMA controller */
//    ENABLE_DMA_MODULE();

    /* set DMA channel 2 priority */
    SET_DMA_CH2_PRI(3);

    /* start on irq enabled, abort transfer on pattern match */
//    DCH2ECONSET = ((1 << DCH2ECON_SIRQEN_POS_BIT) | (1 << DCH2ECON_PATEN_POS_BIT));
    /* start on irq enabled*/
    DCH2ECONSET = (1 << DCHxECON_SIRQEN_POS_BIT);
    /* set UART1 RX as start irq */
    SET_DMA_CH2_START_IRQ(UART1_TX_IRQ_NUM);
    /* set abort pattern match */
//    DCH2DAT = '\r';		/* carriage return */
    /* set source start address as UART TX FIFO buffer */
    DCH2SSA = (uint32 volatile)&(UART_TxFIFOBuffer[ui8TxFIFOBuffLastBusyIndex][UC_NULL]);
    /* set destination start address as UART1 TX register */
    DCH2DSA = (uint32 volatile)&U1TXREG;
    /* set source size */
    DCH2SSIZ = 1;
    /* set destination size */
    DCH2DSIZ = UC_MAX_UART_BUFF_BYTES;
    /* set cell size */
    DCH2CSIZ = 1;

    /* Specific DMA channel 2 interrupts */
    /* reset channel 2 int */
    DISABLE_DMA_CH2_INT();
    /* clear channel 2 int flags */
    CLEAR_DMA_CH2_INT_FLAGS();
    /* enable Block Complete and error interrupts */
    ENABLE_DMA_CH2_ERR_INT();
    ENABLE_DMA_CH2_BLOCK_INT();

    /* General DMA channel 2 interrupts */
    /* clear the DMA channel 2 priority and sub-priority */
    CLEAR_DMA2_INT_PRI();
    /* set the DMA channel 2 priority and sub-priority */
    SET_DMA2_INT_PRI(5);
    SET_DMA2_INT_SUBPRI(0);
    /* enable DMA channel 2 interrupt */
    ENABLE_DMA2_INT();

    /* turn DMA channel 2 on */
    ENABLE_DMA_CH2();
}




/* UART 1 interrupt service routine */
void __ISR(_UART_1_VECTOR, ipl1) UART1Handler(void)
{
    /* clear int flags */
//    CLEAR_U1TX_INT();
//    CLEAR_U1RX_INT();
    CLEAR_U1ERRX_INT();
}


/* DMA channel 1 interrupt service routine */
void __ISR(_DMA_1_VECTOR, ipl5) DMA1Handler(void)
{
    /* increment RX FIFO buffer message counter */
    INCREMENT_RX_MSG_COUNTER();
    /* if buffer is not full - carry on with DMA buffering */
    if(IS_RX_BUFFER_NOT_FULL())
    {
        /* increment next free location index */
        ui8RxFIFOBuffNextFreeIndex++;
        if(UC_MAX_UART_BUFF_MSG == ui8RxFIFOBuffNextFreeIndex)
        {
            ui8RxFIFOBuffNextFreeIndex = UC_NULL;
        }

        /* update destination address to next free buffer location */
        DCH1DSA = (uint32 volatile)&(UART_RxFIFOBuffer[ui8RxFIFOBuffNextFreeIndex][UC_NULL]);

        /* turn DMA channel 1 on again */
        ENABLE_DMA_CH1();	// TODO: try this! maybe it is necessary to clear it first
    }
    /* else if buffer is full */
    else
    {
        //TODO: stop UART 1 reception
    }

    /* clear int flags */
    CLEAR_DMA_CH1_INT_FLAGS();	// TODO: try this! maybe it is not necessary
    CLEAR_DMA1_INT_PRI();

    // TODO: check right order between enable channel and clear int flags
}


/* DMA channel 2 interrupt service routine */
void __ISR(_DMA_2_VECTOR, ipl5) DMA2Handler(void)
{
    /* decrement TX FIFO buffer message counter */
    DECREMENT_TX_MSG_COUNTER();
    /* if TX buffer is not empty - carry on with DMA buffering */
    if(IS_TX_BUFFER_NOT_EMPTY())
    {
        /* increment last busy location index */
        ui8TxFIFOBuffLastBusyIndex++;
        if(UC_MAX_UART_BUFF_MSG == ui8TxFIFOBuffLastBusyIndex)
        {
            ui8TxFIFOBuffLastBusyIndex = UC_NULL;
        }

        /* update destination address to next free buffer location */
        DCH2SSA = (uint32 volatile)&(UART_TxFIFOBuffer[ui8TxFIFOBuffLastBusyIndex][UC_NULL]);
        /* turn DMA channel 2 on again */
        ENABLE_DMA_CH2();	// TODO: try this! maybe it is necessary to clear it first
    }
    /* else if TX buffer is empty */
    else
    {
        //TODO: stop UART 1 transmission
    }

    /* clear int flags */
    CLEAR_DMA_CH2_INT_FLAGS();	// TODO: try this! maybe it is not necessary
    CLEAR_DMA2_INT_PRI();

    // TODO: check right order between enable channel and clear int flags
}




/******************************* END OF FILE ********************************/
