#include "usb_handler.h"
#include "usb_device.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_core.h"
#include "usbd_desc.h"

// usb device handle
USBD_HandleTypeDef g_USBDeviceHndl;

USBStatus usb_handler_initialize()
{
    /* Init Device Library, add supported class and start the library. */
    if (USBD_Init(&g_USBDeviceHndl, &FS_Desc, DEVICE_FS) != USBD_OK)
        return GenericFailure;

    if (USBD_RegisterClass(&g_USBDeviceHndl, &USBD_CDC) != USBD_OK)
        return GenericFailure;

    if (USBD_CDC_RegisterInterface(&g_USBDeviceHndl, &USBD_Interface_fops_FS) != USBD_OK)
        return GenericFailure;

    return Success;
}

USBStatus usb_handler_shutdown()
{
    if (USBD_Stop(&g_USBDeviceHndl) != USBD_OK)
        return GenericFailure;
    if (USBD_DeInit(&g_USBDeviceHndl) != USBD_OK)
        return GenericFailure;
}

USBStatus usb_handler_start()
{
    return USBD_Start(&g_USBDeviceHndl) == USBD_OK ? Success : GenericFailure;
}

USBStatus usb_handler_stop()
{
    return USBD_Stop(&g_USBDeviceHndl) == USBD_OK ? Success : GenericFailure;
}