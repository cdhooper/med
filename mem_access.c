/*
 * This is free and unencumbered software released into the public domain.
 * See the LICENSE file for additional details.
 *
 * Designed by Chris Hooper in August 2020.
 *
 * ---------------------------------------------------------------------
 *
 * Generic physical memory access code.
 */

#ifdef EMBEDDED_CMD
#include "printf.h"
#endif
#include "cmdline.h"
#include "mem_access.h"
#include "util.h"

#if defined(AMIGAOS)
#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#endif

#if defined(AMIGA)
#include "cpu_control.h"
#endif

#if defined(AMIGAOS)
#define AMIGA_BERR_DSACK 0x00de0000  // Bit7=1 for BERR on timeout, else DSACK

#define MEM_FAULT_CAPTURE                                   \
    struct Task *thistask = (struct Task *) FindTask(NULL); \
    uint32_t baseaddr = addr;                               \
    if (baseaddr != AMIGA_BERR_DSACK)                       \
        *ADDR8(AMIGA_BERR_DSACK) = 0x7f;                    \
    old_TrapCode = thistask->tc_TrapCode;                   \
    thistask->tc_TrapCode = (void *)(uintptr_t) trap_handler
#define MEM_FAULT_RESTORE                                   \
    thistask->tc_TrapCode = old_TrapCode;                   \
    if (baseaddr != AMIGA_BERR_DSACK)                       \
        *ADDR8(AMIGA_BERR_DSACK) = 0xff;
void trap_handler(void);
APTR old_TrapCode;
#else
#define MEM_FAULT_CAPTURE   mem_fault_ok = TRUE
#define MEM_FAULT_RESTORE   mem_fault_ok = FALSE
#endif

#if defined(AMIGAOS) && !defined(_DCC)
/*
 * Capture of bus errors which occur in supervisor state.  Exec does not
 * call a task's tc_TrapCode for those (it goes directly to a dead-end
 * Alert), so the CPU's bus error vector is pointed at the trap handler
 * instead.  These must be called in supervisor state with interrupts
 * disabled.
 */
void trap_vector_capture(void);
void trap_vector_restore(void);
#define BERR_VECTOR_CAPTURE() trap_vector_capture()
#define BERR_VECTOR_RESTORE() trap_vector_restore()
#else
#define BERR_VECTOR_CAPTURE()
#define BERR_VECTOR_RESTORE()
#endif

#if defined(AMIGA)
const bool_t  no_unaligned_access = TRUE; // 68000 can't handle unaligned access
#else
const bool_t  no_unaligned_access = FALSE;
#endif

uint8_t       mem_fault_ok    = FALSE;
volatile uint mem_fault_count = 0;

rc_t
mem_read_core(uint64_t addr, uint width, void *bufp)
{
    uint8_t *buf = (uint8_t *) bufp;

    mem_fault_count = 0;

    while (width > 0) {
        uint mode = width;

        /* Handle unaligned source address */
        if (addr & 1)
            mode = 1;
        else if ((mode > 2) && (addr & 2))
            mode = 2;
        else if ((mode > 4) && (addr & 4))
            mode = 4;

        switch (mode) {
            case 0:
                return (RC_FAILURE);
            case 1:
rmode_1:
                mode = 1;
                *buf = *(uint8_t *)(uintptr_t)addr;
                break;
            case 2:
            case 3:
rmode_2:
                if (no_unaligned_access && (addr & 1))
                    goto rmode_1;
                mode = 2;
                *(uint16_t *)buf = *(uint16_t *)(uintptr_t)addr;
                break;
            default:
            case 4:
            case 5:
            case 6:
            case 7:
rmode_4:
                if (no_unaligned_access && (addr & 3))
                    goto rmode_2;
                mode = 4;
                *(uint32_t *)buf = *(uint32_t *)(uintptr_t)addr;
                break;
#ifndef AMIGA
            case 8:
                if (no_unaligned_access && (addr & 7))
                    goto rmode_4;
                mode = 8;
                *(uint64_t *)buf = *(uint64_t *)(uintptr_t)addr;
                break;
#else /* AMIGA */
            case 8:
rmode_8:
                if (no_unaligned_access && (addr & 7))
                    goto rmode_4;
                mode = 8;
                mem_copy8(buf, (void *)(uintptr_t)addr);
                break;
            case 16:
rmode_16:
                if (no_unaligned_access && (addr & 0xf))
                    goto rmode_8;
                mode = 16;
                mem_copy16(buf, (void *)(uintptr_t)addr);
                break;
            case 32:
                if (no_unaligned_access && (addr & 0x1f))
                    goto rmode_16;
                mem_copy32(buf, (void *)(uintptr_t)addr);
                break;
#endif
        }
        buf   += mode;
        addr  += mode;
        width -= mode;
    }

    if (mem_fault_count != 0)
        return (RC_FAILURE);

    return (RC_SUCCESS);
}

