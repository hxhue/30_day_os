#ifndef ASM_FN_H
#define ASM_FN_H

void asm_hlt();
void asm_cli();
void asm_sti();
int  asm_in8(int port);
int  asm_in16(int port);
int  asm_in32(int port);
void asm_out8(int port, int data);
void asm_out16(int port, int data);
void asm_out32(int port, int data);
int  asm_load_eflags();
void asm_store_eflags(int eflags);

#endif
