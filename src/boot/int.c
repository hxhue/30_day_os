#include <boot/boot.h>
#include <boot/int.h>
#include <event/event.h>
#include <graphics/draw.h>
#include <support/asm.h>
#include <support/type.h>
#include <support/xlibc.h>

#define PORT_KEYDAT           0x0060
#define PORT_KEYSTA           0x0064
#define PORT_KEYCMD           0x0064
#define KEYSTA_SEND_NOT_READY 0x02
#define KEYCMD_WRITE_MODE     0x60
#define KBC_MODE              0x47
#define KEYCMD_SENDTO_MOUSE   0xd4
#define MOUSECMD_ENABLE       0xf4

// kbdc is slow so CPU has to wait.
// But kbdc was fast when I tested it in Qemu.
static inline void wait_kbdc_ready() {
  while (asm_in8(PORT_KEYSTA) & KEYSTA_SEND_NOT_READY) {
    continue;
  }
}

static inline void init_keyboard() {
  wait_kbdc_ready();
  // Send command: set mode (0x60).
  asm_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
  wait_kbdc_ready();
  // Send data: a mode that can use mouse (0x47).
  // Mouse communicates through keyboard control circuit.
  asm_out8(PORT_KEYDAT, KBC_MODE);
}

static inline void init_mouse() {
  wait_kbdc_ready();
  // Send command: transfer data to mouse
  asm_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
  wait_kbdc_ready();
  // Send data: tell mouse to start working
  asm_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
}

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

  asm_sti();                 // Allow interrupts
}

void init_devices() {
  asm_out8(PIC0_IMR, 0xf9); /* 0b11111001, keyboard-1 and PIC1-2 */
  asm_out8(PIC1_IMR, 0xef); /* 0b11101111, mouse-12 */
  init_keyboard();
  init_mouse();
}

/* PS/2 keyboard, 0x20 + 1 (kbd) = 0x21. esp is the 32-bit stack register. */
void int_handler0x21(u32 esp) {
  asm_out8(PIC0_OCW2, 0x60 + 0x1); /* Accept interrupt 0x1 */
  u32 data = asm_in8(PORT_KEYDAT);
  raise_event((event_t){.type = EVENT_KEYBOARD, .data = data});
}

/* PS/2 mouse, 0x20 + 12 (mouse) = 0x2c. esp is the 32-bit stack register. */
void int_handler0x2c(u32 esp) {
  asm_out8(PIC1_OCW2, 0x60 + 0x4); // Tell PIC1 IRQ-12 (8+4) is received 
  asm_out8(PIC0_OCW2, 0x60 + 0x2); // Tell PIC0 IRQ-2 is received 
  u32 data = asm_in8(PORT_KEYDAT);
  raise_event((event_t){.type = EVENT_MOUSE, .data = data});
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
