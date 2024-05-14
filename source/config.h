/******************************************************************************
* File Name:   config.h
*
* Description: This file contains the configuration for running either the IMU
* based model, or the PDM based model.
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

#ifndef CONFIG_H
#define CONFIG_H

#ifdef CY_BMI_270_IMU_I2C
#include "bmi2_defs.h"
#else
#include "bmi160_defs.h"
#endif
/******************************************************************************
 * Constants
 *****************************************************************************/

/* Type of inference */
#define IMU_COLLECTION    1
#define PDM_COLLECTION    2
#define BMM_COLLECTION    3
#define DPS_COLLECTION    4
#define RADAR_COLLECTION  5
/* Change below define to IMU_COLLECTION or PDM_COLLECTION */
#define COLLECTION_MODE_SELECT IMU_COLLECTION

/* Set IMU_SAMPLE_RATE to one of the following
 * BMI160_ACCEL_ODR_400HZ / BMI2_ACC_ODR_400HZ
 * BMI160_ACCEL_ODR_200HZ / BMI2_ACC_ODR_200HZ
 * BMI160_ACCEL_ODR_100HZ / BMI2_ACC_ODR_100HZ
 * BMI160_ACCEL_ODR_50HZ / BMI2_ACC_ODR_50HZ */
#ifdef CY_BMI_270_IMU_I2C
#define IMU_SAMPLE_RATE BMI2_ACC_ODR_50HZ
#else
#define IMU_SAMPLE_RATE BMI160_ACCEL_ODR_50HZ
#endif

/*Set IMU_SAMPLE_RANGE to one of the following
 * BMI160_ACCEL_RANGE_2G / BMI2_ACC_RANGE_2G
 * BMI160_ACCEL_RANGE_4G / BMI2_ACC_RANGE_4G
 * BMI160_ACCEL_RANGE_8G / BMI2_ACC_RANGE_8G
 * BMI160_ACCEL_RANGE_16G / BMI2_ACC_RANGE_16G */
#ifdef CY_BMI_270_IMU_I2C
#define IMU_SAMPLE_RANGE BMI2_ACC_RANGE_8G
#else
#define IMU_SAMPLE_RANGE BMI160_ACCEL_RANGE_8G
#endif


/* PDM sample rates */
#define SAMPLE_RATE_8_KHZ    8000u
#define SAMPLE_RATE_16_KHZ   16000u

/* Change below to SAMPLE_RATE_8_KHZ or SAMPLE_RATE_16_KHZ */
#define PDM_SAMPLE_RATE SAMPLE_RATE_16_KHZ

#endif /* CONFIG_H */
