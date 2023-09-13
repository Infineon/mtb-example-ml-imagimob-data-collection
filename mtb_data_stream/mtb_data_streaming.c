/*******************************************************************************
* File Name: mtb_data_streaming.c
*
* Description:
* Implementation of common streaming support library.
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

#include "mtb_data_streaming.h"

typedef union
{
    #if defined(COMPONENT_MW_BTSTACK)
    mtb_data_streaming_ble_t*           ble;
    #endif
    #if defined(CYHAL_DRIVER_AVAILABLE_I2C)
    cyhal_i2c_t*                        i2c;
    #endif
    #if defined(CYHAL_DRIVER_AVAILABLE_SPI)
    cyhal_spi_t*                        spi;
    #endif
    #if defined(COMPONENT_MW_SECURE_SOCKETS)
    cy_socket_t                         tcp;
    #endif
    #if defined(CYHAL_DRIVER_AVAILABLE_UART)
    cyhal_uart_t*                       uart;
    #endif
    #if defined(COMPONENT_MW_EMUSB_DEVICE)
    mtb_data_streaming_usb_t*           usb;
    #endif
} mtb_data_streaming_obj_t;

typedef struct
{
    mtb_data_streaming_obj_t        obj_inst;
    mtb_data_streaming_xfer_done_t  callback;
    void*                           call_tag;
} mtb_data_streaming_context_t;

/*
 * This function is designed to verify that the size of mtb_data_streaming_context_t matches the
 * size of mtb_data_streaming_vcontext_t that is exposed to the user. Any mismatch between the two
 * will lead to runtime failures. This will produce a divide by 0 error if they get of of sync.
 * NOTE: This function should never be called, it is only for a compile time error check
 * NOTE: The suppress is to temporarily disable warnings about an uncalled functions.
 */
static inline void _check_size(void) __attribute__ ((deprecated));
#if __ICCARM__
#pragma diag_suppress=Pe177
#elif __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
//--------------------------------------------------------------------------------------------------
// _check_size
//--------------------------------------------------------------------------------------------------
static inline void _check_size(void)
{
    uint8_t dummy = 1 /
                    (sizeof(mtb_data_streaming_vcontext_t) == sizeof(mtb_data_streaming_context_t));
    (void)dummy;
}


#if __ICCARM__
#pragma diag_default=Pe177
#elif __clang__
#pragma clang diagnostic pop
#endif



