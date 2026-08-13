/*
    hdcp-disable.c - PS4 payload to disable HDCP output without the registry
    https://github.com/InvoxiPlayGames/misc-ps4-payloads

    Copyright (C) 2026 Emma / InvoxiPlayGames
    Licensed under the MIT License.
*/

#include <stdint.h>
#include <ps4.h>

// #define DEBUG_SOCKET
// #define DEBUG_IP "192.168.50.25"
// #define DEBUG_PORT 6767

int _main(struct thread *td)
{
    UNUSED(td);

    // initialise the payload SDK
    initKernel();
    initLibc();
#ifdef DEBUG_SOCKET
    initNetwork();
    DEBUG_SOCK = SckConnect(DEBUG_IP, DEBUG_PORT);
#endif
    jailbreak();
    initSysUtil();

    // uses the IOCTL to disable HDCP
    // TODO: is it possible to get the shell to think that HDCP is disabled
    //   without writing to the registry?
    int fd = open("/dev/hdmi", O_RDWR, 0);
    uint8_t hdcp_mode = 0; // 0 = off, 1 = on
    int r = ioctl(fd, 0xC0048D14, &hdcp_mode);
    printf_debug("[hdcp-disable] %i\n", r);
    close(fd);

    if (r == 0)
    {
        printf_notification("HDCP disabled!");
    }

#ifdef DEBUG_SOCKET
    printf_debug("closing socket...\n");
    SckClose(DEBUG_SOCK);
#endif

    return 0;
}