static rc_t
mem_write_core(uint64_t addr, uint width, void *bufp)
{
    uint8_t *buf = (uint8_t *) bufp;

    mem_fault_count = 0;

    while (width > 0) {
        uint mode = width;

        /* Handle unaligned source address */
        if (addr & 1)
            mode = 1;
        else if ((mode > 2) && (addr & 2))
            mode = 2;
        else if ((mode > 4) && (addr & 4))
            mode = 4;

        switch (mode) {
            case 0:
                return (RC_FAILURE);
            case 1:
wmode_1:
                *(uint8_t *)(uintptr_t)addr = *buf;
                break;
            case 2:
            case 3:
wmode_2:
                if (no_unaligned_access && (addr & 1))
                    goto wmode_1;
                mode = 2;
                *(uint16_t *)(uintptr_t)addr = *(uint16_t *)buf;
                break;
            default:
            case 4:
            case 5:
            case 6:
            case 7:
wmode_4:
                if (no_unaligned_access && (addr & 3))
                    goto wmode_2;
                mode = 4;
                *(uint32_t *)(uintptr_t)addr = *(uint32_t *)buf;
                break;
#ifndef AMIGA
            case 8:
                if (no_unaligned_access && (addr & 7))
                    goto wmode_4;
                mode = 8;
                *(uint64_t *)(uintptr_t)addr = *(uint64_t *)buf;
                break;
#else /* AMIGA */
            case 8:
wmode_8:
                if (no_unaligned_access && (addr & 7))
                    goto wmode_4;
                mode = 8;
                mem_copy8((void *)(uintptr_t)addr, buf);
                break;
            case 16:
wmode_16:
                if (no_unaligned_access && (addr & 0xf))
                    goto wmode_8;
                mode = 16;
                mem_copy16((void *)(uintptr_t)addr, buf);
                break;
            case 32:
                if (no_unaligned_access && (addr & 0x1f))
                    goto wmode_16;
                mem_copy32((void *)(uintptr_t)addr, buf);
                break;
#endif
        }
        buf   += mode;
        addr  += mode;
        width -= mode;
    }

    if (mem_fault_count != 0)
        return (RC_FAILURE);

    return (RC_SUCCESS);
}

rc_t
mem_read(uint64_t addr, uint width, void *bufp)
{
    rc_t rc;
    MEM_FAULT_CAPTURE;
    rc = mem_read_core(addr, width, bufp);
    MEM_FAULT_RESTORE;
    return (rc);
}

rc_t
mem_write(uint64_t addr, uint width, void *bufp)
{
    rc_t rc;
    MEM_FAULT_CAPTURE;
    rc = mem_write_core(addr, width, bufp);
    MEM_FAULT_RESTORE;
    return (rc);
}

#ifdef HAVE_SPACE_PHYS
rc_t
phys_mem_read(uint64_t addr, uint width, void *bufp)
{
    rc_t rc;
#ifdef AMIGA
    SUPERVISOR_STATE_ENTER();
    INTERRUPTS_DISABLE();
    CACHE_DISABLE_DATA();
    MMU_DISABLE();
    BERR_VECTOR_CAPTURE();
#endif
    rc = mem_read_core(addr, width, bufp);
#ifdef AMIGA
    BERR_VECTOR_RESTORE();
    CACHE_FLUSH();
    MMU_RESTORE();
    CACHE_RESTORE_STATE();
    INTERRUPTS_ENABLE();
    SUPERVISOR_STATE_EXIT();
#endif
    return (rc);
}

rc_t
phys_mem_write(uint64_t addr, uint width, void *bufp)
{
    rc_t rc;
#ifdef AMIGA
    SUPERVISOR_STATE_ENTER();
    INTERRUPTS_DISABLE();
    CACHE_DISABLE_DATA();
    MMU_DISABLE();
    BERR_VECTOR_CAPTURE();
#endif
    rc = mem_write_core(addr, width, bufp);
#ifdef AMIGA
    BERR_VECTOR_RESTORE();
    CACHE_FLUSH();
    MMU_RESTORE();
    CACHE_RESTORE_STATE();
    INTERRUPTS_ENABLE();
    SUPERVISOR_STATE_EXIT();
#endif
    return (rc);
}
#endif

#if defined (AMIGAOS) && !defined(_DCC)
/*
 * ---------------------------------------------------------------------
 * Trap handling
 *
 * The task trap handler below catches bus errors (and address errors,
 * illegal instructions, and divide by zero) which occur while memory is
 * being accessed, counts them in mem_fault_count, and resumes execution
 * following the instruction which faulted.
 *
 * How the faulting instruction is skipped depends on the CPU, because
 * each one stacks a different exception frame and reports a different PC:
 *
 *   68000    Group 0 frame (no format word). The stacked PC is 2-10 bytes
 *            beyond the start of the faulting instruction, so the start
 *            is located by searching backward for the stacked instruction
 *            register value, and the length is then decoded. This is a
 *            best effort; the 68000 does not provide enough information
 *            to be certain in every case.
 *   68010    Format $8. The instruction is suspended mid-execution. The
 *            RR bit is set in the special status word, telling the CPU
 *            that the bus cycle has been completed by software, and the
 *            CPU finishes the instruction itself on RTE.
 *   68020/30 Format $A or $B. Same idea as the 68010: the DF bit is
 *            cleared in the special status word and the CPU finishes the
 *            instruction on RTE without rerunning the data cycle.
 *   68040    Format $7. A faulted read leaves the PC at the instruction,
 *            so the instruction length is decoded and added. A faulted
 *            write is usually reported after the instruction has
 *            completed, so the PC is already beyond it. Writes of other
 *            instructions which were still pending in the pipeline are
 *            not done by the CPU; they must be completed here.
 *   68060    Format $4. The PC is at the faulting instruction, so the
 *            instruction length is decoded and added (unless the fault is
 *            an imprecise store buffer / push buffer bus error, where the
 *            instruction has already completed).
 *
 * In all cases a faulted read leaves the destination with undefined
 * content; callers must check mem_fault_count.
 *
 * The handler is entered in one of two ways:
 *   - From exec, as the task's tc_TrapCode. Exec only does this for
 *     exceptions which occur in user state (mem_read / mem_write).
 *   - Directly from the CPU's bus error vector, which is how faults in
 *     supervisor state are caught (phys_mem_read / phys_mem_write). See
 *     trap_vector_capture().
 */

