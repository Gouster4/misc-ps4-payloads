/*
    slot-versions.c - PS4 payload to view the versions in update slots
    https://github.com/InvoxiPlayGames/misc-ps4-payloads

    Copyright (C) 2026 Emma / InvoxiPlayGames
    Licensed under the MIT License.
*/

#include <stdint.h>
#include <ps4.h>

// #define DEBUG_SOCKET
// #define DEBUG_IP "192.168.50.25"
// #define DEBUG_PORT 6767

#define SECTOR_SIZE 0x200

static uint64_t get_fw_version_from_sk_header(void *self, int length)
{
    SelfHeader *hdr = (SelfHeader *)self;
    // do some basic sanity checking
    if (hdr->magic != SELF_MAGIC || hdr->header_size > length || hdr->program_type != 0xF)
    {
        printf_debug("invalid header!\n");
        return 0;
    }
    // "hack": the SELF info segment is always at the end of the unencrypted header
    //  so we just lazily skip there and make an assumption...
    SceHeader *selfInfo = (SceHeader *)((uint8_t *)self + hdr->header_size - sizeof(SceHeader));
    // secure kernel PAID and program type
    if (selfInfo->program_authority_id != 0x3F00000000000001 || selfInfo->program_type != 0xF)
    {
        printf_debug("invalid SELF info!\n");
        return 0;
    }
    return selfInfo->app_version;
}

static inline int base16_to_base10(uint16_t in)
{
    return (((in >> 12) & 0xF) * 1000) + (((in >> 8) & 0xF) * 100) + (((in >> 4) & 0xF) * 10) + (((in >> 0) & 0xF) * 1);
}

static inline void parse_version(uint64_t ver, int *major, int *minor, int *build)
{
    *major = base16_to_base10((ver >> 40) & 0xFF);
    *minor = base16_to_base10((ver >> 28) & 0xFFF);
    *build = base16_to_base10((ver >> 12) & 0xFFF);
}

bool get_fw_versions()
{
    FILE *crypt = NULL;
    void *sectorbuf = NULL;
#define END_FUNC(err)          \
    {                          \
        printf_debug(err);     \
        if (sectorbuf != NULL) \
            free(sectorbuf);   \
        if (crypt != NULL)     \
            fclose(crypt);     \
        return 0;              \
    }
    // open the encrypted flash and make a buffer to read out sectors into
    crypt = fopen("/dev/sflash0s1.crypt", "r");
    if (crypt == NULL)
        END_FUNC("failed to open flash\n");
    sectorbuf = malloc(SECTOR_SIZE);
    if (sectorbuf == NULL)
        END_FUNC("failed to allocate sector buffer\n");

    // read the active version flag
    fseek(crypt, 0x1000, SEEK_SET);
    if (fread(sectorbuf, SECTOR_SIZE, 1, crypt) != 1)
        END_FUNC("failed to read active version sector");
    bool b_active = ((uint8_t *)sectorbuf)[0] != 0x00;

    // read the firmware version of slot A
    int a_maj = 0, a_min = 0, a_build = 0, has_a = 0;
    uint64_t a_ver = 0;
    // seek to what *should* be the secure kernel SELF file
    fseek(crypt, 0x1C0000 + SECTOR_SIZE, SEEK_SET); // sflash0s1.cryptx3, after SLB2 header (first entry)
    if (fread(sectorbuf, SECTOR_SIZE, 1, crypt) != 1)
        END_FUNC("failed to read slot A kernel sector");
    a_ver = get_fw_version_from_sk_header(sectorbuf, SECTOR_SIZE);
    has_a = a_ver != 0;
    if (has_a)
        parse_version(a_ver, &a_maj, &a_min, &a_build);

    // read the firmware version of slot B
    int b_maj = 0, b_min = 0, b_build = 0, has_b = 0;
    uint64_t b_ver = 0;
    // seek to what *should* be the secure kernel SELF file
    fseek(crypt, 0xE80000 + SECTOR_SIZE, SEEK_SET); // sflash0s1.cryptx3b, after SLB2 header (first entry)
    if (fread(sectorbuf, SECTOR_SIZE, 1, crypt) != 1)
        END_FUNC("failed to read slot B kernel sector");
    b_ver = get_fw_version_from_sk_header((void *)sectorbuf, SECTOR_SIZE);
    has_b = b_ver != 0;
    if (has_b)
        parse_version(b_ver, &b_maj, &b_min, &b_build);

    // clean up, we've got everything we need
    free(sectorbuf);
    fclose(crypt);

    // build message
    char msg_buf[256];
    snprintf(msg_buf, sizeof(msg_buf) - 1, "Slot A: %02d.%03d.%03d %s\nSlot B: %02d.%03d.%03d %s",
             a_maj, a_min, a_build, !has_a ? "(Missing?)" : !b_active ? "(Active)"
                                                                      : "",
             b_maj, b_min, b_build, !has_b ? "(Missing?)" : b_active ? "(Active)"
                                                                     : "");

    printf_notification(msg_buf);
#undef END_FUNC
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
    if (!get_fw_versions())
    {
        printf_notification("Failed to get FW slot versions.");
    }

#ifdef DEBUG_SOCKET
    printf_debug("closing socket...\n");
    SckClose(DEBUG_SOCK);
#endif

    return 0;
}
