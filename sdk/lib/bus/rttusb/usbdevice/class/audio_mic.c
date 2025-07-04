/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-09-07     flybreak     the first version
 */

//#include <rthw.h>
#include <rtthread.h>
#include "include/usb_device.h"

#ifdef RT_USB_DEVICE_AUDIO_MIC

#include "stream_frame.h"
#include "jpgdef.h"
#include "uaudioreg.h"

//#define DBG_TAG              "usbd.audio.mic"
//#define DBG_LVL              DBG_INFO
//#include <rtdbg.h>

#define RECORD_SAMPLERATE   16000
#define RECORD_CHANNEL      1
#define RESOLUTION_BITS     16

#define RESOLUTION_BYTE     (RESOLUTION_BITS / 8)
#define RECORD_PER_MS_SZ    ((RECORD_SAMPLERATE * RECORD_CHANNEL * RESOLUTION_BYTE) / 1000)
#define RECORD_BUFFER_SZ    1024//(RECORD_PER_MS_SZ * 20)  /* 20ms */

#if defined(RT_USBD_MIC_DEVICE_NAME)
    #define MIC_DEVICE_NAME    RT_USBD_MIC_DEVICE_NAME
#else
    #define MIC_DEVICE_NAME    "mic0"
#endif

#define UAC_USE_FEATURE_UNIT    1

#define EVENT_RECORD_START   (1 << 0)
#define EVENT_RECORD_STOP    (1 << 1)
#define EVENT_RECORD_DATA    (1 << 2)

#define MIC_INTF_STR_INDEX 8
/*
 * uac mic descriptor define
 */

#define UAC_CS_INTERFACE            0x24
#define UAC_CS_ENDPOINT             0x25

#define UAC_MAX_PACKET_SIZE         64
#define UAC_EP_MAX_PACKET_SIZE      RECORD_PER_MS_SZ
#define UAC_CHANNEL_NUM             RECORD_CHANNEL

struct uac_ac_descriptor
{
#ifdef RT_USB_DEVICE_COMPOSITE
    struct uiad_descriptor iad_desc;
#endif
    struct uinterface_descriptor intf_desc;
    struct usb_audio_control_descriptor hdr_desc;
    struct usb_audio_input_terminal it_desc;
    struct usb_audio_output_terminal ot_desc;
#if UAC_USE_FEATURE_UNIT
    struct usb_audio_feature_unit feature_unit_desc;
#endif
};

struct uac_as_descriptor
{
    struct uinterface_descriptor intf_desc;
    struct usb_audio_streaming_interface_descriptor hdr_desc;
    struct usb_audio_streaming_type1_descriptor format_type_desc;
    struct uendpoint_descriptor ep_desc;
    struct usb_audio_streaming_endpoint_descriptor as_ep_desc;
};

/*
 * uac mic device type
 */

struct uac_audio_mic
{
    rt_device_t  dev;
    rt_event_t   event;
    rt_uint8_t   open_count;

    rt_uint8_t  *buffer;
    rt_uint32_t  buffer_index;

    uep_t        ep;
};
struct uac_audio_mic mic;

rt_align(4)
static struct udevice_descriptor dev_desc =
{
    USB_DESC_LENGTH_DEVICE,     //bLength;
    USB_DESC_TYPE_DEVICE,       //type;
    USB_BCD_VERSION,            //bcdUSB;
    USB_CLASS_DEVICE,           //bDeviceClass;
    0x00,                       //bDeviceSubClass;
    0x00,                       //bDeviceProtocol;
    UAC_MAX_PACKET_SIZE,        //bMaxPacketSize0;
    _VENDOR_ID,                 //idVendor;
    _PRODUCT_ID,                //idProduct;
    USB_BCD_DEVICE,             //bcdDevice;
    USB_STRING_MANU_INDEX,      //iManufacturer;
    USB_STRING_PRODUCT_INDEX,   //iProduct;
    USB_STRING_SERIAL_INDEX,    //iSerialNumber;Unused.
    USB_DYNAMIC,                //bNumConfigurations;
};

