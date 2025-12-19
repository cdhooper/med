    SECTION text,code
    DS.l 0

    XDEF    _mem_copy8
    XDEF    _mem_copy16
    XDEF    _mem_copy32
    XDEF    _swap16
    XDEF    _swap32

;
; void mem_copy8(void *dst, void *src)
;           Copy 8 bytes from src to dst address
;
_mem_copy8:
    move.l $8(sp),a0        ; a0 = src
    move.l $4(sp),a1        ; a1 = dst
    movem.l (a0),d0-d1
    movem.l d0-d1,(a1)
    rts

;
; void mem_copy16(void *dst, void *src)
;           Copy 16 bytes from src to dst address
;
_mem_copy16:
    move.l $8(sp),a0
    move.l $4(sp),a1
    movem.l d2-d3,-(sp)
    movem.l (a0),d0-d3
    movem.l d0-d3,(a1)
    movem.l (sp)+,d2-d3
    rts

;
; uint16_t swap16(uint16_t val)
;           Swap high and low byte of 16-bit value
;
_swap16:
    move.l $4(sp),d0
    ror.w #8,d0
    rts

;
; uint32_t swap32(uint32_t val)
;           Swap byte order of 32-bit value
;
_swap32:
    move.l $4(sp),d0
    ror.w #8,d0
    swap d0
    ror.w #8,d0
    rts

;
; void mem_copy32(void *dst, void *src)
;           Copy 32 bytes from src to dst address
;
_mem_copy32:
    move.l $8(sp),a0
    move.l $4(sp),a1
    movem.l d2-d7,-(sp)
    movem.l (a0),d0-d7
    movem.l d0-d7,(a1)
    movem.l (sp)+,d2-d7
    rts



; Example trap handling code (leaves D0 intact).  Entered
; in supervisor mode with the following on the supervisor stack:
;    0(sp).l = trap#
;    4(sp) Processor dependent exception frame

    XDEF _trap_handler
    XREF _mem_fault_count
    XREF _old_TrapCode

_trap_handler:
;   move.l  (sp),$00150000
;   move.l  4(sp),$00150004
;   move.l  8(sp),$00150008
;   move.l  12(sp),$0015000c
;   move.l  16(sp),$00150010
;   move.l  20(sp),$00150014
;   move.l  24(sp),$00150018
;   move.l  28(sp),$0015001c
;   move.l  32(sp),$00150020
;   move.l  36(sp),$00150024
;   move.l  40(sp),$00150028
;   move.l  44(sp),$0015002c
;   move.l  48(sp),$00150030
;   move.l  52(sp),$00150034
;   move.l  56(sp),$00150038
;   move.l  60(sp),$0015003c
;   move.l  64(sp),$00150040

    cmpi.l  #2,(sp)                 ; is this bus error?
    beq.s   bus_err
    cmpi.l  #3,(sp)                 ; is this address error?
    beq.s   addr_err
    cmpi.l  #4,(sp)                 ; is this illegal instruction?
    beq.s   ill_inst
    cmpi.l  #5,(sp)                 ; is this a divide by zero?
    beq.s   div0_err

;   move.l  #0,$00150044
    tst.l   _old_TrapCode           ; is there another trap handler ?
    beq.s   endtrap                 ; no, so we'll exit
    move.l  _old_TrapCode,-(sp)     ; yes, go on to old TrapCode
    rts                             ; jumps to old TrapCode
endtrap:
    addq    #4,sp                   ; remove exception number from SSP
    rte                             ; return from exception
addr_err:
bus_err:
;   move.l  #1,$00150044
    add.l   #1,_mem_fault_count     ; increment fault count
    addq.l  #6,sp                   ; remove exception number from SSP
    addq.l  #2,(sp)                 ; Skip bad instruction
    subq.l  #2,sp
    addq    #4,sp                   ; remove exception number from SSP
    rte                             ; return from exception
ill_inst:
;   move.l  #2,$00150044
    add.l   #1,_mem_fault_count     ; increment fault count

    addq.l  #2,6(sp)                ; Skip bad instruction
    addq.l  #4,sp                   ; Remove exception number from SSP

; Alternative to the above
;   subq.l  #2,sp                   ; remove exception number from SSP
;   addq.l  #2,8(sp)                ; Skip bad instruction
;   addq.l  #6,sp
    rte                             ; return from exception
div0_err:
;   move.l  #3,$00150044
    add.l   #1,_mem_fault_count     ; increment fault count
    addq    #4,sp                   ; remove exception number from SSP
    rte                             ; return from exception


