/******************************************************************************
* File Name:   radar.c
*
* Description: This file implements the interface with the radar sensor, as
*              a timer to feed the pre-processor at 50Hz.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company) or
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

#include "radar.h"
#include <inttypes.h>
#include <stdio.h>
#include "cyhal.h"
#include "cybsp.h"
#include <stdlib.h>
#include "cy_pdl.h"
#include "xensiv_bgt60trxx_mtb.h"
#include "radar_settings.h"

/*******************************************************************************
* Macros
*******************************************************************************/

#define XENSIV_BGT60TRXX_SPI_FREQUENCY      (25000000UL)

#define NUM_SAMPLES_PER_FRAME               (XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP *\
                                             XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME *\
                                             XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS)

#define NUM_CHIRPS_PER_FRAME                XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME
#define NUM_SAMPLES_PER_CHIRP               XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP
#define RADAR_SCAN_RATE       50
#define RADAR_TIMER_FREQUENCY 100000
#define RADAR_TIMER_PERIOD (RADAR_TIMER_FREQUENCY/RADAR_SCAN_RATE)
#define RADAR_TIMER_PRIORITY  3

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Global timer used for getting data */
cyhal_timer_t radar_timer;

#ifdef TARGET_APP_CY8CKIT_062S2_AI
static cyhal_spi_t spi_obj;
static xensiv_bgt60trxx_mtb_t bgt60_obj;
#endif
uint16_t bgt60_buffer[NUM_SAMPLES_PER_FRAME] __attribute__((aligned(2)));

/*******************************************************************************
* Local Function Prototypes
*******************************************************************************/
void radar_interrupt_handler(void* callback_arg, cyhal_timer_event_t event);
cy_rslt_t radar_timer_init(void);

/*******************************************************************************
* Function Name: radar_init
********************************************************************************
* Summary:
*    A function used to initialize the Radar sensor Present in 
*    Ai Evaluation Kit(CY8CKIT-062S2-AI).
*    Starts a timer that triggers an interrupt at 50Hz.
*
* Parameters:
*   None
*
* Return:
*     The status of the initialization.
*
*
*******************************************************************************/
cy_rslt_t radar_init(void)
{
#ifdef TARGET_APP_CY8CKIT_062S2_AI
    cy_rslt_t result;
    if (cyhal_spi_init(&spi_obj,
                           PIN_XENSIV_BGT60TRXX_SPI_MOSI,
                           PIN_XENSIV_BGT60TRXX_SPI_MISO,
                           PIN_XENSIV_BGT60TRXX_SPI_SCLK,
                           NC,
                           NULL,
                           8,
                           CYHAL_SPI_MODE_00_MSB,
                           false) != CY_RSLT_SUCCESS)
        {
            printf("ERROR: cyhal_spi_init failed\n");
            return -1;
        }
        /* Reduce drive strength to improve EMI */
        Cy_GPIO_SetSlewRate(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CY_GPIO_SLEW_FAST);
        Cy_GPIO_SetDriveSel(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CY_GPIO_DRIVE_1_8);
        Cy_GPIO_SetSlewRate(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CY_GPIO_SLEW_FAST);
        Cy_GPIO_SetDriveSel(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CY_GPIO_DRIVE_1_8);

        /* Set the data rate to 25 Mbps */
        if (cyhal_spi_set_frequency(&spi_obj, XENSIV_BGT60TRXX_SPI_FREQUENCY) != CY_RSLT_SUCCESS)
        {
            printf("ERROR: cyhal_spi_set_frequency failed\n");
            return -1;
        }

        if (xensiv_bgt60trxx_mtb_init(&bgt60_obj,
                                      &spi_obj,
                                      PIN_XENSIV_BGT60TRXX_SPI_CSN,
                                      PIN_XENSIV_BGT60TRXX_RSTN,
                                      register_lst,
                                      XENSIV_BGT60TRXX_CONF_NUM_REGS) != CY_RSLT_SUCCESS)
        {
            printf("ERROR: xensiv_bgt60trxx_mtb_init failed\n");
            return -1;
        }
        radar_flag = false;
        result = radar_timer_init();
        if(CY_RSLT_SUCCESS != result)
        {
            return result;
        }

        return CY_RSLT_SUCCESS;
#else
        return 0;
#endif
}


/*******************************************************************************
* Function Name: radar_timer_init
********************************************************************************
* Summary:
*   Sets up an interrupt that triggers at the desired frequency.
*
* Returns:
*   The status of the initialization.
*
*
*******************************************************************************/
cy_rslt_t radar_timer_init(void)
{
    cy_rslt_t rslt;
    const cyhal_timer_cfg_t timer_cfg =
    {
        .compare_value = 0,                 /* Timer compare value, not used */
        .period = RADAR_TIMER_PERIOD,      /* Defines the timer period */
        .direction = CYHAL_TIMER_DIR_UP,    /* Timer counts up */
        .is_compare = false,                /* Don't use compare mode */
        .is_continuous = true,              /* Run the timer indefinitely */
        .value = 0                          /* Initial value of counter */
    };

    /* Initialize the timer object. Does not use pin output ('pin' is NC) and
     * does not use a pre-configured clock source ('clk' is NULL). */
    rslt = cyhal_timer_init(&radar_timer, NC, NULL);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    /* Apply timer configuration such as period, count direction, run mode, etc. */
    rslt = cyhal_timer_configure(&radar_timer, &timer_cfg);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    /* Set the frequency of timer to 100KHz */
    rslt = cyhal_timer_set_frequency(&radar_timer, RADAR_TIMER_FREQUENCY);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    /* Assign the ISR to execute on timer interrupt */
    cyhal_timer_register_callback(&radar_timer, radar_interrupt_handler, NULL);
    /* Set the event on which timer interrupt occurs and enable it */
    cyhal_timer_enable_event(&radar_timer, CYHAL_TIMER_IRQ_TERMINAL_COUNT, RADAR_TIMER_PRIORITY, true);
    /* Start the timer with the configured settings */
    rslt = cyhal_timer_start(&radar_timer);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    return CY_RSLT_SUCCESS;
}

/*******************************************************************************
* Function Name: radar_interrupt_handler
********************************************************************************
* Summary:
*   Interrupt handler for timer. Interrupt handler will get called at 50Hz and
*   sets a flag that can be checked in main.
*
* Parameters:
*     callback_arg: not used
*     event: not used
*
*
*******************************************************************************/
void radar_interrupt_handler(void *callback_arg, cyhal_timer_event_t event)
{
    (void) callback_arg;
    (void) event;

    radar_flag = true;
}

/*******************************************************************************
* Function Name: radar_feed
********************************************************************************
* Summary:
*   Reads data from the radar sensor and stores it in a buffer.
*
* Parameters:
*     radar_data: Stores RADAR sensor data
*
*
*******************************************************************************/
void radar_get_data(int16_t *radar_data)
{
#ifdef TARGET_APP_CY8CKIT_062S2_AI
    cy_rslt_t result;
    result = xensiv_bgt60trxx_get_fifo_data(&bgt60_obj.dev,bgt60_buffer,NUM_SAMPLES_PER_FRAME);
    if (CY_RSLT_SUCCESS == result)
    {
        for(uint32_t i =0;i< 128; i++)
        {
            radar_data[i] = bgt60_buffer[i];
        }
    }

#endif
}
