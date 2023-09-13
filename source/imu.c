/******************************************************************************
* File Name:   imu.c
*
* Description: This file implements the interface with the motion sensor, as
*              a timer to feed the pre-processor at 50Hz.
*
* Related Document: See README.md
*
*
*******************************************************************************/
#include "imu.h"

#include "mtb_bmi160.h"
#include "mtb_bmx160.h"
#include "cyhal.h"
#include "cybsp.h"

#include "config.h"


/*******************************************************************************
* Macros
*******************************************************************************/
#ifdef CY_BMX_160_IMU_SPI
    #define IMU_SPI_FREQUENCY 10000000
#endif

#ifdef CY_BMI_160_IMU_SPI
    #define IMU_SPI_FREQUENCY 10000000
#endif

#ifdef CY_BMI_160_IMU_I2C
    #define IMU_I2C_MASTER_DEFAULT_ADDRESS  0
    #define IMU_I2C_FREQUENCY               1000000
#endif

#define IMU_SCAN_RATE       50
#define IMU_TIMER_FREQUENCY 100000
#define IMU_TIMER_PERIOD (IMU_TIMER_FREQUENCY/IMU_SCAN_RATE)
#define IMU_TIMER_PRIORITY  3
/*******************************************************************************
* Global Variables
*******************************************************************************/
#ifdef CY_BMX_160_IMU_SPI
    /* BMX160 driver structures */
    mtb_bmx160_data_t data;
    mtb_bmx160_t sensor_bmx160;

    /* SPI object for data transmission */
    cyhal_spi_t spi;
#endif

#ifdef CY_BMI_160_IMU_SPI
    /* BMI160 driver structures */
    mtb_bmi160_data_t data;
    mtb_bmi160_t sensor_bmi160;

    /* SPI object for data transmission */
    cyhal_spi_t spi;
#endif

#ifdef CY_BMI_160_IMU_I2C
    /* BMI160 driver structures */
    mtb_bmi160_data_t data;
    mtb_bmi160_t sensor_bmi160;

    /* I2C object for data transmission */
    cyhal_i2c_t i2c;
#endif

/* Global timer used for getting data */
cyhal_timer_t imu_timer;

float imu_data[IMU_AXIS];
/*******************************************************************************
* Local Function Prototypes
*******************************************************************************/
void imu_interrupt_handler(void* callback_arg, cyhal_timer_event_t event);
cy_rslt_t imu_timer_init(void);

/*******************************************************************************
* Function Name: imu_init
********************************************************************************
* Summary:
*    A function used to initialize the IMU based on the shield selected in the
*    makefile. Starts a timer that triggers an interrupt at 50Hz.
*
* Parameters:
*   None
*
* Return:
*     The status of the initialization.
*
*
*******************************************************************************/
cy_rslt_t imu_init(void)
{
    cy_rslt_t result;

#ifdef CY_IMU_SPI
    /* Initialize SPI for IMU communication */
       result = cyhal_spi_init(&spi, CYBSP_SPI_MOSI, CYBSP_SPI_MISO, CYBSP_SPI_CLK, NC, NULL, 8, CYHAL_SPI_MODE_00_MSB, false);
       if(CY_RSLT_SUCCESS != result)
       {
           return result;
       }

       /* Set SPI frequency to 10MHz */
       result = cyhal_spi_set_frequency(&spi, IMU_SPI_FREQUENCY);
       if(CY_RSLT_SUCCESS != result)
       {
           return result;
       }

       /* Initialize the chip select line */
       result = cyhal_gpio_init(CYBSP_SPI_CS, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);
       if(CY_RSLT_SUCCESS != result)
       {
           return result;
       }
#endif

#ifdef CY_BMX_160_IMU_SPI
    /* Initialize the IMU */
    result = mtb_bmx160_init_spi(&sensor_bmx160, &spi, CYBSP_SPI_CS);
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    /* Set the output data rate and range of the accelerometer */
    sensor_bmx160.sensor1.accel_cfg.odr = IMU_SAMPLE_RATE;
    sensor_bmx160.sensor1.accel_cfg.range = IMU_SAMPLE_RANGE;

    /* Set the sensor configuration */
    bmi160_set_sens_conf(&(sensor_bmx160.sensor1));
#endif

#ifdef CY_BMI_160_IMU_SPI
    /* Initialize the IMU */
    result = mtb_bmi160_init_spi(&sensor_bmi160, &spi, CYBSP_SPI_CS);
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    /* Set the output data rate and range of the accelerometer */
    sensor_bmi160.sensor.accel_cfg.odr = IMU_SAMPLE_RATE;
    sensor_bmi160.sensor.accel_cfg.range = IMU_SAMPLE_RANGE;

    /* Set the sensor configuration */
    bmi160_set_sens_conf(&(sensor_bmi160.sensor));
#endif

#ifdef CY_BMI_160_IMU_I2C
    /* Configure the I2C mode, the address, and the data rate */
    cyhal_i2c_cfg_t i2c_config =
    {
            CYHAL_I2C_MODE_MASTER,
            IMU_I2C_MASTER_DEFAULT_ADDRESS,
            IMU_I2C_FREQUENCY
    };

    /* Initialize I2C for IMU communication */
    result = cyhal_i2c_init(&i2c, CYBSP_I2C_SDA, CYBSP_I2C_SCL, NULL);
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    /* Configure the I2C */
    result = cyhal_i2c_configure(&i2c, &i2c_config);
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    /* Initialize the IMU */
    result = mtb_bmi160_init_i2c(&sensor_bmi160, &i2c, MTB_BMI160_DEFAULT_ADDRESS);
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    /* Set the default configuration for the BMI160 */
    result = mtb_bmi160_config_default(&sensor_bmi160);
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    /* Set the output data rate and range of the accelerometer */
    sensor_bmi160.sensor.accel_cfg.odr = IMU_SAMPLE_RATE;
    sensor_bmi160.sensor.accel_cfg.range = IMU_SAMPLE_RANGE;

    /* Set the sensor configuration */
    bmi160_set_sens_conf(&(sensor_bmi160.sensor));
#endif

    imu_flag = false;

    /* Timer for data collection */
    result = imu_timer_init();
    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

    return CY_RSLT_SUCCESS;
}


