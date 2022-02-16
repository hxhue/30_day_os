#include <boot/boot_info.h>
#include <boot/int.h>
#include <graphics/draw.h>
#include <inst.h>
#include <assert.h>
#include <type.h>

/**
 * CPU has only 1 port.
 * x86 has 2 PIC, and 15 external interrupts.
 * CPU <-- PIC0: 0
 *               1
 *               2 <-- PIC1: 0~7
 *               3
 *               4
 *               5
 *               6
 *               7
 * Every PIC port has a responding interrupt request (IRQ).
 * It's numbered 0~15. 0~7 for PIC0, 8~15 for PIC1. IRQ 2 is not used.
 */

void init_pic() {
  /* Ignore all external interrupts (IMR: Interrupt mask register) */
  asm_out8(PIC0_IMR, 0xff);
  asm_out8(PIC1_IMR, 0xff);

  /* Set PIC initial control word. The order matters. */
  asm_out8(PIC0_ICW1, 0x11); /* edge trigger mode */
  asm_out8(PIC0_ICW2, 0x20); /* Register IRQ 0~7 to int 0x20~0x27 */
  asm_out8(PIC0_ICW3, 4);    /* Connect PIC1 to IRQ2 */
  asm_out8(PIC0_ICW4, 0x01); /* No buffer */

  asm_out8(PIC1_ICW1, 0x11); /* edge trigger mode */
  asm_out8(PIC1_ICW2, 0x28); /* Register IRQ 8~15 to int 0x28~0x2f */
  asm_out8(PIC1_ICW3, 2);    /* No idea... */
  asm_out8(PIC1_ICW4, 0x01); /* No buffer */

  asm_out8(PIC0_IMR, 0xfb); /* PIC0: 11111011: allow PIC1 */
  asm_out8(PIC1_IMR, 0xff); /* PIC1: Allow none */
}

/* PS/2 keyboard, 0x20 + 1 (keyboard) = 0x21. esp is the 32-bit stack register.
 */
void int_handler0x21(u32 esp) {
  fill_rect(RGB_BLACK, 0, 0, g_boot_info.width, 16);
  put_string(RGB_WHITE, 0, 0, "INT 21 (IRQ-1) : PS/2 keyboard");
  for (;;) {
    asm_hlt();
  }
}

/* PS/2 mouse, 0x20 + 12 (mouse) = 0x2c. esp is the 32-bit stack register. */
void int_handler0x2c(u32 esp) {
  fill_rect(RGB_BLACK, 0, 0, g_boot_info.width, 16);
  put_string(RGB_WHITE, 0, 0, "INT 2C (IRQ-12) : PS/2 mouse");
  for (;;) {
    asm_hlt();
  }
}

/* PIC0からの不完全割り込み対策
 * Athlon64X2機などではチップセットの都合によりPICの初期化時にこの割り込みが1度だけおこる
 * この割り込み処理関数は、その割り込みに対して何もしないでやり過ごす
 * なぜ何もしなくていいの？->
 * この割り込みはPIC初期化時の電気的なノイズによって発生したものなので、
 * まじめに何か処理してやる必要がない。
 */
void int_handler0x27(u32 esp) {
  asm_out8(PIC0_OCW2, 0x67); /* IRQ-07受付完了をPICに通知(7-1参照) */
}