/* Offsets into the saved register array provided by trap_handler */
#define TRAP_REG_A0  8              // D0-D7 are 0-7, A0-A6 are 8-14
#define TRAP_REG_A7  15             // Not saved

#define TRAP_NOFIX   (-1)           // trap_fixup() could not handle the trap

/*
 * 68040 writes which were still pending in the CPU pipeline at the time of
 * an access fault.  These are filled in by trap_fixup() and performed by
 * trap_wb040_run after return from the exception.
 *     [0]   = PC at which to resume
 *     [1-3] = Write-back 2 address, data, size (0 = none)
 *     [4-6] = Write-back 3 address, data, size (0 = none)
 */
uint32_t trap_wb040[7];
void     trap_wb040_run(void);
int32_t  trap_fixup(uint32_t trapnum, uint16_t *frame, const uint32_t *regs);

static inline uint32_t
trap_get32(const uint16_t *ptr)
{
    return (((uint32_t) ptr[0] << 16) | ptr[1]);
}

static inline void
trap_put32(uint16_t *ptr, uint32_t value)
{
    ptr[0] = (uint16_t) (value >> 16);
    ptr[1] = (uint16_t) value;
}

/*
 * trap_ea_len() returns the number of bytes of extension words which
 *               follow an instruction for the specified effective address.
 *
 * mode   is the EA mode (0-7)
 * reg    is the EA register (0-7)
 * immlen is the number of bytes of extension that immediate data takes
 * ext    is a pointer to the first extension word
 */
static uint
trap_ea_len(uint mode, uint reg, uint immlen, const uint16_t *ext)
{
    uint len;

    if (mode == 7)
        mode += reg;

    switch (mode) {
        case 5:          // (d16,An)
        case 7 + 0:      // (xxx).W
        case 7 + 2:      // (d16,PC)
            return (2);
        case 7 + 1:      // (xxx).L
            return (4);
        case 7 + 4:      // #imm
            return (immlen);
        case 6:          // (d8,An,Xn) or 68020+ full format
        case 7 + 3:      // (d8,PC,Xn) or 68020+ full format
            if ((*ext & 0x0100) == 0)
                return (2);              // Brief extension word
            len = 2;
            if (*ext & 0x0020)           // Base displacement (word or long)
                len += (*ext & 0x0010) ? 4 : 2;
            if (*ext & 0x0002)           // Outer displacement (word or long)
                len += (*ext & 0x0001) ? 4 : 2;
            return (len);
        default:         // Dn, An, (An), (An)+, -(An)
            return (0);
    }
}

/*
 * trap_insn_decode() returns the length in bytes of the 68k instruction at
 *                    the specified address. If the instruction has an
 *                    effective address operand, then the mode, register,
 *                    and a pointer to the extension words of that operand
 *                    (the destination, in the case of MOVE) are also
 *                    returned. Otherwise, the returned mode is 0.
 *
 * This function understands the 68000-68060 integer instructions
 * (including the 68020+ full format indexed addressing modes), the
 * 6888x/68040/68060 FPU instructions, the 68030/68851 and 68040/68060 MMU
 * instructions, and MOVE16. An unrecognized opcode is assumed to be a
 * single word. It may be called on any CPU, and only reads the instruction
 * stream.
 */
