
#ifndef USB_HANDLER_H_INCLUDED
#define USB_HANDLER_H_INCLUDED

#include "usb_device.h"

USBD_StatusTypeDef usb_handler_initialize();
USBD_StatusTypeDef usb_handler_shutdown();
USBD_StatusTypeDef usb_handler_start();
USBD_StatusTypeDef usb_handler_stop();

#endif // USB_HANDLER_H_INCLUDED