//FS and HS needed
rt_align(4)
static struct usb_qualifier_descriptor dev_qualifier =
{
    sizeof(dev_qualifier),          //bLength
    USB_DESC_TYPE_DEVICEQUALIFIER,  //bDescriptorType
    0x0200,                         //bcdUSB
    USB_CLASS_AUDIO,                //bDeviceClass
    0x00,                           //bDeviceSubClass
    0x00,                           //bDeviceProtocol
    64,                             //bMaxPacketSize0
    0x01,                           //bNumConfigurations
    0,
};

rt_align(4)
const static char *_ustring[] =
{
    "Language",
    "RT-Thread Team.",
    "RT-Thread Audio Microphone",
    "32021919830108",
    "Configuration",
    "Interface",
};

rt_align(4)
static struct uac_ac_descriptor ac_desc =
{
#ifdef RT_USB_DEVICE_COMPOSITE
    /* Interface Association Descriptor */
    {
        USB_DESC_LENGTH_IAD,
        USB_DESC_TYPE_IAD,
        USB_DYNAMIC,
        0x02,
        USB_CLASS_AUDIO,
        USB_SUBCLASS_AUDIOSTREAMING,
        0x00,
        0x00,
    },
#endif
    /* Interface Descriptor */
    {
        USB_DESC_LENGTH_INTERFACE,
        USB_DESC_TYPE_INTERFACE,
        USB_DYNAMIC,
        0x00,
        0x00,
        USB_CLASS_AUDIO,
        USB_SUBCLASS_AUDIOCONTROL,
        0x00,
#ifdef RT_USB_DEVICE_COMPOSITE
        MIC_INTF_STR_INDEX,
#else
        0x00,
#endif
    },
    /* Header Descriptor */
    {
        sizeof(struct usb_audio_control_descriptor),
        UAC_CS_INTERFACE,
        UDESCSUB_AC_HEADER,
        0x0100,    /* Version: 1.00 */
        0x001E,    /* Total length: 30 */
        0x01,      /* Total number of interfaces: 1 */
        {0x01},    /* Interface number: 1 */
    },
    /*  Input Terminal Descriptor */
    {
        sizeof(struct usb_audio_input_terminal),
        UAC_CS_INTERFACE,
        UDESCSUB_AC_INPUT,
        0x01,      /* Terminal ID: 1 */
        0x0201,    /* Terminal Type: Microphone (0x0201) */
        0x00,      /* Assoc Terminal: 0 */
        0x01,      /* Number Channels: 1 */
        0x0000,    /* Channel Config: 0x0000 */
        0x00,      /* Channel Names: 0 */
        0x00,      /* Terminal: 0 */
    },
    /*  Output Terminal Descriptor */
    {
        sizeof(struct usb_audio_output_terminal),
        UAC_CS_INTERFACE,
        UDESCSUB_AC_OUTPUT,
        0x02,      /* Terminal ID: 2 */
        0x0101,    /* Terminal Type: USB Streaming (0x0101) */
        0x00,      /* Assoc Terminal: 0 */
        0x01,      /* Source ID: 1 */
        0x00,      /* Terminal: 0 */
    },
#if UAC_USE_FEATURE_UNIT
    /*  Feature unit Descriptor */
    {
        sizeof(struct usb_audio_feature_unit),
        UAC_CS_INTERFACE,
        UDESCSUB_AC_FEATURE,
        0x02,
        0x01,
        0x01,
        {0x43,0x00},
    },
#endif
};

rt_align(4)
static struct uinterface_descriptor as_desc0 =
{
    USB_DESC_LENGTH_INTERFACE,
    USB_DESC_TYPE_INTERFACE,
    USB_DYNAMIC,
    0x00,
    0x00,
    USB_CLASS_AUDIO,
    USB_SUBCLASS_AUDIOSTREAMING,
    0x00,
    0x00,
};

