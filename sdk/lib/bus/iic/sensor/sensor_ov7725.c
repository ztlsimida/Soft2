#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/i2c.h"

#if DEV_SENSOR_OV7725


SENSOR_INIT_SECTION static const unsigned char OV7725InitTable[CMOS_INIT_LEN]= 
{	
	0x12,0x80,
	0x3d,0x03,
	0x15,0x02,
	0x17,0x22,//{0x17, 0x26}, hstart
	0x18,0xa0,//{0x18, 0xa0}, hsize
	0x19,0x07,//{0x19, 0x05}, vstart
	0x1a,0xf0,//{0x1a, 0xf2}, vsize
	0x32,0x00,
	0x29,0xa0,
//	0x2a,0x00,//0x10
//	0x2b,0x80,//hb 80
	0x2c,0xf0,
//	0x2d,0x0a,//vb 0a
	0x65,0x20,
	0x0c,0x10,
	0x11,0x01,
	0x42,0x7f,
	0x22,0x7f,
	0x23,0x03,
	0x63,0xe0,
	0x64,0xff,
	0x66,0x00,
	0x13,0xff,
	0x0d,0x41,
	0x0e,0x65,//0xd5
	0x0f,0xa5,
	0x14,0x10,//0x20
	0x22,0x84,//0x67
	0x23,0x03,
	0x24,0x70,//0x60
	0x25,0x30,//0x40
	0x26,0xa1,
	0x6b,0xaa,
	0x90,0x05,
	0x91,0x01,
	0x92,0x03,
	0x93,0x00,
	0x9c,0x20,
	0x9e,0x81,
	0x7e,0x0c,
	0x7f,0x16,
	0x80,0x2a,
	0x81,0x4e,
	0x82,0x61,
	0x83,0x6f,
	0x84,0x7b,
	0x85,0x86,
	0x86,0x8e,
	0x87,0x97,
	0x88,0xa4,
	0x89,0xaf,
	0x8a,0xc5,
	0x8b,0xd7,
	0x8c,0xe8,
	0x8d,0x20,
	0x94,0x6a,
	0x95,0x60,
	0x96,0x0e,
	0x97,0x0e,
	0x98,0x71,
	0x99,0x83,
	0x9a,0x1e,
	0x9b,0x02,
	0x9c,0x1d,
	0xa6,0x04,
	0x46,0x05,
	0x47,0x21,
	0x48,0x10,
	0x49,0x0d,
	0x4a,0x00,
	0x4b,0x0a,
	0x4c,0x11,
	0x10,0x7f,
	0xac,0xdf,
	0x8f,0x05,
	0x09,0x03,
	0x33,0x00,
	0x34,0x00,
   
-1,-1};


extern struct i2c_device *iic_test;

static void OV7725_rotate(uint32 r)
{
	unsigned char buf[2];
	buf[0] = 0x0c;
	buf[1] = (r<<6)|0x10;
	
	//i2c_sensor_write(iic_test,buf,1,1,0x42,0x43);
	i2c_write(iic_test,(int8 *)buf,1,(int8 *)&buf[1],1);

}


uint32 OV7725_hvblank(int8 dh,int8 dv)
{
	uint32 i;
#define HBLANK_BASE	0x80UL//0xb0UL
#define VBLANK_BASE	0x0AUL//0x20UL
	int32 h = (int32)HBLANK_BASE + dh; if(h < 0) h = 0;
	int32 v = (int32)VBLANK_BASE + dv; if(v < 0) v = 0;
	const uint8 t[][2] = {
		{0x2A,(h >> 8)<<4},//HB[11:8]
		{0x2B,h & 0xff},//HB[7:0]
		{0x2D,v & 0xff},//VB[11:8]
		{0x2E,v >> 8},//VB[7:0]
	};
	
	for(i=0;i<4;i++)
		i2c_write(iic_test,(int8 *)&t[i][0],1,(int8 *)&t[i][1],1);

	return (h<<16)|v;
}


SENSOR_OP_SECTION const _Sensor_Adpt_ ov7725_cmd= 
{	
	.typ = 1, //YUV
	.pixelw = 640,
	.pixelh= 480,
	.hsyn = 1,
	.vsyn = 1,
	.rduline = 0,//
	.rawwide = 1,//10bit
	.colrarray = 2,//0:_RGRG_ 1:_GRGR_,2:_BGBG_,3:_GBGB_
	.init = (uint8 *)OV7725InitTable,
	.rotate_adapt = {0},
	.hvb_adapt = {0x80,0x0a,0x80,0x0a},
	. mclk = 4000000,
	.p_fun_adapt = {OV7725_rotate,OV7725_hvblank,NULL},
	.p_xc7016_adapt = {NULL}, 	
};

const _Sensor_Ident_ ov7725_init=
{
	0x77,0x42,0x43,0x01,0x01,0x0a
};


#endif
