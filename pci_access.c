#include <stdio.h>
#include <string.h>
#ifndef _DCC
#include <stdint.h>
#endif

#ifdef __linux__
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#endif

#include "cmdline.h"
#include "pci_access.h"

#ifdef __linux__

#define PCI_SYSFS_DIR  "/sys/bus/pci/devices"

static struct {
    uint     pac_bus;      /**< PCI bus of file currently opened. */
    uint     pac_dev;      /**< PCI device of file currently opened. */
    uint     pac_func;     /**< PCI function of file currently opened. */
    int      pac_fd;       /**< File descriptor of opened file or -1. */
    bool_t   pac_is_write; /**< TRUE if file is open for write. */
} pci_af_cache = {
    .pac_bus      = 0xff,
    .pac_dev      = 0xff,
    .pac_func     = 0xff,
    .pac_fd       = -1,
    .pac_is_write = FALSE,
};

static int
pci_sysfs_open(uint p_bus, uint p_dev, uint p_func, bool_t is_write)
{
    if ((pci_af_cache.pac_fd       == -1) ||
        (pci_af_cache.pac_bus      != p_bus) ||
        (pci_af_cache.pac_dev      != p_dev) ||
        (pci_af_cache.pac_func     != p_func) ||
        (pci_af_cache.pac_is_write == FALSE && is_write == TRUE)) {
        char   path[PATH_MAX];
        int    pathsize = (int) sizeof (path);
        int    open_mode;

        if (pci_af_cache.pac_fd != -1)
            (void) close(pci_af_cache.pac_fd);

        if (is_write)
            open_mode = O_RDWR;
        else
            open_mode = O_RDONLY;
        if (snprintf(path, sizeof (path), "%s/%04x:%02x:%02x.%d/config",
                     PCI_SYSFS_DIR, 0, p_bus, p_dev, p_func) >= pathsize) {
            printf("Path \"%.*s\" too long", pathsize, path);
            return (RC_FAILURE);
        }

        if ((pci_af_cache.pac_fd = open(path, open_mode)) < 0)
            printf("Unable to open %s for %s", path,
                   is_write ? "write" : "read");

        pci_af_cache.pac_bus      = p_bus;
        pci_af_cache.pac_dev      = p_dev;
        pci_af_cache.pac_func     = p_func;
        pci_af_cache.pac_is_write = is_write;
    }
    return (pci_af_cache.pac_fd);
}

static rc_t
pci_sysfs_read(uint p_bus, uint p_dev, uint p_func, uint p_offset, uint mode,
               uint64_t *value)
{
    int     fd;
    uint    tmode  = mode;
    uint8_t *valuep = (uint8_t *) value;
    ssize_t count;

    *value = 0;

    while (mode > 0) {
        if (tmode > 4)
            tmode = 4;
        if ((fd = pci_sysfs_open(p_bus, p_dev, p_func, FALSE)) < 0)
            return (RC_FAILURE);

        count = pread(fd, valuep, tmode, p_offset);
        if (count != (ssize_t) tmode) {
            if (count < 0)
                printf("Unable to read from PCI address %x.%x.%x.%x (sysfs)",
                       p_bus, p_dev, p_func, p_offset);
            else
                printf("Unable to read from PCI address %x.%x.%x.%x (sysfs)",
                       p_bus, p_dev, p_func, p_offset);
            return (RC_FAILURE);
        }
        mode     -= tmode;
        p_offset += tmode;
        valuep   += tmode;
    }
    return (RC_SUCCESS);
}

static rc_t
pci_sysfs_write(uint p_bus, uint p_dev, uint p_func, uint p_offset, uint mode,
                const uint64_t *value)
{
    int         fd;
    uint        tmode  = mode;
    const uint8_t *valuep = (uint8_t *)value;
    ssize_t     count;

    while (mode > 0) {
        if (tmode > 4)
            tmode = 4;
        if ((fd = pci_sysfs_open(p_bus, p_dev, p_func, TRUE)) < 0)
            return (RC_FAILURE);

        count = pwrite(fd, valuep, tmode, p_offset);
        if (count != (ssize_t) tmode) {
            if (count < 0)
                printf("Unable to write to PCI address %x.%x.%x.%x (sysfs)",
                      p_bus, p_dev, p_func, p_offset);
            else
                printf("Unable to write to PCI address %x.%x.%x.%x (sysfs)",
                       p_bus, p_dev, p_func, p_offset);
            return (RC_FAILURE);
        }
        mode     -= tmode;
        p_offset += tmode;
        valuep   += tmode;
    }
    return (RC_SUCCESS);
}
#endif

