#ifndef ASM_H
#define ASM_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <support/type.h>

void asm_hlt();
void asm_cli();
void asm_sti();
void asm_sti_hlt();
u32  asm_in8( u32 port );
u32  asm_in16( u32 port );
u32  asm_in32( u32 port );
void asm_out8( u32 port, u32 data );
void asm_out16( u32 port, u32 data );
void asm_out32( u32 port, u32 data );
u32  asm_load_eflags();
void asm_store_eflags( u32 eflags );
void asm_load_gdtr( u32 limit, u32 addr );
void asm_load_idtr( u32 limit, u32 addr );
void asm_int_handler0x20();
void asm_int_handler0x21();
void asm_int_handler0x27();
void asm_int_handler0x2c();
u32  asm_load_cr0();
void asm_store_cr0( u32 cr0 );
void asm_load_tr(int tr);
void asm_farjmp(int eip, int cs);

#if (defined(__cplusplus))
}
#endif

#endif
