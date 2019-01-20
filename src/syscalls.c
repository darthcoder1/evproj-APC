#include  <errno.h>
#include  <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include "usbd_cdc_if.h"

int _write(int file, char *data, int len)
{
    if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
    {
        errno = EBADF;
        return -1;
    }

    uint8_t status = CDC_Transmit_FS(data, len);

    return (status == USBD_OK ? len : 0);
}