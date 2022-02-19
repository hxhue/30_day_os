
[FORMAT "WCOFF"]                
[INSTRSET "i486p"]              
[BITS 32]                       
[FILE "inst.nas"]           ; Name of this file

    GLOBAL _asm_hlt,  _asm_cli,   _asm_sti, _asm_sti_hlt
    GLOBAL _asm_in8,  _asm_in16,  _asm_in32
    GLOBAL _asm_out8, _asm_out16, _asm_out32
    GLOBAL _asm_load_eflags,      _asm_store_eflags
    GLOBAL _asm_load_gdtr,        _asm_load_idtr
    GLOBAL _asm_int_handler0x21,  _asm_int_handler0x27
    GLOBAL _asm_int_handler0x2c,  _asm_load_cr0, _asm_store_cr0

    EXTERN _int_handler0x21, _int_handler0x27, _int_handler0x2c

[SECTION .text]

; x86 default calling convention:
; +----------------------+
; |         ...          |
; +----------------------+
; |        Arg 0         |
; +----------------------+ <---- esp + 4
; |  Return address: 32  |
; +----------------------+ <---- esp
; 
; If the callee uses ebp to create a stack frame:
;   1 push  ebp
;   2 mov   ebp, esp
;   ...
;   3 leave
;   4 ret
; Between line 2 and 3:
; +----------------------+
; |         ...          |
; +----------------------+
; |        Arg 0         |
; +----------------------+ <---- ebp + 8
; |  Return address: 32  |
; +----------------------+ <---- ebp + 4
; |      Saved ebp       | 
; +----------------------+ <---- ebp (now equal to esp)
;
; If an argument less than 4 bytes is passed, 4 bytes are still occupied. 
; (The argument will be promoted to a 4-byte integer.)

_asm_hlt:                  ; void asm_hlt(void)
    HLT
    RET

_asm_cli:                  ; void asm_cli(void)
    CLI
    RET

_asm_sti:                  ; void asm_sti(void)
    STI
    RET

; HLT after STI has a special effect:
; interrupts bewteen them will stop HLT from making CPU sleep.
_asm_sti_hlt:              ; void _asm_sti_hlt(void)
    STI
    HLT
    RET

_asm_in8:                  ; int asm_in8(int port)
    MOV  EDX, [ESP+4]      ; port
    MOV  EAX, 0
    IN   AL,  DX
    RET

_asm_in16:                 ; int asm_in16(int port)
    MOV  EDX, [ESP+4]      ; port
    MOV  EAX, 0
    IN   AX,  DX
    RET

_asm_in32:                 ; int asm_in32(int port)    
    MOV  EDX, [ESP+4]      ; port
    IN   EAX, DX
    RET

_asm_out8:                 ; void asm_out8(int port, int data)
    MOV  EDX, [ESP+4]      ; port
    MOV  AL,  [ESP+8]      ; data
    OUT  DX,  AL
    RET

_asm_out16:                ; void asm_out16(int port, int data)    
    MOV  EDX, [ESP+4]      ; port
    MOV  EAX, [ESP+8]      ; data
	OUT  DX,  AX
    RET

_asm_out32:                ; void asm_out32(int port, int data)
    MOV  EDX, [ESP+4]      ; port
    MOV  EAX, [ESP+8]      ; data
    OUT  DX,  EAX
    RET

_asm_load_eflags:          ; int asm_load_eflags(void)
    PUSHFD
    POP    EAX
    RET

_asm_store_eflags:         ; void asm_store_eflags(int eflags)
    MOV    EAX, [ESP+4]
    PUSH   EAX
    POPFD        
    RET

; When operand is 32-bit, LGDT/LIDT needs a 6-byte argument stored in memory.
; The lower 2 bytes are used as limit, and higher 4 bytes are used as base address.
; Ref: https://www.felixcloutier.com/x86/lgdt:lidt

; x86 uses little endian: lsb is stored at the lowest address
_asm_load_gdtr:            ; void asm_load_gdtr(unsigned short limit, int addr)
    MOV   EAX, [ESP+4]
    MOV   [ESP+6], AX
    LGDT  [ESP+6]
    RET

_asm_load_idtr:            ; void asm_load_idtr(unsigned short limit, int addr)
    MOV   EAX, [ESP+4]
    MOV   [ESP+6], AX
    LIDT  [ESP+6]
    RET

; PUSHAD pushes 8 registers:
; PUSH EAX
; PUSH ECX
; PUSH EDX
; PUSH EBX
; PUSH ESP
; PUSH EBP
; PUSH ESI
; PUSH EDI
; POPAD pops them in the reverse order.

_asm_int_handler0x21:
    PUSH	ES
    PUSH	DS
    PUSHAD
    MOV		EAX,ESP
    PUSH	EAX
    MOV		AX,SS
    MOV		DS,AX
    MOV		ES,AX
    CALL	_int_handler0x21
    POP		EAX
    POPAD
    POP		DS
    POP		ES
    IRETD

_asm_int_handler0x27:
    PUSH	ES
    PUSH	DS
    PUSHAD
    MOV		EAX,ESP
    PUSH	EAX
    MOV		AX,SS
    MOV		DS,AX
    MOV		ES,AX
    CALL	_int_handler0x27
    POP		EAX
    POPAD
    POP		DS
    POP		ES
    IRETD

_asm_int_handler0x2c:
    PUSH	ES
    PUSH	DS
    PUSHAD
    MOV		EAX,ESP
    PUSH	EAX
    MOV		AX,SS
    MOV		DS,AX
    MOV		ES,AX
    CALL	_int_handler0x2c
    POP		EAX
    POPAD
    POP		DS
    POP		ES
    IRETD

_asm_load_cr0:              ; int asm_load_cr0(void)
    MOV     EAX, CR0
    RET

_asm_store_cr0:             ; void asm_store_cr0(u32 cr0)
    MOV     EAX, [ESP+4]
    MOV     CR0, EAX
    RET