static uint
trap_insn_decode(const uint16_t *pc, uint *eamode, uint *eareg,
                 const uint16_t **eaext)
{
    uint op      = pc[0];
    uint mode    = (op >> 3) & 7;
    uint reg     = op & 7;
    uint size    = (op >> 6) & 3;
    uint immlen  = (size == 2) ? 4 : 2;  // Immediate length for size field
    uint len     = 2;                    // Length up to the EA extension
    uint ext;

    *eamode = 0;

    switch (op >> 12) {
        case 0x0:
            if (op & 0x0100) {
                if (mode == 1)
                    return (4);                 // MOVEP
                break;                          // BTST/BCHG/BCLR/BSET Dn,<ea>
            }
            if ((op & 0x0e00) == 0x0800) {
                len = 4;                        // BTST/BCHG/BCLR/BSET #n,<ea>
                break;
            }
            if (size == 3) {
                if ((op & 0x0e00) == 0x0600) {
                    if (mode < 2)
                        return (2);             // RTM
                    len = 4;                    // CALLM
                    break;
                }
                if ((op & 0x083f) == 0x083c)
                    return (6);                 // CAS2
                len = 4;                        // CHK2/CMP2/CAS
                break;
            }
            if ((op & 0x0e00) == 0x0e00) {
                len = 4;                        // MOVES
                break;
            }
            len = 2 + immlen;                   // ORI/ANDI/SUBI/ADDI/EORI/CMPI
            if ((mode == 7) && (reg == 4))
                return (len);                   // ORI/ANDI/EORI to CCR/SR
            break;
        case 0x1:
        case 0x2:
        case 0x3:                               // MOVE/MOVEA
            immlen = ((op >> 12) == 2) ? 4 : 2;
            len += trap_ea_len(mode, reg, immlen, pc + 1);
            mode = (op >> 6) & 7;
            reg  = (op >> 9) & 7;
            break;
        case 0x4:
            if (op & 0x0100) {
                immlen = (size == 0) ? 4 : 2;   // CHK.L / CHK.W / LEA / EXTB
                break;
            }
            switch ((op >> 9) & 7) {
                case 4:
                    if (size == 0) {
                        if (mode == 1)
                            return (6);         // LINK.L
                        break;                  // NBCD
                    }
                    if (mode == 0)
                        return (2);             // SWAP, EXT
                    if (size == 1)
                        break;                  // PEA (or BKPT)
                    len = 4;                    // MOVEM regs,<ea>
                    break;
                case 5:
                    if (op == 0x4afc)
                        return (2);             // ILLEGAL
                    break;                      // TST/TAS
                case 6:
                    len = 4;                    // MULx.L/DIVx.L/MOVEM <ea>,regs
                    immlen = 4;
                    break;
                case 7:
                    if (size >= 2)
                        break;                  // JSR/JMP
                    if ((op & 0x00f8) == 0x0050)
                        return (4);             // LINK.W
                    switch (op & 0x00ff) {
                        case 0x72:              // STOP
                        case 0x74:              // RTD
                        case 0x7a:              // MOVEC
                        case 0x7b:              // MOVEC
                            return (4);
                    }
                    return (2);                 // TRAP/UNLK/NOP/RTS/RTE/...
                default:
                    break;      // NEGX/CLR/NEG/NOT/MOVE to/from SR,CCR
            }
            break;
        case 0x5:
            if (size != 3)
                break;                          // ADDQ/SUBQ
            if (mode == 1)
                return (4);                     // DBcc
            if ((mode == 7) && (reg >= 2)) {    // TRAPcc
                if (reg == 2)
                    return (4);
                if (reg == 3)
                    return (6);
                return (2);
            }
            break;                              // Scc
        case 0x6:                               // Bcc/BRA/BSR
            if ((op & 0xff) == 0x00)
                return (4);
            if ((op & 0xff) == 0xff)
                return (6);
            return (2);
        case 0x8:
        case 0xc:
            if (size == 3) {
                immlen = 2;                     // DIVx.W/MULx.W
                break;
            }
            if ((op & 0x0100) && (mode < 2)) {
                if (((op >> 12) == 0x8) && (size != 0))
                    return (4);                 // PACK/UNPK
                return (2);                     // SBCD/ABCD/EXG
            }
            break;                              // OR/AND
        case 0x9:
        case 0xb:
        case 0xd:
            if (size == 3) {
                immlen = (op & 0x0100) ? 4 : 2; // SUBA/CMPA/ADDA
                break;
            }
            if ((op & 0x0100) && (mode < 2))
                return (2);                     // SUBX/CMPM/ADDX
            break;                              // SUB/CMP/EOR/ADD
        case 0xe:
            if (size != 3)
                return (2);                     // Shift/rotate register
            if (op & 0x0800)
                len = 4;                        // Bit field
            break;                              // Shift/rotate memory
        case 0xf:
            ext = pc[1];
            switch ((op >> 6) & 0x3f) {
                case 0x00:                      // 68030/68851 MMU general
                    len = 4;
                    break;
                case 0x01:                      // PScc/PDBcc/PTRAPcc
                case 0x09:                      // FScc/FDBcc/FTRAPcc
                    len = 4;
                    if (mode == 1)
                        return (6);             // xDBcc
                    if ((mode == 7) && (reg >= 2)) {
                        if (reg == 2)
                            return (6);         // xTRAPcc.W
                        if (reg == 3)
                            return (8);         // xTRAPcc.L
                        return (4);             // xTRAPcc
                    }
                    break;                      // xScc
                case 0x02:                      // PBcc.W
                case 0x0a:                      // FBcc.W (FNOP)
                    return (4);
                case 0x03:                      // PBcc.L
                case 0x0b:                      // FBcc.L
                    return (6);
                case 0x04:                      // PSAVE
                case 0x05:                      // PRESTORE
                case 0x0c:                      // FSAVE
                case 0x0d:                      // FRESTORE
                    break;
                case 0x08:                      // FPU general
                    len = 4;
                    switch (ext >> 13) {
                        case 0:                 // FPm to FPn
                            return (4);
                        case 2:                 // <ea> to FPn (or FMOVECR)
                            switch ((ext >> 10) & 7) {
                                case 0:         // Long
                                case 1:         // Single
                                    immlen = 4;
                                    break;
                                case 2:         // Extended
                                case 3:         // Packed
                                    immlen = 12;
                                    break;
                                case 5:         // Double
                                    immlen = 8;
                                    break;
                                case 7:
                                    return (4); // FMOVECR
                                default:        // Word, Byte
                                    immlen = 2;
                                    break;
                            }
                            break;
                        case 4:                 // FMOVE(M) <ea>,control regs
                            immlen = 0;
                            if (ext & 0x1000)
                                immlen += 4;
                            if (ext & 0x0800)
                                immlen += 4;
                            if (ext & 0x0400)
                                immlen += 4;
                            break;
                        default:                // FMOVE/FMOVEM to <ea>, etc
                            break;
                    }
                    break;
                case 0x18:                      // MOVE16
                    if (mode == 4)
                        return (4);             // MOVE16 (Ax)+,(Ay)+
                    return (6);                 // MOVE16 absolute
                case 0x20:
                    if ((op == 0xf800) && (ext == 0x01c0))
                        return (6);             // LPSTOP
                    return (2);
                default:                        // CINV/CPUSH/PFLUSH/PTEST/PLPA
                    return (2);
            }
            break;
        default:                                // MOVEQ, Line-A
            return (2);
    }

    *eamode = mode;
    *eareg  = reg;
    *eaext  = pc + (len >> 1);
    return (len + trap_ea_len(mode, reg, immlen, pc + (len >> 1)));
}

/*
 * trap_insn_len() returns the length in bytes of the instruction at the
 *                 specified address.
 */
static uint
trap_insn_len(uint32_t pc)
{
    uint            eamode;
    uint            eareg;
    const uint16_t *eaext;

    return (trap_insn_decode((const uint16_t *)(uintptr_t) pc,
                             &eamode, &eareg, &eaext));
}

/*
 * trap_insn_refs() returns non-zero if the instruction at the specified PC
 *                  has a memory operand which is at the specified address.
 *                  Only the destination of MOVE is checked. Operands which
 *                  can not be simply computed (68020+ full format indexed,
 *                  or anything involving A7) return zero.
 *
 * pc   is the address of the instruction
 * regs is the D0-D7/A0-A6 register content before the instruction
 * addr is the address of the access
 * size is the size of the access in bytes
 */
static uint
trap_insn_refs(uint32_t pc, const uint32_t *regs, uint32_t addr, uint size)
{
    const uint16_t *insn = (const uint16_t *)(uintptr_t) pc;
    const uint16_t *ext;
    uint            mode;
    uint            reg;
    uint            xreg;
    uint32_t        ea;
    uint32_t        index;

    if ((insn[0] & 0xffc0) == 0xf600) {
        /* MOVE16: either operand may be in the faulted cache line */
        reg = TRAP_REG_A0 + (insn[0] & 7);
        if ((reg != TRAP_REG_A7) && (((regs[reg] ^ addr) & ~0xfU) == 0))
            return (1);
        if (insn[0] & 0x0020) {
            reg = TRAP_REG_A0 + ((insn[1] >> 12) & 7);  // (Ax)+,(Ay)+
            if (reg == TRAP_REG_A7)
                return (0);
            ea = regs[reg];
        } else {
            ea = trap_get32(insn + 1);                  // Absolute
        }
        return (((ea ^ addr) & ~0xfU) == 0);
    }

    (void) trap_insn_decode(insn, &mode, &reg, &ext);
    if ((mode < 2) || ((mode < 7) && (reg == 7)))
        return (0);  // Not memory or A7 relative
    if (mode < 7)
        ea = regs[TRAP_REG_A0 + reg];
    else
        ea = (uint32_t)(uintptr_t) ext;  // PC relative base

    if (mode == 7)
        mode += reg;

    switch (mode) {
        case 2:          // (An)
        case 3:          // (An)+
            break;
        case 4:          // -(An)
            ea -= size;
            break;
        case 5:          // (d16,An)
        case 7 + 2:      // (d16,PC)
            ea += (int16_t) ext[0];
            break;
        case 7 + 0:      // (xxx).W
            ea = (uint32_t)(int32_t)(int16_t) ext[0];
            break;
        case 7 + 1:      // (xxx).L
            ea = trap_get32(ext);
            break;
        case 6:          // (d8,An,Xn)
        case 7 + 3:      // (d8,PC,Xn)
            xreg = ext[0] >> 12;
            if ((ext[0] & 0x0100) || (xreg == TRAP_REG_A7))
                return (0);
            index = regs[xreg];
            if ((ext[0] & 0x0800) == 0)
                index = (uint32_t)(int32_t)(int16_t) index;
            ea += (index << ((ext[0] >> 9) & 3)) + (int8_t) ext[0];
            break;
        default:         // #imm
            return (0);
    }
    return ((uint32_t) (addr - ea) < size);
}

/*
 * trap_frame_replace() replaces a CPU exception frame with a four word
 *                      (format $0) frame at the end of where the original
 *                      frame was, so that RTE resumes at the specified PC.
 *                      Returns the number of bytes between the start of
 *                      the old frame and the start of the new frame.
 */
static int32_t
trap_frame_replace(uint16_t *frame, uint framesize, uint32_t pc)
{
    uint      skip = framesize - 8;
    uint16_t *nframe = frame + (skip >> 1);
    uint16_t  sr = frame[0];
    uint16_t  vector = frame[3] & 0x0fff;

    nframe[0] = sr;
    trap_put32(nframe + 1, pc);
    nframe[3] = vector;
    return ((int32_t) skip);
}

/*
 * trap_fixup_68000() handles a 68000 bus error or address error frame:
 *     frame[0]   = Status: R/W (bit 4), I/N (bit 3), function code
 *     frame[1-2] = Access address
 *     frame[3]   = Instruction register
 *     frame[4]   = Status register
 *     frame[5-6] = Program counter
 *
 * The 68000 keeps one word of prefetch beyond the instruction register.
 * At the time of a fault, the stacked PC is the address of the instruction
 * plus 2, plus 2 for every word that the instruction has since prefetched
 * (address errors may add 2 more).  The stacked instruction register is
 * the opcode of the faulting instruction, unless that instruction had
 * already done its final prefetch, in which case it is the opcode of the
 * following instruction.
 */