#ifdef AMIGA
#include <exec/types.h>
#include <libraries/expansionbase.h>
#include <clib/exec_protos.h>
#include <clib/expansion_protos.h>
#ifdef _DCC
#include "amiga_stdint.h"
typedef unsigned char * CONST_STRPTR;
#else
#include <inline/expansion.h>
#endif
#include "cpu_control.h"


#define ZORRO_MFG_MATAY         0xad47
#define ZORRO_PROD_MATAY_BD     0x0001
#define ZORRO_MFG_E3B           0x0e3b
#define ZORRO_PROD_FIRESTORM    0x00c8

#define FS_PCI_ADDR_CONFIG0     0x1fc00000  // Normal config space
#define FS_PCI_ADDR_CONFIG1     0x1fd00000  // Config1 space
#define FS_PCI_ADDR_IO          0x1fe00000
#define FS_PCI_ADDR_CONTROL     0x1fc08000  // Control (PCI reset) register

#define FS_PCI_CONTROL_NO_RESET 0x80000000  // 0=Bridge in reset
#define FS_PCI_CONTROL_EN_INTS  0x40000000  // 1=Interrupts enabled

#define MAYTAY_PCI_ADDR_CONFIG  0x000f0000

#define BRIDGE_TYPE_UNKNOWN   0
#define BRIDGE_TYPE_MAYTAY    1
#define BRIDGE_TYPE_FIRESTORM 2
#define BRIDGE_TYPE_AMIGAPCI  3

static uint8_t *bridge_pci0_base;
static uint8_t *bridge_pci1_base;
static APTR     bridge_io_base;
static APTR     bridge_mem_base;
static APTR     bridge_control_reg;
static uint8_t  bridge_type = BRIDGE_TYPE_UNKNOWN;
static uint16_t bridge_zorro_mfg;
static uint16_t bridge_zorro_prod;
static struct ConfigDev *zorro_cdev = NULL;

static const char * expansion_library = "expansion.library";
struct Library *ExpansionBase;

static APTR
find_zorro_pci_bridge(uint bus)
{
    struct ConfigDev      *cdev = NULL;
    uint8_t               *base = NULL;
    uint                   curbus = 0;

    ExpansionBase = OpenLibrary((CONST_STRPTR) expansion_library, 0);
    if (ExpansionBase == NULL) {
        printf("Could not open %s\n", expansion_library);
        return (NULL);
    }
    bridge_zorro_mfg  = ZORRO_MFG_MATAY;
    bridge_zorro_prod = ZORRO_PROD_MATAY_BD;
    for (curbus = 0; curbus < 8; curbus++) {
        cdev = FindConfigDev(cdev, bridge_zorro_mfg, bridge_zorro_prod);
        if (cdev == NULL)
            break;
        if (curbus == bus) {
            base = cdev->cd_BoardAddr;
            bridge_pci0_base = base + MAYTAY_PCI_ADDR_CONFIG;
            bridge_pci1_base = base + MAYTAY_PCI_ADDR_CONFIG;
            bridge_io_base = base;
            bridge_mem_base = base;
            bridge_control_reg = NULL;
            bridge_type = BRIDGE_TYPE_MAYTAY;
            goto done;
        }
    }
    bridge_zorro_mfg  = ZORRO_MFG_E3B;
    bridge_zorro_prod = ZORRO_PROD_FIRESTORM;
    for (; curbus < 8; curbus++) {
        cdev = FindConfigDev(cdev, bridge_zorro_mfg, bridge_zorro_prod);
        if (cdev == NULL)
            break;
        if (curbus == bus) {
            base = cdev->cd_BoardAddr;
            bridge_pci0_base = base + FS_PCI_ADDR_CONFIG0;
            bridge_pci1_base = base + FS_PCI_ADDR_CONFIG1;
            bridge_io_base = base + FS_PCI_ADDR_IO;
            bridge_mem_base = base;
            bridge_control_reg = base + FS_PCI_ADDR_CONTROL;
            if (base == ADDR8(0x80000000))
                bridge_type = BRIDGE_TYPE_AMIGAPCI;
            else
                bridge_type = BRIDGE_TYPE_FIRESTORM;
            break;
        }
    }

done:
    zorro_cdev = cdev;
    CloseLibrary(ExpansionBase);

    return (base);
}

