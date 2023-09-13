/*******************************************************************************
* File Name: mtb_data_streaming.h
*
* Description:
* Provides common interface for streaming data to or from the device over a wide
* range of communication protocols. The chosen protocol is up to the user
* application.
*
********************************************************************************
* \copyright
* Copyright 2023 Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation
*
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include "cy_result.h"
#include "cyhal.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * \addtogroup group_data_streaming Data Streaming Library
 * \{
 * This library provides a convenient way to stream data asynchronously between the micro controller
 * and a host machine. It defines APIs that can be used, but does not make any assumptions about the
 * protocol that is chosen. This allows the interface to be used by a wide range of middleware
 * libraries while still leaving the communication interface up to the end application. Some
 * communication interfaces are already supported. Others are expected to be added in a future
 * release, or can be supported by the application itself.
 *
 * Available:
 *     BLE
 *     I2C
 *     SPI
 *     TCP
 *     UART
 *     USB: Communication Device Class (emusb-device)
 *
 * \section section_data_streaming_getting_started Getting Started
 * This section provides steps for getting started with this library by providing examples
 * using the TCP, UART and USB interfaces. Note that this section assumes that the required
 * libraries are already included in the application and that a compatible host is present. For more
 * information on how to include libraries in the application please refer to the
 * [ModusToolbox™ User Guide]
 * (https://www.cypress.com/products/modustoolbox-software-environment)
 * \note Error checking has been omitted from snippets below for readability.
 *
 * -# Include the data-streaming library header in the application.
 *    \snippet data_streaming_example.c snippet_mtb_data_streaming_include
 *
 * -# Select the streaming interface. The following snippets show how TCP, UART or USB can be
 * initialized for use as the streaming interface. Other interfaces are provided by the library,
 * or can be defined by the application as needed.
 *      - Example implementation using TCP.
 *        \snippet data_streaming_example.c snippet_mtb_data_streaming_usb
 *      - Example implementation using UART.
 *        \snippet data_streaming_example.c snippet_mtb_data_streaming_uart
 *      - Example implementation using USB.
 *        \snippet data_streaming_example.c snippet_mtb_data_streaming_usb
 *
 * -# The library can now be used to perform streaming operations.
 *      - Send operation.
 *        \snippet data_streaming_example.c snippet_mtb_data_streaming_send
 *      - Receive operation.
 *        \snippet data_streaming_example.c snippet_mtb_data_streaming_receive
 */


/** An operation is already in progress by this interface. It must be completed before another can
   start. */
#define MTB_DATA_STREAMING_IN_PROGRESS_ERR          \
    (CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_DATA_STREAMING, 0))
/** An error occurred while attempting to process the data transfer. */
#define MTB_DATA_STREAMING_XFER_ERR                 \
    (CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_DATA_STREAMING, 1))
/** An overflow error occurred while attempting to process the data transfer. */
#define MTB_DATA_STREAMING_OVERFLOW_ERR             \
    (CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_DATA_STREAMING, 2))
/** An underflow error occurred while attempting to process the data transfer. */
#define MTB_DATA_STREAMING_UNDERFLOW_ERR             \
    (CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_DATA_STREAMING, 3))


/** Function prototype for handling callback operations when data transfer operations are completed.
 *
 * @param[in]  tag  Context information provided as part of the original send/receive function call
 * @param[in]  rslt Result of the transfer operation. Can be success or an error.
 */
typedef void (* mtb_data_streaming_xfer_done_t)(const void* tag, cy_rslt_t rslt);

/** Virtual context structure for the data streaming interface. This is a placeholder that is
   implementation defined. */
typedef struct
{
    void* placeholder[3]; // Implementation relies on 96 bytes (3 32 bits) of data
} mtb_data_streaming_vcontext_t;

/** Function prototype for sending data to the host machine. Send operations are all asynchronous
 * and will return immediately. When the operation is complete, the callback that was provided as
 * part of the setup function will be called.
 * \note Only one operation is allowed on a streaming interface at a time.
 *
 * @param[in]  context  Context object that stored on the \ref mtb_data_streaming_interface_t by the
 *                      setup function
 * @param[in]  data     Data to transmit to the host.
 * @param[in]  count    The number of bytes to transfer.
 * @param[in]  tag      Arbitrary information to associate with this call. This will be provided as
 *                      part of the callback when the operation completes.
 * @return              Result of the send operation.
 */
typedef cy_rslt_t (* mtb_data_streaming_send_t)(mtb_data_streaming_vcontext_t* context,
                                                /*const*/ uint8_t* data, size_t count, void* tag);

