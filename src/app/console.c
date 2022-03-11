#include <boot/boot.h>
#include <boot/gdt.h>
#include <boot/int.h>
#include <event/event.h>
#include <event/keyboard.h>
#include <event/mouse.h>
#include <event/timer.h>
#include <graphics/draw.h>
#include <graphics/layer.h>
#include <memory/memory.h>
#include <stdlib.h>
#include <support/asm.h>
#include <support/debug.h>
#include <support/priority_queue.h>
#include <support/queue.h>
#include <task/task.h>

// TODO: 打印之后窗口周围有残影，成行存在
// TODO: Close button
// TODO:
// 有问题的按键：上下左右显示了数字、tab、shift、capslk不支持
#define COLMAX 80
#define ROWMAX 24
#define CHARW 8
#define CHARH 16
#define WINDOW_TEXT_OFFSET_X 8
#define WINDOW_TEXT_OFFSET_Y 28
#define WINDOW_TITLE_HEIGHT 21

static layer_t *window_console;
static int cursor_x = 0, cursor_y = 0;
static char window_buffer[2][COLMAX];

void console_putchar_at(char ch, int cursor_x, int cursor_y) {
  int x0 = WINDOW_TEXT_OFFSET_X + cursor_x * CHARW;
  int y0 = WINDOW_TEXT_OFFSET_Y + cursor_y * CHARH;
  draw_rect(window_console, RGB_BLACK, x0, y0, x0 + CHARW, y0 + CHARH);
  draw_char(window_console, RGB_WHITE, x0, y0, ch);
}

// Scroll the console up "lines" lines.
void console_scrollup(int lines) { xprintf("console_scrollup()\n"); }
void console_scrolldown(int lines) { xprintf("console_scrolldown()\n"); }

void console_putchar(char ch) {
  console_putchar_at(ch, cursor_x++, cursor_y);
  if (cursor_x >= COLMAX) {
    cursor_x -= COLMAX;
    cursor_y++;
  }
  if (cursor_y >= ROWMAX) {
    console_scrollup(1);
    cursor_y--;
  }
}

void console_backspace(void) {
  int x0 = 8 + cursor_x * 8;
  int y0 = 28 + cursor_y * 16;
  draw_rect(window_console, RGB_BLACK, x0, y0, x0 + 8, y0 + 16);
  cursor_x--;
  if (cursor_x < 0) {
    cursor_x += COLMAX;
    cursor_y--;
  }
  if (cursor_y < 0) {
    console_scrolldown(1);
    cursor_y++;
  }
}

// msg.layer must be a non-NULL window layer.
static inline void check_region(decoded_mouse_msg_t msg, int *in_window,
                                int *in_title_bar) {
  int x0 = msg.layer->x;
  int y0 = msg.layer->y;
  int x1 = x0 + msg.layer->width;
  int y1 = y0 + msg.layer->height;
  int y2 = y0 + WINDOW_TITLE_HEIGHT;
  *in_window = msg.x >= x0 && msg.x < x1 && msg.y >= y0 && msg.y < y1;
  *in_title_bar = msg.x >= x0 && msg.x < x1 && msg.y >= y0 && msg.y < y2;
}

void console_main(void) {
  window_console = make_window(16 + 640, 28 + 9 + 384, "Console");
  layer_move_to(window_console, 300, 130);
  draw_textbox(window_console, 8, 28, 640, 384, RGB_BLACK);
  layer_bring_to_front(window_console);

  const int TIMER_DATA_CURSOR_BLINK = 1;
  add_timer(500, current_proc_node, TIMER_DATA_CURSOR_BLINK);
  // Register mouse event
  process_event_listen(current_proc_node, EVENTNO_MOUSE);
  process_event_listen(current_proc_node, EVENTNO_KEYBOARD);
  decoded_mouse_msg_t last_msg = {0};
  int drag_mode = 0;
  int has_focus = 0;

  int cursor_show = 0; // Toggle every 500 ms?

  for (;;) {
    // Check events
    process_t *proc = get_proc_from_node(current_proc_node);
    queue_t *q;

    // Mouse
    q = &proc->mouse_msg_queue;
    while (!queue_is_empty(q)) {
      decoded_mouse_msg_t msg;
      queue_pop(q, &msg);

      if (msg.control == MOUSE_EVENT_LOSE_CONTROL) {
        has_focus = 0;
        redraw_window_title(msg.layer, "Console", RGB_GRAY_DARK);
      } else if (msg.button[0] && !has_focus && msg.layer) {
        has_focus = 1;
        redraw_window_title(msg.layer, "Console", RGB_CYAN_DARK);
      }

      int in_title_bar = 0, in_window = 0;
      if (msg.button[0] && msg.layer == window_console) {
        check_region(msg, &in_window, &in_title_bar);
      }

      if (!last_msg.button[0] && msg.button[0] && in_window) {
        layer_bring_to_front(msg.layer);
      }

      if (!last_msg.button[0] && msg.button[0] && in_title_bar) {
        drag_mode = 1;
      } else if (!msg.button[0]) {
        drag_mode = 0;
      }

      if (drag_mode) {
        layer_move_by(msg.layer, msg.mx, msg.my);
      }

      last_msg = msg;
    }

    // Keyboard
    q = &proc->keyboard_msg_queue;
    while (!queue_is_empty(q)) {
      int key;
      queue_pop(q, &key);
      // Press
      if (is_pressed_key(key)) {
        if (is_plain_key(key)) {
          char ch = g_keycode_table[key];
          console_putchar(ch);
        } else if (key == KEY_BACKSPACE) {
          console_backspace();
        } else if (key == KEY_UPARROW) {
          xprintf("Up arrow pressed\n");
        }
      }
    }

    // Timer
    q = &proc->timer_msg_queue;
    while (!queue_is_empty(q)) {
      int timer_data;
      queue_pop(q, &timer_data);
      if (timer_data == TIMER_DATA_CURSOR_BLINK) {
        add_timer(250, current_proc_node, TIMER_DATA_CURSOR_BLINK);
        // console_putchar(rand() % 26 + 'A');
        cursor_show = !cursor_show;
      }
    }

    // Update cursor
    int px = 8 + cursor_x * 8;
    int py = 28 + cursor_y * 16;
    draw_rect(window_console, cursor_show ? RGB_WHITE : RGB_BLACK, px, py,
              px + 8, py + 16);
    int wx = window_console->x, wy = window_console->y;
    emit_draw_event(wx + px, wy + py, wx + px + 8, wy + py + 16, 0);

    asm_hlt();
  }
}