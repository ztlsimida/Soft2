/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-12-12     Yi Qiu      first version
 */

#ifndef __MASS_H__
#define __MASS_H__

#include <rtthread.h>
#include "dev.h"
#include "devid.h"
#include "diskio.h"
#include "ff.h"


#define MAX_PARTITION_COUNT        4
#define SECTOR_SIZE                512

struct ustor_data
{
    struct uhintf* intf;
    int udisk_id;
    const char path;
};

struct ustor_device
{
    rt_uint32_t type;
    fatfs_disk_status status;
    fatfs_disk_initialize init;
    fatfs_disk_read read;
    fatfs_disk_write write;
    fatfs_disk_ioctl ioctl;
    void *user_data; 
};

struct ustor
{
    upipe_t pipe_in;
    upipe_t pipe_out;
    rt_uint32_t capicity[2];

    struct ustor_device dev[MAX_PARTITION_COUNT];
    rt_uint8_t dev_cnt;
    rt_uint32_t ref;
};
typedef struct ustor* ustor_t;

rt_err_t rt_usbh_storage_get_max_lun(struct uhintf* intf, rt_uint8_t* max_lun);
rt_err_t rt_usbh_storage_reset(struct uhintf* intf);
rt_err_t rt_usbh_storage_read10(struct uhintf* intf, rt_uint8_t *buffer,
    rt_uint32_t sector, rt_size_t count, int timeout);
rt_err_t rt_usbh_storage_write10(struct uhintf* intf, rt_uint8_t *buffer,
    rt_uint32_t sector, rt_size_t count, int timeout);
rt_err_t rt_usbh_storage_request_sense(struct uhintf* intf, rt_uint8_t* buffer);
rt_err_t rt_usbh_storage_test_unit_ready(struct uhintf* intf);
rt_err_t rt_usbh_storage_inquiry(struct uhintf* intf, rt_uint8_t* buffer);
rt_err_t rt_usbh_storage_get_capacity(struct uhintf* intf, rt_uint8_t* buffer);

#endif
