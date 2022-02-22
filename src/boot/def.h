#ifndef PORT_H
#define PORT_H

#if (defined(__cplusplus))
extern "C" {
#endif

#define PORT_KEYDAT 0x0060
#define PORT_KEYSTA 0x0064
#define PORT_KEYCMD 0x0064

#define PIC0_ICW1   0x0020
#define PIC0_OCW2   0x0020
#define PIC0_IMR    0x0021
#define PIC0_ICW2   0x0021
#define PIC0_ICW3   0x0021
#define PIC0_ICW4   0x0021
#define PIC1_ICW1   0x00a0
#define PIC1_OCW2   0x00a0
#define PIC1_IMR    0x00a1
#define PIC1_ICW2   0x00a1
#define PIC1_ICW3   0x00a1
#define PIC1_ICW4   0x00a1

// Only the higher 13 bits of a segment register's can be used,
// so the max possible segment count is 8192.
#define GDT_LIMIT     0x0000ffff // 8192 * 8 - 1
#define IDT_LIMIT     0x000007ff // 256  * 8 - 1
#define ACC_INT_ENTRY 0x008e

#define KEYSTA_SEND_NOT_READY 0x02
#define KEYCMD_WRITE_MODE     0x60
#define KBC_MODE              0x47
#define KEYCMD_SENDTO_MOUSE   0xd4
#define MOUSECMD_ENABLE       0xf4

// PIT: Programmable Interval Counter
#define PIT_CTRL              0x0043
#define PIT_CNT0              0x0040

// TODO: remove counter_t
typedef struct counter_t {
  unsigned count;
} counter_t;

extern counter_t g_counter;

#if (defined(__cplusplus))
}
#endif

#endif
