org $008000
incsrc mmio.asm

base $7E1F80
	stack_end: skip $7F
	stack:
base off

reset:
	sei 	  
	clc 	  
	xce 	  
	stz CPU.enable_interrupts
	jml +
	+
	
	phk
	plb
 	
 	;setup stack/direct page/clear decimal
 	rep #$38 
 	lda #stack
 	tcs 	 
 	lda #$0000 
 	tcd
 	
 	jsr register_init
	jsr clear_RAM
        ;init stack
	cli
	lda #$0EFF
	sta $18
	rep #$10
	jsr main
	stp
	
	
	
register_init:  
 	sep #$10
 	rep #$20
 	ldx #$80 		;\ enable force blank
 	stx PPU.screen		;/
 	stz PPU.sprite_select	; use 16x16 and 8x8 sprites, graphics at vram $0000
 	stz PPU.oam_address	; reset oam address
 	
 	ldx #$01	
 	stx PPU.layer_mode			; Mode 1
 	stz PPU.mosaic				; disable mosaic
 	stz PPU.layer_0_1_tilemap_select	; clear 0/1 map VRAM location
 	stz PPU.layer_2_3_tilemap_select	; clear 2/3 map VRAM location
 	stz PPU.layer_all_tiledata_select	; clear 0/1/2/3 Tile data location
 	
 	dex
 	stx PPU.layer_0_scroll_x	;\ layer 0 X
 	stx PPU.layer_0_scroll_x	;/
 	stx PPU.layer_1_scroll_x	;\ layer 1 X
 	stx PPU.layer_1_scroll_x	;/
 	stx PPU.layer_2_scroll_x	;\ layer 2 X
 	stx PPU.layer_2_scroll_x	;/
 	stx PPU.layer_3_scroll_x	;\ layer 3 X
 	stx PPU.layer_3_scroll_x	;/
 	
 	ldy #$07			;\ 1 pixel down so you can see the top
 	dex				;/
 	stx PPU.layer_0_scroll_y	;\ layer 0 Y
 	stx PPU.layer_0_scroll_y	;/
 	stx PPU.layer_1_scroll_y	;\ layer 1 Y
 	stx PPU.layer_1_scroll_y	;/
 	stx PPU.layer_2_scroll_y	;\ layer 2 Y
 	stx PPU.layer_2_scroll_y	;/
 	stx PPU.layer_3_scroll_y	;\ layer 3 Y
 	stx PPU.layer_3_scroll_y	;/

 	stz PPU.window_layer_all_settings	; clear window masks
 	stz PPU.window_sprite_color_settings	; clear color masks
 	stz PPU.window_1			; Clear window 1 left/right positions
 	stz PPU.window_2			; Clear window 2 left/right positions
 	stz PPU.window_logic			; Clear windowing logic
 	
 	lda #$0013 		;\ enable sprites, layer 0, layer 1 on main screen, clear subscreen
 	sta PPU.screens		;/
 	
 	stz PPU.window_masks	; Window mask for Main Screen
 	lda #$0030
 	sta PPU.color_math	; Disable color math
 	
 	lda #$00E0	
 	sta PPU.display_control	; reset color intensity and clear screen mode
 	rts

clear_RAM:
	jsr clear_wram
	jsr clear_vram
	jsr clear_cgram
	jsr clear_oam
	rts

clear_wram:
	stz WRAM.word
	stz WRAM.high
	
	lda #$8008
	sta DMA[0].settings
	lda #.fill_byte
	sta DMA[0].source_word
	ldx #.fill_byte>>16
	stx DMA[0].source_bank
	lda #stack-4	;Clear up to the last 4 bytes of stack
	sta DMA[0].size
	ldx #$01
	stx CPU.enable_dma

	lda #$2000
	sta WRAM.word
	stz WRAM.high
		
	lda #$E000
	sta DMA[0].size
	stx CPU.enable_dma
	
	stz DMA[0].size
	stx CPU.enable_dma
	
	rts
	
.fill_byte
	db $00


clear_vram:
	ldx #$80
	stx PPU.vram_control
	lda #$1809
	sta DMA[0].settings
	stz PPU.vram_address
	stz DMA[0].source_word
	stz DMA[0].source_bank
	stz DMA[0].size  
	
	ldx #$01
	stx CPU.enable_dma
	rts

clear_cgram:
	stz PPU.cgram_address
	ldy #$00
	ldx #$00
	-
		sty PPU.cgram_write
		sty PPU.cgram_write
		dex
	bne -
	rts
	
clear_oam:
	stz PPU.oam_address
	ldx #$80
	ldy #$F0
	-
		stz PPU.oam_write
		sty PPU.oam_write
		stz PPU.oam_write
		stz PPU.oam_write
		dex
	bne -
	
	ldx #$20
	-
		stz PPU.oam_write
		dex
	bne -
	rts

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

ah_damn:
stp


org $80FFC0
internal_header:
	db $20,$20,$20,$20,$20,$20,$20,$20	;\ Blank title since it is unneeded.
	db $20,$20,$20,$20,$20,$20,$20,$20 	; |
	db $20,$20,$20,$20,$20			;/
	db $21 					; Rom type: HiROM
	db $02					; ROM+SRAM
	db $0A					; ROM size: 1MB
	db $03					; RAM size: 8KB
	db $01					; Country code: NTSC        
	db $00					; License code: N/A
	db $00					; Version: zero
	dw $0000				; Checksum complement. (uncalculated)
	dw $0000				; Checksum (uncalculated)
	db $FF,$FF,$FF,$FF			;
						; Table of interrupt vectors for native mode:
	dw ah_damn				; COP
	dw ah_damn				; BRK
	dw ah_damn				; Abort
	dw nmi_wrapper				; NMI
	dw ah_damn				; Unused
	dw irq_wrapper                		; IRQ
	db $FF,$FF,$FF,$FF 			; Freespace
						;
	dw ah_damn				; COP
	dw ah_damn				; Unused
	dw ah_damn				; Abort
	dw ah_damn				; NMI
	dw reset				; Reset
	dw ah_damn				; IRQ or BRK
