#include <boot/desctbl.h>
#include <support/asm.h>
#include <string.h>
#include <support/type.h>

/**
 * IDT      : 0x26f800～0x26ffff
 * GDT      : 0x270000～0x27ffff
 * BOOTSTRAP: 0x280000～0x2fffff
 */

/* A segment register's higher 13 bits can be used, so the max possible segment
 * count is 8192. */
#define SEGMENT_COUNT 8192
#define INTERRUPT_COUNT 256
#define GDT_LIMIT 0x0000ffff /* 8192 * 8 - 1 */
#define IDT_LIMIT 0x000007ff
#define ACC_INT_ENTRY 0x008e

/* Inits Global segment descriptor table and Interrupt descriptor table. */
void init_descriptor_tables() {
  /* GDT */
  segment_descriptor_t *gdt = (segment_descriptor_t *)0x00270000;
  memset(gdt, 0, GDT_LIMIT + 1);
  asm_load_gdtr(GDT_LIMIT, 0x00270000);
  set_gdt_entry(gdt + 1, 0xffffffff, 0,          0x92, 0x04);
  set_gdt_entry(gdt + 2, 0x0007ffff, 0x00280000, 0x9a, 0x04);

  /* IDT */
  gate_descriptor_t *idt = (gate_descriptor_t *)0x0026f800;
  memset(idt, 0, IDT_LIMIT + 1);
  asm_load_idtr(IDT_LIMIT, 0x0026f800);
  set_idt_entry(idt + 0x21, (u32)asm_int_handler0x21, 2 * 8, ACC_INT_ENTRY);
  set_idt_entry(idt + 0x27, (u32)asm_int_handler0x27, 2 * 8, ACC_INT_ENTRY);
  set_idt_entry(idt + 0x2c, (u32)asm_int_handler0x2c, 2 * 8, ACC_INT_ENTRY);
}

/* Argument "limit" (sizeof(segment) - 1) and "base" support 32 bits.
 * Argument "access" only has its lower 20 bits in use.
 *
 * Unit of argument "limit" is byte.
 *
 * Field "limit" in entry has only 3 bytes (limit_low: 2 and limit_high: 1),
 * which are not enough to represent all 32-bit addresses. G_bit (Granularity
 * Bit) makes unit of "limit" a page of 4 KB instead of 1 byte, so only 20
 * bits are needed (, and we have 24 bits).
 */
void set_gdt_entry(segment_descriptor_t *entry, u32 limit, u32 base, u8 ar,
                   u8 flag) {
  if (limit > 0xfffff) {
    flag |= 0x08; /* G_bit = 1 */
    limit >>= 12;
  } else {
    flag &= 0xf7; /* G_bit = 0 */ 
  }
  /* limit: 20 bits */
  entry->limit_low = limit & 0xffff;
  entry->limit_high = (limit >> 16) & 0x0f;
  /* base: 32 bits */
  entry->base_low = base & 0xffff;
  entry->base_mid = (base >> 16) & 0xff;
  entry->base_high = (base >> 24) & 0xff;
  /**
   * access: 8 bits: 0xMT
   * M: 9 => sys;  f => user
   * T: 2 => data; a => code    
   */
  entry->access = ar & 0xff;
  /**
   * flag: 4 bits: gd__
   * g: 1 => Use 4 KB as limit unit; 0 => 1 byte as unit
   * d: 1 => 32-bit;                 0 => 16-bit          
   */
  entry->flag = flag & 0x0f;
}

/**
 * offset: 32-bit address of handler function
 * selector: n << 3: Segment n is where handler can be found.
 */
void set_idt_entry(gate_descriptor_t *entry, u32 offset, u32 selector, u16 ar) {
  entry->offset_low = offset & 0xffff;
  entry->offset_high = (offset >> 16) & 0xffff;
  entry->access = ar & 0xff;
  entry->dw_count = (ar >> 8) & 0xff;
  entry->selector = selector;
}
