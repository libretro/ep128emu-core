; File handling extension for TVC - ep128emu bridge
; Will be embedded to RST $30 calls as standard cassette functions.
; CAS identifiers (b7 0: out, b7 1: in, b6-b4: 101, b3-b0: index in jump table)
;   $D0 / $50: IRQ service routine
;   $D1 / $51: CAS_CHIN / CAS_CHOUT char in/out
;   $D2 / $52: CAS_BKIN / CAS_BKOUT block in/out
;   $D3 / $53: CAS_OPEN / CAS_CRTE  open file for read/write
;   $D4 / $54: CAS_CLOSE  close file / close and flush file
;   $D5      : CAS_VERIFY compare memory and file

; Extensions for floppy:
; note: $EOF is not set??
;   $D6 / $56: Seek within file   (absolute, from file start)
;     returns new pointer in BCDE
;   $D7 / $57: Seek within file   (relative, from current position)
;     returns new pointer in BCDE
;   $D8 / $58: Seek to file end   (offset is irrelevant)
;   $D9 / $59: Define CLI buffer  (not emulated in tvcfileio)
;   $DA / $5A: Execute CLI buffer (not emulated in tvcfileio)
;   $DB / $5B: Run FISH function  (not emulated in tvcfileio)

; Extensions for tvc256++:
;   $D6 / $56: GET_PWD / CH_DIR     Get current dir / change dir
;   $D7 / $57: OPEN_DIR             Open directory
;   $D8 / $58: READ_DIR / CLOSE_DIR Read one dir entry / Close dir
;   $D9 / $59: MK_DIR               Make dir
;   $DA / $5A: DELETE               Delete file or empty dir
;   $DB / $5B: RENAME               Rename file or dir
;   $DC / $5C: SEEK_FILE            Seek within file (absolute, from file start)
;   $DD / $5D: GET_IOBASE           Return the I/O base of the card
;   $DE / $5E: GET_MEMBASE          Return the memory base of the card

        org   0c000h

        defm  "MOPS"                        ; Fixed ID string
        defb  4                             ; Length of extension ID
        defm  "FILE"                        ; Identifier for tvcfileio

        defw  0
        defw  initDevice                    ; Extension init - must be on 0xC00B
        defb  15                            ; number of functions
        defw  irqRoutine                    ; IRQ service - no-op
        defw  characterIO
        defw  blockIO
        defw  fileOpen
        defw  fileClose
        defw  blockVerify
        defw  fileSeekSet                   ; overloaded - get_pwd/ch_dir
        defw  fileSeekCur                   ; overloaded - open_dir
        defw  fileSeekEnd                   ; overloaded - read_dir/close_dir
        defw  mkdir
        defw  delete
        defw  rename
        defw  fileSeekSet2
        defw  iobase
        defw  membase

; ep128emuSystemCall identifiers
; n =  0: initialization (close file)
; n =  1: open input file (DE = name address), store file name at IX+8
; n =  2: open output file (DE = name address), store file name at IX+8
; n =  3: close file
; n =  4: close file
; n =  5: read character to C
; n =  6: write character from C
; Identifiers between 7-9 are used for both floppy emulation and tvc256++
; n =  7: fileSeekEnd / readdir-closedir
; n =  8: fileSeekCur / opendir
; n =  9: fileSeekSet / getpwd-chdir 
; Identifiers above 9 are used for tvc256++
; n = 10 : mkdir
; n = 11 : delete
; n = 12 : rename
; n = 13 : fileSeekSet2 (tvc256++ params)

    macro ep128emuSystemCall n
        defb  0edh, 0feh, 0feh, 0f0h | n
    endm

initDevice:
        push  ix
        ex    (sp), hl
        ld    a, l
        and   0f0h
        ld    l, a
        sub   10h
        rlca
        rlca
        and   03h
        or    80h
        ld    (0b05h), a
        ld    (0b0dh), a
        ld    (hl), 4
        inc   l
        ld    (hl), 'F'
        inc   l
        ld    (hl), 'I'
        inc   l
        ld    (hl), 'L'
        inc   l
        ld    (hl), 'E'
        inc   l
        set   1, l
        set   3, l
        ld    (hl), 0                   ; FFh = file was written to
        inc   hl
        ld    (hl), 0ffh                ; name length or FFh if file not open
        ld    l, a
        ld    a, 0f7h
