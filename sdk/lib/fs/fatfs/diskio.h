/*-----------------------------------------------------------------------/
/  Low level disk interface modlue include file   (C)ChaN, 2014          /
/-----------------------------------------------------------------------*/

#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "integer.h"
#ifndef NULL
#define NULL ((void *)0)
#endif

#define DEV_RAM 0 /* Example: Map Ramdisk to physical drive 0 */
#define DEV_FLASH 1 /* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB 2 /* Example: Map USB MSD to physical drive 2 */


/* Status of Disk Functions */
typedef BYTE DSTATUS;

/* Results of Disk Functions */
typedef enum {
  RES_OK = 0, /* 0: Successful */
  RES_ERROR,  /* 1: R/W Error */
  RES_WRPRT,  /* 2: Write Protected */
  RES_NOTRDY, /* 3: Not Ready */
  RES_PARERR  /* 4: Invalid Parameter */
} DRESULT;

/*---------------------------------------*/
/* Prototypes for disk control functions */

DSTATUS disk_initialize(BYTE pdrv);
DSTATUS disk_status(BYTE pdrv);
DRESULT disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DRESULT disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff);

/* Disk Status Bits (DSTATUS) */

#define STA_NOINIT 0x01  /* Drive not initialized */
#define STA_NODISK 0x02  /* No medium in the drive */
#define STA_PROTECT 0x04 /* Write protected */

/* Command code for disk_ioctrl fucntion */

/* Generic command (Used by FatFs) */
#define CTRL_SYNC 0        /* Complete pending write process (needed at _FS_READONLY == 0) */
#define GET_SECTOR_COUNT 1 /* Get media size (needed at _USE_MKFS == 1) */
#define GET_SECTOR_SIZE 2  /* Get sector size (needed at _MAX_SS != _MIN_SS) */
#define GET_BLOCK_SIZE 3   /* Get erase block size (needed at _USE_MKFS == 1) */
#define CTRL_TRIM \
  4 /* Inform device that the data on the block of sectors is no longer used (needed at _USE_TRIM == 1) */

/* Generic command (Not used by FatFs) */
#define CTRL_POWER 5  /* Get/Set power status */
#define CTRL_LOCK 6   /* Lock/Unlock media removal */
#define CTRL_EJECT 7  /* Eject media */
#define CTRL_FORMAT 8 /* Create physical format on the media */

/* MMC/SDC specific ioctl command */
#define MMC_GET_TYPE 10   /* Get card type */
#define MMC_GET_CSD 11    /* Get CSD */
#define MMC_GET_CID 12    /* Get CID */
#define MMC_GET_OCR 13    /* Get OCR */
#define MMC_GET_SDSTAT 14 /* Get SD status */
#define ISDIO_READ 55     /* Read data form SD iSDIO register */
#define ISDIO_WRITE 56    /* Write data to SD iSDIO register */
#define ISDIO_MRITE 57    /* Masked write data to SD iSDIO register */

/* ATA/CF specific ioctl command */
#define ATA_GET_REV 20   /* Get F/W revision */
#define ATA_GET_MODEL 21 /* Get model name */
#define ATA_GET_SN 22    /* Get serial number */

typedef DSTATUS (*fatfs_disk_status)(void*);
typedef DSTATUS (*fatfs_disk_initialize)(void*);
typedef DRESULT (*fatfs_disk_read)(void*, BYTE*, DWORD, UINT);
typedef DRESULT (*fatfs_disk_write)(void*,BYTE*, DWORD, UINT);
typedef DRESULT (*fatfs_disk_ioctl)(void*, BYTE, void*);
struct fatfs_diskio {
  fatfs_disk_status status;
  fatfs_disk_initialize init;
  fatfs_disk_read read;
  fatfs_disk_write write;
  fatfs_disk_ioctl ioctl;
};
void fatfs_register_drive(BYTE, struct fatfs_diskio*, void*);

#ifdef __cplusplus
}
#endif

#endif
