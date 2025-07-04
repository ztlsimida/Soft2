/*
 * File      : hid.c
 * COPYRIGHT (C) 2006 - 2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-10-02     Yi Qiu       first version
 */

#include <rtthread.h>
//#include <rtdevice.h>
#include <rtservice.h>
#include "include/usb_device.h"

#ifdef RT_USING_USB_DEVICE

#define USB_DEVICE_CONTROLLER_NAME      "usbd"

#ifdef RT_USB_DEVICE_COMPOSITE
const static char* ustring[] =
{
    "Language",
    "RT-Thread Team.",
    "RTT Composite Device",
    "320219198301",
    "Configuration",
    "Interface",
    USB_STRING_OS
};

static struct udevice_descriptor compsit_desc =
{
    USB_DESC_LENGTH_DEVICE,     //bLength;
    USB_DESC_TYPE_DEVICE,       //type;
    USB_BCD_VERSION,            //bcdUSB;
    USB_CLASS_MISC,             //bDeviceClass;
    0x02,                       //bDeviceSubClass;
    0x01,                       //bDeviceProtocol;
    0x40,                       //bMaxPacketSize0;
    _VENDOR_ID,                 //idVendor;
    _PRODUCT_ID,                //idProduct;
    USB_BCD_DEVICE,             //bcdDevice;
    USB_STRING_MANU_INDEX,      //iManufacturer;
    USB_STRING_PRODUCT_INDEX,   //iProduct;
    USB_STRING_SERIAL_INDEX,    //iSerialNumber;
    USB_DYNAMIC,                //bNumConfigurations;
};

//FS and HS needed
static struct usb_qualifier_descriptor dev_qualifier =
{
    sizeof(dev_qualifier),          //bLength
    USB_DESC_TYPE_DEVICEQUALIFIER,  //bDescriptorType
    0x0200,                         //bcdUSB
    USB_CLASS_MISC,                 //bDeviceClass
    0x02,                           //bDeviceSubClass
    0x01,                           //bDeviceProtocol
    64,                             //bMaxPacketSize0
    0x01,                           //bNumConfigurations
    0,
};
#endif

struct usb_os_comp_id_descriptor usb_comp_id_desc =
{
    //head section
    {
        USB_DYNAMIC,
        0x0100,
        0x04,
        USB_DYNAMIC,
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    },
};
static rt_list_t class_list;
int rt_usbd_class_list_init(void)
{
    rt_list_init(&class_list);
    return 0;
}
INIT_BOARD_EXPORT(rt_usbd_class_list_init);

int rt_usbd_class_list_deinit(void)
{
    if(!rt_list_isempty(&class_list)){
        rt_list_del(&class_list);
    }
    return 0;
}

rt_err_t rt_usbd_class_register(udclass_t udclass)
{
#ifndef RT_USB_DEVICE_COMPOSITE
    if(!rt_list_isempty(&class_list))
    {
        rt_kprintf("[D/USBD] If you want to use usb composite device please define RT_USB_DEVICE_COMPOSITE\n");
        return -RT_ERROR;
    }
#endif
    if(rt_usbd_class_driver_find(udclass) == RT_NULL)
    {
        os_printf("%s %d\n",__FUNCTION__,__LINE__);
        rt_list_insert_before(&class_list,&udclass->list);
    }
    return RT_EOK;
}

rt_err_t rt_usbd_class_driver_unregister(udclass_t udclass)
{
    if(udclass == RT_NULL)
    {
        return -RT_ERROR;
    }
    if(rt_usbd_class_driver_find(udclass) != RT_NULL)
    {
        os_printf("%s %d\n",__FUNCTION__,__LINE__);
        rt_list_remove(&udclass->list);
    }
	return RT_EOK;
}

udclass_t rt_usbd_class_driver_find(udclass_t udclass)
{
    struct rt_list_node *node;
    for (node = class_list.next; node != &class_list; node = node->next)
    {
        udclass_t udc = (udclass_t)rt_list_entry(node, struct udclass, list);
        if(udc == udclass)
        {
            return udc;
        }
    }

    return RT_NULL;
}