/** Function prototype for receiving data from the block device. Receive operations are all
   asynchronous and will return immediately. When the operation is complete, the callback that was
 * provided as part of the setup function will be called.
 * \note Only one operation is allowed on a streaming interface at a time.
 *
 * @param[in]  context  Context object that stored on the \ref mtb_data_streaming_interface_t by the
 *                      setup function
 * @param[in]  data     Buffer to store the data received from the host into.
 * @param[in]  count    The number of bytes to accept.
 * @param[in]  tag      Arbitrary information to associate with this call. This will be provided as
 *                      part of the callback when the operation completes.
 * @return              Result of the receive operation.
 */
typedef cy_rslt_t (* mtb_data_streaming_receive_t)(mtb_data_streaming_vcontext_t* context,
                                                   uint8_t* data, size_t count, void* tag);

/** Data streaming interface */
typedef struct
{
    mtb_data_streaming_send_t send;         /**< Function to send data to a host device. */
    mtb_data_streaming_receive_t receive;   /**< Function to receive data from a host device. */
    mtb_data_streaming_vcontext_t context;  /**< Context data for performing operations. */
} mtb_data_streaming_interface_t;

/** Utility function for sending data to the host machine. Send operations are all asynchronous
 * and will return immediately. When the operation is complete, the callback that was provided as
 * part of the setup function will be called.
 * \note Only one operation (send or receive) is allowed on a streaming interface at a time.
 *
 * @param[in]  iface    The streaming interface that was initialized by calling one of the
 *                      setup function
 * @param[in]  data     Data to transmit to the host.
 * @param[in]  count    The number of bytes to transfer.
 * @param[in]  tag      Arbitrary information to associate with this call. This will be provided
 *                      as part of the callback when the operation completes.
 * @return              Result of the send operation.
 */
static inline cy_rslt_t mtb_data_streaming_send(mtb_data_streaming_interface_t* iface,
                                                /*const*/ uint8_t* data, size_t count, void* tag)
{
    return iface->send(&(iface->context), data, count, tag);
}


/** Function prototype for receiving data from the host machine. Receive operations are all
   asynchronous and will return immediately. When the operation is complete, the callback that was
 * provided as part of the setup function will be called.
 * \note Only one operation (send or receive) is allowed on a streaming interface at a time.
 *
 * @param[in]  iface    The streaming interface that was initialized by calling one of the
 *                       setup function
 * @param[in]  data     Buffer to store the data received from the host into.
 * @param[in]  count    The number of bytes to accept.
 * @param[in]  tag      Arbitrary information to associate with this call. This will be provided
 *                      as part of the callback when the operation completes.
 * @return              Result of the receive operation.
 */
static inline cy_rslt_t mtb_data_streaming_receive(mtb_data_streaming_interface_t* iface,
                                                   uint8_t* data, size_t count, void* tag)
{
    return iface->receive(&(iface->context), data, count, tag);
}


#if defined(COMPONENT_MW_BTSTACK)
/** Configuration for a BLE instance. User is expected to set all values before calling \ref
 * mtb_data_streaming_setup_ble.
 */
typedef struct
{
    uint16_t    conn_id;        /**< The connection ID generated for the currently connected host */
    uint16_t    attribute;      /**< The characteristic attribute */
    uint8_t*    client_config;  /**< The configuration of the characteristic */
    uint8_t*    buffer;         /**< The global buffer allocated for the attribute to send data */
    uint32_t    buffer_size;    /**< The number of bytes available in the \ref buffer */
} mtb_data_streaming_ble_t;

/** Sets up a streaming interface for BLE communication over the provided Bluetooth instance.
 * This expects that the BLE stack is already started. The BLE implementation is mostly just
 * a wrapper around a shared memory buffer. Receive requests simply return what was already placed
 * in the buffer. A send request will update the contents of the buffer and (if the device is
 * configured to support it) will send a notify/indicate signal.
 *
 * @param[in]  ble   Struct with the handle set to an already started BLE connection.
 * @param[in]  cb    Callback function to run when a transfer operation is complete.
 * @param[out] iface Streaming interface object to be populated by this setup function.
 * @return Result of the setup operation.
 */
cy_rslt_t mtb_data_streaming_setup_ble(mtb_data_streaming_ble_t* ble,
                                       mtb_data_streaming_xfer_done_t cb,
                                       mtb_data_streaming_interface_t* iface);
#endif // defined(COMPONENT_MW_BTSTACK)

