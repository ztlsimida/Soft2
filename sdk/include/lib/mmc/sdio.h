/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2012-01-15     weety         first version
 */

#ifndef __SDIO_H__
#define __SDIO_H__


#include "lib/mmc/mmcsd_host.h"
#include "lib/mmc/mmcsd_card.h"
#include "lib/mmc/sdio_func_ids.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Card Common Control Registers (CCCR)
 */

#define SDIO_REG_CCCR_CCCR_REV      0x00

#define  SDIO_CCCR_REV_1_00 0   /* CCCR/FBR Version 1.00 */
#define  SDIO_CCCR_REV_1_10 1   /* CCCR/FBR Version 1.10 */
#define  SDIO_CCCR_REV_1_20 2   /* CCCR/FBR Version 1.20 */
#define  SDIO_CCCR_REV_3_00 3   /* CCCR/FBR Version 2.00 */

#define  SDIO_SDIO_REV_1_00 0   /* SDIO Spec Version 1.00 */
#define  SDIO_SDIO_REV_1_10 1   /* SDIO Spec Version 1.10 */
#define  SDIO_SDIO_REV_1_20 2   /* SDIO Spec Version 1.20 */
#define  SDIO_SDIO_REV_2_00 3   /* SDIO Spec Version 2.00 */

#define SDIO_REG_CCCR_SD_REV        0x01

#define  SDIO_SD_REV_1_01   0   /* SD Physical Spec Version 1.01 */
#define  SDIO_SD_REV_1_10   1   /* SD Physical Spec Version 1.10 */
#define  SDIO_SD_REV_2_00   2   /* SD Physical Spec Version 2.00 */

#define SDIO_REG_CCCR_IO_EN         0x02
#define SDIO_REG_CCCR_IO_RDY        0x03

#define SDIO_REG_CCCR_INT_EN        0x04    /* Function/Master Interrupt Enable */
#define SDIO_REG_CCCR_INT_PEND      0x05    /* Function Interrupt Pending */

#define SDIO_REG_CCCR_IO_ABORT      0x06    /* function abort/card reset */

#define SDIO_REG_CCCR_BUS_IF        0x07    /* bus interface controls */

#define  SDIO_BUS_WIDTH_1BIT        0x00
#define  SDIO_BUS_WIDTH_4BIT        0x02
#define  SDIO_BUS_ECSI              0x20    /* Enable continuous SPI interrupt */
#define  SDIO_BUS_SCSI              0x40    /* Support continuous SPI interrupt */

#define  SDIO_BUS_ASYNC_INT         0x20

#define  SDIO_BUS_CD_DISABLE        0x80   /* disable pull-up on DAT3 (pin 1) */

#define SDIO_REG_CCCR_CARD_CAPS     0x08

#define  SDIO_CCCR_CAP_SDC  0x01    /* can do CMD52 while data transfer */
#define  SDIO_CCCR_CAP_SMB  0x02    /* can do multi-block xfers (CMD53) */
#define  SDIO_CCCR_CAP_SRW  0x04    /* supports read-wait protocol */
#define  SDIO_CCCR_CAP_SBS  0x08    /* supports suspend/resume */
#define  SDIO_CCCR_CAP_S4MI 0x10    /* interrupt during 4-bit CMD53 */
#define  SDIO_CCCR_CAP_E4MI 0x20    /* enable ints during 4-bit CMD53 */
#define  SDIO_CCCR_CAP_LSC  0x40    /* low speed card */
#define  SDIO_CCCR_CAP_4BLS 0x80    /* 4 bit low speed card */

#define SDIO_REG_CCCR_CIS_PTR       0x09    /* common CIS pointer (3 bytes) */

/* Following 4 regs are valid only if SBS is set */
#define SDIO_REG_CCCR_BUS_SUSPEND   0x0c
#define SDIO_REG_CCCR_FUNC_SEL      0x0d
#define SDIO_REG_CCCR_EXEC_FLAG     0x0e
#define SDIO_REG_CCCR_READY_FLAG    0x0f

#define SDIO_REG_CCCR_FN0_BLKSIZE   0x10 /* 2bytes, 0x10~0x11 */

#define SDIO_REG_CCCR_POWER_CTRL    0x12

#define  SDIO_POWER_SMPC            0x01    /* Supports Master Power Control */
#define  SDIO_POWER_EMPC            0x02    /* Enable Master Power Control */

#define SDIO_REG_CCCR_SPEED         0x13

#define  SDIO_SPEED_SHS             0x01    /* Supports High-Speed mode */
#define  SDIO_SPEED_EHS             0x02    /* Enable High-Speed mode */

/*
 * Function Basic Registers (FBR)
 */

#define SDIO_REG_FBR_BASE(f)    ((f) * 0x100) /* base of function f's FBRs */

#define SDIO_REG_FBR_STD_FUNC_IF    0x00

