#ifndef ASM_FN_H
#define ASM_FN_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <type.h>

// TODO: Change data types to i_/u_.
void asm_hlt();
void asm_cli();
void asm_sti();
int asm_in8(int port);
int asm_in16(int port);
int asm_in32(int port);
void asm_out8(int port, int data);
void asm_out16(int port, int data);
void asm_out32(int port, int data);
int asm_load_eflags();
void asm_store_eflags(int eflags);
void asm_load_gdtr(u16 limit, u32 addr);
void asm_load_idtr(u16 limit, u32 addr);
void asm_int_handler0x21();
void asm_int_handler0x27();
void asm_int_handler0x2c();

#if (defined(__cplusplus))
}
#endif

#endif