#if defined(CYHAL_DRIVER_AVAILABLE_I2C)
#include "cyhal_i2c.h"

/** Sets up a streaming interface for I2C communication over the provided I2C instance.
 * This expects that the I2C interface is already initialized.
 *
 * @param[in]  i2c      Existing, pre-initialized, I2C interface to use for data transfers.
 * @param[in]  cb       Callback function to run when a transfer operation is complete.
 * @param[out] iface    Streaming interface object to be populated by this setup function.
 * @return              Result of the setup operation.
 */
cy_rslt_t mtb_data_streaming_setup_i2c(cyhal_i2c_t* i2c, mtb_data_streaming_xfer_done_t cb,
                                       mtb_data_streaming_interface_t* iface);
#endif // defined(CYHAL_DRIVER_AVAILABLE_I2C)

#if defined(CYHAL_DRIVER_AVAILABLE_SPI)
#include "cyhal_spi.h"

/** Sets up a streaming interface for SPI communication over the provided SPI instance.
 * This expects that the SPI interface is already initialized.
 *
 * @param[in]  spi      Existing, pre-initialized, SPI interface to use for data transfers.
 * @param[in]  cb       Callback function to run when a transfer operation is complete.
 * @param[out] iface    Streaming interface object to be populated by this setup function.
 * @return              Result of the setup operation.
 */
cy_rslt_t mtb_data_streaming_setup_spi(cyhal_spi_t* spi, mtb_data_streaming_xfer_done_t cb,
                                       mtb_data_streaming_interface_t* iface);
#endif // defined(CYHAL_DRIVER_AVAILABLE_SPI)

#if defined(COMPONENT_MW_SECURE_SOCKETS)
#include "cy_secure_sockets.h"

/** Sets up a streaming interface for TCP communication over the provided socket.
 * This expects that the TCP socket is already initialized.
 *
 * @param[in]  socket   Existing, pre-initialized, TCP socket handle.
 * @param[in]  cb       Callback function to run when a transfer operation is complete.
 * @param[out] iface    Streaming interface object to be populated by this setup function.
 * @return              Result of the setup operation.
 */
cy_rslt_t mtb_data_streaming_setup_tcp(cy_socket_t* socket, mtb_data_streaming_xfer_done_t cb,
                                       mtb_data_streaming_interface_t* iface);
#endif // defined(COMPONENT_MW_SECURE_SOCKETS)

#if defined(CYHAL_DRIVER_AVAILABLE_UART)
#include "cyhal_uart.h"

/** Sets up a streaming interface for UART communication over the provided UART instance.
 * This expects that the UART interface is already initialized.
 *
 * @param[in]  uart     Existing, pre-initialized, UART interface to use for data transfers.
 * @param[in]  cb       Callback function to run when a transfer operation is complete.
 * @param[out] iface    Streaming interface object to be populated by this setup function.
 * @return              Result of the setup operation.
 */
cy_rslt_t mtb_data_streaming_setup_uart(cyhal_uart_t* uart, mtb_data_streaming_xfer_done_t cb,
                                        mtb_data_streaming_interface_t* iface);
#endif // defined(CYHAL_DRIVER_AVAILABLE_UART)

#if defined(COMPONENT_MW_EMUSB_DEVICE)
#include "USB.h"
#include "USB_CDC.h"

/** Configuration for a USB instance. User is expected to set the \ref handle, but the
 * data-streaming library will manage setting the \ref async_ctx as appropriate.
 */
typedef struct
{
    USB_CDC_HANDLE          handle;     /**< Handle object from a pre-configured USB CDC instance */
    USB_ASYNC_IO_CONTEXT    async_ctx;  /**< Context used by the library to support callbacks */
} mtb_data_streaming_usb_t;

/** Sets up a streaming interface for USB CDC communication over the provided USB instance.
 * This expects that the UDB CDC stack is already started.
 *
 * @param[in]  usb      Struct with the handle set to an already started USB CDC instance.
 * @param[in]  cb       Callback function to run when a transfer operation is complete.
 * @param[out] iface    Streaming interface object to be populated by this setup function.
 * @return              Result of the setup operation.
 */
cy_rslt_t mtb_data_streaming_setup_usb(mtb_data_streaming_usb_t* usb,
                                       mtb_data_streaming_xfer_done_t cb,
                                       mtb_data_streaming_interface_t* iface);
#endif // defined(COMPONENT_MW_EMUSB_DEVICE)

#if defined(__cplusplus)
}
#endif

/** \} group_data_streaming */