#define  SDIO_FBR_SUPPORTS_CSA      0x40    /* supports Code Storage Area */
#define  SDIO_FBR_ENABLE_CSA        0x80    /* enable Code Storage Area */

#define SDIO_REG_FBR_STD_IF_EXT     0x01

#define SDIO_REG_FBR_POWER          0x02

#define  SDIO_FBR_POWER_SPS         0x01    /* Supports Power Selection */
#define  SDIO_FBR_POWER_EPS         0x02    /* Enable (low) Power Selection */

#define SDIO_REG_FBR_CIS            0x09    /* CIS pointer (3 bytes) */


#define SDIO_REG_FBR_CSA            0x0C    /* CSA pointer (3 bytes) */

#define SDIO_REG_FBR_CSA_DATA       0x0F

#define SDIO_REG_FBR_BLKSIZE        0x10    /* block size (2 bytes) */

/* SDIO CIS Tuple code */
#define CISTPL_NULL      0x00
#define CISTPL_CHECKSUM  0x10
#define CISTPL_VERS_1    0x15
#define CISTPL_ALTSTR    0x16
#define CISTPL_MANFID    0x20
#define CISTPL_FUNCID    0x21
#define CISTPL_FUNCE     0x22
#define CISTPL_SDIO_STD  0x91
#define CISTPL_SDIO_EXT  0x92
#define CISTPL_END       0xff

/* SDIO device id */
#define SDIO_ANY_FUNC_ID    0xff
#define SDIO_ANY_MAN_ID     0xffff
#define SDIO_ANY_PROD_ID    0xffff

struct sdio_device_id {
    uint8   func_code;
    uint16  manufacturer;
    uint16  product;
};

struct sdio_device_driver {
    char *name;
    struct list_head  list;
    uint32(*probe)(struct mmcsd_card *card);
    uint32(*remove)(struct mmcsd_card *card);
    struct sdio_device_id *id;
};

int32 sdio_io_send_op_cond(struct mmcsd_host *host,
                            uint32           ocr,
                            uint32          *cmd5_resp);
int32 sdio_io_rw_direct(struct mmcsd_card *card,
                         uint32            rw,
                         uint32           fn,
                         uint32           reg_addr,
                         uint8           *pdata,
                         uint8            raw);
int32 sdio_io_rw_extended(struct mmcsd_card *card,
                           uint32            rw,
                           uint32           fn,
                           uint32           addr,
                           uint32            op_code,
                           uint8           *buf,
                           uint32           blocks,
                           uint32           blksize);
int32 sdio_io_rw_extended_block(struct sdio_function *func,
                                 uint32               rw,
                                 uint32              addr,
                                 uint32               op_code,
                                 uint8              *buf,
                                 uint32              len);
uint8 sdio_io_readb(struct sdio_function *func,
                    uint32              reg,
                    int32              *err);
int32 sdio_io_writeb(struct sdio_function *func,
                      uint32              reg,
                      uint8               data);
uint16 sdio_io_readw(struct sdio_function *func,
                     uint32              addr,
                     int32              *err);
int32 sdio_io_writew(struct sdio_function *func,
                      uint16              data,
                      uint32              addr);
uint32 sdio_io_readl(struct sdio_function *func,
                     uint32              addr,
                     int32              *err);
int32 sdio_io_writel(struct sdio_function *func,
                      uint32              data,
                      uint32              addr);
int32 sdio_io_read_multi_fifo_b(struct sdio_function *func,
                                 uint32              addr,
                                 uint8              *buf,
                                 uint32              len);
int32 sdio_io_write_multi_fifo_b(struct sdio_function *func,
                                  uint32              addr,
                                  uint8              *buf,
                                  uint32              len);
int32 sdio_io_read_multi_incr_b(struct sdio_function *func,
                                 uint32              addr,
                                 uint8              *buf,
                                 uint32              len);
int32 sdio_io_write_multi_incr_b(struct sdio_function *func,
                                  uint32              addr,
                                  uint8              *buf,
                                  uint32              len);
int32 sdio_card_detect(struct mmcsd_host *host, uint32 ocr);
int32 sdio_attach_irq(struct sdio_function *func, sdio_irq_handler_t handler);
int32 sdio_detach_irq(struct sdio_function *func);
void sdio_irq_wakeup(struct mmcsd_host *host);
int32 sdio_enable_func(struct sdio_function *func);
int32 sdio_disable_func(struct sdio_function *func);
void sdio_set_drvdata(struct sdio_function *func, void *data);
void *sdio_get_drvdata(struct sdio_function *func);
int32 sdio_set_block_size(struct sdio_function *func,       uint32 blksize);
int32 sdio_register_driver(struct sdio_device_driver *driver);
int32 sdio_unregister_driver(struct sdio_device_driver *driver);
uint32 sdio_unregister_card(struct mmcsd_card *card);
void sdio_free_cis(struct sdio_function *func);
void mmcsd_sdio_init(void);

#ifdef __cplusplus
}
#endif

#endif