rt_align(4)
static struct uac_as_descriptor as_desc =
{
    /* Interface Descriptor */
    {
        USB_DESC_LENGTH_INTERFACE,
        USB_DESC_TYPE_INTERFACE,
        USB_DYNAMIC,
        0x01,
        0x01,
        USB_CLASS_AUDIO,
        USB_SUBCLASS_AUDIOSTREAMING,
        0x00,
        0x00,
    },
    /* General AS Descriptor */
    {
        sizeof(struct usb_audio_streaming_interface_descriptor),
        UAC_CS_INTERFACE,
        AS_GENERAL,
        0x02,      /* Terminal ID: 2 */
        0x01,      /* Interface delay in frames: 1 */
        UA_FMT_PCM,
    },
    /* Format type i Descriptor */
    {
        sizeof(struct usb_audio_streaming_type1_descriptor),
        UAC_CS_INTERFACE,
        FORMAT_TYPE,
        FORMAT_TYPE_I,
        UAC_CHANNEL_NUM,
        2,         /* Subframe Size: 2 */
        RESOLUTION_BITS,
        0x01,      /* Samples Frequence Type: 1 */
        {0},       /* Samples Frequence */
    },
    /* Endpoint Descriptor */
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DYNAMIC | USB_DIR_IN,
        0x05,
        UAC_EP_MAX_PACKET_SIZE,
        0x04,                       //HS:0x04 FS:0x01
    },
    /* AS Endpoint Descriptor */
    {
        sizeof(struct usb_audio_streaming_endpoint_descriptor),
        UAC_CS_ENDPOINT,
        AS_GENERAL,
    },
};

static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;

	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
            enable_stream(s,1);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		default:
			//默认都返回成功
		break;
	}
	return res;
}

void mic_entry(void *parameter)
{
    struct rt_audio_caps caps = {0};
    rt_uint32_t e, index;
    ufunction_t func = (ufunction_t)parameter;
    void *buf = RT_NULL;
    rt_uint32_t timeout = 0;

    mic.buffer = rt_malloc(RECORD_BUFFER_SZ);
    if (mic.buffer == RT_NULL)
    {
        LOG_E("malloc failed");
        goto __exit;
    }

    stream *audio_s = NULL;
    struct data_structure *audio_f = NULL;
    rt_uint32_t data_len = 0;
    audio_s = open_stream(R_USB_AUDIO_MIC,0,16,opcode_func,NULL);

    if(!audio_s)
    {
        _os_printf("creat audio_s stream fail\n");
        goto __exit;
    }

    if(audio_s)
    {
        enable_stream(audio_s,1);
    }

    while (1)
    {
        if (rt_event_recv(mic.event, EVENT_RECORD_START | EVENT_RECORD_STOP,
                          RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                          1000, &e) != RT_EOK)
        {
            continue;
        }

        if (e & EVENT_RECORD_START)
        {

        }else
        {
            continue;
        }

        os_printf("record start");

        caps.udata.config.samplerate = RECORD_SAMPLERATE;
        caps.udata.config.channels   = RECORD_CHANNEL;
        caps.udata.config.samplebits = RESOLUTION_BITS;

        //audio_da_init();

        os_sleep_ms(200);

        while (1)
        {
            audio_f = recv_real_data(audio_s);
            if(audio_f)
            {
                data_len = get_stream_real_data_len(audio_f);
                printf("U");
                buf = (rt_uint8_t *)GET_DATA_BUF(audio_f);
                memcpy(mic.buffer, buf, data_len);
                os_printf("len:%d mid.buffer:%x\n",data_len,mic.buffer);
                while(data_len)
                {
                    if (rt_event_recv(mic.event, EVENT_RECORD_DATA | EVENT_RECORD_STOP,
                                    RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                                    1000, &e) != RT_EOK)
                    {
                        //printf("R");
                        continue;
                    }
                    
                    if (e & EVENT_RECORD_DATA)
                    {
                        if(data_len >= UAC_EP_MAX_PACKET_SIZE)
                        {
                            mic.ep->request.buffer = mic.buffer + index;
                            mic.ep->request.size = UAC_EP_MAX_PACKET_SIZE;
                            mic.ep->request.req_type = UIO_REQUEST_WRITE;
                            rt_usbd_io_request(func->device, mic.ep, &mic.ep->request);
                            data_len -= UAC_EP_MAX_PACKET_SIZE;
                            index += UAC_EP_MAX_PACKET_SIZE;
                        }else{
                            mic.ep->request.buffer = mic.buffer + index;
                            mic.ep->request.size = data_len;
                            mic.ep->request.req_type = UIO_REQUEST_WRITE;
                            rt_usbd_io_request(func->device, mic.ep, &mic.ep->request);   
                            data_len = 0;
                            index = 0;
                            break;                         
                        }

                    }
                    else if (e & EVENT_RECORD_STOP)
                    {
                        index = 0;
                        timeout = 0; 
                        os_printf("EVENT RECORD STOP\n");
                        break;
                    }
                }

                if(audio_f)
                {
                    free_data(audio_f);
                    audio_f = RT_NULL;
                }
                index = 0;
                timeout = 0;

                if(e & EVENT_RECORD_STOP)
                {
                    break;
                }

            }
            else
            {
                timeout++;
                printf("n");
                //os_sleep_ms(1);
                if(timeout > 100)
                {
                    timeout = 0;
                    break;
                }                
            }       
        }
        LOG_D("record stop");
        //audio_adc_deinit();
    }

__exit:

    if(audio_s)
	{
		close_stream(audio_s);
	}

    if (mic.buffer)
        rt_free(mic.buffer);
}