;   b2:   1490            move.b (a0),(a2)
;   c8:   3490            move.w (a0),(a2)
;   de:   2490            move.l (a0),(a2)
;   f0:   2f07            move.l d7,-(sp)
;   f2:   2f0a            move.l a2,-(sp)
;  13a:   2d6c 0000 0032  move.l 0(a4),50(a6)
;  156:   2e2f 001c       move.l 28(sp),d7
;  15a:   2c2f 0020       move.l 32(sp),d6
;  1fa:   1092            move.b (a2),(a0)
;  212:   3092            move.w (a2),(a0)
;  22a:   2092            move.l (a2),(a0)

;
; 68k CPU special register access functions
;
    XDEF    _cpu_get_cacr
    XDEF    _cpu_set_cacr
    XDEF    _cpu_get_dtt0
    XDEF    _cpu_set_dtt0
    XDEF    _cpu_get_dtt1
    XDEF    _cpu_set_dtt1
    XDEF    _cpu_get_itt0
    XDEF    _cpu_set_itt0
    XDEF    _cpu_get_itt1
    XDEF    _cpu_set_itt1
    XDEF    _cpu_get_pcr
    XDEF    _cpu_set_pcr
    XDEF    _cpu_get_sr
    XDEF    _cpu_set_sr
    XDEF    _cpu_get_tc
    XDEF    _cpu_set_tc
    XDEF    _cpu_get_tt0
    XDEF    _cpu_set_tt0
    XDEF    _cpu_get_tt1
    XDEF    _cpu_set_tt1
    XDEF    _cpu_get_vbr
    XDEF    _cpu_set_vbr
    XDEF    _fpu_get_fpcr
    XDEF    _fpu_set_fpcr
    XDEF    _fpu_get_fpsr
    XDEF    _fpu_set_fpsr
    XDEF    _cpu_cache_flush_040
    XDEF    _cpu_cache_flush_040_data
    XDEF    _cpu_cache_flush_040_inst
    XDEF    _cpu_cache_invalidate_040
    XDEF    _cpu_cache_invalidate_040_inst
    XDEF    _cpu_cache_invalidate_040_data
    XDEF    _flush_tlb_030
    XDEF    _flush_tlb_040
    XDEF    _mmu_get_tc_030
    XDEF    _mmu_set_tc_030
    XDEF    _mmu_get_tc_040
    XDEF    _mmu_set_tc_040

; uint32_t cpu_get_cacr(void);
_cpu_get_cacr:
    dc.w $4e7a          ; movec cacr,d0
    dc.w $0002
    rts

; void cpu_set_cacr(uint32_t value);
_cpu_set_cacr:
    move.l $4(sp),d0
    dc.w $4e7b          ; movec d0,cacr
    dc.w $0002
    rts

; uint32_t cpu_get_dtt0(void);
_cpu_get_dtt0:
    dc.w $4e7a          ; movec dtt0,d0
    dc.w $0006
    rts

; void cpu_set_dtt0(uint32_t value);
_cpu_set_dtt0:
    move.l $4(sp),d0
    dc.w $4e7b          ; movec d0,dtt0
    dc.w $0006
    rts

; uint32_t cpu_get_dtt1(void);
_cpu_get_dtt1:
    dc.w $4e7a          ; movec dtt1,d0
    dc.w $0007
    rts

; void cpu_set_dtt1(uint32_t value);
_cpu_set_dtt1:
    move.l $4(sp),d0
    dc.w $4e7b          ; movec d0,dtt1
    dc.w $0007
    rts

; uint32_t cpu_get_itt0(void);
_cpu_get_itt0:
    dc.w $4e7a          ; movec itt0,d0
    dc.w $0004
    rts

; void cpu_set_itt0(uint32_t value);
_cpu_set_itt0:
    move.l $4(sp),d0
    dc.w $4e7b          ; movec d0,itt0
    dc.w $0004
    rts

; uint32_t cpu_get_itt1(void);
_cpu_get_itt1:
    dc.w $4e7a          ; movec itt1,d0
    dc.w $0005
    rts

; void cpu_set_itt1(uint32_t value);
_cpu_set_itt1:
    move.l $4(sp),d0
    dc.w $4e7b          ; movec d0,itt1
    dc.w $0005
    rts

; uint32_t cpu_get_pcr(void);
_cpu_get_pcr:
    dc.w $4e7a          ; movec pcr,d0
    dc.w $0808
    rts

