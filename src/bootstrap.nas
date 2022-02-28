
MAIN_PROGRAM  EQU  0x00280000		; bootpackのロード先
DSKCAC        EQU  0x00100000		; ディスクキャッシュの場所
DSKCAC0       EQU  0x00008000		; ディスクキャッシュの場所（リアルモード）

; BOOT_INFO関係
CYLS	    EQU  0x0ff0    ; ブートセクタが設定する
LEDS	    EQU  0x0ff1    
VMODE	    EQU  0x0ff2    ; 色数に関する情報。何ビットカラーか？
SCRNX	    EQU  0x0ff4    ; 解像度のX
SCRNY	    EQU  0x0ff6    ; 解像度のY
VRAM	    EQU  0x0ff8    ; グラフィックバッファの開始番地

VBE_MODE  EQU  0x0105    ; 1024 * 768 * 8 bit

; GLOBAL start

		ORG		0xc200        ; このプログラムがどこに読み込まれるのか

; start:
; 画面モードを設定
; Try high resolution VBE first
		; Get VBE version
		MOV   AX,0x9000
		MOV   ES,AX
		MOV   DI,0
		MOV   AX,0x4f00
		INT   0x10
		CMP   AX,0x004f
		JNE   scrn320         ; No VBE controller
		MOV   AX,[ES:DI+4]    ; Get VBE version
		CMP   AX,0x0200       ; Make sure version >= 2
		JB    scrn320
		; Get VBE information
		; Information is stored at: 0x9000 * 16 + 0
		MOV   AX,0x4f01
		MOV   CX,VBE_MODE
		INT   0x10
		CMP   AX,0x004f
		JNE   scrn320         ; Failure
		; Check VBE information
		CMP   BYTE [ES:DI+0x19],8
		JNE   scrn320
		CMP   BYTE [ES:DI+0x1b],4
		JNE   scrn320
		MOV   AX,[ES:DI+0x00]
		AND   AX,0x0080
		JZ    scrn320
		; Set VBE mode
		MOV		BX,VBE_MODE+0x4000 ; 0x4000: Use linear buffer
		MOV		AX,0x4f02
		INT		0x10
		; Save information
		MOV		BYTE [VMODE],8
		MOV   AX,[ES:DI+18]
		MOV		[SCRNX],AX
		MOV   AX,[ES:DI+20]
		MOV		[SCRNY],AX
		; Don't know why I cannot use EAX to copy 4 bytes directly.
		; Just use copy twice to overcome the problem.
		MOV   AX,[ES:DI+40]
		MOV		[VRAM],AX
		MOV   AX,[ES:DI+42]
		MOV		[VRAM+2],AX
		
		; ; Set palette. (Useless. Don't know why.)
		; MOV   DI,VBE_COLOR_TABLE ; table address
		; MOV   AX,0x4f09          ; function number
		; MOV   BL,0x00            ; select "set"
		; MOV   CX,16              ; number of registers to update
		; MOV   DX,0               ; first register to update

		; VBE setting succeeded
		JMP keystatus

; Fall back to 320x200 VGA
scrn320:
		MOV		AL,0x13			; VGAグラフィックス、320x200x8bitカラー
		MOV		AH,0x00
		INT		0x10
		MOV		BYTE [VMODE],8	; 画面モードをメモする（C言語が参照する）
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000

; キーボードのLED状態をBIOSに教えてもらう
keystatus:
		MOV		AH,0x02
		INT		0x16 			; keyboard BIOS
		MOV		[LEDS],AL

; PICが一切の割り込みを受け付けないようにする
;	AT互換機の仕様では、PICの初期化をするなら、
;	こいつをCLI前にやっておかないと、たまにハングアップする
;	PICの初期化はあとでやる
; Disable all kinds of interrupts

		MOV		AL,0xff
		OUT		0x21,AL
		NOP						; OUT命令を連続させるとうまくいかない機種があるらしいので
		OUT		0xa1,AL

		CLI						; さらにCPUレベルでも割り込み禁止

; CPUから1MB以上のメモリにアクセスできるように、A20GATEを設定
; 16-bit CPU can only use <1MB memory. (Default mode)
; A20 enables CPU to make use of 32-bit address, so we can use more memory.
		CALL	waitkbdout
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL
		CALL	waitkbdout

; プロテクトモード移行
; Real mode: addr = seg_reg * 16 + offset
; Protected mode: addr = (GDT_start + seg_reg).base + offset

[INSTRSET "i486p"]		    ; 486の命令まで使いたいという記述

		LGDT	[GDTR0]			    ; 暫定GDTを設定
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; bit31 = 0; Disable paging
		OR		EAX,0x00000001	; bit0 = 1;  Protected virtual address mode
		MOV		CR0,EAX
		JMP		pipelineflush
; Skip first 8 bytes so it means GDT[1].
; Force CPU to discard pipelined instructions which are in different mode.
pipelineflush:
		MOV		AX,1*8			    ;  読み書き可能セグメント32bit
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; bootpackの転送
; Copy the main program to location 0x00280000.
; The specified size is 512 KB, which is larger than our program.
		MOV		ESI,entry_point	 ; 転送元
		MOV		EDI,MAIN_PROGRAM ; 転送先
		MOV		ECX,512*1024/4
		CALL	memcpy

; ついでにディスクデータも本来の位置へ転送

; まずはブートセクタから
; Copy IPL (512 bytes) from 0x7c00 to 0x00100000
		MOV		ESI,0x7c00		; 転送元
		MOV		EDI,DSKCAC		; 転送先
		MOV		ECX,512/4
		CALL	memcpy

; 残り全部
; Copy the remaining system image to the space after IPL. Now the system image
; has two copies in memory, but the address is much easier to remember and 
; manage. (Main program has 3 copies.)
		MOV		ESI,DSKCAC0+512	; 転送元
		MOV		EDI,DSKCAC+512	; 転送先
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; シリンダ数からバイト数/4に変換
		SUB		ECX,512/4		; IPLの分だけ差し引く
		CALL	memcpy

; asmheadでしなければいけないことは全部し終わったので、
;	あとはbootpackに任せる

; bootpackの起動
; Other information, e.g. stack.
		MOV		EBX,MAIN_PROGRAM
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; 転送するべきものがない
		MOV		ESI,[EBX+20]	; 転送元
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 転送先
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; スタック初期値
		JMP		DWORD 2*8:0x0000001b

waitkbdout:
		IN		 AL,0x64
		AND		 AL,0x02
		JNZ		waitkbdout		; ANDの結果が0でなければwaitkbdoutへ
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 引き算した結果が0でなければmemcpyへ
		RET
; memcpyはアドレスサイズプリフィクスを入れ忘れなければ、ストリング命令でも書ける

		ALIGNB	16

VBE_COLOR_TABLE:
		; 4th byte: alignment or alpha?
		DB    0x00, 0x00, 0x00, 0x00  ;  0:黒
		DB    0xff, 0x00, 0x00, 0x00  ;  1:明るい赤
		DB    0x00, 0xff, 0x00, 0x00  ;  2:明るい緑
		DB    0xff, 0xff, 0x00, 0x00  ;  3:明るい黄色
		DB    0x00, 0x00, 0xff, 0x00  ;  4:明るい青
		DB    0xff, 0x00, 0xff, 0x00  ;  5:明るい紫
		DB    0x00, 0xff, 0xff, 0x00  ;  6:明るい水色
		DB    0xff, 0xff, 0xff, 0x00  ;  7:白
		DB    0xc6, 0xc6, 0xc6, 0x00  ;  8:明るい灰色
		DB    0x84, 0x00, 0x00, 0x00  ;  9:暗い赤
		DB    0x00, 0x84, 0x00, 0x00  ; 10:暗い緑
		DB    0x84, 0x84, 0x00, 0x00  ; 11:暗い黄色
		DB    0x00, 0x00, 0x84, 0x00  ; 12:暗い青
		DB    0x84, 0x00, 0x84, 0x00  ; 13:暗い紫
		DB    0x00, 0x84, 0x84, 0x00  ; 14:暗い水色
		DB    0x84, 0x84, 0x84, 0x00  ; 15:暗い灰色
GDT0:
		RESB	8				; ヌルセレクタ
		DW		0xffff,0x0000,0x9200,0x00cf	; 読み書き可能セグメント32bit
		DW		0xffff,0x0000,0x9a28,0x0047	; 実行可能セグメント32bit（bootpack用）

		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
entry_point:
