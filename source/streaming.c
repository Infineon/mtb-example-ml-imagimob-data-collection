/******************************************************************************
* File Name:   streaming.c
*
* Description: This file contains setup functions for initializing the streaming
* interface. It supports using either UART or USB CDC. The selected protocol is
* based on a define from the Makefile.
*
*******************************************************************************
* Copyright 2023, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#include "streaming.h"
#include "cybsp.h"
#include "config.h"

/*******************************************************************************
* Function Name: mtb_data_streaming_xfer_done
********************************************************************************
* Summary:
*  Process any completion steps necessary for the streaming interface.
*
*******************************************************************************/
static void mtb_data_streaming_xfer_done(const void* tag, cy_rslt_t result)
{
    CY_UNUSED_PARAMETER(tag);
    //HALT_ON_ERROR(result);
}

#include "cyhal_uart.h"

#if COLLECTION_MODE_SELECT == IMU_COLLECTION
#define UART_BAUD_RATE              (115200u)
#else
#define UART_BAUD_RATE              (1000000u)
#endif
#define RX_BUF_SIZE                 (4u)

static cyhal_uart_t uart_obj;
static uint8_t      uart_rx_buffer[RX_BUF_SIZE];

cyhal_uart_t* get_uart()
{
    return &uart_obj;
}

/*******************************************************************************
* Function Name: streaming_init
********************************************************************************
* Summary:
*  If UART_STREAM is selected in the makefile then this function initializes
*  the UART as the streamer to collect data.
*
* Parameters:
*  stream: Pass in the stream object
*
*******************************************************************************/
void streaming_init(mtb_data_streaming_interface_t* stream)
{
    cy_rslt_t result;

    /* UART configuration structure */
    const cyhal_uart_cfg_t uart_config =
    {
        .data_bits       = 8,
        .stop_bits       = 1,
        .parity          = CYHAL_UART_PARITY_NONE,
        .rx_buffer       = uart_rx_buffer,
        .rx_buffer_size  = RX_BUF_SIZE,
    };

    /* Initialize the UART */
    result = cyhal_uart_init(&uart_obj, CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, NC, NC, NULL, &uart_config);
    HALT_ON_ERROR(result);
    result = cyhal_uart_set_baud(&uart_obj, UART_BAUD_RATE, NULL);
    HALT_ON_ERROR(result);
    result = mtb_data_streaming_setup_uart(&uart_obj, mtb_data_streaming_xfer_done, stream);
    HALT_ON_ERROR(result);
}