/*******************************************************************************
* Function Name: imu_timer_init
********************************************************************************
* Summary:
*   Sets up an interrupt that triggers at the desired frequency.
*
* Returns:
*   The status of the initialization.
*
*
*******************************************************************************/
cy_rslt_t imu_timer_init(void)
{
    cy_rslt_t rslt;
    const cyhal_timer_cfg_t timer_cfg =
    {
        .compare_value = 0,                 /* Timer compare value, not used */
        .period = IMU_TIMER_PERIOD,      /* Defines the timer period */
        .direction = CYHAL_TIMER_DIR_UP,    /* Timer counts up */
        .is_compare = false,                /* Don't use compare mode */
        .is_continuous = true,              /* Run the timer indefinitely */
        .value = 0                          /* Initial value of counter */
    };

    /* Initialize the timer object. Does not use pin output ('pin' is NC) and
     * does not use a pre-configured clock source ('clk' is NULL). */
    rslt = cyhal_timer_init(&imu_timer, NC, NULL);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    /* Apply timer configuration such as period, count direction, run mode, etc. */
    rslt = cyhal_timer_configure(&imu_timer, &timer_cfg);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    /* Set the frequency of timer to 100KHz */
    rslt = cyhal_timer_set_frequency(&imu_timer, IMU_TIMER_FREQUENCY);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    /* Assign the ISR to execute on timer interrupt */
    cyhal_timer_register_callback(&imu_timer, imu_interrupt_handler, NULL);
    /* Set the event on which timer interrupt occurs and enable it */
    cyhal_timer_enable_event(&imu_timer, CYHAL_TIMER_IRQ_TERMINAL_COUNT, IMU_TIMER_PRIORITY, true);
    /* Start the timer with the configured settings */
    rslt = cyhal_timer_start(&imu_timer);
    if (CY_RSLT_SUCCESS != rslt)
    {
        return rslt;
    }

    return CY_RSLT_SUCCESS;
}

/*******************************************************************************
* Function Name: imu_interrupt_handler
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
void imu_interrupt_handler(void *callback_arg, cyhal_timer_event_t event)
{
    (void) callback_arg;
    (void) event;

    imu_flag = true;
}

/*******************************************************************************
* Function Name: imu_get_data
********************************************************************************
* Summary:
*   Reads accelerometer data from the IMU and stores it in a buffer.
*
* Parameters:
*     imu_data: Stores IMU accelerometer data
*
*
*******************************************************************************/
void imu_get_data(float *imu_data)
{
    /* Read data from IMU sensor */
    cy_rslt_t result;
#ifdef CY_BMX_160_IMU_SPI
    result = mtb_bmx160_read(&sensor_bmx160, &data);
#endif
#ifdef CY_BMI_160_IMU_SPI
    result = mtb_bmi160_read(&sensor_bmi160, &data);
#endif
#ifdef CY_BMI_160_IMU_I2C
    result = mtb_bmi160_read(&sensor_bmi160, &data);
#endif
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

#ifdef CY_IMU_SPI
    imu_data[0] = ((float)data.accel.y) / (float)0x1000;
    imu_data[1] = ((float)data.accel.x) / (float)0x1000;
    imu_data[2] = ((float)data.accel.z) / (float)0x1000;
#else
    imu_data[0] = ((float)data.accel.x) / (float)0x1000;
    imu_data[1] = ((float)data.accel.y) / (float)0x1000;
    imu_data[2] = ((float)data.accel.z) / (float)0x1000;
#endif
}
