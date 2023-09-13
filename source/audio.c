/******************************************************************************
* File Name:   audio.c
*
* Description: This file implements the interface with the PDM, as
*              well as the PDM ISR to feed the pre-processor.
*
* Related Document: See README.md
*
*
*******************************************************************************/
#include "cyhal.h"
#include "cybsp.h"

#include "audio.h"
#include "config.h"

/******************************************************************************
 * Macros
 *****************************************************************************/
/* Desired sample rate. Typical values: 8/16/22.05/32/44.1/48kHz */
#define SAMPLE_RATE_HZ              PDM_SAMPLE_RATE
/* Decimation Rate of the PDM/PCM block. Typical value is 64 */
#define DECIMATION_RATE             64u
/* Audio Subsystem Clock. Typical values depends on the desire sample rate:
- 8/16/48kHz    : 24.576 MHz
- 22.05/44.1kHz : 22.579 MHz */
#define AUDIO_SYS_CLOCK_HZ          24576000u
/* PDM/PCM Pins */
#define PDM_DATA                    P10_5
#define PDM_CLK                     P10_4

/* Set up one buffer for data collection and one for processing */
int16_t audio_buffer0[FRAME_SIZE] = {0};
int16_t audio_buffer1[FRAME_SIZE] = {0};
int16_t* active_rx_buffer;
int16_t* full_rx_buffer;

/******************************************************************************
 * Global Variables
 *****************************************************************************/
/* HAL Object */
cyhal_pdm_pcm_t pdm_pcm;
cyhal_clock_t   audio_clock;
cyhal_clock_t   pll_clock;

/* HAL PDM Configuration */
const cyhal_pdm_pcm_cfg_t pdm_pcm_cfg =
{
    .sample_rate     = SAMPLE_RATE_HZ,
    .decimation_rate = DECIMATION_RATE,
    .mode            = CYHAL_PDM_PCM_MODE_LEFT,
    .word_length     = 16,  /* bits */
    .left_gain       = 3,   /* dB */
    .right_gain      = 0,   /* dB */
};


/*******************************************************************************
* Local Function Prototypes
*******************************************************************************/
cy_rslt_t pdm_clock_init(void);
void pdm_pcm_event_handler(void *arg, cyhal_pdm_pcm_event_t event);
/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: pdm_init
********************************************************************************
* Summary:
*    A function used to initialize and configure the PDM based on the shield
*    selected in the makefile. Starts an asynchronous read which triggers an
*    interrupt when completed.
*
* Parameters:
*   None
*
* Return:
*     The status of the initialization.
*
*
*******************************************************************************/
cy_rslt_t pdm_init(void)
{
    cy_rslt_t result;

    /* Initialize the PDM clock */
    result = pdm_clock_init();
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    /* Initialize the PDM/PCM block */
    result = cyhal_pdm_pcm_init(&pdm_pcm, PDM_DATA, PDM_CLK, &audio_clock, &pdm_pcm_cfg);
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    /* Register the PDM callback and set the interrupt event */
    cyhal_pdm_pcm_register_callback(&pdm_pcm, pdm_pcm_event_handler, NULL);
    cyhal_pdm_pcm_enable_event(&pdm_pcm, CYHAL_PDM_PCM_ASYNC_COMPLETE, CYHAL_ISR_PRIORITY_DEFAULT, true);

    result = cyhal_pdm_pcm_start(&pdm_pcm);
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    /* Set up pointers to two buffers to implement a ping-pong buffer system.
     * One gets filled by the PDM while the other can be processed. */
    active_rx_buffer = audio_buffer0;
    full_rx_buffer = audio_buffer1;

    pdm_pcm_flag = false;

    /* Start an asynchronous read */
    cyhal_pdm_pcm_read_async(&pdm_pcm, active_rx_buffer, FRAME_SIZE);

    return CY_RSLT_SUCCESS;
}

/*******************************************************************************
* Function Name: pdm_clock_init
********************************************************************************
* Summary:
*    A function used to initialize and configure PDM clocks.
*
* Parameters:
*   None
*
* Return:
*     The status of the initialization.
*
*
*******************************************************************************/
cy_rslt_t pdm_clock_init(void)
{
    cy_rslt_t result;

    /* Initialize the PLL */
    result = cyhal_clock_reserve(&pll_clock, &CYHAL_CLOCK_PLL[1]);
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    result = cyhal_clock_set_frequency(&pll_clock, AUDIO_SYS_CLOCK_HZ, NULL);
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    result = cyhal_clock_set_enabled(&pll_clock, true, true);
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    /* Initialize the audio subsystem clock (CLK_HF[1])
     * The CLK_HF[1] is the root clock for the I2S and PDM/PCM blocks */
    result = cyhal_clock_reserve(&audio_clock, &CYHAL_CLOCK_HF[1]);
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    /* Source the audio subsystem clock from PLL */
    result = cyhal_clock_set_source(&audio_clock, &pll_clock);
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    result = cyhal_clock_set_enabled(&audio_clock, true, true);
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    return CY_RSLT_SUCCESS;
}

/*******************************************************************************
* Function Name: pdm_pcm_event_handler
********************************************************************************
* Summary:
*  PDM/PCM ISR handler. Swaps the two buffers and restarts the PDM async read.
*  Set a flag to be processed in the main loop.
*
* Parameters:
*  arg: not used
*  event: event that occurred
*
*******************************************************************************/
void pdm_pcm_event_handler(void *arg, cyhal_pdm_pcm_event_t event)
{
    (void) arg;
    (void) event;

    if(false == pdm_pcm_flag)
    {

        pdm_pcm_flag = true;

        /* Flip the active and the next rx buffers */
        int16_t* temp = active_rx_buffer;
        active_rx_buffer = full_rx_buffer;
        full_rx_buffer = temp;

    }
    /* Initiate the next pdm read */
    cyhal_pdm_pcm_read_async(&pdm_pcm, active_rx_buffer, FRAME_SIZE);
}

/*******************************************************************************
* Function Name: pdm_preprocessing_feed
********************************************************************************
* Summary:
*  This function returns the pdm data.
*
* Parameters:
*  preprocessed_data: Stores float PDM values
*
*******************************************************************************/
void pdm_preprocessing_feed(int16_t *preprocessed_data)
{

    for (uint32_t index = 0; index < FRAME_SIZE ; index ++)
    {
        preprocessed_data[index] = full_rx_buffer[index];
    }
}

/* [] END OF FILE */
