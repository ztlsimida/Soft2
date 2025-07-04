/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-10-03     Yi Qiu       first version
 * 2012-12-12     heyuanjie87  add CDC endpoints collection
 */

#ifndef  __CDC_VCOM_H__
#define  __CDC_VCOM_H__

#define USB_CDC_BUFSIZE                 0x200

#define USB_CDC_CLASS_COMM              0x02
#define USB_CDC_CLASS_DATA              0x0A

#define USB_CDC_SUBCLASS_NONE           0x00
#define USB_CDC_SUBCLASS_DLCM           0x01
#define USB_CDC_SUBCLASS_ACM            0x02
#define USB_CDC_SUBCLASS_TCM            0x03
#define USB_CDC_SUBCLASS_MCCM           0x04
#define USB_CDC_SUBCLASS_CCM            0x05
#define USB_CDC_SUBCLASS_ETH            0x06
#define USB_CDC_SUBCLASS_ATM            0x07
#define USB_CDC_SUBCLASS_EEM            0x0C

#define USB_CDC_PROTOCOL_NONE           0x00
#define USB_CDC_PROTOCOL_V25TER         0x01
#define USB_CDC_PROTOCOL_I430           0x30
#define USB_CDC_PROTOCOL_HDLC           0x31
#define USB_CDC_PROTOCOL_TRANS          0x32
#define USB_CDC_PROTOCOL_Q921M          0x50
#define USB_CDC_PROTOCOL_Q921           0x51
#define USB_CDC_PROTOCOL_Q921TM         0x52
#define USB_CDC_PROTOCOL_V42BIS         0x90
#define USB_CDC_PROTOCOL_Q931           0x91
#define USB_CDC_PROTOCOL_V120           0x92
#define USB_CDC_PROTOCOL_CAPI20         0x93
#define USB_CDC_PROTOCOL_HOST           0xFD
#define USB_CDC_PROTOCOL_PUFD           0xFE
#define USB_CDC_PROTOCOL_VENDOR         0xFF
#define USB_CDC_PROTOCOL_EEM            0x07

#define USB_CDC_CS_INTERFACE            0x24
#define USB_CDC_CS_ENDPOINT             0x25

#define USB_CDC_SCS_HEADER              0x00
#define USB_CDC_SCS_CALL_MGMT           0x01
#define USB_CDC_SCS_ACM                 0x02
#define USB_CDC_SCS_UNION               0x06
#define USB_CDC_SCS_ETH                 0x0F

#define CDC_SEND_ENCAPSULATED_COMMAND   0x00
#define CDC_GET_ENCAPSULATED_RESPONSE   0x01
#define CDC_SET_COMM_FEATURE            0x02
#define CDC_GET_COMM_FEATURE            0x03
#define CDC_CLEAR_COMM_FEATURE          0x04
#define CDC_SET_AUX_LINE_STATE          0x10
#define CDC_SET_HOOK_STATE              0x11
#define CDC_PULSE_SETUP                 0x12
#define CDC_SEND_PULSE                  0x13
#define CDC_SET_PULSE_TIME              0x14
#define CDC_RING_AUX_JACK               0x15
#define CDC_SET_LINE_CODING             0x20
#define CDC_GET_LINE_CODING             0x21
#define CDC_SET_CONTROL_LINE_STATE      0x22
#define CDC_SEND_BREAK                  0x23
#define CDC_SET_RINGER_PARMS            0x30
#define CDC_GET_RINGER_PARMS            0x31
#define CDC_SET_OPERATION_PARMS         0x32
#define CDC_GET_OPERATION_PARMS         0x33
#define CDC_SET_LINE_PARMS              0x34
#define CDC_GET_LINE_PARMS              0x35
#define CDC_DIAL_DIGITS                 0x36
#define CDC_SET_UNIT_PARAMETER          0x37
#define CDC_GET_UNIT_PARAMETER          0x38
#define CDC_CLEAR_UNIT_PARAMETER        0x39
#define CDC_GET_PROFILE                 0x3A
#define CDC_SET_ETH_MULTICAST_FILTERS   0x40
#define CDC_SET_ETH_POWER_MGMT_FILT     0x41
#define CDC_GET_ETH_POWER_MGMT_FILT     0x42
#define CDC_SET_ETH_PACKET_FILTER       0x43
#define CDC_GET_ETH_STATISTIC           0x44
#define CDC_SET_ATM_DATA_FORMAT         0x50
#define CDC_GET_ATM_DEVICE_STATISTICS   0x51
#define CDC_SET_ATM_DEFAULT_VC          0x52
#define CDC_GET_ATM_VC_STATISTICS       0x53