; void cpu_set_pcr(uint32_t value);
_cpu_set_pcr:
    move.l $4(sp),d0
    dc.w $4e7b          ; movec d0,pcr
    dc.w $0808
    rts

; uint16_t cpu_get_sr(void);
_cpu_get_sr:
    move.w sr,d0
    rts

; void cpu_set_sr(uint16_t value);
_cpu_set_sr:
    move.l $4(sp),d0
    move.w d0,sr
    rts

; uint32_t cpu_get_tc(void);
_cpu_get_tc:
    dc.w $4e7a          ; movec tc,d0
    dc.w $0003
    rts

; void cpu_set_tc(uint32_t value);
_cpu_set_tc:
    move.l $4(sp),d0
    dc.w $4e7b          ; movec d0,tc
    dc.w $0003
    rts

; uint32_t cpu_get_tt0(void);
_cpu_get_tt0:
    subq.l #4,sp
    dc.l $f0170a00      ; pmove.l tt0,(sp)
    move.l (sp)+,d0
    rts

; void cpu_set_tt0(uint32_t value);
_cpu_set_tt0:
    move.l d0,-(sp)
    dc.l $f0170800      ; pmove.l (sp),tt0
    adda.l #4,sp
    rts

; uint32_t cpu_get_tt1(void);
_cpu_get_tt1:
    subq.l #4,sp
    dc.l $f0170e00      ; pmove.l tt1,(sp)
    move.l (sp)+,d0
    rts

; void cpu_set_tt1(uint32_t value);
_cpu_set_tt1:
    move.l d0,-(sp)
    dc.l $f0170c00      ; pmove.l (sp),tt1
    adda.l #4,sp
    rts

; uint32_t cpu_get_vbr(void);
_cpu_get_vbr:
    dc.w $4e7a          ; movec vbr,d0
    dc.w $0801
    rts

; void cpu_set_vbr(uint32_t value);
_cpu_set_vbr:
    move.l $4(sp),d0
    dc.w $4e7b          ; movec d0,vbr
    dc.w $0801
    rts

; uint32_t fpu_get_fpcr(void)
_fpu_get_fpcr:
    dc.w $f200          ; fmove.l fpcr,d0
    dc.w $b000
    rts

; void fpu_set_fpcr(uint32_t value)
_fpu_set_fpcr:
    move.l $4(sp),d0
    dc.w $f200          ; fmove.l d0,fpcr
    dc.w $9000
    rts

; uint32_t fpu_get_fpsr(void);
_fpu_get_fpsr:
    dc.w $f200          ; fmove.l fpsr,d0
    dc.w $a800
    rts

; void fpu_set_fpsr(uint32_t value);
_fpu_set_fpsr:
    move.l $4(sp),d0
    dc.w $f200          ; fmove.l d0,fpsr
    dc.w $8800
    rts

; void cpu_cache_flush_040(void);
_cpu_cache_flush_040:
    nop
    dc.w $f4f8          ; cpusha %bc
    rts

; void cpu_cache_flush_040_data(void);
_cpu_cache_flush_040_data:
    nop
    dc.w $f478          ; cpusha %dc
    rts

; void cpu_cache_flush_040_inst(void);
_cpu_cache_flush_040_inst:
    nop
    dc.w $f4b8          ; cpusha %ic
    rts

; void cpu_cache_invalidate_040(void);
_cpu_cache_invalidate_040:
    nop
    dc.w $f4d8          ; cinva %bc
    rts

; void cpu_cache_invalidate_040_inst(void);
_cpu_cache_invalidate_040_inst:
    nop
    dc.w $f498          ; cinva %ic
    rts

; void cpu_cache_invalidate_040_data(void);
_cpu_cache_invalidate_040_data:
    nop
    dc.w $f458          ; cinva %dc
    rts

; void flush_tlb_030(void);
_flush_tlb_030:
    dc.w $f000          ; 68030 pflusha
    dc.w $2400
    rts

; void flush_tlb_040(void);
_flush_tlb_040:
    dc.w $f518          ; 68040 pflusha
    rts

; uint32_t mmu_get_tc_030(void);
_mmu_get_tc_030:
    subq.l #4,sp
    dc.l $f0174200      ; pmove tc,(sp)
    move.l (sp)+,d0
    rts

