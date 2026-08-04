/*
    sflash-dumper.c - PS4 payload to dump the NOR flash
    https://github.com/InvoxiPlayGames/misc-ps4-payloads

    Copyright (C) 2026 Emma / InvoxiPlayGames
    Licensed under the MIT License.
*/

#include <stdint.h>
#include <ps4.h>

// #define DEBUG_SOCKET
// #define DEBUG_IP "192.168.50.25"
// #define DEBUG_PORT 6767

#define DUMP_NOR_PATH "/data/sflash0.bin"
#define DUMP_CRYPT_PATH "/data/sflash0s1.crypt.bin"

bool dump_serial_flash(const char *nor_path, const char *crypt_path)
{
    void *databuf = malloc(0x10000);
    FILE *readf = fopen("/dev/sflash0", "r");
    FILE *writef = fopen(nor_path, "w");
    if (readf == NULL)
    {
        printf_debug("Failed to open flash.\n");
        return 0;
    }
    if (writef == NULL)
    {
        printf_debug("Failed to open output '%s'\n", nor_path);
        return 0;
    }

    for (int i = 0; i < 0x200; i++)
    {
        // print the progress over debug
        if ((i % 0x10) == 0)
            printf_debug("0x%03x/0x200\n", i + 1);

        if (fread(databuf, 1, 0x10000, readf) != 0x10000)
        {
            printf_debug("Read failed at 0x%03x\n", i);
            return 0;
        }
        fwrite(databuf, 1, 0x10000, writef);
    }
    printf_notification("Flash dumped to\n%s", nor_path);
    fclose(readf);
    fclose(writef);

    // if we were given a path to dump the encrypted segment to, do that
    if (crypt_path != NULL)
    {
        readf = fopen("/dev/sflash0s1.crypt", "r");
        writef = fopen(crypt_path, "w");
        if (readf == NULL)
        {
            printf_debug("Failed to open encrypted flash.\n");
            return 0;
        }
        if (writef == NULL)
        {
            printf_debug("Failed to open output '%s'\n", crypt_path);
            return 0;
        }

        for (int i = 0; i < 0x1E0; i++)
        {
            // print the progress over debug
            if ((i % 0x10) == 0)
                printf_debug("0x%03x/0x1E0\n", i + 1);

            if (fread(databuf, 1, 0x10000, readf) != 0x10000)
            {
                printf_debug("Read failed at 0x%03x\n", i);
                return 0;
            }
            fwrite(databuf, 1, 0x10000, writef);
        }
        printf_notification("Decrypted flash dumped to\n%s", crypt_path);
        fclose(readf);
        fclose(writef);
    }

    return 1;
}

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

    // run our code
    if (!dump_serial_flash(DUMP_NOR_PATH, DUMP_CRYPT_PATH))
    {
        printf_notification("Failed to dump flash.");
    }

#ifdef DEBUG_SOCKET
    printf_debug("closing socket...\n");
    SckClose(DEBUG_SOCK);
#endif

    return 0;
}