.l1:    rlca
        dec   l
        jp    m, .l1
        ld    hl, 0b10h
        and   (hl)
        ld    (hl), a
        pop   hl
        ep128emuSystemCall  0
        or    a

irqRoutine:
        ret

characterIO:
        jp    p, .l1                    ; write character?
        ep128emuSystemCall  5
        or    a
        ret
.l1:    bit   7, (ix + 8)
        ld    a, 0e9h                   ; file not open
        ret   nz
        ld    (ix + 7), 0ffh
        ep128emuSystemCall  6
        or    a
        ret

blockIO:
        rla
        bit   7, (ix + 8)
        ld    a, 0e9h                   ; file not open
        ret   nz
        push  hl
        ex    de, hl
        ld    e, c
        ld    d, b
        jr    c, .l2                    ; read block?
        ld    (ix + 7), 0ffh
        jp    .l5
.l1:    ld    (hl), c
        inc   hl
        dec   de
.l2:    ld    a, e
        or    d
        jr    z, .l3
        ep128emuSystemCall  5
        or    a
        jp    z, .l1                    ; no error?
.l3:    ld    c, e
        ld    b, d
        ex    de, hl
        pop   hl
        ret
.l4:    inc   hl
        dec   de
.l5:    ld    a, e                      ; write block
        or    d
        jr    z, .l3
        ld    c, (hl)
        ep128emuSystemCall  6
        or    a
        jp    z, .l4                    ; no error?
        jr    .l3

checkCASExtension:
        push  ix
        ex    (sp), hl
        ld    a, l
        and   0f0h
        add   a, 16
        ld    l, a
        ld    a, (hl)
        add   a, 256 - 4
        jr    nc, .l1
        adc   a, l
        ld    l, a
        ld    a, '.'
        cp    (hl)
        jr    nz, .l1
        inc   l
        ld    a, 'C'
        cp    (hl)
        jr    nz, .l1
        inc   l
        ld    a, 'A'
        cp    (hl)
        jr    nz, .l1
        inc   l
        ld    a, 'S'
        cp    (hl)
.l1:    pop   hl
        ret

fileOpen:
        jp    m, fileOpenR              ; open input file?
        xor   a

; Carry = 1: open input file

addCASExtension:
        push  af
        push  bc
        push  de
        ld    a, (de)
        ld    b, a
        dec   a
        cp    25
        jr    c, .l2                    ; not empty or too long name?
.l1:    pop   de
        pop   bc
        pop   af
        or    a
        jp    z, fileOpenW
        ret
.l2:    inc   de
        ld    a, (de)
        cp    '.'
        jr    z, .l1                    ; there is already an extension?
        djnz  .l2
        ex    (sp), hl
        ld    c, (hl)
        add   hl, bc
        ld    a, c
        add   a, 13
        add   a, ixl
        ld    e, a
        ld    d, ixh
        ex    de, hl
        ld    (hl), 'S'                 ; add .CAS extension
        dec   hl
        ld    (hl), 'A'
        dec   hl
        ld    (hl), 'C'
        dec   hl
        ld    (hl), '.'
        dec   hl
        ex    de, hl
        lddr
        sub   e
        ld    (de), a
        pop   hl
        pop   bc
        pop   af
        jr    nc, fileOpenW

fileOpenR:
        ep128emuSystemCall  3
        ld    (ix + 7), 0
        ld    (ix + 8), 0ffh
        or    a
        jr    z, .l1
        cp    0e9h                      ; file not open
        ret   nz
.l1:    ep128emuSystemCall  1
        or    a
        jr    nz, .l5
        call  checkCASExtension
        jr    z, .l2
        xor   a
        ret
.l2:    push  bc
        ld    b, 80h                    ; skip .CAS header
.l3:    ep128emuSystemCall  5
        or    a
        jr    nz, .l4
        djnz  .l3
        pop   bc
        ret
.l4:    pop   bc
        ep128emuSystemCall  3
        ld    (ix + 8), 0ffh
        ld    a, 0e7h                   ; invalid .CAS header
        ret
.l5:    cp    0a1h                      ; file not found
        scf
        jp    z, addCASExtension
        ret

fileOpenW:
        ep128emuSystemCall  4
        ld    (ix + 7), 0
        ld    (ix + 8), 0ffh
        or    a
        jr    z, .l1
        cp    0e9h                      ; file not open
        ret   nz
