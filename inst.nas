; fn.asm

[FORMAT "WCOFF"]                
[INSTRSET "i486p"]              
[BITS 32]                       
[FILE "fn.asm"]            ; Name of this file

    GLOBAL _asm_hlt,  _asm_cli,   _asm_sti
    GLOBAL _asm_in8,  _asm_in16,  _asm_in32
    GLOBAL _asm_out8, _asm_out16, _asm_out32
    GLOBAL _asm_load_eflags,      _asm_store_eflags

[SECTION .text]
_asm_hlt:                  ; void asm_hlt(void)
    HLT
    RET

_asm_cli:                  ; void asm_cli(void)
    CLI
    RET

_asm_sti:                  ; void asm_sti(void)
    STI
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
