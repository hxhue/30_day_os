#include <boot/int.h>
#include <event/mouse.h>
#include <event/keyboard.h>
#include <event/timer.h>
#include <support/asm.h>
#include <support/type.h>
#include <support/debug.h>
#include <string.h>
#include <boot/def.h>

typedef struct gate_descriptor_t {
  u16 offset_low, selector;
  u8 dw_count, access;
  u16 offset_high;
} gate_descriptor_t;

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

  asm_out8(PIC0_IMR, 0xfb);  /* PIC0: 11111011: allow PIC1 */
  asm_out8(PIC1_IMR, 0xff);  /* PIC1: Allow none */
}

// NOTE:
// emit_*() are constant-time functions and are safe to call in interrupt 
// handlers.

void int_handler0x20(u32 esp) {
  asm_out8(PIC0_OCW2, 0x60 + 0x0); /* Accept interrupt 0x0 */
  ++g_counter.count;
}

/* PS/2 keyboard, 0x20 + 1 (kbd) = 0x21. esp is the 32-bit stack register. */
void int_handler0x21(u32 esp) {
  asm_out8(PIC0_OCW2, 0x60 + 0x1); /* Accept interrupt 0x1 */
  u32 data = asm_in8(PORT_KEYDAT);
  emit_keyboard_event(data);
}

/* PS/2 mouse, 0x20 + 12 (mouse) = 0x2c. esp is the 32-bit stack register. */
void int_handler0x2c(u32 esp) {
  asm_out8(PIC1_OCW2, 0x60 + 0x4); // Tell PIC1 IRQ-12 (8+4) is received 
  asm_out8(PIC0_OCW2, 0x60 + 0x2); // Tell PIC0 IRQ-2 is received 
  u32 data = asm_in8(PORT_KEYDAT);

  // Every mouse event has 3 bytes. Since the port is 8 bit, a mouse event will
  // cause 3 interrupts. The first byte a mouse will receive is 0xfa meaning
  // that mouse initialization is ready.
  static int mouse_state = 3;
  static mouse_msg_t msg = {{0}};

  // xprintf("Received IRQ-12: case %d, data %d\n", mouse_state, data);

  switch(mouse_state) {
    case 0:
      // Move bits:  0x0~0x3
      // Click bits: 0x8~0xf
      if ((data & 0xc8) == 0x08)
        msg.buf[mouse_state++] = data;
      break;
    case 1:
      msg.buf[mouse_state++] = data;
      break;
    case 2:
      msg.buf[2] = data;
      mouse_state = 0;
      emit_mouse_event(msg);
      break;
    case 3:
      if (data == 0xfa)
        mouse_state = 0;
      break;
    default:
      xassert(!"Unreachable");
      break;
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

void init_idt() {
  /* IDT */
  gate_descriptor_t *idt = (gate_descriptor_t *)0x0026f800;
  memset(idt, 0, IDT_LIMIT + 1);
  asm_load_idtr(IDT_LIMIT, 0x0026f800);
  set_idt_entry(idt + 0x20, (u32)asm_int_handler0x20, 2 * 8, ACC_INT_ENTRY);
  set_idt_entry(idt + 0x21, (u32)asm_int_handler0x21, 2 * 8, ACC_INT_ENTRY);
  set_idt_entry(idt + 0x27, (u32)asm_int_handler0x27, 2 * 8, ACC_INT_ENTRY);
  set_idt_entry(idt + 0x2c, (u32)asm_int_handler0x2c, 2 * 8, ACC_INT_ENTRY);
}

// Initialize IDT, PIC, and allow interrupts.
// External interrupts (devices) are not enabled by now.
void init_interrupt() {
  init_idt();
  init_pic();
  asm_sti();
}