static int32_t
trap_fixup_68000(uint16_t *frame)
{
    uint            ir   = frame[3];
    uint32_t        pc   = trap_get32(frame + 5);
    const uint16_t *code = (const uint16_t *)(uintptr_t) pc;
    const uint16_t *insn;
    uint            back;
    uint            len;

    if ((pc & 1) || ((frame[0] & 3) == 2))
        return (TRAP_NOFIX);    // Fault is on an instruction fetch

    /*
     * Check for a faulted write which occurred after the final prefetch.
     * This is only the case for MOVE to -(An). In that case, the PC is
     * 2 beyond the start of the next instruction, and the stacked
     * instruction register is the opcode of the next instruction. If the
     * next instruction could itself have caused this fault (a MOVE to
     * (An) or (An)+ from a source with no extension words, which writes
     * before it prefetches), then there is no way to know which it was,
     * so the next instruction is assumed.
     */
    if (((frame[0] & 0x0010) == 0) && (code[-1] == ir) &&
        (((ir >> 12) < 1) || ((ir >> 12) > 3) ||
         (((ir >> 6) & 6) != 2) || (((ir >> 3) & 7) > 4))) {
        for (len = 2; len <= 6; len += 2) {
            insn = code - 1 - (len >> 1);
            if (((*insn >> 12) >= 1) && ((*insn >> 12) <= 3) &&
                (((*insn >> 6) & 7) == 4) &&
                (trap_insn_len((uint32_t)(uintptr_t) insn) == len)) {
                pc -= 2;
                goto resume;
            }
        }
    }

    /*
     * Otherwise, the faulting instruction starts with the stacked
     * instruction register value, and is 2 to (its length + 2) bytes
     * before the stacked PC.
     */
    for (back = 2; back <= 10; back += 2) {
        insn = code - (back >> 1);
        if (*insn == ir) {
            len = trap_insn_len((uint32_t)(uintptr_t) insn);
            if (back <= len + 2) {
                pc = pc - back + len;
                goto resume;
            }
        }
    }
    return (TRAP_NOFIX);

resume:
    trap_put32(frame + 5, pc);
    mem_fault_count++;
    return (8);         // Discard status, access address, and IR
}

/*
 * trap_size040() returns the size in bytes of a 68040 access, given the
 *                special status word or a write-back status.
 */
static uint
trap_size040(uint status)
{
    switch (status & 0x0060) {
        case 0x0000:
            return (4);     // Long
        case 0x0020:
            return (1);     // Byte
        case 0x0040:
            return (2);     // Word
        default:
            return (16);    // Line (MOVE16)
    }
}

/*
 * trap_wb040_pending() checks one of the 68040 pending write-backs. If it
 *                      is valid and is not the write which faulted, it is
 *                      saved for trap_wb040_run to complete.
 *
 * wbs  is the write-back status (V, SIZE, TT, TM)
 * wb   is a pointer to the write-back address and data in the frame
 * fa   is the fault address
 * save is where to save the address, data, and size
 */
static uint
trap_wb040_pending(uint wbs, const uint16_t *wb, uint32_t fa, uint32_t *save)
{
    uint32_t addr = trap_get32(wb);
    uint     size = trap_size040(wbs);

    save[2] = 0;
    if (((wbs & 0x0080) == 0) || (wbs & 0x0018) || (size > 4))
        return (0);  // Not valid, or not a normal access (MOVE16, etc)

    if ((uint32_t) (fa - addr) < size)
        return (0);  // This is the write which faulted

    save[0] = addr;
    save[1] = trap_get32(wb + 2);
    save[2] = size;
    return (1);
}

/*
 * trap_fixup_68040() handles a 68040 access error frame:
 *     frame[0]     = Status register
 *     frame[1-2]   = Program counter
 *     frame[3]     = Format $7 and vector offset
 *     frame[4-5]   = Effective address
 *     frame[6]     = Special status word
 *     frame[7-9]   = Write-back 3, 2, and 1 status
 *     frame[10-11] = Fault address
 *     frame[12-15] = Write-back 3 address and data
 *     frame[16-19] = Write-back 2 address and data
 *     frame[20-29] = Write-back 1 address and data, push data
 */
static int32_t
trap_fixup_68040(uint16_t *frame, const uint32_t *regs)
{
    uint     ssw = frame[6];
    uint32_t pc  = trap_get32(frame + 1);
    uint32_t fa  = trap_get32(frame + 10);
    uint     pending;

    /*
     * A fault on an instruction fetch (TM is user code or supervisor code)
     * can not be skipped. The fault address is also checked, because
     * there are emulators which report code access for data faults.
     */
    if (((ssw & 0x001b) == 0x0002) && ((uint32_t) (fa - (pc & ~0xfU)) < 64))
        return (TRAP_NOFIX);

    /*
     * A faulted read (RW=1) or a fault during MOVEM (CM=1) is reported
     * with the PC of the instruction, which the CPU would restart.
     * A faulted write is normally reported after the instruction has
     * completed, with the PC of a following instruction. It is possible
     * for a write fault to be reported with the PC of the instruction
     * (emulators do this), so check whether the instruction at the PC is
     * one that would access the fault address.
     */
    if ((ssw & 0x1100) ||
        trap_insn_refs(pc, regs, fa, trap_size040(ssw)))
        pc += trap_insn_len(pc);

    /*
     * Writes of other instructions may still be pending in write-back
     * 2 and 3. The CPU discards them when the exception is taken, so
     * they must be done by software. (The write which got the bus error
     * is in write-back 1, and is not done.) Since those writes might
     * also fault, they are not done here in the exception handler.
     * Instead, if any are pending, RTE to trap_wb040_run, which will do
     * them and then resume.
     */
    pending  = trap_wb040_pending(frame[8], frame + 16, fa, trap_wb040 + 1);
    pending += trap_wb040_pending(frame[7], frame + 12, fa, trap_wb040 + 4);
    if (pending != 0) {
        trap_wb040[0] = pc;
        pc = (uint32_t)(uintptr_t) trap_wb040_run;
    }

    mem_fault_count++;
    return (trap_frame_replace(frame, 60, pc));
}

/*
 * trap_fixup() is called by trap_handler to modify the CPU exception frame
 *              so that return from the exception will resume following the
 *              instruction which caused the exception.
 *
 * trapnum is the exception vector number
 * frame   is a pointer to the CPU exception frame
 * regs    is a pointer to the saved D0-D7/A0-A6 registers
 *
 * This function returns the number of bytes of the CPU exception frame
 * which trap_handler must discard before it executes RTE, or TRAP_NOFIX
 * if the trap was not handled.
 *
 * It is called in supervisor mode with both stack and register arguments,
 * so it may be compiled for either calling convention.
 */
