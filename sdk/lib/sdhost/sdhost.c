#include "sys_config.h"
#include "typesdef.h"
#include "devid.h"
#include "list.h"
#include "dev.h"
#include "osal/task.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "lib/sdhost/sdhost.h"
#include "hal/gpio.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "osal/sleep.h"
#include "osal/timer.h"
#include "osal/work.h"



struct sdh_device *sdh_test;
struct os_semaphore sem;

#define be32_to_cpu(x) ((uint32_t)(              \
    (((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |        \
    (((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |        \
    (((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |        \
    (((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24)))

static uint32_t __rt_fls(uint32_t val)
{
    uint32_t  bit = 32;

    if (!val)
        return 0;
    if (!(val & 0xffff0000u)) 
    {
        val <<= 16;
        bit -= 16;
    }
    if (!(val & 0xff000000u)) 
    {
        val <<= 8;
        bit -= 8;
    }
    if (!(val & 0xf0000000u)) 
    {
        val <<= 4;
        bit -= 4;
    }
    if (!(val & 0xc0000000u)) 
    {
        val <<= 2;
        bit -= 2;
    }
    if (!(val & 0x80000000u)) 
    {
        bit -= 1;
    }

    return bit;
}




static const uint32_t tran_unit[] =
{
    10000, 100000, 1000000, 10000000,
    0,     0,      0,       0
};

static const uint8_t tran_value[] =
{
    0,  10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};

static const uint32_t tacc_uint[] =
{
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
};

static const uint8_t tacc_value[] =
{
    0,  10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};

uint32_t GET_BITS(uint32_t *resp,
                               uint32_t  start,
                               uint32_t  size)
{                               
        const int32_t __size = size;
        const uint32_t __mask = (__size < 32 ? 1 << __size : 0) - 1; 
        const int32_t __off = 3 - ((start) / 32);
        const int32_t __shft = (start) & 31;
        uint32_t __res;

        __res = resp[__off] >> __shft;
        if (__size + __shft > 32)
            __res |= resp[__off-1] << ((32 - __shft) % 32);

        return __res & __mask;
}


void sdhost_io_func_init(uint32 req){
    if(req == 1)
        pin_func(HG_SDIOHOST_DEVID,4);
    else
        pin_func(HG_SDIOHOST_DEVID,1);
}

static int32_t sd_parse_scr(struct sdh_device *host)
{
    struct rt_sd_scr *scr = &host->scr;
    uint32_t resp[4];

    resp[3] = host->resp_scr[1];
    resp[2] = host->resp_scr[0];
    scr->sd_version = GET_BITS(resp, 56, 4);
    scr->sd_bus_widths = GET_BITS(resp, 48, 4);

    os_printf("sd_version : %d \t %d\r\n", scr->sd_version, scr->sd_bus_widths);
    return 0;
}
#ifdef TXW81X
static int32_t sd_switch(struct sdh_device *host)
{
    int32_t ret;
    struct rt_mmcsd_cmd cmd;
    uint8_t *buf = os_malloc(64);
    if (!buf)
    {
        os_printf("mallo err!\r\n");
        return 1;
    }

    host->data.blksize = 64;
    host->data.blks    = 1;
    host->data.err     = 0;
    if (host->read)
    {
        ret = host->read(host, buf);
        if (ret)
            return 1;
    }
    
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = SD_SWITCH;
    cmd.arg = 0x00FFFFF1;
    cmd.flags = RESP_R1 | CMD_ADTC;
    if (host->cmd)
    {
        ret = host->cmd(host, &cmd);
        if (ret)
            return 1;
    }

    if (host->complete)
    {
        ret = host->complete(host);
        if (ret)
            return 1;
    }
        
    if (buf[13] & 0x02)
        host->max_data_rate = 50*1000*1000;

	#if 0
	for(int itk = 0;itk <64;itk++){	
		if(itk%32 == 0)
			os_printf("\r\n");

		os_printf("%02x ",buf[itk]);
	}
	#endif

    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    host->data.blksize = 64;
    host->data.blks    = 1;
    host->data.err     = 0;
    if (host->read)
    {
        ret = host->read(host, buf);
        if (ret)
            return 1;
    }
    cmd.cmd_code = SD_SWITCH;
    cmd.arg = 0x80FFFFF1;
    cmd.flags = RESP_R1 | CMD_ADTC;
    if (host->cmd)
    {
        ret = host->cmd(host, &cmd);
        if (ret)
            return 1;
    }

    if (host->complete)
    {
        ret = host->complete(host);
        if (ret)
            return 1;
    }

	#if 0
	for(int itk = 0;itk <64;itk++){	
		if(itk%32 == 0)
			os_printf("\r\n");
		
		os_printf("%02x ",buf[itk]);
	}
	os_printf("\r\n");
	#endif

    if ((buf[16] & 0xF) != 1) 
    {
        os_printf("switching card to high speed failed!");
        return 1;
    }
	os_printf("switch finish\r\n");
    host->cardflags |= CARD_FLAG_HIGHSPEED;
    os_free(buf);
    return 0;
}
#endif

unsigned int sd_dwCap;
static int32_t sd_parse_csd(struct sdh_device *host)
{
    struct rt_mmcsd_csd *csd = &host->csd;
    uint32_t *resp = host->resp_csd;

    csd->csd_structure = GET_BITS(resp, 126, 2);

    switch (csd->csd_structure)
    {
    case 0:
		host->cardflags &= ~CARD_FLAG_SDHC;
        csd->taac = GET_BITS(resp, 112, 8);
        csd->nsac = GET_BITS(resp, 104, 8);
        csd->tran_speed = GET_BITS(resp, 96, 8);
        csd->card_cmd_class = GET_BITS(resp, 84, 12);
        csd->rd_blk_len = GET_BITS(resp, 80, 4);
        csd->rd_blk_part = GET_BITS(resp, 79, 1);
        csd->wr_blk_misalign = GET_BITS(resp, 78, 1);
        csd->rd_blk_misalign = GET_BITS(resp, 77, 1);
        csd->dsr_imp = GET_BITS(resp, 76, 1);
        csd->c_size = GET_BITS(resp, 62, 12);
        csd->c_size_mult = GET_BITS(resp, 47, 3);
        csd->r2w_factor = GET_BITS(resp, 26, 3);
        csd->wr_blk_len = GET_BITS(resp, 22, 4);
        csd->wr_blk_partial = GET_BITS(resp, 21, 1);
        csd->csd_crc = GET_BITS(resp, 1, 7);

        host->card_blksize      = 1 << csd->rd_blk_len;
        host->card_capacity     = (csd->c_size + 1) << (csd->c_size_mult + 2);
        host->card_capacity *= host->card_blksize;
        host->card_capacity >>= 10; /* unit:KB */
        host->tacc_clks = csd->nsac * 100;
        host->tacc_ns = (tacc_uint[csd->taac&0x07] * tacc_value[(csd->taac&0x78)>>3] + 9) / 10;
        host->max_data_rate = tran_unit[csd->tran_speed&0x07] * tran_value[(csd->tran_speed&0x78)>>3];

    #if 0
        val = GET_BITS(resp, 115, 4);
        unit = GET_BITS(resp, 112, 3);
        csd->tacc_ns     = (tacc_uint[unit] * tacc_value[val] + 9) / 10;
        csd->tacc_clks   = GET_BITS(resp, 104, 8) * 100;

        val = GET_BITS(resp, 99, 4);
        unit = GET_BITS(resp, 96, 3);
        csd->max_data_rate    = tran_unit[unit] * tran_value[val];
        csd->ccc      = GET_BITS(resp, 84, 12);

        unit = GET_BITS(resp, 47, 3);
        val = GET_BITS(resp, 62, 12);
        csd->device_size      = (1 + val) << (unit + 2);

        csd->read_bl_len = GET_BITS(resp, 80, 4);
        csd->write_bl_len = GET_BITS(resp, 22, 4);
        csd->r2w_factor = GET_BITS(resp, 26, 3);
    #endif
        break;
    case 1:
        host->cardflags |= CARD_FLAG_SDHC;

        /*This field is fixed to 0Eh, which indicates 1 ms. 
          The host should not use TAAC, NSAC, and R2W_FACTOR
          to calculate timeout and should uses fixed timeout
          values for read and write operations*/
        csd->taac = GET_BITS(resp, 112, 8);
        csd->nsac = GET_BITS(resp, 104, 8);
        csd->tran_speed = GET_BITS(resp, 96, 8);
        csd->card_cmd_class = GET_BITS(resp, 84, 12);
        csd->rd_blk_len = GET_BITS(resp, 80, 4);
        csd->rd_blk_part = GET_BITS(resp, 79, 1);
        csd->wr_blk_misalign = GET_BITS(resp, 78, 1);
        csd->rd_blk_misalign = GET_BITS(resp, 77, 1);
        csd->dsr_imp = GET_BITS(resp, 76, 1);
        csd->c_size = GET_BITS(resp, 48, 22);

        csd->r2w_factor = GET_BITS(resp, 26, 3);
        csd->wr_blk_len = GET_BITS(resp, 22, 4);
        csd->wr_blk_partial = GET_BITS(resp, 21, 1);
        csd->csd_crc = GET_BITS(resp, 1, 7);

        host->card_blksize = 512;
        host->card_capacity = (csd->c_size + 1) * 512;  /* unit:KB */
        host->tacc_clks = 0;
        host->tacc_ns = 0;
        host->max_data_rate = tran_unit[csd->tran_speed&0x07] * tran_value[(csd->tran_speed&0x78)>>3];
    #if 0
        csd->tacc_ns     = 0;
        csd->tacc_clks   = 0;

        val = GET_BITS(resp, 99, 4);
        unit = GET_BITS(resp, 96, 3);
        csd->max_data_rate    = tran_unit[unit] * tran_value[val];
        csd->ccc      = GET_BITS(resp, 84, 12);

        val = GET_BITS(resp, 48, 22);
        csd->device_size     = (1 + val) << 10;

        csd->read_bl_len = 9;
        csd->write_bl_len = 9;
        /* host should not use this factor and should use 250ms for write timeout */
        csd->r2w_factor = 2;
    #endif
        break;
    default:
        os_printf("unrecognised CSD structure version %d!", csd->csd_structure);

        return -EINVAL;
    }
    host->card_max_blk_num = host->card_capacity << 1;
    os_printf("SD card capacity %d KB.\r\n" , host->card_capacity);
    os_printf("SD card max block num %d\r\n", host->card_max_blk_num);
    sd_dwCap = host->card_capacity;

    return 0;
}

uint32 select_voltage(struct sdh_device *host, uint32_t ocr)
{
    int bit;
    //extern int ffs32_lsb(uint32_t value);

    ocr &= host->valid_ocr;

    bit = 15;//ffs32_lsb(ocr);
    if (bit) 
    {
        bit -= 1;

        ocr &= 3 << bit;

        host->io_cfg.vdd = bit;
        //mmcsd_set_iocfg(host);
        if(host->iocfg)
            host->iocfg(host,&host->io_cfg);
    } 
    else 
    {
        os_printf("host doesn't support card's voltages!");
        ocr = 0;
    }

    return ocr;	
}

uint32 sd_power_up(struct sdh_device *host,uint8 bus_w)
{
    int bit = __rt_fls(host->valid_ocr) - 1;
    
     host->io_cfg.vdd = bit;
     if (controller_is_spi(host))
     {
         host->io_cfg.chip_select = MMCSD_CS_HIGH;
         host->io_cfg.bus_mode = MMCSD_BUSMODE_PUSHPULL;
     } 
     else
     {
         host->io_cfg.chip_select = MMCSD_CS_IGNORE;
         host->io_cfg.bus_mode = MMCSD_BUSMODE_OPENDRAIN;
     }
     host->io_cfg.power_mode = MMCSD_POWER_UP;
     if(bus_w == MMCSD_BUSWIDTH_4)
         host->io_cfg.bus_width = MMCSD_BUS_WIDTH_4;
     else
         host->io_cfg.bus_width = MMCSD_BUS_WIDTH_1;
     
     host->io_cfg.clock = 400000;
     
     if(host->iocfg)
          host->iocfg(host,&host->io_cfg);

    
     /*
      * This delay should be sufficient to allow the power supply
      * to reach the minimum voltage.
      */
     os_sleep_ms(10);
    
     host->io_cfg.clock = host->freq_min;
     host->io_cfg.power_mode = MMCSD_POWER_ON;
     if(host->iocfg)
          host->iocfg(host,&host->io_cfg);

    
     /*
      * This delay must be at least 74 clock sizes, or 1 ms, or the
      * time required to reach a stable voltage.
      */
     os_sleep_ms(10);
     return 0;
}

void sd_set_clk(struct sdh_device * host,uint32_t clk)
{
    host->io_cfg.clock 		 = clk;
    host->io_cfg.ioctl_type = LL_SDHC_IOCTRL_SET_CLOCK;
    host->iocfg(host,&host->io_cfg);
}

void sd_set_bus_width(struct sdh_device * host,uint32_t width)
{
    host->io_cfg.bus_width   = width;
    host->io_cfg.ioctl_type = LL_SDHC_IOCTRL_SET_BUS_WIDTH;
    host->iocfg(host,&host->io_cfg);
}

#ifdef TXW81X
void sd_set_sample(struct sdh_device *host, TYPE_LL_SDHC_SMP_CFG type, uint8_t cmd_cmp, uint8_t dat_cmp)
{
    host->io_cfg.smp_type       = type;
    host->io_cfg.cmd_crc_sample = cmd_cmp;
    host->io_cfg.dat_crc_sample = dat_cmp;
    host->io_cfg.ioctl_type 	= LL_SDHC_IOCTRL_SET_SMP;
    host->iocfg(host, &host->io_cfg);
}

void sd_delay_config(struct sdh_device *host, TYPE_LL_SDHC_DELAY_SYSCLK dly_cfg, uint8_t chain)
{
    if (dly_cfg == LL_SDHC_DLY_NONE)
    {
        host->io_cfg.delay_flag      = 0;
    }else{
        host->io_cfg.delay_flag      = 1;
        host->io_cfg.delay_type      = dly_cfg;
        host->io_cfg.delay_chain_cnt = chain;
    }
    host->io_cfg.ioctl_type 	 = LL_SDHC_IOCTRL_SET_DELAY_TYPE;
    host->iocfg(host, &host->io_cfg);
}

void sd_dat_of_stop_clk_cfg(struct sdh_device *host, uint8_t flag)
{
    host->io_cfg.dat_overflow_stop_flag = flag;
    host->io_cfg.ioctl_type 		    = LL_SDHC_IOCTRL_SET_DAT_OF_STOP_CLK;
    host->iocfg(host, &host->io_cfg);
}
#endif

uint32 send_idle(struct sdh_device * host)
{
    uint32 ret;
    struct rt_mmcsd_cmd cmd;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = GO_IDLE_STATE;
    cmd.arg = 0;
    cmd.flags = RESP_SPI_R1 | RESP_NONE | CMD_BC;

    if(host->cmd)
        ret = host->cmd(host,&cmd);
    else{
        os_printf("no cmd action register\r\n");
        return 0;
    }
    return ret;
}

uint32 send_all_get_cid(struct sdh_device * host,uint32_t *cid)
{
    uint32 ret;
    struct rt_mmcsd_cmd cmd;

    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = ALL_SEND_CID;
    cmd.arg = 0;
    cmd.flags = RESP_R2 | CMD_BCR;
    ret = host->cmd(host,&cmd);

    if(ret==0)
        memcpy(cid, cmd.resp, sizeof(uint32_t) * 4);
    
    return ret;
}

uint32 send_get_card_addr(struct sdh_device * host,uint32_t *rca)
{
    uint32 ret;
    struct rt_mmcsd_cmd cmd;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    cmd.cmd_code = SD_SEND_RELATIVE_ADDR;
    cmd.arg = 0;
    cmd.flags = RESP_R6 | CMD_BCR;	
    ret = host->cmd(host,&cmd);

    *rca = cmd.resp[0] >> 16;
    return 0;
}

uint32 send_card_status(struct sdh_device * host){
    struct rt_mmcsd_cmd cmd;
    int     ret    = 0;
    uint32  status = 0;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = SEND_STATUS;

    cmd.arg = host->rca << 16;
    cmd.flags = RESP_R1 | CMD_AC;


    if(host->cmd)
       ret = host->cmd(host,&cmd);

    status = (cmd.resp[0] >> 9) & 0xf;

    if (status != MMCSD_CARD_STATUS_TRAN)
    {
        os_printf("card status : %d\r\n", status);
        return RET_ERR;
    }
    return ret;	
}

uint32 send_select_card(struct sdh_device * host)
{
    struct rt_mmcsd_cmd cmd;
    int ret = 0;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = SELECT_CARD;

    if (host->rca) 
    {
        cmd.arg = host->rca << 16;
        cmd.flags = RESP_R1 | CMD_AC;
    } 
    else 
    {
        cmd.arg = 0;
        cmd.flags = RESP_NONE | CMD_AC;
    }
    
    if(host->cmd)
       ret = host->cmd(host,&cmd);

    return ret;
}

uint32 send_if_cond(struct sdh_device * host,uint32_t ocr)
{
    struct rt_mmcsd_cmd cmd;
    int ret = 0;
    uint8_t pattern;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = SD_SEND_IF_COND;
    cmd.arg = ((ocr & 0xFF8000) != 0) << 8 | 0xAA;
    cmd.flags = RESP_SPI_R7 | RESP_R7 | CMD_BCR;

    if(host->cmd)
       ret = host->cmd(host,&cmd);

    //if (controller_is_spi(host))
    //   pattern = cmd.resp[1] & 0xFF;
    //else
    pattern = cmd.resp[0] & 0xFF;

    if (pattern != 0xAA)
       return -EINVAL;

    return ret;

}

uint32 send_get_csd(struct sdh_device * host,uint32_t *csd)
{
    int ret;	
    struct rt_mmcsd_cmd cmd;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    
    cmd.cmd_code = SEND_CSD;
    cmd.arg = host->rca << 16;
    cmd.flags = RESP_R2 | CMD_AC;
    ret = host->cmd(host,&cmd);
    
    memcpy(csd, cmd.resp, sizeof(uint32_t) * 4);
    return ret;
}


uint32 send_app_cmd(struct sdh_device *host,uint32 rca)
{
    struct rt_mmcsd_cmd cmd = {0};
    int ret = 0;

    cmd.cmd_code = APP_CMD;
    if(rca){
        cmd.arg = rca << 16;
        cmd.flags = RESP_R1 | CMD_AC;		
    }
    else
    {
        cmd.arg = 0;
        cmd.flags = RESP_R1 | CMD_BCR;		
    }
    if(host->cmd)
       ret = host->cmd(host,&cmd);
    return ret;
}

uint32 sd_app_set_bus_width(struct sdh_device *host,int32_t width)
{
    int ret = 0;
    struct rt_mmcsd_cmd cmd;

    send_app_cmd(host,host->rca);


    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = SD_APP_SET_BUS_WIDTH;
    cmd.flags = RESP_R1 | CMD_AC;

    switch (width) 
    {
    case MMCSD_BUS_WIDTH_1:
        cmd.arg = MMCSD_BUS_WIDTH_1;
        break;
    case MMCSD_BUS_WIDTH_4:
        cmd.arg = MMCSD_BUS_WIDTH_4;
        break;
    default:
        return -EINVAL;
    }	

    if(host->cmd)
       ret = host->cmd(host,&cmd);

    return ret;
}

uint32 send_get_scr(struct sdh_device *host,uint32* scr)
{
    struct rt_mmcsd_cmd cmd;
    int ret;

    if(host->flags & MMCSD_BUSWIDTH_4)
        sd_set_bus_width(host, MMCSD_BUS_WIDTH_1);	

    host->data.blksize = 8;
    host->data.blks    = 1;
    host->data.err     = 0;
    if(host->read)
        ret = host->read(host,(uint8*)scr);

    send_app_cmd(host,host->rca);
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    cmd.cmd_code = SD_APP_SEND_SCR;
    cmd.arg = 0;
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;
    if(host->cmd)
       ret = host->cmd(host,&cmd);

    if (host->complete)
    {
        ret = host->complete(host);
        if (ret)
            return 1;
    }
    
    if(host->data.err != 0)
        return 0;

    scr[0] = be32_to_cpu(scr[0]);
    scr[1] = be32_to_cpu(scr[1]);

    os_printf("scr:%x %x\r\n",scr[0],scr[1]);
    return 1;
}


uint32 sd_tran_stop(struct sdh_device * host)
{
    int ret = 0;	
    struct rt_mmcsd_cmd  cmd;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    cmd.cmd_code = STOP_TRANSMISSION;
    cmd.arg = 0;
    cmd.flags = RESP_SPI_R1B | RESP_R1B | CMD_AC;
    host->sd_opt  = SD_IDLE;
    if(host->cmd)
       ret = host->cmd(host,&cmd);

    host->sd_stop = 0;

    if(!ret)
    {
        for (int i = 0; i < 50; i++)
        {
            ret = send_card_status(host);
            if(!ret)    break;
        }
        if(ret)
        {
            os_printf("%s status err!\r\n", __func__);
            ret = RET_ERR;
        }
        
    }
    return ret;
    
}

//给外部接口专门用的停止命令,现在暂时是给文件系统
uint32 fatfs_sd_tran_stop(struct sdh_device * host)
{
    int ret;
    os_mutex_lock(&host->lock,osWaitForever);
    ret = sd_tran_stop(host);
    os_mutex_unlock(&host->lock);
    return ret;
    
}

int sd_multiple_write(struct sdh_device * host,uint32 lba,uint32 len,uint8* buf)
{
    int ret;
    int send_cmd = 1;
    uint32_t backup_lba = host->new_lba;
    struct rt_mmcsd_cmd  cmd;	
    os_mutex_lock(&host->lock,osWaitForever);
    if(((lba != host->new_lba)||(host->sd_opt != SD_M_W))&& host->sd_stop)
    {
        ret = sd_tran_stop(host);	
        if (ret)    goto __err;
        host->new_lba = lba;
    }
    else if(host->sd_opt == SD_IDLE)
    {
        send_cmd = 1;
    }
    else
    {
        send_cmd = 0;
    }

    if ((host->new_lba + len/SECTOR_SIZE) >= host->card_max_blk_num)
    {
        os_printf("%s operation lba %d size %d max : %d err\r\n", __func__, host->new_lba, len/SECTOR_SIZE, host->card_max_blk_num);
        host->new_lba = backup_lba;
        ret = RET_ERR;
        goto __err;
    }

    host->sd_stop = 1;
    host->new_lba = host->new_lba + len/SECTOR_SIZE;
    host->sd_opt  = SD_M_W;
    ///////////////////////////////////////////////////////
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    cmd.cmd_code = WRITE_MULTIPLE_BLOCK;
    cmd.arg = lba;
    if (!(host->cardflags & CARD_FLAG_SDHC)) 
    {
        cmd.arg <<= 9;
    }	
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;
    if((host->cmd) && send_cmd){	
        ret = host->cmd(host,&cmd);  
        if(ret){
			sd_tran_stop(host);
            ret = -1;
            goto __err;
        }
    }
    
    ///////////////////////////////////////////////////////
    host->data.blksize = SECTOR_SIZE;
    host->data.blks    = len/SECTOR_SIZE;
    host->data.err     = 0;	
    if(host->write)
        ret = host->write(host,buf);

    if (host->complete)
        ret = host->complete(host);

    if (ret) {
        sd_tran_stop(host);
    }  
__err:
    os_mutex_unlock(&host->lock);
    return ret;
}

int sd_multiple_read(struct sdh_device * host,uint32 lba,uint32 len,uint8* buf)
{
    int ret      = 0;
    int send_cmd = 1;
    uint32_t backup_lba = host->new_lba;
    struct rt_mmcsd_cmd  cmd;	
    os_mutex_lock(&host->lock,osWaitForever);
    if(((lba != host->new_lba)||(host->sd_opt != SD_M_R))&& host->sd_stop)
    {
        ret = sd_tran_stop(host);	
        if (ret)    goto __err;
        host->new_lba = lba;
    }
    else if(host->sd_opt == SD_IDLE)
    {
        send_cmd = 1;
    }
    else
    {
        send_cmd = 0;
    }
    if ((host->new_lba + len/SECTOR_SIZE) >= host->card_max_blk_num)
    {
        os_printf("%s operation lba %d size %d max : %d err\r\n", __func__, host->new_lba, len/SECTOR_SIZE, host->card_max_blk_num);
        host->new_lba = backup_lba;
        ret = RET_ERR;
        goto __err;
    }
    host->sd_stop = 1;
    host->new_lba = host->new_lba + len/SECTOR_SIZE;
    host->sd_opt  = SD_M_R;
    ///////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////
    host->data.blksize = SECTOR_SIZE;
    host->data.blks    = len/SECTOR_SIZE;
    host->data.err     = 0;	
    if(host->read)
        ret = host->read(host,buf);    
    
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    cmd.cmd_code = READ_MULTIPLE_BLOCK;
    cmd.arg = lba;
    if (!(host->cardflags & CARD_FLAG_SDHC)) 
    {
        cmd.arg <<= 9;
    }	
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;
    if((host->cmd) && send_cmd){
        ret = host->cmd(host,&cmd);
        if(ret){
			sd_tran_stop(host);
            ret = MMCSD_CMD_ERR;
            goto __err;
        }
    }
    

    if (host->complete)
        ret = host->complete(host);
    if (ret) {
        sd_tran_stop(host);
        ret = MMCSD_DAT_ERR;
        goto __err;
    }

__err:
    os_mutex_unlock(&host->lock);
    return ret;
}


uint32 send_app_op_cond(struct sdh_device *host,
                                uint32_t           ocr,
                                uint32_t          *rocr)
{
    struct rt_mmcsd_cmd cmd;
    uint32_t i;
    int ret;
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    cmd.cmd_code = SD_APP_OP_COND;
    cmd.arg = ocr;
    cmd.flags = RESP_SPI_R1 | RESP_R3 | CMD_BCR;

    for(i = 100;i;i--){
        ret = send_app_cmd(host,0);
        if(ret){
            os_printf("cmd err\r\n");
            break;
        }
        
//        memset(cmd->resp, 0, sizeof(cmd->resp));		
        ret = host->cmd(host,&cmd);
        if(ret){
            os_printf("cmd2 err\r\n");
            break;
        }
        os_printf("cmd resp:%x\r\n",cmd.resp[0]);
        if (cmd.resp[0] & CARD_BUSY){
            os_printf("card busy ok\r\n");
            break;
        }
        os_sleep_ms(10);		
    }

    if(rocr)
        *rocr = cmd.resp[0];

    if(!(cmd.resp[0] & CARD_BUSY))
        return RET_ERR;

    return ret;
}

void stop_card(){	
    sd_tran_stop(sdh_test);
}

uint8_t get_sd_status(void) {
	//os_printf("%s:%d\r\n",__FUNCTION__,sdh_test->sd_opt);
	return sdh_test->sd_opt;
}

uint8_t get_sd_status2(void) {
	//os_printf("%s:%d\r\n",__FUNCTION__,sdh_test->sd_opt);
	return sdh_test->sd_opt == SD_OFF;
}

int sd_scsi_read2(uint32 lba,uint8* buf) {
	return sd_multiple_read(sdh_test,lba,SECTOR_SIZE ,buf);
}

int sd_scsi_write2(uint32 lba,uint8* buf) {
	return sd_multiple_write(sdh_test,lba,SECTOR_SIZE ,buf);
}

int usb_sd_scsi_read(uint32 lba, uint32 count, uint8* buf) {
    return sd_multiple_read(sdh_test,lba,SECTOR_SIZE*count,buf);
}

int usb_sd_scsi_write(uint32 lba, uint32 count, uint8* buf) {
    return sd_multiple_write(sdh_test,lba,SECTOR_SIZE*count,buf);
}


//返回sd卡的扇区大小,现在是固定的
uint32_t get_sd_sector_size()
{
    return 512;
}

uint32 get_sd_cap() 
{
	return sd_dwCap * 2;//Blocks
}

#if TXW81X
uint32 sd_sample_point_cfg(struct sdh_device *host, uint32 clk)
{
    uint32_t        ret        = 0;
    uint32_t        last_ret   = MMCSD_INT_VLE;
    uint8_t         loop       = MMCSD_SMP_DELAY_CHAIN;
    uint8_t         index      = 0;
    uint8_t         buf_index  = 0;
    uint8_t         cmd_smp    = 0;
    uint8_t         dat_smp    = 0;
    uint8_t         smp[2][5]  = {0};
    uint8_t         dly_chain  = 0x7; 
    uint8_t         flag       = 0; 
    uint8_t         *data      = os_malloc(SECTOR_SIZE);

    if (host->card_type != CARD_TYPE_SD)
    {
        os_free(data);
        return RET_ERR;
    }
		
    sd_set_clk(host, clk);
    host->io_cfg.self_adaption_flag = MMCSD_SMP_EN;

__switch_type:
    sd_delay_config(host, loop, dly_chain);
    sd_set_sample(host, LL_SDHC_ALL_SMP_CFG_EN, 0, 0);

    for (int i = 0; i < 2; i++)
    {
        do
        {
            ret = sd_multiple_read(host, 0, SECTOR_SIZE, data);
            switch (ret)
            {
                case MMCSD_NO_ERR:
                    smp[i][buf_index++] = index;
                    break;
                case MMCSD_CMD_ERR:
                    if(i)   flag |= MMCSD_SMP_ERR;    
                    break;
                case MMCSD_DAT_ERR:
                    if(!i)  smp[i][buf_index++] = index;
                    break;
                default:
                    break;
            }

            if((last_ret == MMCSD_NO_ERR) && (ret == BIT(i)))                       
                flag |= MMCSD_BUF_ERR;       
            if((flag & (MMCSD_SMP_ERR | MMCSD_BUF_ERR)) || (buf_index > 5)) 
                break;
            
            last_ret = ret;
            (i) ? (sd_set_sample(host, LL_SDHC_ALL_SMP_CFG_EN, cmd_smp, ++index)):
                  (sd_set_sample(host, LL_SDHC_ALL_SMP_CFG_EN, ++index, dat_smp));
        } while (index < host->io_cfg.crc_sample_max);

        if(!buf_index || (flag & MMCSD_SMP_ERR))
        {
            os_printf("%s smp err!\r\n", (i) ? ("dat") : ("cmd"));
            break;
        }else{
            flag |= BIT(i);
        }

        if(!i)
            cmd_smp = (((buf_index - 1)>>1) == 0) ? (0) : (((uint32)*(smp[i] + (buf_index - 1))>>1));
        else
            dat_smp = (((buf_index - 1)>>1) == 0) ? (0) : (((uint32)*(smp[i] + (buf_index - 1))>>1));

        sd_set_sample(host, LL_SDHC_ALL_SMP_CFG_EN, cmd_smp, dat_smp);

        index = 0;buf_index = 0;
        flag &= ~(MMCSD_BUF_ERR);
    }

    if(flag != 0x3)
    {
        os_printf("loop:%d\r\n", loop);
        flag = 0;cmd_smp = 0; dat_smp = 0;dly_chain += 8;
        if(!((loop == MMCSD_SMP_DELAY_CHAIN)&&(dly_chain < 0x20)))  loop++;
        if(loop <= MMCSD_SMP_DELAY_ONE_CLOCK)                       goto __switch_type; 
        os_free(data);
		sd_set_sample(host, LL_SDHC_ALL_SMP_CFG_EN, 0, 0);
        sd_tran_stop(host);
        return RET_ERR;
    }
	os_printf("********cmd_index: %d \t dat_index:%d \t dly_chain :%d***********\r\n", cmd_smp, dat_smp, dly_chain);  
    os_free(data);
    sd_tran_stop(host);
    return RET_OK;
}
#endif

uint32 sd_init(struct sdh_device * host, uint32 clk)
{
    uint32   ret;
    uint32_t resp[4];
    uint32_t ocr;
    uint8_t  bw  = 1;
    os_printf("open_width:%d\r\n",bw);


    if(host->open)
        host->open(host,bw);
    
    sdhost_io_func_init(host->flags&MMCSD_BUSWIDTH_4);
    os_printf("host->flags:%x\r\n",host->flags);
    if(bw == 4)
        sd_power_up(host,MMCSD_BUSWIDTH_4);
    else
        sd_power_up(host,0);
    
    void __delay_asm(uint32 n);
    ret = send_idle(host);
    if(ret)
    {
        os_printf("idle cmd err\r\n");
        return RET_ERR;
    }
	__delay_asm(100);
    ret = send_if_cond(host,host->valid_ocr);
    if(ret)
    {
        os_printf("SEND_IF_COND cmd err\r\n");
        return RET_ERR;
    }

    ret = send_app_op_cond(host,0x40ff8000,&ocr);
    if(ret){
        os_printf("init card err\r\n");
        return RET_ERR;
    }
    os_printf("ocr:%x\r\n",ocr);
    ocr = select_voltage(host,ocr);
    os_printf("cur_ocr:%x\r\n",ocr);

    if (!ocr)
    {
        os_printf("cal ocr error\r\n");
        return RET_ERR;
    }	

    send_idle(host);
    __delay_asm(100);
    ret = send_if_cond(host,ocr);
    if(ret==0)
        ocr |= 1 << 30;

    ret = send_app_op_cond(host,ocr,NULL);
    if(ret){
        os_printf("init card app_op_cond err\r\n");
        return RET_ERR;
    }	

    send_all_get_cid(host,resp);
    host->card_type = CARD_TYPE_SD;
    memcpy(host->resp_cid,resp,sizeof(host->resp_cid));
    send_get_card_addr(host,&host->rca);
    send_get_csd(host,host->resp_csd);
    sd_parse_csd(host);
    send_select_card(host);
    send_get_scr(host,host->resp_scr);
    sd_parse_scr(host);
	
	/*switch bus width*/
	if ((host->flags & MMCSD_BUSWIDTH_4) &&
		(host->scr.sd_bus_widths & SD_SCR_BUS_WIDTH_4)) 
	{
		ret = sd_app_set_bus_width(host, MMCSD_BUS_WIDTH_4);
		if (ret){
			os_printf("set bus width 4 err\r\n");
			return RET_ERR;
		}
		sd_set_bus_width(host, MMCSD_BUS_WIDTH_4);	
	}

#if defined (TXW81X)
    if ((host->flags & MMCSD_SUP_HIGHSPEED) &&
        (!host->io_cfg.self_adaption_flag)  &&
        (host->scr.sd_version)              &&
        (clk > 25*1000*1000))
    {		
        ret = sd_switch(host);
        if (ret)
        {
            sd_set_clk(host, 24*1000*1000);
        }else{
            os_printf("********** test SD start  ********\r\n");
            ret = sd_sample_point_cfg(host, clk);
            os_printf("********** test SD finish ********\r\n");
            if (ret)
            {
                os_printf("set highspeed sampling point err\r\n");
                host->io_cfg.self_adaption_flag = MMCSD_SMP_DIS;
                sd_set_clk(host, 24*1000*1000);
                return RET_OK;
            }
            host->io_cfg.self_adaption_flag = MMCSD_SMP_SUCC;
        }	
    }else{
        sd_set_clk(host, clk);
    }
#else
    sd_set_clk(host, clk);
#endif
    ((struct hgsdh *)host)->opened = 1;
    return RET_OK;
}





extern bool fatfs_register();
extern void fatfs_unregister();
extern void fatfs_unregister_loop();
void hg_sdh_status(struct sdh_device *host){
    uint8_t opt = SD_IDLE;
    uint32_t lba = 0;
    int flag = 0;
    uint32 ret;
    uint8_t count = 0;
    while(1){
        os_sleep_ms(500);
        if(SD_OFF == host->sd_opt){
            os_printf("sdh no online\r\n");
            if(flag == 1)
            {
                flag = 0;
                os_mutex_del(&host->lock);
                fatfs_unregister();
            }

            //判断状态,是否重新挂在文件系统
            fatfs_register();
        }else{
            os_mutex_lock(&host->lock,osWaitForever);
            if(SD_IDLE != host->sd_opt){
                if((opt != host->sd_opt)||(lba != host->new_lba))
                {
                    opt = host->sd_opt;
                    lba = host->new_lba;
                    count = 0;
                }else{
                    count++;
                }
            }else{
                ret = send_card_status(host);
                if(ret != 0){
                    host->sd_opt = SD_OFF;
                    count = 0;
                    flag = 1;
                }
            }
            
            if(count >= 2){
                count = 0;			
                sd_tran_stop(host);
            }
            os_mutex_unlock(&host->lock);
        }

    }
}

void sd_open()
{
    sdh_test = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
#if SDH_I2C2_REUSE
    os_sema_init(&sem,1);
#endif
}

void sdhost_test()
{
    os_printf("enter sdhost test\r\n");
    sdh_test = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
    //sdhost_io_func_init();

    sd_init(sdh_test, 48*1000*1000);

}

uint32 sdhost_suspend(struct sdh_device * host){
    uint32 ret = -1;
    if(host->suspend){
        ret = host->suspend(host);
    }
    return ret;
}

uint32 sdhost_resume(struct sdh_device * host){
    uint32 ret = -1;
    if(host->resume){
        ret = host->resume(host);
    }
    return ret;

}

static struct os_work sdhost_wk = {
	.running=0
};



int32 sdh_loop(struct os_work *work)
{
    struct sdh_device *host = NULL;
    host = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
    static uint8_t opt = SD_IDLE;
    static uint32_t lba = 0;
    static uint8_t flag = 0;
    static uint8_t count = 0;
    uint32_t sleep_time = 500;
    uint32 ret;

    if(SD_OFF == host->sd_opt)
    {
        os_printf("sdh no online\r\n");
        if(flag == 1)
        {
            flag = 0;
            fatfs_unregister_loop();
        }

        //判断状态,是否重新挂在文件系统
        fatfs_register();
    }
    else
    {
            ret = os_mutex_lock(&host->lock,0);
            if(ret)
            {
                sleep_time = 1;
                //获取锁失败
                goto sdh_loop_end;
            }
            if(SD_IDLE != host->sd_opt)
            {
                if((opt != host->sd_opt)||(lba != host->new_lba))
                {
                    opt = host->sd_opt;
                    lba = host->new_lba;
                    count = 0;
                }
                else
                {
                    count++;
                }
            }
            else
            {
                ret = send_card_status(host);
                if(ret != 0)
                {
                    host->sd_opt = SD_OFF;
                    count = 0;
                    flag = 1;
                }
            }
            
            if(count >= 2){
                count = 0;			
                sd_tran_stop(host);
            }
            os_mutex_unlock(&host->lock);
    }
sdh_loop_end:
    os_run_work_delay(work, sleep_time);
	return 0;

}



uint32 sdhost_deinit_for_sleep()
{
    uint32 err = 0;
    struct sdh_device *sdh = NULL;
    sdh = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);
    os_work_cancle2(&sdhost_wk,1);
    return err;
}


uint32 sdhost_init(uint32 clk)
{
    uint32 err = 1;
    struct sdh_device *sdh = NULL;
    sdh = (struct sdh_device *)dev_get(HG_SDIOHOST_DEVID);

#if SDH_I2C2_REUSE
    os_sema_down(&sem,osWaitForever);
#endif

    if(sdh)
    {
        err = sd_init(sdh, clk);
        if(err)
            sdh->sd_opt = SD_OFF;

        if(sdhost_wk.init == 0)
        {
            OS_WORK_INIT(&sdhost_wk, sdh_loop, 0);
            os_run_work_delay(&sdhost_wk, 500);
        }
    }

#if SDH_I2C2_REUSE
    os_sema_up(&sem);
#endif

    return err;
}


#if SDH_I2C2_REUSE
/**
 * @brief 该函数用于SDH和I2C2端口复用的切换
 * 
 * @param sdh_stop_en 停止SDH、使用I2C2，则置1，否则置0
 * @return uint32 
 */
uint32 sdhost_i2c2_exchange(int sdh_stop_en)
{
    if(sdh_stop_en) {
        os_sema_down(&sem,osWaitForever);
        os_work_cancle2(&sdhost_wk,1);
        pin_func(HG_I2C2_DEVID,1);
    } else {
        os_sema_up(&sem);
        OS_WORK_INIT(&sdhost_wk, sdh_loop, 0);
        os_run_work_delay(&sdhost_wk, 500);
    }
}

#endif

uint32 get_sdhost_status(struct sdh_device *host)
{
    if(host->sd_opt == SD_OFF)
    {
        return 1;
    }
    return 0;
}


