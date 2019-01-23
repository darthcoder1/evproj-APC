
#ifndef USB_HANDLER_H_INCLUDED
#define USB_HANDLER_H_INCLUDED

typedef enum
{
    Success = 0,
    GenericFailure,
} USBStatus;

USBStatus usb_handler_initialize();
USBStatus usb_handler_shutdown();
USBStatus usb_handler_start();
USBStatus usb_handler_stop();

#endif // USB_HANDLER_H_INCLUDED