static uint
check_pci_init(void)
{
    static uint8_t did_pci_init = 0;

    if (did_pci_init == 0) {
        if (find_zorro_pci_bridge(0) == NULL)
            return (1);
        did_pci_init = 1;
    }
    return (0);
}

static APTR
pci_cfg_base(uint bus, uint dev, uint func, uint off)
{
    if (bus == 0) {
        if (bridge_pci0_base == NULL)
            return (NULL);
        if (dev <= 3)
            return (bridge_pci0_base + (0x10000 << dev) + (func << 8) + off);

        if ((dev == 4) && (bridge_type == BRIDGE_TYPE_AMIGAPCI)) {
            /* Only AmigaPCI has 4 slots */
            return (bridge_pci0_base + 0x30000 + (func << 8) + off);
        } else {
            return (bridge_pci0_base);  // Fail with no slot selected
        }
    }
    if (bridge_pci1_base == NULL)
        return (NULL);
    return (bridge_pci1_base + (bus << 16) + (dev << 11) + (func << 8) + off);
}

static rc_t
pci_amiga_read(uint bus, uint dev, uint func, uint offset, uint mode,
               void *value)
{
    if (check_pci_init())
        return (RC_NO_DATA);

    switch (mode) {
        case 1: {
            uint8_t tvalue = *ADDR8(pci_cfg_base(bus, dev, func, offset));
            *ADDR8(value) = tvalue;
            break;
        }
        case 2: {
            uint16_t tvalue = *ADDR16(pci_cfg_base(bus, dev, func, offset));
            *ADDR16(value) = swap16(tvalue);
            break;
        }
        case 4: {
            uint32_t tvalue = *ADDR32(pci_cfg_base(bus, dev, func, offset));
            *ADDR32(value) = swap32(tvalue);
            break;
        }
    }
    return (RC_SUCCESS);
}

static rc_t
pci_amiga_write(uint bus, uint dev, uint func, uint offset, uint mode,
                const uint64_t *value)
{
    if (check_pci_init())
        return (RC_NO_DATA);

    switch (mode) {
        case 1:
            *ADDR8(pci_cfg_base(bus, dev, func, offset)) = *value;
            break;
        case 2: {
            uint16_t tvalue = *value;
            *ADDR16(pci_cfg_base(bus, dev, func, offset)) = swap16(tvalue);
            break;
        }
        case 4: {
            uint32_t tvalue = *value;
            *ADDR32(pci_cfg_base(bus, dev, func, offset)) = swap32(tvalue);
            break;
        }
    }
    return (RC_SUCCESS);
}
#endif

rc_t
pci_read(uint bus, uint dev, uint func, uint offset, uint width, void *bufp)
{
    rc_t rc = RC_SUCCESS;
    while (width > 0) {
        uint len = width;
        if (len > 4)
            len = 4;
#ifdef __linux__
        rc = (pci_sysfs_read(bus, dev, func, offset, len, bufp));
#endif
#ifdef AMIGA
        rc = (pci_amiga_read(bus, dev, func, offset, len, bufp));
#endif
        if (rc != RC_SUCCESS)
            break;
        width  -= len;
        offset += len;
        bufp = (void *)((uintptr_t)bufp + len);
    }
    return (rc);
}

rc_t
pci_write(uint bus, uint dev, uint func, uint offset, uint width, void *bufp)
{
    rc_t rc = RC_SUCCESS;
    while (width > 0) {
        uint len = width;
        if (len > 4)
            len = 4;
#ifdef __linux__
        rc = pci_sysfs_write(bus, dev, func, offset, len, bufp);
#endif
#ifdef AMIGA
        rc = pci_amiga_write(bus, dev, func, offset, len, bufp);
#endif
        if (rc != RC_SUCCESS)
            break;
        width  -= len;
        offset += len;
        bufp = (void *)((uintptr_t)bufp + len);
    }
    return (rc);
}