////////////////////////////////////////////////////////////////////////////////////////////////////
// BLE SUPPORT
////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(COMPONENT_MW_BTSTACK)
//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_ble_cb
//--------------------------------------------------------------------------------------------------
static void mtb_data_streaming_ble_cb(void* callback_arg)
{
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)async_ctx->pContext;
    mtb_data_streaming_xfer_done_t callback = context->callback;
    void* tag = context->call_tag;
    context->call_tag = NULL;

    if (NULL != callback)
    {
        cy_rslt_t rslt = (0 == async_ctx->Status)
            ? CY_RSLT_SUCCESS
            : MTB_DATA_STREAMING_XFER_ERR;
        callback(tag, rslt);
    }
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_ble_send
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_data_streaming_ble_send(mtb_data_streaming_vcontext_t* vcontext,
                                             /*const*/ uint8_t* data, size_t count, void* tag)
{
    cy_rslt_t rslt;
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)vcontext;

    uint32_t state = cyhal_system_critical_section_enter();
    if (NULL == context->call_tag)
    {
        context->call_tag = tag;
        cyhal_system_critical_section_exit(state);

        mtb_data_streaming_ble_t* ble = context->obj_inst.ble;

        if (count <= ble->send_buffer_size)
        {
            memcpy(ble->send_buffer, data, count);

            wiced_bt_gatt_status_t status = WICED_BT_GATT_WRITE_REQ_REJECTED;
            if (0 == ble->client_config[0] & GATT_CLIENT_CONFIG_NOTIFICATION)
            {
                status = wiced_bt_gatt_server_send_notification(
                    ble->conn_id, ble->attribute, count, ble->send_buffer, NULL);
            }
            else if (0 == ble->client_config[0] & GATT_CLIENT_CONFIG_INDICATION)
            {
                status = wiced_bt_gatt_server_send_indication(
                    ble->conn_id, ble->attribute, count, ble->send_buffer, NULL);
            }

            rslt = (WICED_BT_GATT_SUCCESS == status)
                ? CY_RSLT_SUCCESS
                : MTB_DATA_STREAMING_XFER_ERR;
        }
        else
        {
            rslt = MTB_DATA_STREAMING_OVERFLOW_ERR;
        }
    }
    else
    {
        cyhal_system_critical_section_exit(state);
        rslt = MTB_DATA_STREAMING_IN_PROGRESS_ERR;
    }

    return rslt;
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_ble_receive
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_data_streaming_ble_receive(mtb_data_streaming_vcontext_t* vcontext,
                                                uint8_t* data, size_t count, void* tag)
{
    cy_rslt_t rslt;
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)vcontext;

    uint32_t state = cyhal_system_critical_section_enter();
    if (NULL == context->call_tag)
    {
        context->call_tag = tag;
        cyhal_system_critical_section_exit(state);

        if (count <= ble->receive_buffer_size)
        {
            memcpy(data, ble->receive_buffer, count);
            rslt =  CY_RSLT_SUCCESS;
        }
        else
        {
            rslt = MTB_DATA_STREAMING_UNDERFLOW_ERR;
        }
    }
    else
    {
        cyhal_system_critical_section_exit(state);
        rslt = MTB_DATA_STREAMING_IN_PROGRESS_ERR;
    }

    return rslt;
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_setup_ble
//--------------------------------------------------------------------------------------------------
cy_rslt_t mtb_data_streaming_setup_ble(mtb_data_streaming_ble_t* ble,
                                       mtb_data_streaming_xfer_done_t cb,
                                       mtb_data_streaming_interface_t* iface)
{
    iface->send     = mtb_data_streaming_ble_send;
    iface->receive  = mtb_data_streaming_ble_receive;
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)&(iface->context);
    context->obj_inst.ble  = ble;
    context->callback  = cb;
    context->call_tag  = NULL;

    return CY_RSLT_SUCCESS;
}


#endif // if defined(COMPONENT_MW_BTSTACK)


