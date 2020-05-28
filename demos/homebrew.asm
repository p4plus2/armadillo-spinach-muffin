org    $808000

RESET:
        SEI

        ; Exit emulation mode
        CLC
        XCE
        
        ; Clear interrupts, DMA, HDMA
        STZ $4200
        STZ $420B
        STZ $420C

        ; Jump to FastROM area. Works because ORG specifies bank $80
        JML +

+        REP #$20

        ; Set STAck pointer to $00:0FFF
        LDA #$0FFF
        TCS
        
        ; Set data bank to $80 (FastROM area)
        PHK
        PLB
        
        ; Set direct page register to $0000
.zero   PEA $0000
        PLD

        ; Enable FastROM through hardware register
        LDX #$01
        STX $420D

        SEP #$20
        REP #$10

        ; Clear RAM $7E:0000-$7F:FFFF
        LDX #$8008
        STX $4300
        LDX.w #.zero+1
        STX $4302
        LDA.b #.zero+1>>16
        STA $4304
        LDX #$0000
        STX $4305
        STX $2181
        STZ $2183
        LDA #$01
        STA $420B
        LDA #$01
        STA $2183
        STA $420B
        SEP #$10
        
        ; Initialize every single hardware register ($21xx & $42xx)
        PHD
        PEA $2100
        PLD
        LDA #$80
        STA $00
        STZ $01
        STZ $02
        STZ $03
        STZ $04
        STZ $05
        STZ $06
        STZ $07
        STZ $08
        STZ $09
        STZ $0A
        STZ $0B
        STZ $0C
        STZ $0D
        STZ $0D
        STZ $0E
        STZ $0E
        STZ $0F
        STZ $0F
        STZ $10
        STZ $10
        STZ $11
        STZ $11
        STZ $12
        STZ $12
        STZ $13
        STZ $13
        STZ $14
        STZ $14
        STZ $15
        STZ $16
        STZ $17
        STZ $18
        STZ $19
        STZ $1A
        STZ $1B
        STZ $1B
        STZ $1C
        STZ $1C
        STZ $1D
        STZ $1D
        STZ $1E
        STZ $1E
        STZ $1F
        STZ $1F
        STZ $20
        STZ $20
        STZ $21
        STZ $22
        STZ $22
        STZ $23
        STZ $24
        STZ $25
        STZ $26
        STZ $27
        STZ $28
        STZ $29
        STZ $2A
        STZ $2B
        STZ $2C
        STZ $2D
        STZ $2E
        STZ $2F
        STZ $30
        STZ $31
        LDA #$80
        STA $32
        LDA #$40
        STA $32
        LDA #$20
        STA $32
        STZ $33
        STZ $40
        STZ $41
        STZ $42
        STZ $43
        PEA $4200
        PLD
        STZ $01
        STZ $02
        STZ $03
        STZ $04
        STZ $05
        STZ $06
        STZ $07
        STZ $08
        STZ $09
        STZ $0A
        STZ $0B
        STZ $0C
        PLD
        pea $8080
        plb
        plb
        rep #$30
        stz $00
        stz $02
        stz $04
        stz $06
        stz $08
        stz $0A
        stz $0C
        stz $0E
        stz $10
        stz $12
        stz $14
        stz $16
        stz $18
        stz $1A
        
        ;init stack
        LDA #$0EFF
        sta $18
        jsr main
        stp

incsrc "test_code.asm"

nmi_wrapper:
	rep #$30
	pha
	phx
	phy
	pei ($00)	;preserve EAX ECX and EDX
	pei ($08)
	pei ($0C)
	jsr _Z3nmiv
	rep #$30
	pla
	sta $0C
	pla
	sta $08
	pla
	sta $00
	ply
	plx
	pla
	rti
	
irq_wrapper:
	rti

youscrewedup:
stp

    ; I mostly copypasted this header ROM from an old project of mine
; I'm not sure if the values are correct or not BUT HEY the rom works
org $00FFB0
        db "FF"                ;maker code.
        db "FFFF"            ;game code.
        db $00,$00,$00,$00,$00,$00,$00    ;fixed value, must be 0
        db $00                ;expansion RAM size. SRAM size. 128kB
        db $00                ;special version, normally 0
        db $00                ;cartridge sub number, normally 0s

        db "DEMODEMO             "    ;ROM NAME
        db $30                ;MAP MODE. Mode 30 = fastrom
        db $02                ;cartridge type. ROM AND RAM AND SRAM
        db $09                ;3-4 MBit ROM        
        db $00                ;64K RAM        
        db $00                ;Destination code: Japan
        db $33                ;Fixed Value    
        db $00                ;Mask ROM. This ROM is NOT revised.
        dw $B50F            ;Complement Check.
        dw $4AF0            ;Checksum

        ;emulation mode
        dw $FFFF            ;Unused
        dw $FFFF            ;Unused
        dw youscrewedup        ;COP
        dw youscrewedup        ;BRK
        dw youscrewedup        ;ABORT
        dw nmi_wrapper                ;NMI
        dw $FFFF            ;Unused
        dw $FFFF                ;IRQ

        ;native mode
        dw $FFFF            ;Unused
        dw $FFFF            ;Unused
        dw youscrewedup        ;COP
        dw youscrewedup        ;BRK
        dw youscrewedup        ;ABORT
        dw $FFFF            ;NMI
        dw RESET            ;RESET
        dw $FFFF            ;IRQ