__attribute__((used)) int32_t
trap_fixup(uint32_t trapnum, uint16_t *frame, const uint32_t *regs)
{
    uint     ssw;
    uint     format;
    uint32_t pc;

    switch (trapnum) {
        case 2:   // Bus error
        case 3:   // Address error
            break;
        case 4:   // Illegal instruction (PC is at the instruction)
            pc = trap_get32(frame + 1);
            trap_put32(frame + 1, pc + trap_insn_len(pc));
            mem_fault_count++;
            return (0);
        case 5:   // Divide by zero (PC is at the next instruction)
            mem_fault_count++;
            return (0);
        default:
            return (TRAP_NOFIX);
    }

    if ((SysBase->AttnFlags & AFF_68010) == 0)
        return (trap_fixup_68000(frame));

    /*
     * 68010 and higher
     *     frame[0]   = Status register
     *     frame[1-2] = Program counter
     *     frame[3]   = Frame format and vector offset
     */
    format = frame[3] >> 12;
    switch (format) {
        case 0x8:  // 68010 bus error or address error
            ssw = frame[4];
            if (ssw & 0x2000)
                break;  // Fault is on an instruction fetch
            frame[4] = (uint16_t) (ssw | 0x8000);  // RR: software did cycle
            mem_fault_count++;
            return (0);
        case 0xa:  // 68020/68030 short bus cycle fault
        case 0xb:  // 68020/68030 long bus cycle fault
            ssw = frame[5];
            if (ssw & 0x0100) {
                frame[5] = (uint16_t) (ssw & ~0x0100U);  // DF: do not rerun
                mem_fault_count++;
                return (0);
            }
            pc = trap_get32(frame + 1);
            if ((trapnum == 2) && ((ssw & 0xc000) == 0) &&
                ((uint32_t) (trap_get32(frame + 8) - (pc & ~0xfU)) >= 64)) {
                /*
                 * Not a data fault and not an instruction fetch fault.
                 * No real CPU does this, but emulators which do not
                 * fully implement bus errors do. Assume the PC is at
                 * the faulting instruction (unless the fault address
                 * says that it was the instruction which was bad).
                 */
                mem_fault_count++;
                return (trap_frame_replace(frame, (format == 0xa) ? 32 : 92,
                                           pc + trap_insn_len(pc)));
            }
            break;
        case 0x7:  // 68040 access error
            if (trapnum == 2)
                return (trap_fixup_68040(frame, regs));
            break;
        case 0x4:  // 68060 access error
            /*
             * frame[4-5] = Fault address
             * frame[6-7] = Fault status long word
             */
            if ((trapnum != 2) || (frame[7] & 0x8000))
                break;  // Fault is on an instruction fetch
            if ((frame[7] & 0x6000) == 0) {
                /* Not an imprecise store buffer or push buffer error */
                pc = trap_get32(frame + 1);
                trap_put32(frame + 1, pc + trap_insn_len(pc));
            }
            mem_fault_count++;
            return (0);
        default:   // Includes 68040/68060 address error (odd PC)
            break;
    }
    return (TRAP_NOFIX);
}

/*
 * Bus error vector capture
 *
 * trap_old_vector is the bus error vector which was replaced. If the trap
 * handler can not handle a fault, it continues at that vector (by way of
 * old_TrapCode pointing to trap_berr_chain).
 */
uint32_t                  trap_old_vector;
static volatile uint32_t *trap_vector_ptr;
static APTR               trap_vector_old_trapcode;
void                      trap_berr_vector(void);
void                      trap_berr_chain(void);
uint32_t                  trap_get_vbr(void);

/*
 * trap_vector_capture() points the CPU's bus error vector at the trap
 *                       handler. This is required to catch a bus error
 *                       which occurs in supervisor state, as exec will not
 *                       pass those to a task's trap handler. It must be
 *                       called in supervisor state with interrupts
 *                       disabled, and trap_vector_restore() must be called
 *                       before interrupts are enabled again. If the MMU
 *                       is to be disabled, do that first, so that the
 *                       vector which is changed is the one the CPU will
 *                       use.
 */
void
trap_vector_capture(void)
{
    uint32_t vbr = 0;

    if (SysBase->AttnFlags & AFF_68010)
        vbr = trap_get_vbr();

    trap_vector_ptr = (volatile uint32_t *)(uintptr_t) (vbr + 2 * 4);
    trap_old_vector = *trap_vector_ptr;
    trap_vector_old_trapcode = old_TrapCode;
    old_TrapCode = (APTR)(uintptr_t) trap_berr_chain;
    *trap_vector_ptr = (uint32_t)(uintptr_t) trap_berr_vector;
}

/*
 * trap_vector_restore() restores the CPU's bus error vector. The 68040 and
 *                       68060 report a bus error on a write some time after
 *                       the instruction which did the write. NOP waits for
 *                       pending writes to complete, so that such a bus
 *                       error is taken before the vector is restored.
 */
void
trap_vector_restore(void)
{
    __asm__ __volatile__("nop");
    *trap_vector_ptr = trap_old_vector;
    old_TrapCode = trap_vector_old_trapcode;
}

/*
 * void trap_handler(cpu frame, uint32_t trap#)
 *
 * Task trap handler (preserves all registers).  Entered in supervisor
 * mode with the following on the supervisor stack:
 *    0(sp).l = trap#
 *    4(sp) Processor dependent exception frame
 *
 * trap_berr_vector is an alternate entry for when the CPU's bus error
 * vector points here.  It has only the exception frame on the stack.
 *
 * The registers are saved and trap_fixup() is called to modify the
 * exception frame.  It returns the number of bytes between the start of
 * the original frame and the frame to return with (or negative if the
 * trap was not handled, in which case the previous trap handler is
 * called).
 */
