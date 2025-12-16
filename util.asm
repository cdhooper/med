        SECTION text,code
        DS.l 0

        XDEF    _mem_copy8
        XDEF    _mem_copy16
        XDEF    _mem_copy32

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
;   XREF _countdiv0
    XREF _old_TrapCode

_trap_handler:
    move.l  (sp),$00150000
    move.l  4(sp),$00150004
    move.l  8(sp),$00150008
    move.l  12(sp),$0015000c
    move.l  16(sp),$00150010
    move.l  20(sp),$00150014
    move.l  24(sp),$00150018
    move.l  28(sp),$0015001c
    move.l  32(sp),$00150020
    move.l  36(sp),$00150024
    move.l  40(sp),$00150028
    move.l  44(sp),$0015002c
    move.l  48(sp),$00150030
    move.l  52(sp),$00150034
    move.l  56(sp),$00150038
    move.l  60(sp),$0015003c
    move.l  64(sp),$00150040

    cmpi.l  #2,(sp)                 ; is this bus error?
    beq.s   bus_err
    cmpi.l  #3,(sp)                 ; is this address error?
    beq.s   addr_err
    cmpi.l  #4,(sp)                 ; is this illegal instruction?
    beq.s   ill_inst
    cmpi.l  #5,(sp)                 ; is this a divide by zero?
    beq.s   div0_err

    move.l  #0,$00150044
    tst.l   _old_TrapCode           ; is there another trap handler ?
    beq.s   endtrap                 ; no, so we'll exit
    move.l  _old_TrapCode,-(sp)     ; yes, go on to old TrapCode
    rts                             ; jumps to old TrapCode
endtrap:
    addq    #4,sp                   ; remove exception number from SSP
    rte                             ; return from exception
addr_err:
bus_err:
    move.l  #1,$00150044
    add.l   #1,_mem_fault_count     ; increment fault count
    addq.l  #6,sp                   ; remove exception number from SSP
    addq.l  #2,(sp)                 ; Skip bad instruction
    subq.l  #2,sp
    addq    #4,sp                   ; remove exception number from SSP
    rte                             ; return from exception
ill_inst:
    move.l  #2,$00150044
    add.l   #1,_mem_fault_count     ; increment fault count

    addq.l  #2,6(sp)                ; Skip bad instruction
    addq.l  #4,sp                   ; Remove exception number from SSP

; Alternative to the above
;   subq.l  #2,sp                   ; remove exception number from SSP
;   addq.l  #2,8(sp)                ; Skip bad instruction
;   addq.l  #6,sp
    rte                             ; return from exception
div0_err:
    move.l  #3,$00150044
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
