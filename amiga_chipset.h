/*
 * Amiga chipset registers
 *
 * This header file is part of the code base for a simple Amiga ROM
 * replacement sufficient to allow programs using some parts of GadTools
 * to function.
 *
 * Copyright 2025 Chris Hooper. This program and source may be used
 * and distributed freely, for any purpose which benefits the Amiga
 * community. All redistributions must retain this Copyright notice.
 *
 * DISCLAIMER: THE SOFTWARE IS PROVIDED "AS-IS", WITHOUT ANY WARRANTY.
 * THE AUTHOR ASSUMES NO LIABILITY FOR ANY DAMAGE ARISING OUT OF THE USE
 * OR MISUSE OF THIS UTILITY OR INFORMATION REPORTED BY THIS UTILITY.
 */
#ifndef _AMIGA_CHIPSET_H
#define _AMIGA_CHIPSET_H

/* Amiga chipset registers */
#define GARY_BTIMEOUT  VADDR8(0x00de0000)  // Timeout type: 1=BERR 0=DSACK (b7)

/* Amiga CIA (8520) registers */
#define CIA_A_BASE    0x00bfe001
#define CIA_B_BASE    0x00bfd000

#endif /* _AMIGA_CHIPSET_H */