/* The baudrate can be defined as*/
#define BAUD_RATE_2400                  2400
#define BAUD_RATE_4800                  4800
#define BAUD_RATE_9600                  9600
#define BAUD_RATE_19200                 19200
#define BAUD_RATE_38400                 38400
#define BAUD_RATE_57600                 57600
#define BAUD_RATE_115200                115200
#define BAUD_RATE_230400                230400
#define BAUD_RATE_460800                460800
#define BAUD_RATE_921600                921600
#define BAUD_RATE_2000000               2000000
#define BAUD_RATE_3000000               3000000
/* Data bits can be defined as*/
#define DATA_BITS_5                     5
#define DATA_BITS_6                     6
#define DATA_BITS_7                     7
#define DATA_BITS_8                     8
#define DATA_BITS_9                     9
/* Stop bits can be defined as */
#define STOP_BITS_1                     0
#define STOP_BITS_2                     1
#define STOP_BITS_3                     2
#define STOP_BITS_4                     3
/* Parity bits can be defined as */
#define PARITY_NONE                     0
#define PARITY_ODD                      1
#define PARITY_EVEN                     2
/* Bit order can be defined as */
#define BIT_ORDER_LSB                   0
#define BIT_ORDER_MSB                   1
/* Mode canbe defined as */
#define NRZ_NORMAL                      0   /* normal mode */
#define NRZ_INVERTED                    1   /* inverted mode */
/* Default size of the receive data buffer */
#define RT_SERIAL_RB_BUFSZ              64

#pragma pack(1)

enum vcom_control_cmd
{
    USBD_VCOM_CTRL_GET,
    USBD_VCOM_CTRL_SET,
    USBD_VCOM_CTRL_CONNECTED,
    
};

struct ucdc_header_descriptor
{
    rt_uint8_t length;
    rt_uint8_t type;
    rt_uint8_t subtype;
    rt_uint16_t bcd;
};
typedef struct ucdc_header_descriptor* ucdc_hdr_desc_t;

struct ucdc_acm_descriptor
{
    rt_uint8_t length;
    rt_uint8_t type;
    rt_uint8_t subtype;
    rt_uint8_t capabilties;
};
typedef struct ucdc_acm_descriptor* ucdc_acm_desc_t;

struct ucdc_call_mgmt_descriptor
{
    rt_uint8_t length;
    rt_uint8_t type;
    rt_uint8_t subtype;
    rt_uint8_t capabilties;
    rt_uint8_t data_interface;
};
typedef struct ucdc_call_mgmt_descriptor* ucdc_call_mgmt_desc_t;

struct ucdc_union_descriptor
{
    rt_uint8_t length;
    rt_uint8_t type;
    rt_uint8_t subtype;
    rt_uint8_t master_interface;
    rt_uint8_t slave_interface0;
};
typedef struct ucdc_union_descriptor* ucdc_union_desc_t;

struct ucdc_comm_descriptor
{
#ifdef RT_USB_DEVICE_COMPOSITE
    struct uiad_descriptor iad_desc;
#endif
    struct uinterface_descriptor intf_desc;
    struct ucdc_header_descriptor hdr_desc;
    struct ucdc_call_mgmt_descriptor call_mgmt_desc;
    struct ucdc_acm_descriptor acm_desc;
    struct ucdc_union_descriptor union_desc;
    struct uendpoint_descriptor ep_desc;
};
typedef struct ucdc_comm_descriptor* ucdc_comm_desc_t;

