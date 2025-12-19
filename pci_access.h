#ifndef _PCI_ACCESS_H
#define _PCI_ACCESS_H

#if defined(AMIGA) || defined(__linux__)
#define HAVE_SPACE_PCI
#endif

rc_t pci_read(uint bus, uint dev, uint func, uint offset, uint width, void *bufp);
rc_t pci_write(uint bus, uint dev, uint func, uint offset, uint width, void *bufp);

#endif

