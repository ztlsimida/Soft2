/**
  ******************************************************************************
  * @file    sdk\lib\ble\ble_demo.c
  * @author  TAIXIN-SEMI Application Team
  * @version V1.0.0
  * @date    24-07-2023
  * @brief   This file contains examples of three configuration network modes for BLE.
  ******************************************************************************
  * @attention
  *
  * COPYRIGHT 2023 TAIXIN-SEMI
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"

#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "osal/sleep.h"
#include "osal/timer.h"
#include "osal/work.h"

#include "hal/gpio.h"
#include "hal/uart.h"
#include "hal/crc.h"
#include "hal/netdev.h"

#include "lib/heap/sysheap.h"
#include "lib/common/common.h"
#include "lib/common/sysevt.h"
#include "lib/syscfg/syscfg.h"
#include "lib/lmac/lmac.h"
#include "lib/skb/skbpool.h"
#include "lib/skb/skbuff.h"
#include "lib/bus/macbus/mac_bus.h"
#include "lib/atcmd/libatcmd.h"
#include "lib/bus/xmodem/xmodem.h"
#include "lib/umac/ieee80211.h"
#include "lib/umac/umac.h"
#include "lib/ble/uble.h"
#include "lib/ble/ble_def.h"
#include "lib/ble/ble_adv.h"
#include "lib/ble/ble_demo.h"
#include "lib/lmac/lmac.h"
#include "lib/common/sysevt.h"

#include "syscfg.h"

/**
 * @brief   If you want to start BLE to configure the network, you need to open this macro.
 *          Simultaneously configuring BLE_DEMO_MODE.
 *          BLE_DEMO_MODE = 1   mode1(广播配网)
 *                        = 2   mode2(可扫描配网)
 *                        = 3   mode3(BLE协议配网)
 *          else universal interface supporting the selection of three modes.
 */

#if BLE_SUPPORT

/**
 * @brief   This function is a necessary parameter for configuring connections to the network.
 *
 * @return  int32
 */
static int32 ble_network_configured(void)
{
    extern void *g_ops;

    struct lmac_ops *lops = (struct lmac_ops *)g_ops;
    struct bt_ops *bt_ops = (struct bt_ops *)lops->btops;

    ble_demo_stop(bt_ops);

    wpa_passphrase(sys_cfgs.ssid, sys_cfgs.passwd, sys_cfgs.psk);
    ieee80211_conf_set_ssid(WIFI_MODE_STA, sys_cfgs.ssid);
    ieee80211_conf_set_psk(WIFI_MODE_STA, sys_cfgs.psk);
    ieee80211_conf_set_keymgmt(WIFI_MODE_STA, sys_cfgs.key_mgmt);
    sys_cfgs.wifi_mode = WIFI_MODE_STA;
    syscfg_save();

    ieee80211_iface_stop(WIFI_MODE_AP);
    wificfg_flush(WIFI_MODE_STA);
    netdev_set_wifi_mode((struct netdev *)dev_get(HG_WIFI0_DEVID), WIFI_MODE_STA);
    ieee80211_iface_start(WIFI_MODE_STA);

    return RET_OK;
}
/**
 * @brief   This function is a response callback function that handles ATT requests.
 *
 * @param   entry
 * @param   read    ATT read request
 * @param   buff    ATT write request
 * @param   size    buffer size
 * @param   offset  read/write offset.
 * @return  int32
 */
static int32 uble_test_hdlval(const struct uble_value_entry *entry, uint8 read, uint8 *buff, int32 size, uint32 offset)
{
    char *ptr1, *ptr2;
    if (read) { /*get value*/

        //buff: used to store response data.
        //size: buff's size.

        os_printf("BLE READ: offset = %d\r\n", offset);
        os_memset(buff, 'U', size);

        //return response len.
        return offset == 0 ? size : 4;

    } else { /*set value*/
        os_printf("BLE WRITE: buf=%s, size = %d\r\n", buff, size);
        if (buff[0] == ':') {

            ptr1 = os_strchr(buff, ',');
            if (ptr1 == NULL) { return UBLE_ATT_ERR_REQ_NOT_SUPPORTED; }
            *ptr1++ = 0;
            os_strcpy(sys_cfgs.ssid, buff + 1);
            os_printf("SET ssid:%s\r\n", buff + 1);

            ptr2 = os_strchr(ptr1, ',');
            if (ptr2 == NULL) { return UBLE_ATT_ERR_REQ_NOT_SUPPORTED; }
            *ptr2++ = 0;
            os_strcpy(sys_cfgs.passwd, ptr1);
            os_printf("SET passwd:%s\r\n", ptr1);

            sys_cfgs.key_mgmt = ptr2[0] == '1' ? WPA_KEY_MGMT_PSK : WPA_KEY_MGMT_NONE;
            os_printf("SET passwd:%c\r\n", ptr2[0]);
            os_printf("SET keymgmt:%s\r\n", ptr2[0] == '1' ? "WPA-PSK" : "NONE");

            syscfg_dump();

            ble_network_configured();
            SYSEVT_NEW_BLE_EVT(SYSEVT_BLE_NETWORK_CONFIGURED, 0);
        }
        return 0;
    }
}

