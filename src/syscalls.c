#include "usbd_cdc_if.h"
#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO

int _write(int file, char* data, int len)
{
    if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
    {
        errno = EBADF;
        return -1;
    }

    uint8_t status = CDC_Transmit_FS((uint8_t*)data, len);

    return (status == USBD_OK ? len : 0);
}