static rt_err_t _record_start(ufunction_t func)
{
    rt_event_send(mic.event, EVENT_RECORD_START);
    memset(mic.buffer, 1, 64);
    mic.ep->request.buffer = mic.buffer;
    mic.ep->request.size = UAC_EP_MAX_PACKET_SIZE;
    mic.ep->request.req_type = UIO_REQUEST_WRITE;
    os_printf("record start mic.buffer:%x ep.addr:%x\n",mic.buffer,EP_ADDRESS(mic.ep));
    rt_usbd_io_request(func->device, mic.ep, &mic.ep->request);

    mic.open_count = 0;
    mic.open_count ++;
    return 0;
}

static rt_err_t _record_stop(ufunction_t func)
{
    //audio_adc_deinit();

    mic.open_count --;
    os_printf("%s mic.open_count:%d\n",__FUNCTION__,mic.open_count);
    rt_event_send(mic.event, EVENT_RECORD_STOP);
    return 0;
}

static rt_err_t _ep_data_in_handler(ufunction_t func, rt_size_t size)
{
    RT_ASSERT(func != RT_NULL);
    //os_printf("_ep_data_in_handler");

    rt_event_send(mic.event, EVENT_RECORD_DATA);

    return RT_EOK;
}

static rt_err_t _interface_as_handler(ufunction_t func, ureq_t setup)
{
    RT_ASSERT(func != RT_NULL);
    RT_ASSERT(func->device != RT_NULL);
    RT_ASSERT(setup != RT_NULL);

    os_printf("_interface_as_handler request_type:%x setup->bRequest:%x\n",(setup->request_type & USB_REQ_TYPE_MASK),setup->bRequest);

    if ((setup->request_type & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_STANDARD)
    {
        switch (setup->bRequest)
        {
        case USB_REQ_GET_INTERFACE:
            break;
        case USB_REQ_SET_INTERFACE:
            os_printf("set interface handler setup->wvalue:%d\n",setup->wValue);
            if (setup->wValue == 1)
            {
                _record_start(func);
            }
            else if (setup->wValue == 0)
            {
                _record_stop(func);
            }
            break;
        default:
            LOG_D("unknown uac request 0x%x", setup->bRequest);
            return -RT_ERROR;
        }
    }

    return RT_EOK;
}

static rt_err_t _function_enable(ufunction_t func)
{
    RT_ASSERT(func != RT_NULL);

    LOG_D("uac function enable");

    return RT_EOK;
}

static rt_err_t _function_disable(ufunction_t func)
{
    RT_ASSERT(func != RT_NULL);

    LOG_D("uac function disable");
    _record_stop(func);
    return RT_EOK;
}

static struct ufunction_ops ops =
{
    _function_enable,
    _function_disable,
    RT_NULL,
};
/**
 * This function will configure uac descriptor.
 *
 * @param comm the communication interface number.
 * @param data the data interface number.
 *
 * @return RT_EOK on successful.
 */
static rt_err_t _uac_descriptor_config(struct uac_ac_descriptor *ac,
                                       rt_uint8_t cintf_nr, struct uac_as_descriptor *as, rt_uint8_t sintf_nr)
{
    ac->hdr_desc.baInterfaceNr[0] = sintf_nr;
#ifdef RT_USB_DEVICE_COMPOSITE
    ac->iad_desc.bFirstInterface = cintf_nr;
#endif

    return RT_EOK;
}

static rt_err_t _uac_samplerate_config(struct uac_as_descriptor *as, rt_uint32_t samplerate)
{
    as->format_type_desc.tSamFreq[0 * 3 + 2] = samplerate >> 16 & 0xff;
    as->format_type_desc.tSamFreq[0 * 3 + 1] = samplerate >> 8 & 0xff;
    as->format_type_desc.tSamFreq[0 * 3 + 0] = samplerate & 0xff;
    return RT_EOK;
}

/**
 * This function will create a uac function instance.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK on successful.
 */
ufunction_t rt_usbd_function_uac_mic_create(udevice_t device)
{
    ufunction_t func;
    uintf_t intf_ac, intf_as;
    ualtsetting_t setting_as0;
    ualtsetting_t setting_ac, setting_as;
    struct uac_as_descriptor *as_desc_t;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);

#ifdef RT_USB_DEVICE_COMPOSITE
    rt_usbd_device_set_interface_string(device, MIC_INTF_STR_INDEX, _ustring[2]);
#else
    /* set usb device string description */
    rt_usbd_device_set_string(device, _ustring);
#endif
    /* create a uac function */
    func = rt_usbd_function_new(device, &dev_desc, &ops);
    //not support HS
    //rt_usbd_device_set_qualifier(device, &dev_qualifier);

    int audio_mic_init(ufunction_t func);
    audio_mic_init(func);

    /* create interface */
    intf_ac = rt_usbd_interface_new(device, RT_NULL);
    intf_as = rt_usbd_interface_new(device, _interface_as_handler);

    /* create alternate setting */
    setting_ac = rt_usbd_altsetting_new(sizeof(struct uac_ac_descriptor));
    setting_as0 = rt_usbd_altsetting_new(sizeof(struct uinterface_descriptor));
    setting_as = rt_usbd_altsetting_new(sizeof(struct uac_as_descriptor));
    /* config desc in alternate setting */
    rt_usbd_altsetting_config_descriptor(setting_ac, &ac_desc,
                                         (rt_off_t) & ((struct uac_ac_descriptor *)0)->intf_desc);
    rt_usbd_altsetting_config_descriptor(setting_as0, &as_desc0, 0);
    rt_usbd_altsetting_config_descriptor(setting_as, &as_desc,
                                         (rt_off_t) & ((struct uac_as_descriptor *)0)->intf_desc);
    /* configure the uac interface descriptor */
    _uac_descriptor_config(setting_ac->desc, intf_ac->intf_num, setting_as->desc, intf_as->intf_num);
    _uac_samplerate_config(setting_as->desc, RECORD_SAMPLERATE);

    /* create endpoint */
    as_desc_t = (struct uac_as_descriptor *)setting_as->desc;
    mic.ep = rt_usbd_endpoint_new(&as_desc_t->ep_desc, _ep_data_in_handler);

    /* add the endpoint to the alternate setting */
    rt_usbd_altsetting_add_endpoint(setting_as, mic.ep);

    /* add the alternate setting to the interface, then set default setting of the interface */
    rt_usbd_interface_add_altsetting(intf_ac, setting_ac);
    rt_usbd_set_altsetting(intf_ac, 0);
    rt_usbd_interface_add_altsetting(intf_as, setting_as0);
    rt_usbd_interface_add_altsetting(intf_as, setting_as);
    rt_usbd_set_altsetting(intf_as, 0);

    /* add the interface to the uac function */
    rt_usbd_function_add_interface(func, intf_ac);
    rt_usbd_function_add_interface(func, intf_as);

    return func;
}

int audio_mic_init(ufunction_t func)
{
    rt_thread_t mic_tid;
    mic.event = rt_event_create("mic_event", RT_IPC_FLAG_FIFO);

    mic_tid = rt_thread_create("mic_thread",
                               mic_entry, func,
                               1024,
                               OS_TASK_PRIORITY_NORMAL, 10);

    if (mic_tid != RT_NULL)
        rt_thread_startup(mic_tid);
    return RT_EOK;
}


/*
 *  register uac class
 */
struct udclass uac_class =
{
    .rt_usbd_function_create = rt_usbd_function_uac_mic_create
};

//需要初始化ADC的采样率，与UAC一致
int rt_usbd_uac_mic_class_register(void)
{
    rt_usbd_class_register(&uac_class);
    return 0;
}


#endif