/**
 * @brief define BLE value table
 */
static const struct uble_value_entry uble_demo_values[] = {
    {.type = UBLE_VALUE_TYPE_HDL, .value = uble_test_hdlval, .size = 0, .bitoff = 0, .maskbit = 0,},
    {.type = UBLE_VALUE_TYPE_STRING, .value = sys_cfgs.ssid, .size = 21, .bitoff = 0, .maskbit = 0,},
    {.type = UBLE_VALUE_TYPE_STRING, .value = sys_cfgs.passwd, .size = 21, .bitoff = 0, .maskbit = 0,},
};

#if BLE_UUID_128
/*360 service*/
#define BLE_360_SERVICE   {0x06,0x05,0x04,0x03,0x02,0x01,0x03,0x00,0x02,0x00,0x01,0x00,0x01,0x00,0x00,0x36}
#define BLE_360_WR_UUID   {0x06,0x05,0x04,0x03,0x02,0x01,0x03,0x00,0x02,0x00,0x01,0x00,0x02,0x00,0x00,0x36}
#define BLE_360_RD_UUID   {0x06,0x05,0x04,0x03,0x02,0x01,0x03,0x00,0x02,0x00,0x01,0x00,0x03,0x00,0x00,0x36}
#define BLE_360_CCCD_UUID {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0x02,0x29,0x00,0x00}
#define BLE_360_properties1   UBLE_GATT_CHARAC_WRITE|UBLE_GATT_CHARAC_WRITE_WITHOUT_RESPONSE
#define BLE_360_properties2   UBLE_GATT_CHARAC_READ|UBLE_GATT_CHARAC_NOTIFY
#endif

#define BLE_TUYA_properties1 UBLE_GATT_CHARAC_WRITE_WITHOUT_RESPONSE|UBLE_GATT_CHARAC_READ

/**
 * @brief ATT service
 */
static const struct uble_gatt_data uble_demo_att_table[] = {
    /* Generic Access service ***************************************************/
    { .att_type=0x2800, .properties=0,                     .att_value=0x1800},
    /* Characteristic: *Device Name *********************************************/
    { .att_type=0x2803, .properties=UBLE_GATT_CHARAC_READ, .att_value=0x2A00},
    { .att_type=0x2A00, .properties=0,                     .att_value=0},
    /* Characteristic: Appearance ***********************************************/
    { .att_type=0x2803, .properties=UBLE_GATT_CHARAC_READ, .att_value=0x2A01},
    { .att_type=0x2A01, .properties=0,                     .att_value=0},

#if BLE_UUID_128
    /* 360 service: 360 service  ******************************************************************/
    { .att_type=0x2800,              .properties=0,                   .att_value_128=BLE_360_SERVICE},
    /* Characteristic: app write ******************************************************************/
    { .att_type=0x2803,              .properties=BLE_360_properties1, .att_value_128=BLE_360_WR_UUID},
    { .att_type_128=BLE_360_WR_UUID, .properties=0,                   .att_value=(uint32)&uble_demo_values[0]},
    /* Characteristic: read, notify ****************************************************************/
    { .att_type=0x2803,              .properties=BLE_360_properties2, .att_value_128= BLE_360_RD_UUID},
    { .att_type_128=BLE_360_RD_UUID, .properties=0,                   .att_value=0},
    { .att_type=0x2902,              .properties=0,                   .att_value=0}, /*Characteristic CCCD*/
#else
    /* Tuya service ************************************************************************/
    { .att_type=0x2800, .properties=0,                    .att_value=0x1910},
    /* Characteristic: app write ***********************************************************/
    { .att_type=0x2803, .properties=BLE_TUYA_properties1, .att_value=0x2b11},
    { .att_type=0x2b11, .properties=0,                    .att_value=(uint32)&uble_demo_values[0]},
    /* Characteristic: notify **************************************************************/
    { .att_type=0x2803, .properties=UBLE_GATT_CHARAC_NOTIFY, .att_value=0x2b10},
    { .att_type=0x2b10, .properties=0,                       .att_value=0},
    { .att_type=0x2902, .properties=0,                       .att_value=0}, /*Characteristic CCCD*/
#endif

    /* Characteristic: ssid (read) **************************************************************/
    { .att_type=0x2803, .properties=UBLE_GATT_CHARAC_READ, .att_value=0x2b12},
    { .att_type=0x2b12, .properties=0,                     .att_value=(uint32)&uble_demo_values[1]},
    /* Characteristic: password (read) **********************************************************/
    { .att_type=0x2803, .properties=UBLE_GATT_CHARAC_READ, .att_value=0x2b13},
    { .att_type=0x2b13, .properties=0,                     .att_value=(uint32)&uble_demo_values[2]},
};