; void mmu_set_tc_030(uint32_t tc);
_mmu_set_tc_030:
    adda.l  #4,sp
    dc.l    $f0174000       ; pmove.l (sp),tc
    suba.l  #4,sp
    rts

; uint32_t mmu_get_tc_040(void);
_mmu_get_tc_040:
    dc.l $4e7a0003    ; movec.l tc.d0
    rts

; void mmu_set_tc_040(uint32_t tc);
_mmu_set_tc_040:
    move.l $4(sp),d0
    dc.l $4e7b0003    ; movec.l d0,tc
    rts

;
; 68k CPU fault injection functions
;
    XDEF _CPU_FAULT_ALINE
    XDEF _CPU_FAULT_ADDR
    XDEF _CPU_FAULT_CHK
    XDEF _CPU_FAULT_DIV0
    XDEF _CPU_FAULT_FLINE
    XDEF _CPU_FAULT_FMT
    XDEF _CPU_FAULT_FDIV
    XDEF _CPU_FAULT_FPCP
    XDEF _CPU_FAULT_FPUC
    XDEF _CPU_FAULT_ILL_INST
    XDEF _CPU_FAULT_PRIV
    XDEF _CPU_FAULT_TRAP
    XDEF _CPU_FAULT_TRAPV

; void CPU_FAULT_ALINE(void);
_CPU_FAULT_ALINE:
    dc.w $a000          ; Any instruction whose opcode begins with A
    rts

; void CPU_FAULT_ADDR(void);
_CPU_FAULT_ADDR:
    lea.l $1(pc),a0     ; Branch to unaligned address
    jmp (a0)
    rts

; void CPU_FAULT_CHK(void);
_CPU_FAULT_CHK:
    moveq #-1,d0        ; CHK compares register in range 0..X
    dc.w $413c          ; chk.l #10,d0
    dc.w $0000
    dc.w $000a
    rts

; void CPU_FAULT_DIV0(void);
_CPU_FAULT_DIV0:
    move.l #0,d0        ; Divide by Zero
    divs.w #0,d0
    rts

; void CPU_FAULT_FLINE(void);
_CPU_FAULT_FLINE:
    dc.w $f000          ; Any instruction whose opcode begins with F
    dc.w $0000
    rts

; void CPU_FAULT_FMT(void);
_CPU_FAULT_FMT:
    ; Push an invalid FPU frame format and attempt to restore it
    move.l #-16777216,-(sp)
    dc.w $f35f          ; frestore (sp)+
    rts

; void CPU_FAULT_FDIV(void);
_CPU_FAULT_FDIV:
    ; Generate FPU Divide by Zero Error
    dc.w $f23c          ; fmove.l #1024,fpcr
    dc.w $9000
    dc.w $0000
    dc.w $0400
    dc.w $f23c          ; fmove.l #0,fpsr
    dc.w $8800
    dc.w $0000
    dc.w $0000
    dc.w $f23c          ; fmove.l #42,fp0
    dc.w $4000
    dc.w $0000
    dc.w $002a
    dc.w $f23c          ; fmove.l #0,fp1
    dc.w $4000
    dc.w $0000
    dc.w $0000
    dc.w $f200          ; fdivx fp1,fp0
    dc.w $0420
    rts

; void CPU_FAULT_FPCP(void);
_CPU_FAULT_FPCP:
    ; Generate FPCP Operand Error
    dc.w $f23c          ; fmove.l #8192,fpcr
    dc.w $9000
    dc.w $0000
    dc.w $2000
    dc.w $f200          ; fmove.l fp0,d0
    dc.w $6000
    move.w #0,-(sp)
    dc.w $f35f          ; frestore (sp)+
    rts

; void CPU_FAULT_FPUC(void);
_CPU_FAULT_FPUC:
    ; Clear FPU fault state
    move.l #0,-(sp)
    dc.w $f35f          ; frestore (sp)+
    rts

; void CPU_FAULT_ILL_INST(void);
_CPU_FAULT_ILL_INST:
    illegal             ; Illegal instruction
    rts

; void CPU_FAULT_PRIV(void);
_CPU_FAULT_PRIV:
    ; Drop to user mode and issue STOP, which requires supervisor
    move.w #0,sr
    stop #$2700
    rts

; void CPU_FAULT_TRAP(void);
_CPU_FAULT_TRAP:
    trap #7             ; CPU TRAP
    rts

; void CPU_FAULT_TRAPV(void);
_CPU_FAULT_TRAPV:
    move.l #2147483647,d0
    addq.l #2,d0
    trapv
    rts