.l1:    ep128emuSystemCall  2
        or    a
        ret   nz
        call  checkCASExtension
        jr    z, .l2
        xor   a
        ret
.l2:    push  bc                        ; write .CAS header for empty file
        push  de
        ld    bc, 0000h
        ld    de, 0080h
        call  writeCASHeader.l1
        pop   de
        pop   bc
        ret   z
        push  af
        ep128emuSystemCall  4
        ld    (ix + 7), 0
        ld    (ix + 8), 0ffh
        pop   af
        ret

updateCASHeader:
        ep128emuSystemCall  7
        or    a
        ret   nz

writeCASHeader:
        push  bc
        push  de
        ld    bc, 0000h
        ld    de, 0000h
        ep128emuSystemCall  9
        pop   de
        pop   bc
        or    a
        ret   nz
.l1:    ld    a, e
        or    d
        jr    nz, .l2
        dec   bc
.l2:    dec   de
        sla   e
        rl    d
        rl    c
        rl    b
        ld    a, 0e7h                   ; invalid .CAS header
        ret   nz
        ld    b, c
        ld    a, d
        or    b
        cp    1
        sbc   a, a                      ; A = FFh if 0 blocks
        or    e
        rrca
        inc   a
        ld    e, a                      ; E = number of bytes used in last block
        ld    c, 11h                    ; byte 0 = 11h
        ep128emuSystemCall  6
        or    a
        ret   nz
        ld    c, a                      ; byte 1 = 00h
        ep128emuSystemCall  6
        or    a
        ret   nz
        ld    c, d                      ; byte 2 = number of blocks LSB
        ep128emuSystemCall  6
        or    a
        ret   nz
        ld    c, b                      ; byte 3 = number of blocks MSB
        ep128emuSystemCall  6
        or    a
        ret   nz
        ld    a, e                      ; byte 4 = bytes used in last block
        ld    b, 7ch
.l3:    ld    c, a
        ep128emuSystemCall  6
        or    a
        ret   nz
        djnz  .l3
        ret

fileClose:
        bit   7, (ix + 8)
        ld    a, 0e9h                   ; file not open
        ret   nz
        bit   7, (ix + 7)
        jr    z, .l1                    ; file was not changed?
        ld    (ix + 7), 0
        call  checkCASExtension
        jr    nz, .l1                   ; not .CAS format?
        push  bc
        push  de
        call  updateCASHeader
        pop   de
        pop   bc
        or    a
        jr    z, .l1
        push  af
        ld    (ix + 8), 0ffh
        ep128emuSystemCall  3
        pop   af
        ret
.l1:    ld    (ix + 8), 0ffh
        ep128emuSystemCall  3
        or    a
        ret

blockVerify:
        push  hl
        ex    de, hl
        ld    e, c
        ld    d, b
        bit   7, (ix + 8)
        jr    z, .l2
        ld    a, 0e9h                   ; file not open
        jr    .l4
.l1:    ld    a, (hl)
        xor   c
        jr    nz, .l3
        inc   hl
        dec   de
.l2:    ld    a, e
        or    d
        jr    z, .l4
        ep128emuSystemCall  5
        or    a
        jp    z, .l1                    ; no error?
        defb  0cah                      ; = JP Z, nnnn
.l3:    ld    a, 0e8h                   ; verify error
.l4:    ld    c, e
        ld    b, d
        ex    de, hl
        pop   hl
        ret

; Overlapping functions are handled on emu side completely
; Indexes 7-9 are reversed vs. function order due to historical reasons
fileSeekEnd:
        ep128emuSystemCall  7
        ret

fileSeekCur:
        ep128emuSystemCall  8
        ret

fileSeekSet:
        ep128emuSystemCall  9
        ret

; tvc256++ specific functions
mkdir:
        ep128emuSystemCall 10
        ret

delete:
        ep128emuSystemCall 11
        ret

rename:
        ep128emuSystemCall 12
        ret

fileSeekSet2:
        ep128emuSystemCall 13
        ret

; extension slot 2 is fixed in emulation, so these are also fixed
iobase:
        ld c, $30
        xor a
        ret

membase:
        ld c, $A0
        xor a
        ret

        block 0e000h - $, 0ffh