/**
 * @brief   This function is used to process data obtained from mode 1(Broadcast Configuration Network).
 *
 * @param   data    buffer
 * @param   len     buffer size
 */
void ble_adv_parse_param(uint8 *data, int len)
{
    uint8 *ptr = data;
    extern struct sys_config sys_cfgs;
#if 1 //sample code
    uint8 buff[33];
    while (ptr < data + len) {
        switch (ptr[0]) {
            case 1: //SSID
                os_memset(buff, 0, sizeof(buff));
                os_memcpy(buff, ptr + 2, ptr[1]);
                os_printf("SET ssid:%s\r\n", buff);
                os_memcpy(sys_cfgs.ssid, buff, sizeof(buff));
                break;
            case 2: //PassWord
                os_memset(buff, 0, sizeof(buff));
                os_memcpy(buff, ptr + 2, ptr[1]);
                os_printf("SET passwd:%s\r\n", buff);
                os_memcpy(sys_cfgs.passwd, buff, sizeof(buff));
                break;
            case 3: //Keymgmt
                os_printf("SET keymgmt:%s\r\n", ptr[2] ? "WPA-PSK" : "NONE");
                sys_cfgs.key_mgmt = ptr[2] ? WPA_KEY_MGMT_PSK : WPA_KEY_MGMT_NONE;
                break;
            case 4: //auth
                os_printf("AUTH %d\r\n", ptr[2]);
                break;
            default:
                os_printf("Unsupport ID:%d\r\n", ptr[0]);
                break;
        }
        ptr += (ptr[1] + 2);
    }

    syscfg_dump();

    ble_network_configured();
    SYSEVT_NEW_BLE_EVT(SYSEVT_BLE_NETWORK_CONFIGURED, 0);
#endif
}
int32 ble_set_coexist_en(struct bt_ops *btops, uint8 coexist, uint8 dec_duty)
{
    return btops ? ble_ll_set_coexist_en(btops, coexist, dec_duty) : -RET_ERR;
}
#if (BLE_DEMO_MODE == 1)
/**
 * @brief   This function activates distribution network mode 1(Broadcast Configuration Network).
 *
 * @param   bt_ops  bt_ops
 * @return  int32
 */
int32 ble_demo_mode1_init(struct bt_ops *bt_ops)
{
    ble_adv_init(bt_ops);

    return ble_ll_open(bt_ops, 0, 38);
}

#elif (BLE_DEMO_MODE == 2)
/**
 * @brief   This function activates distribution network mode 2(Scanable configuration network).
 *
 * @param   bt_ops  bt_ops
 * @return  int32
 */
int32 ble_demo_mode2_init(struct bt_ops *bt_ops)
{
    ble_adv_init(bt_ops);
    uint8 scan_resp[] = {0x04, 0x09, 0x53, 0x53, 0x53, 0x19, 0xFF, 0xD0, 0x07, 0x01, 0x03, 0x00, 0x00, 0x0C, 0x00, 0x88, 0xD1, 0xC4, 0x89, 0x2B, 0x56, 0x7D, 0xE5, 0x65, 0xAC, 0xA1, 0x3F, 0x09, 0x1C, 0x43, 0x92};
    uint8 adv_data[] = {0x02, 0x01, 0x06, 0x03, 0x02, 0x01, 0xA2, 0x14, 0x16, 0x01, 0xA2, 0x01, 0x6B, 0x65, 0x79, 0x79, 0x66, 0x67, 0x35, 0x79, 0x33, 0x34, 0x79, 0x71, 0x78, 0x71, 0x67, 0x64};

    ble_ll_set_advdata(bt_ops, adv_data, sizeof(adv_data));
    ble_ll_set_scan_rsp(bt_ops, scan_resp, sizeof(scan_resp));
    ble_ll_set_adv_interval(bt_ops, 100);
    ble_ll_set_adv_en(bt_ops, 1);

    return ble_ll_open(bt_ops, 1, 38);
}