////////////////////////////////////////////////////////////////////////////////////////////////////
// I2C SUPPORT
////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(CYHAL_DRIVER_AVAILABLE_I2C)
//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_i2c_cb
//--------------------------------------------------------------------------------------------------
static void mtb_data_streaming_i2c_cb(void* callback_arg, cyhal_i2c_event_t event)
{
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)callback_arg;
    mtb_data_streaming_xfer_done_t callback = context->callback;
    void* tag = context->call_tag;
    context->call_tag = NULL;

    if (NULL != callback)
    {
        switch (event)
        {
            case CYHAL_I2C_MASTER_RD_CMPLT_EVENT:
            case CYHAL_I2C_MASTER_WR_CMPLT_EVENT:
                callback(tag, CY_RSLT_SUCCESS);
                break;

            case CYHAL_I2C_MASTER_ERR_EVENT:
                callback(tag, MTB_DATA_STREAMING_XFER_ERR);
                break;

            default:
                CY_ASSERT(false); // Unexpected event
                break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_i2c_transfer
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_data_streaming_i2c_transfer(mtb_data_streaming_context_t* context,
                                                 uint8_t* rx_data, size_t rx_count,
                                                 const uint8_t* tx_data, size_t tx_count,
                                                 void* tag)
{
    cy_rslt_t rslt;
    uint32_t state = cyhal_system_critical_section_enter();
    if (NULL == context->call_tag)
    {
        context->call_tag = tag;
        cyhal_system_critical_section_exit(state);
        rslt = cyhal_i2c_master_transfer_async(context->obj_inst.i2c, 0 /*FIXME: uint16_t address*/,
                                               tx_data, tx_count, rx_data, rx_count);
        if (CY_RSLT_SUCCESS != rslt)
        {
            context->call_tag = NULL;
        }
    }
    else
    {
        cyhal_system_critical_section_exit(state);
        rslt = MTB_DATA_STREAMING_IN_PROGRESS_ERR;
    }
    return rslt;
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_i2c_send
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_data_streaming_i2c_send(mtb_data_streaming_vcontext_t* context,
                                             /*const*/ uint8_t* data, size_t count, void* tag)
{
    return mtb_data_streaming_i2c_transfer((mtb_data_streaming_context_t*)context, NULL, 0, data,
                                           count, tag);
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_i2c_receive
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_data_streaming_i2c_receive(mtb_data_streaming_vcontext_t* context,
                                                uint8_t* data, size_t count, void* tag)
{
    return mtb_data_streaming_i2c_transfer((mtb_data_streaming_context_t*)context, data, count,
                                           NULL, 0, tag);
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_setup_i2c
//--------------------------------------------------------------------------------------------------
cy_rslt_t mtb_data_streaming_setup_i2c(cyhal_i2c_t* i2c, mtb_data_streaming_xfer_done_t cb,
                                       mtb_data_streaming_interface_t* iface)
{
    iface->send     = mtb_data_streaming_i2c_send;
    iface->receive  = mtb_data_streaming_i2c_receive;
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)&(iface->context);
    context->obj_inst.i2c  = i2c;
    context->callback  = cb;
    context->call_tag  = NULL;

    cyhal_i2c_register_callback(i2c, mtb_data_streaming_i2c_cb, context);
    cyhal_i2c_event_t events = (cyhal_i2c_event_t)(
        CYHAL_I2C_MASTER_RD_CMPLT_EVENT |
        CYHAL_I2C_MASTER_WR_CMPLT_EVENT |
        CYHAL_I2C_MASTER_ERR_EVENT);
    cyhal_i2c_enable_event(i2c, events, CYHAL_ISR_PRIORITY_DEFAULT, true);

    return CY_RSLT_SUCCESS;
}


#endif // if defined(CYHAL_DRIVER_AVAILABLE_I2C)


////////////////////////////////////////////////////////////////////////////////////////////////////
// SPI SUPPORT
////////////////////////////////////////////////////////////////////////////////////////////////////
#if CYHAL_DRIVER_AVAILABLE_SPI
//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_spi_cb
//--------------------------------------------------------------------------------------------------
static void mtb_data_streaming_spi_cb(void* callback_arg, cyhal_spi_event_t event)
{
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)callback_arg;
    mtb_data_streaming_xfer_done_t callback = context->callback;
    void* tag = context->call_tag;
    context->call_tag = NULL;

    if (NULL != callback)
    {
        switch (event)
        {
            case CYHAL_SPI_IRQ_DONE:
                callback(tag, CY_RSLT_SUCCESS);
                break;

            case CYHAL_SPI_IRQ_ERROR:
                callback(tag, MTB_DATA_STREAMING_XFER_ERR);
                break;

            default:
                CY_ASSERT(false); // Unexpected event
                break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_i2c_transfer
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_data_streaming_spi_transfer(mtb_data_streaming_context_t* context,
                                                 uint8_t* rx_data, size_t rx_count,
                                                 const uint8_t* tx_data, size_t tx_count,
                                                 void* tag)
{
    cy_rslt_t rslt;
    uint32_t state = cyhal_system_critical_section_enter();
    if (NULL == context->call_tag)
    {
        context->call_tag = tag;
        cyhal_system_critical_section_exit(state);
        rslt =
            cyhal_spi_transfer_async(context->obj_inst.spi, tx_data, tx_count, rx_data, rx_count);
        if (CY_RSLT_SUCCESS != rslt)
        {
            context->call_tag = NULL;
        }
    }
    else
    {
        cyhal_system_critical_section_exit(state);
        rslt = MTB_DATA_STREAMING_IN_PROGRESS_ERR;
    }
    return rslt;
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_spi_send
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_data_streaming_spi_send(mtb_data_streaming_vcontext_t* context,
                                             /*const*/ uint8_t* data, size_t count, void* tag)
{
    return mtb_data_streaming_spi_transfer((mtb_data_streaming_context_t*)context, NULL, 0, data,
                                           count, tag);
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_spi_receive
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_data_streaming_spi_receive(mtb_data_streaming_vcontext_t* context,
                                                uint8_t* data, size_t count, void* tag)
{
    return mtb_data_streaming_spi_transfer((mtb_data_streaming_context_t*)context, data, count,
                                           NULL, 0, tag);
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_setup_spi
//--------------------------------------------------------------------------------------------------
cy_rslt_t mtb_data_streaming_setup_spi(cyhal_spi_t* spi, mtb_data_streaming_xfer_done_t cb,
                                       mtb_data_streaming_interface_t* iface)
{
    iface->send     = mtb_data_streaming_spi_send;
    iface->receive  = mtb_data_streaming_spi_receive;
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)&(iface->context);
    context->obj_inst.spi  = spi;
    context->callback  = cb;
    context->call_tag  = NULL;

    cyhal_spi_register_callback(spi, mtb_data_streaming_spi_cb, context);
    cyhal_spi_event_t events = (cyhal_spi_event_t)(CYHAL_SPI_IRQ_DONE | CYHAL_SPI_IRQ_ERROR);
    cyhal_spi_enable_event(spi, events, CYHAL_ISR_PRIORITY_DEFAULT, true);

    return CY_RSLT_SUCCESS;
}


#endif // if defined(CYHAL_DRIVER_AVAILABLE_SPI)


////////////////////////////////////////////////////////////////////////////////////////////////////
// TCP SUPPORT
////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(COMPONENT_MW_SECURE_SOCKETS)
//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_tcp_send
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_data_streaming_tcp_send(mtb_data_streaming_vcontext_t* vcontext,
                                             /*const*/ uint8_t* data, size_t count, void* tag)
{
    cy_rslt_t rslt;
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)vcontext;

    uint32_t state = cyhal_system_critical_section_enter();
    if (NULL == context->call_tag)
    {
        uint32_t sent;
        rslt = cy_socket_send(context->obj_inst.tcp, data, count, CY_SOCKET_FLAGS_NONE, &sent);
        if (CY_RSLT_SUCCESS != rslt)
        {
            if (result == CY_RSLT_MODULE_SECURE_SOCKETS_CLOSED)
            {
                cy_socket_disconnect(context->obj_inst.tcp, 0);
                cy_socket_delete(context->obj_inst.tcp);
            }
        }
        else if (sent != count)
        {
            rslt = MTB_DATA_STREAMING_XFER_ERR;
        }
        else
        {
            mtb_data_streaming_xfer_done_t callback = context->callback;
            if (NULL != callback)
            {
                callback(tag, CY_RSLT_SUCCESS);
            }
        }
    }
    else
    {
        rslt = MTB_DATA_STREAMING_IN_PROGRESS_ERR;
    }
    cyhal_system_critical_section_exit(state);

    return rslt;
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_tcp_receive
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_data_streaming_tcp_receive(mtb_data_streaming_vcontext_t* vcontext,
                                                uint8_t* data, size_t count, void* tag)
{
    cy_rslt_t rslt;
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)vcontext;

    uint32_t state = cyhal_system_critical_section_enter();
    if (NULL == context->call_tag)
    {
        uint32_t received;
        rslt = cy_socket_recv(context->obj_inst.tcp, data, count, CY_SOCKET_FLAGS_NONE, &received);
        if (CY_RSLT_SUCCESS != rslt)
        {
            if (result == CY_RSLT_MODULE_SECURE_SOCKETS_CLOSED)
            {
                cy_socket_disconnect(context->obj_inst.tcp, 0);
                cy_socket_delete(context->obj_inst.tcp);
            }
        }
        else if (received != count)
        {
            rslt = MTB_DATA_STREAMING_XFER_ERR;
        }
        else
        {
            mtb_data_streaming_xfer_done_t callback = context->callback;
            if (NULL != callback)
            {
                callback(tag, CY_RSLT_SUCCESS);
            }
        }
    }
    else
    {
        rslt = MTB_DATA_STREAMING_IN_PROGRESS_ERR;
    }
    cyhal_system_critical_section_exit(state);

    return rslt;
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_setup_tcp
//--------------------------------------------------------------------------------------------------
cy_rslt_t mtb_data_streaming_setup_tcp(cy_socket_t tcp, mtb_data_streaming_xfer_done_t cb,
                                       mtb_data_streaming_interface_t* iface)
{
    iface->send     = mtb_data_streaming_tcp_send;
    iface->receive  = mtb_data_streaming_tcp_receive;
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)&(iface->context);
    context->obj_inst.tcp  = tcp;
    context->callback  = cb;
    context->call_tag  = NULL;

    return CY_RSLT_SUCCESS;
}


#endif // if defined(COMPONENT_MW_SECURE_SOCKETS)


////////////////////////////////////////////////////////////////////////////////////////////////////
// UART SUPPORT
////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(CYHAL_DRIVER_AVAILABLE_UART)
//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_uart_cb
//--------------------------------------------------------------------------------------------------
static void mtb_data_streaming_uart_cb(void* callback_arg, cyhal_uart_event_t event)
{
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)callback_arg;
    mtb_data_streaming_xfer_done_t callback = context->callback;
    void* tag = context->call_tag;
    context->call_tag = NULL;

    if (NULL != callback)
    {
        switch (event)
        {
            case CYHAL_UART_IRQ_TX_DONE:
            case CYHAL_UART_IRQ_RX_DONE:
                callback(tag, CY_RSLT_SUCCESS);
                break;

            case CYHAL_UART_IRQ_TX_ERROR:
            case CYHAL_UART_IRQ_RX_ERROR:
                callback(tag, MTB_DATA_STREAMING_XFER_ERR);
                break;

            default:
                CY_ASSERT(false); // Unexpected event
                break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_uart_send
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_data_streaming_uart_send(mtb_data_streaming_vcontext_t* vcontext,
                                              /*const*/ uint8_t* data, size_t count, void* tag)
{
    cy_rslt_t rslt;
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)vcontext;

    uint32_t state = cyhal_system_critical_section_enter();
    if (NULL == context->call_tag)
    {
        context->call_tag = tag;
        cyhal_system_critical_section_exit(state);
        rslt = cyhal_uart_write_async(context->obj_inst.uart, data, count);
        if (CY_RSLT_SUCCESS != rslt)
        {
            context->call_tag = NULL;
        }
    }
    else
    {
        cyhal_system_critical_section_exit(state);
        rslt = MTB_DATA_STREAMING_IN_PROGRESS_ERR;
    }
    return rslt;
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_uart_receive
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_data_streaming_uart_receive(mtb_data_streaming_vcontext_t* vcontext,
                                                 uint8_t* data, size_t count, void* tag)
{
    cy_rslt_t rslt;
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)vcontext;

    uint32_t state = cyhal_system_critical_section_enter();
    if (NULL == context->call_tag)
    {
        context->call_tag = tag;
        cyhal_system_critical_section_exit(state);
        rslt = cyhal_uart_read_async(context->obj_inst.uart, data, count);
        if (CY_RSLT_SUCCESS != rslt)
        {
            context->call_tag = NULL;
        }
    }
    else
    {
        cyhal_system_critical_section_exit(state);
        rslt = MTB_DATA_STREAMING_IN_PROGRESS_ERR;
    }
    return rslt;
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_setup_uart
//--------------------------------------------------------------------------------------------------
cy_rslt_t mtb_data_streaming_setup_uart(cyhal_uart_t* uart, mtb_data_streaming_xfer_done_t cb,
                                        mtb_data_streaming_interface_t* iface)
{
    iface->send     = mtb_data_streaming_uart_send;
    iface->receive  = mtb_data_streaming_uart_receive;
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)&(iface->context);
    context->obj_inst.uart  = uart;
    context->callback  = cb;
    context->call_tag  = NULL;

    cyhal_uart_register_callback(uart, mtb_data_streaming_uart_cb, context);
    cyhal_uart_event_t events = (cyhal_uart_event_t)(
        CYHAL_UART_IRQ_TX_DONE | CYHAL_UART_IRQ_TX_ERROR |
        CYHAL_UART_IRQ_RX_DONE | CYHAL_UART_IRQ_RX_ERROR);
    cyhal_uart_enable_event(uart, events, CYHAL_ISR_PRIORITY_DEFAULT, true);

    return CY_RSLT_SUCCESS;
}


#endif // if defined(CYHAL_DRIVER_AVAILABLE_UART)


////////////////////////////////////////////////////////////////////////////////////////////////////
// USB SUPPORT
////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(COMPONENT_MW_EMUSB_DEVICE)
//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_usb_cb
//--------------------------------------------------------------------------------------------------
static void mtb_data_streaming_usb_cb(USB_ASYNC_IO_CONTEXT* async_ctx)
{
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)async_ctx->pContext;
    mtb_data_streaming_xfer_done_t callback = context->callback;
    void* tag = context->call_tag;
    context->call_tag = NULL;

    if (NULL != callback)
    {
        cy_rslt_t rslt = (0 == async_ctx->Status)
            ? CY_RSLT_SUCCESS
            : MTB_DATA_STREAMING_XFER_ERR;
        callback(tag, rslt);
    }
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_usb_send
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_data_streaming_usb_send(mtb_data_streaming_vcontext_t* vcontext,
                                             /*const*/ uint8_t* data, size_t count, void* tag)
{
    cy_rslt_t rslt;
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)vcontext;

    uint32_t state = cyhal_system_critical_section_enter();
    if (NULL == context->call_tag)
    {
        context->call_tag = tag;
        cyhal_system_critical_section_exit(state);

        USB_ASYNC_IO_CONTEXT* async_ctx = &(context->obj_inst.usb->async_ctx);
        async_ctx->NumBytesToTransfer = count;
        async_ctx->pData = data;
        async_ctx->pfOnComplete = mtb_data_streaming_usb_cb;
        async_ctx->pContext = context;

        USBD_CDC_WriteAsync(context->obj_inst.usb->handle, async_ctx, 0);
        rslt = CY_RSLT_SUCCESS;
    }
    else
    {
        cyhal_system_critical_section_exit(state);
        rslt = MTB_DATA_STREAMING_IN_PROGRESS_ERR;
    }

    return rslt;
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_usb_receive
//--------------------------------------------------------------------------------------------------
static cy_rslt_t mtb_data_streaming_usb_receive(mtb_data_streaming_vcontext_t* vcontext,
                                                uint8_t* data, size_t count, void* tag)
{
    cy_rslt_t rslt;
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)vcontext;

    uint32_t state = cyhal_system_critical_section_enter();
    if (NULL == context->call_tag)
    {
        context->call_tag = tag;
        cyhal_system_critical_section_exit(state);

        USB_ASYNC_IO_CONTEXT* async_ctx = &(context->obj_inst.usb->async_ctx);
        async_ctx->NumBytesToTransfer = count;
        async_ctx->pData = data;
        async_ctx->pfOnComplete = mtb_data_streaming_usb_cb;
        async_ctx->pContext = context;

        USBD_CDC_ReadAsync(context->obj_inst.usb->handle, async_ctx, 0);
        rslt =  CY_RSLT_SUCCESS;
    }
    else
    {
        cyhal_system_critical_section_exit(state);
        rslt = MTB_DATA_STREAMING_IN_PROGRESS_ERR;
    }

    return rslt;
}


//--------------------------------------------------------------------------------------------------
// mtb_data_streaming_setup_usb
//--------------------------------------------------------------------------------------------------
cy_rslt_t mtb_data_streaming_setup_usb(mtb_data_streaming_usb_t* usb,
                                       mtb_data_streaming_xfer_done_t cb,
                                       mtb_data_streaming_interface_t* iface)
{
    iface->send     = mtb_data_streaming_usb_send;
    iface->receive  = mtb_data_streaming_usb_receive;
    mtb_data_streaming_context_t* context = (mtb_data_streaming_context_t*)&(iface->context);
    context->obj_inst.usb = usb;
    context->callback  = cb;
    context->call_tag  = NULL;

    return CY_RSLT_SUCCESS;
}


#endif // if defined(COMPONENT_MW_EMUSB_DEVICE)