__asm(".text                          \n"
      ".even                          \n"
      "_trap_berr_vector:             \n"  // CPU bus error vector entry
      "move.l  #2,-(sp)               \n"  // Push trap number, as exec does
      "_trap_handler:                 \n"  // Task trap handler entry
      "movem.l d0-d7/a0-a6,-(sp)      \n"  // Save registers (60 bytes)
      "move.l  60(sp),d0              \n"  // d0 = Trap number
      "lea     64(sp),a0              \n"  // a0 = CPU exception frame
      "move.l  sp,a1                  \n"  // a1 = Saved registers
      "move.l  a1,-(sp)               \n"  // Arguments are passed both on
      "move.l  a0,-(sp)               \n"  //   the stack and in registers
      "move.l  d0,-(sp)               \n"
      "jsr     _trap_fixup            \n"
      "lea     12(sp),sp              \n"
      "tst.l   d0                     \n"
      "bmi.s   trap_nofix             \n"  // Not handled
      "lea     64(sp,d0.l),a0         \n"  // a0 = Frame to return with
      "move.l  32(sp),-(a0)           \n"  // Copy saved A0 to just below it
      "movem.l (sp)+,d0-d7            \n"
      "addq.l  #4,sp                  \n"  // Skip saved A0
      "movem.l (sp)+,a1-a6            \n"
      "move.l  a0,sp                  \n"  // Discard trap number and frame
      "move.l  (sp)+,a0               \n"
      "rte");                              // Return from exception

__asm("trap_nofix:                    \n"
      "movem.l (sp)+,d0-d7/a0-a6      \n"
      "tst.l   _old_TrapCode          \n"  // is there another trap handler ?
      "beq.s   trap_end               \n"  // no, so we'll exit
      "move.l  _old_TrapCode,-(sp)    \n"  // yes, go on to old TrapCode
      "rts");                              // jumps to old TrapCode

__asm("trap_end:                      \n"
      "addq.l  #4,sp                  \n"  // Remove exception number from SSP
      "rte");                              // Return from exception

/*
 * void trap_berr_chain(void)
 *
 * This takes the place of the previous task trap handler when the trap
 * handler was entered from the CPU's bus error vector. It removes the
 * trap number and continues at the previous bus error vector.
 */
__asm("_trap_berr_chain:              \n"
      "addq.l  #4,sp                  \n"  // Remove trap number
      "move.l  _trap_old_vector,-(sp) \n"
      "rts");                              // jumps to old bus error vector

/*
 * uint32_t trap_get_vbr(void)
 *
 * Returns the vector base register. Must only be called on a 68010 or
 * higher, and in supervisor state.
 */
__asm("_trap_get_vbr:                 \n"
      ".word   0x4e7a                 \n"  // movec vbr,d0  [68010+]
      ".word   0x0801                 \n"
      "rts");

/*
 * void trap_wb040_run(void)
 *
 * Complete the writes which were pending in the 68040 pipeline when an
 * access fault occurred, and then resume.  The exception handler returns
 * to this code (in the mode of the code which took the fault) instead of
 * returning directly to the faulted code.  This is done so that if one of
 * the pending writes also faults, that fault is just another fault to be
 * counted, and not a fault within the exception handler.  All registers
 * and the condition codes are preserved.
 *
 * The saved state is loaded into registers before the first write. Each
 * write is followed by NOP, which on the 68040 does not complete until all
 * pending writes have completed.
 */
__asm("_trap_wb040_run:               \n"
      "subq.l  #4,sp                  \n"  // Space for PC to resume at
      ".word   0x42e7                 \n"  // move.w ccr,-(sp)  [68010+]
      "movem.l d0-d3/a0-a1,-(sp)      \n"  // Save registers (24 bytes)
      "lea     _trap_wb040,a1         \n"
      "move.l  (a1)+,26(sp)           \n"  // PC to resume at
      "move.l  (a1)+,a0               \n"  // Write-back 2 address
      "move.l  (a1)+,d0               \n"  // Write-back 2 data
      "move.l  (a1)+,d2               \n"  // Write-back 2 size
      "move.l  8(a1),d3               \n"  // Write-back 3 size
      "move.l  4(a1),d1               \n"  // Write-back 3 data
      "move.l  (a1),a1                \n"  // Write-back 3 address
      "bsr.s   trap_wb040_put         \n"
      "move.l  a1,a0                  \n"
      "move.l  d1,d0                  \n"
      "move.l  d3,d2                  \n"
      "bsr.s   trap_wb040_put         \n"
      "movem.l (sp)+,d0-d3/a0-a1      \n"
      "rtr");                              // Restore CCR and resume

__asm("trap_wb040_put:                \n"
      "subq.l  #1,d2                  \n"
      "bmi.s   trap_wb040_none        \n"  // Size 0: nothing to write
      "beq.s   trap_wb040_byte        \n"  // Size 1
      "subq.l  #1,d2                  \n"
      "beq.s   trap_wb040_word        \n"  // Size 2
      "move.l  d0,(a0)                \n"  // Size 4
      "nop                            \n"
      "rts");

__asm("trap_wb040_word:               \n"
      "move.w  d0,(a0)                \n"
      "nop                            \n"
      "rts");

__asm("trap_wb040_byte:               \n"
      "move.b  d0,(a0)                \n"
      "nop                            \n"
      "trap_wb040_none:               \n"
      "rts");
#endif /* AMIGAOS && !_DCC */