#elif (BLE_DEMO_MODE == 3)
/**
 * @brief   This function activates distribution network mode 3(BLE protocol configuration network).
 *
 * @param   bt_ops  bt_ops
 * @return  int32
 */
int32 ble_demo_mode3_init(struct bt_ops *bt_ops)
{
    uble_init(bt_ops, uble_demo_att_table, ARRAY_SIZE(uble_demo_att_table), 512);
    uint8 scan_resp[] = {0x04, 0x09, 0x53, 0x53, 0x53, 0x19, 0xFF, 0xD0, 0x07, 0x01, 0x03, 0x00, 0x00, 0x0C, 0x00, 0x88, 0xD1, 0xC4, 0x89, 0x2B, 0x56, 0x7D, 0xE5, 0x65, 0xAC, 0xA1, 0x3F, 0x09, 0x1C, 0x43, 0x92};
    uint8 adv_data[] = {0x02, 0x01, 0x06, 0x03, 0x02, 0x01, 0xA2, 0x14, 0x16, 0x01, 0xA2, 0x01, 0x6B, 0x65, 0x79, 0x79, 0x66, 0x67, 0x35, 0x79, 0x33, 0x34, 0x79, 0x71, 0x78, 0x71, 0x67, 0x64};

    ble_ll_set_advdata(bt_ops, adv_data, sizeof(adv_data));
    ble_ll_set_scan_rsp(bt_ops, scan_resp, sizeof(scan_resp));
    ble_ll_set_adv_interval(bt_ops, 100);

    return ble_ll_open(bt_ops, 2, 38);
}

#else
/**
 * @brief   This function integrates three modes of network configuration.
 *
 * @param   bt_ops  bt_ops
 * @param   type    type = mode-1
 * @return  int32
 */
int32 ble_demo_start(struct bt_ops *bt_ops, uint8 type)
{
    uint8 scan_resp[] = {0x04, 0x09, 0x53, 0x53, 0x53, 0x19, 0xFF, 0xD0, 0x07, 0x01, 0x03, 0x00, 0x00, 0x0C, 0x00, 0x88, 0xD1, 0xC4, 0x89, 0x2B, 0x56, 0x7D, 0xE5, 0x65, 0xAC, 0xA1, 0x3F, 0x09, 0x1C, 0x43, 0x92};
    uint8 adv_data[] = {0x02, 0x01, 0x06, 0x03, 0x02, 0x01, 0xA2, 0x14, 0x16, 0x01, 0xA2, 0x01, 0x6B, 0x65, 0x79, 0x79, 0x66, 0x67, 0x35, 0x79, 0x33, 0x34, 0x79, 0x71, 0x78, 0x71, 0x67, 0x64};

    if (type == BLE_LL_TYPE_CONNECT) {
        uble_init(bt_ops, uble_demo_att_table, ARRAY_SIZE(uble_demo_att_table), 512);
    } else {
        ble_adv_init(bt_ops);
    }
    ble_ll_set_advdata(bt_ops, adv_data, sizeof(adv_data));
    ble_ll_set_scan_rsp(bt_ops, scan_resp, sizeof(scan_resp));
    ble_ll_set_adv_interval(bt_ops, 100);
    ble_ll_set_adv_en(bt_ops, 1);
	ble_ll_set_ll_length(bt_ops, 251);
	
    return ble_ll_open(bt_ops, type, 38);
}
#endif

/**
 * @brief   This function is to disable BLE configuration network.
 *
 * @param   bt_ops  bt_ops
 * @return  int32
 */
int32 ble_demo_stop(struct bt_ops *bt_ops)
{
    ble_adv_deinit();

    return ble_ll_close(bt_ops);
}

#endif

/*************************** (C) COPYRIGHT 2023 TAIXIN-SEMI ***** END OF FILE *****/