rt_err_t rt_usb_device_init(void)
{
    rt_device_t udc;
    udcd_t udc_t;
    udevice_t udevice;
    uconfig_t cfg;
    ufunction_t func = RT_NULL;
    rt_list_t *i;
    udclass_t udclass;

    if(rt_list_isempty(&class_list))
    {
        rt_kprintf("[D/USBD] No class register on usb device\n");
        return -RT_ERROR;
    }
    /* create and startup usb device thread */
    rt_usbd_core_init();

    /* create a device object */
    udevice = rt_usbd_device_new();

#if 0
    udc = rt_device_find(USB_DEVICE_CONTROLLER_NAME);
#else
    udc = dev_get(HG_USB_DEV_CONTROLLER_DEVID);
#endif
    if(udc == RT_NULL)
    {
        rt_kprintf("can't find usb device controller %s\n", USB_DEVICE_CONTROLLER_NAME);
        return -RT_ERROR;
    }

    udc_t = (udcd_t)udc;
    os_printf("%s %d\n",__FUNCTION__,__LINE__);
    udc_t->p_udevice = udevice;
    //os_printf("udevice:%x udc_t->p_udevice:%x\n",udevice,udc_t->p_udevice);

    /* set usb controller driver to the device */
    rt_usbd_device_set_controller(udevice, (udcd_t)udc);

    /* create a configuration object */
    cfg = rt_usbd_config_new();
    udc_t->cfg = cfg;

    rt_usbd_device_set_os_comp_id_desc(udevice, &usb_comp_id_desc);

    for(i = class_list.next; i!= &class_list; i = i->next)
    {
        /* get a class creater */
        udclass = rt_list_entry(i, struct udclass, list);
        /* create a function object */
        func = udclass->rt_usbd_function_create(udevice);
        /* add the function to the configuration */
        rt_usbd_config_add_function(cfg, func);
    }
    /* set device descriptor to the device */
#ifdef RT_USB_DEVICE_COMPOSITE
    rt_usbd_device_set_descriptor(udevice, &compsit_desc);
    rt_usbd_device_set_string(udevice, ustring);
    if(udevice->dcd->device_is_hs)
    {
        rt_usbd_device_set_qualifier(udevice, &dev_qualifier);
    }
#else
    rt_usbd_device_set_descriptor(udevice, func->dev_desc);
#endif

    /* add the configuration to the device */
    rt_usbd_device_add_config(udevice, cfg);

    /* initialize usb device controller */
    rt_device_init(udc);

    /* set default configuration to 1 */
    rt_usbd_set_config(udevice, 1);

    return RT_EOK;
}

rt_err_t rt_usb_device_deinit(void)
{
    udcd_t udc;
    rt_list_t *i;
    udclass_t udclass;


    udc = (udcd_t)dev_get(HG_USB_DEV_CONTROLLER_DEVID);
    if(udc == RT_NULL)
    {
        rt_kprintf("can't find usb device controller %s\n", USB_DEVICE_CONTROLLER_NAME);
        return -RT_ERROR;
    }

    rt_usbd_core_deinit();

    if(rt_list_isempty(&class_list))
    {
        rt_kprintf("[D/USBD] No class register on usb device\n");
    }
    else
    {
        for(i = class_list.next; i!= &class_list; i = i->next)
        {
            /* get a class creater */
            udclass = rt_list_entry(i, struct udclass, list);
            /* create a function object */
            udclass->rt_usbd_function_delete();
        }   
    }
    
    rt_usbd_ep_unassign(udc->p_udevice, RT_NULL);

    if(udc->cfg != RT_NULL)
    {
        rt_free(udc->cfg);
        udc->cfg = RT_NULL;
    }

    if(udc->p_udevice != RT_NULL)
    {
        rt_free(udc->p_udevice);
        udc->p_udevice = RT_NULL;
    }
	
	return RT_EOK;
}

#endif
