#include "usb_handler.h"
#include "usb_device.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_core.h"
#include "usbd_desc.h"

// usb device handle
extern USBD_HandleTypeDef hUsbDeviceFS;

USBD_StatusTypeDef usb_handler_initialize()
{
    /* Init Device Library, add supported class and start the library. */
    USBD_StatusTypeDef result = USBD_OK;

    result = USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS);
    if (result != USBD_OK)
        return result;

    result = USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC);
    if (result != USBD_OK)
        return result;

    result = (USBD_StatusTypeDef)USBD_CDC_RegisterInterface(
        &hUsbDeviceFS, &USBD_Interface_fops_FS);

    return result;
}

USBD_StatusTypeDef usb_handler_shutdown()
{
    USBD_StatusTypeDef result = USBD_OK;

    result = USBD_Stop(&hUsbDeviceFS);

    if (result != USBD_OK)
        return result;

    result = USBD_DeInit(&hUsbDeviceFS);

    return result;
}

USBD_StatusTypeDef usb_handler_start()
{
    return USBD_Start(&hUsbDeviceFS);
}

USBD_StatusTypeDef usb_handler_stop()
{
    return USBD_Stop(&hUsbDeviceFS);
}