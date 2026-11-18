; Standalone RC3 test loaded at &040000 on a disposable emulator.
.assume adl=1
.org $040000

gateway:   .equ $0bc01c
oldTarget: .equ $0005e4
shortSP:   .equ $ef00
longSP:    .equ $05f000
bank:      .equ $040000
repeats:   .equ 8

    jp entry
    .align 64
    .db "MOS",0,1

entry:
    di
    ld sp,longSP
    ld a,(gateway)
    cp $c3
    jp nz,unsupported
    ld hl,(gateway+1)
    ld de,oldTarget
    or a
    sbc hl,de
    jp nz,unsupported
    in0 a,($93)
    or a
    jp nz,unsupported

    ld a,$a9
    ld (bank+shortSP),a
    ld (longSP),a
    ld (longSP-13),a
    ld hl,handler
    ld (gateway+1),hl
    ld a,4
    ld mb,a
    stmix
    jp.sis native-bank

.assume adl=0
native:
    ld sp,shortSP
    ld hl,0
    ld bc,$a947
    ld de,$58c2
    ld a,$c3
    out0 ($93),a
next_halt:
    inc hl
    ld (progress-bank),hl
    ld a,$7d
    scf
    halt
after_halt:
    jp nc,native_fail-bank
    cp $7d
    jp nz,native_fail-bank
    ld a,(nmi_count-bank)
    cp l
    jp nz,native_fail-bank
    ld a,(progress-bank)
    cp l
    jp nz,native_fail-bank
    cp repeats
    jr nz,next_halt
    ld a,b
    cp $a9
    jp nz,native_fail-bank
    ld a,c
    cp $47
    jp nz,native_fail-bank
    ld a,d
    cp $58
    jp nz,native_fail-bank
    ld a,e
    cp $c2
    jp nz,native_fail-bank
    xor a
    jp.lil restore
native_fail:
    ld a,3
    jp.lil restore

.assume adl=1
handler:
    push af
    push hl
    push de
    ld hl,9
    add hl,sp
    ld a,(hl)
    cp 2                         ; Mixed-mode frame from ADL=0.
    jp nz,handler_fail
    ld de,after_halt-bank
    inc hl
    ld a,(hl)
    cp e
    jp nz,handler_fail
    inc hl
    ld a,(hl)
    cp d
    jp nz,handler_fail
    ld a,i
    jp pe,handler_fail            ; The interrupted guest was under DI.
    in0 a,($93)
    cp $e3
    jp nz,handler_fail
    ld hl,nmi_count
    inc (hl)
    ld a,'N'
    out ($30),a
    ld a,(hl)
    cp repeats
    jr z,handler_return
    ld a,$a5
    out0 ($94),a
    ld a,$5a
    out0 ($94),a
handler_return:
    pop de
    pop hl
    pop af
    retn.l
handler_fail:
    ld a,4
    jr restore

restore:
    ld b,a
    ld hl,oldTarget
    ld (gateway+1),hl
    ld a,(bank+shortSP)
    cp $a9
    jr nz,guard_error
    ld a,(longSP)
    cp $a9
    jr nz,guard_error
    ld a,(longSP-13)
    cp $a9
    jr nz,guard_error
    ld a,b
    or a
    jr nz,exit
    ld a,'P'
    out ($30),a
    ld a,10
    out ($30),a
    xor a
    jr exit
guard_error:
    ld a,5
    jr exit
unsupported:
    ld a,2
exit:
    out (0),a
    halt
    jr exit

nmi_count: .db 0
progress:  .dw 0