struct ucdc_enet_descriptor
{
  rt_uint8_t    bFunctionLength;
  rt_uint8_t    bDescriptorType;
  rt_uint8_t    bDescriptorSubtype;
  rt_uint8_t    iMACAddress;
  rt_uint8_t    bmEthernetStatistics[4];
  rt_uint16_t   wMaxSegmentSize;
  rt_uint16_t   wMCFilters;
  rt_uint8_t    bNumberPowerFilters;
};
struct ucdc_eth_descriptor
{
#ifdef RT_USB_DEVICE_COMPOSITE
    struct uiad_descriptor iad_desc;
#endif
    struct uinterface_descriptor    intf_desc;
    struct ucdc_header_descriptor   hdr_desc;
    struct ucdc_union_descriptor    union_desc;
    struct ucdc_enet_descriptor     enet_desc;
    struct uendpoint_descriptor     ep_desc;
};
typedef struct ucdc_eth_descriptor* ucdc_eth_desc_t;

struct ucdc_data_descriptor
{
    struct uinterface_descriptor intf_desc;
    struct uendpoint_descriptor ep_out_desc;
    struct uendpoint_descriptor ep_in_desc;
};
typedef struct ucdc_data_descriptor* ucdc_data_desc_t;

struct ucdc_line_coding
{
    rt_uint32_t dwDTERate;
    rt_uint8_t bCharFormat;
    rt_uint8_t bParityType;
    rt_uint8_t bDataBits;
};
typedef struct ucdc_line_coding* ucdc_line_coding_t;

struct cdc_eps
{
    uep_t ep_out;
    uep_t ep_in;
    uep_t ep_cmd;
};
typedef struct cdc_eps* cdc_eps_t;



struct ucdc_management_element_notifications
{
    rt_uint8_t bmRequestType;
    rt_uint8_t bNotificatinCode;
    rt_uint16_t wValue;
    rt_uint16_t wIndex;
    rt_uint16_t wLength;
};
typedef struct ucdc_management_element_notifications * ucdc_mg_notifications_t;

struct ucdc_connection_speed_change_data
{
    rt_uint32_t down_bit_rate;
    rt_uint32_t up_bit_rate;
};
typedef struct connection_speed_change_data * connect_speed_data_t;

enum ucdc_notification_code
{
    UCDC_NOTIFI_NETWORK_CONNECTION      = 0x00,
    UCDC_NOTIFI_RESPONSE_AVAILABLE      = 0x01,
    UCDC_NOTIFI_AUX_JACK_HOOK_STATE     = 0x08,
    UCDC_NOTIFI_RING_DETECT             = 0x09,
    UCDC_NOTIFI_SERIAL_STATE            = 0x20,
    UCDC_NOTIFI_CALL_STATE_CHANGE       = 0x28,
    UCDC_NOTIFI_LINE_STATE_CHANGE       = 0x29,
    UCDC_NOTIFI_CONNECTION_SPEED_CHANGE = 0x2A,
};
typedef enum ucdc_notification_code ucdc_notification_code_t;

/**
 * Notify structure
 */
struct rt_device_notify
{
    void (*notify)(rt_device_t dev);
    struct rt_device *dev;
};

struct serial_configure
{
    rt_uint32_t baud_rate;

    rt_uint32_t data_bits               :4;
    rt_uint32_t stop_bits               :2;
    rt_uint32_t parity                  :2;
    rt_uint32_t bit_order               :1;
    rt_uint32_t invert                  :1;
    rt_uint32_t bufsz                   :16;
    rt_uint32_t flowcontrol             :1;
    rt_uint32_t reserved                :5;
};

struct rt_serial_device
{
    struct rt_device          parent;

    const struct rt_uart_ops *ops;
    struct serial_configure   config;

    void *serial_rx;
    void *serial_tx;

    struct rt_device_notify rx_notify;

    struct ufunction *func;
};

/**
 * uart operators
 */
struct rt_uart_ops
{
    rt_err_t (*configure)(struct rt_serial_device       *serial,
                          struct serial_configure       *cfg);

    rt_err_t (*control)(struct rt_serial_device         *serial,
                                            int          cmd,
                                            void        *arg);

    int (*putc)(struct rt_serial_device *serial, char c);
    int (*getc)(struct rt_serial_device *serial);

    rt_ssize_t (*transmit)(struct rt_serial_device       *serial,
                                 rt_uint8_t             *buf,
                                 rt_size_t               size,
                                 rt_uint32_t             tx_flag);
};


#pragma pack()

#endif
