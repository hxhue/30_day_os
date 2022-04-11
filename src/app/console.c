#include "memory/memory.h"
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
#include <ctype.h>

// TODO: 打印之后窗口周围有残影，成行存在
// TODO: Close button
// TODO: 对control set 1中的所有按键的支持
#define COLMAX               80
#define ROWMAX               24
#define CHARW                 8
#define CHARH                16
#define TEXT_OFFSET_X         8
#define TEXT_OFFSET_Y        28
#define WINDOW_TITLE_HEIGHT  21
#define CHAR_BOTTOM_MARGIN    3
#define CHAR_RIGHT_MARGIN     1
#define LINE_MAX_CHARNUM   2048
#define TIMER_DATA_CURSOR_BLINK 1

static layer_t *window_console;
static int cursor_x = 0, cursor_y = 0;
static char currentline[LINE_MAX_CHARNUM];
static int currentline_pos = 0;

const char *shell_exec(const char *cmd);
int shell_split(char *cmd, int *pargc, char ***pargv);
char *echo(int argc, char **argv);
void console_backspace(void);
void console_blink_cursor(int visible);

void console_drawchar(char ch, int cursor_x, int cursor_y) {
  int x0 = TEXT_OFFSET_X + cursor_x * (CHARW + CHAR_RIGHT_MARGIN);
  int y0 = TEXT_OFFSET_Y + cursor_y * (CHARH + CHAR_BOTTOM_MARGIN);
  draw_rect(window_console, RGB_BLACK, x0, y0, x0 + CHARW, y0 + CHARH);
  draw_char(window_console, RGB_WHITE, x0, y0, ch);
}

// Scroll the console up "lines" lines.
void console_scrollup(int lines) { xprintf("console_scrollup()\n"); }

// Scroll the console down "lines" lines.
void console_scrolldown(int lines) { xprintf("console_scrolldown()\n"); }

void console_newline(void) {
  console_blink_cursor(0);
  cursor_x = 0;
  if (++cursor_y >= ROWMAX) {
    console_scrollup(1);
    --cursor_y;
  }
}

void console_putchar(char ch) {
  if (ch == '\n') {
    console_newline();
  } else if (ch == '\b') {
    console_backspace();
  } else {
    console_drawchar(ch, cursor_x++, cursor_y);
    if (cursor_x >= COLMAX) {
      cursor_x -= COLMAX;
      cursor_y++;
    }
    if (cursor_y >= ROWMAX) {
      console_scrollup(1);
      cursor_y--;
    }
  }
}

void console_putstr(const char *s) {
  const char *p;
  for (p = s; *p; ++p) {
    console_putchar(*p);
  }
}

void console_backspace(void) {
  if (currentline_pos > 0) {
    currentline_pos--;
    int x0 = TEXT_OFFSET_X + cursor_x * (CHARW + CHAR_RIGHT_MARGIN);
    int y0 = TEXT_OFFSET_Y + cursor_y * (CHARH + CHAR_BOTTOM_MARGIN);
    draw_rect(window_console, RGB_BLACK, x0, y0, x0 + CHARW, y0 + CHARH);
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
}

// Returns 0 only on success.
int console_currentline_push(char ch) {
  if (currentline_pos + 1 < LINE_MAX_CHARNUM) {
    currentline[currentline_pos++] = ch;
    return 0;
  }
  return -1;
}

void console_prompt() {
  console_putstr("> ");
}

void console_commit(void) {
  currentline[currentline_pos] = '\0';
  // size_t len = currentline_pos;
  char *cmd = currentline;
  const char *result;
  currentline_pos = 0;
  xprintf("[INFO] Executing: `%s`\n", cmd);
  if ((result = shell_exec(cmd))) {
    console_putstr(result);
    reclaim((void *)result);
  } else {
    // xprintf("[ERR ] Invalid command.\n");
    console_putstr("[ERR ] Invalid command.\n");
  }
  console_prompt();
}

void console_blink_cursor(int visible) {
  int x0 = TEXT_OFFSET_X + cursor_x * (CHARW + CHAR_RIGHT_MARGIN);
  int y0 = TEXT_OFFSET_Y + cursor_y * (CHARH + CHAR_BOTTOM_MARGIN);
  int x1 = x0 + CHARW;
  int y1 = y0 + CHARH;
  int wx = window_console->x, wy = window_console->y;
  draw_rect(window_console, visible ? RGB_WHITE : RGB_BLACK, x0, y0, x1, y1);
  emit_draw_event(wx + x0, wy + y0, wx + x1, wy + y1, 0);
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

static void init_window(void) {
  int textw = COLMAX * (CHARW + CHAR_RIGHT_MARGIN);
  int texth = ROWMAX * (CHARH + CHAR_BOTTOM_MARGIN);
  int winw = TEXT_OFFSET_X * 2 + textw;
  int winh = TEXT_OFFSET_Y + 9 + texth;
  window_console = make_window(winw, winh, "Console");
  layer_move_to(window_console, (g_boot_info.width - winw) / 2,
                (g_boot_info.height - winh) / 2);
  draw_textbox(window_console, TEXT_OFFSET_X, TEXT_OFFSET_Y, textw, texth,
               RGB_BLACK);
  layer_bring_to_front(window_console);
}

void console_main(void) {
  init_window();
  add_timer(500, current_proc_node, TIMER_DATA_CURSOR_BLINK);
  // Register mouse event
  process_event_listen(current_proc_node, EVENTNO_MOUSE);
  process_event_listen(current_proc_node, EVENTNO_KEYBOARD);
  decoded_mouse_msg_t last_msg = {0};
  int drag_mode = 0;
  int has_focus = 0;

  int cursor_is_visible = 0; // Toggle every 500 ms?

  console_prompt();

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
      if (is_pressed_key(key)) {
        char ch;
        if ((ch = to_plain_char(key))) {
          if (console_currentline_push(ch) == 0) {
            console_putchar(ch);
          }
          // xprintf("key: 0X%08X, char: %c\n", key, ch);
        } else if (key_equal(key, KEY_BACKSPACE)) {
          console_backspace();
        } else if (key_equal(key, KEY_ENTER)) {
          // xprintf("Enter is pressed\n");
          console_newline();
          console_commit();
        }
      }
      // xprintf(">>> key: 0X%08X\n", key);
    }

    // Timer
    q = &proc->timer_msg_queue;
    while (!queue_is_empty(q)) {
      int timer_data;
      queue_pop(q, &timer_data);
      if (timer_data == TIMER_DATA_CURSOR_BLINK) {
        add_timer(250, current_proc_node, TIMER_DATA_CURSOR_BLINK);
        // console_putchar(rand() % 26 + 'A');
        cursor_is_visible = !cursor_is_visible;
      }
    }

    // Update cursor
    console_blink_cursor(cursor_is_visible);

    asm_hlt();
  }
}

char *echo(int argc, char **argv) {
  size_t len = 1;                /* One more byte for '\n' */
  int i;
  for (i = 1; i < argc; ++i) {
    len += strlen(argv[i]) + 1;  /* One more byte for ' ' */
  }
  char *result = alloc(len + 1); /* One more byte for '\0' */
  char *dst = result;
  for (i = 1; i < argc; ++i) {
    const char *src = argv[i];
    while (*src) {
      *dst++ = *src++;
    }
    *dst++ = ' ';
  }

  if (dst != result) {
    dst[-1] = '\n';       /* Replace trailing ' ' with '\n' */
  }

  *dst++  = '\0';
  return result;
}

// cmd:   Command to split. '\0' will be inserted between words.
// argc:  shell_split() will store the argument count in argc.
// argv:  shell_split() will store the pointer to argument vector in argv.
//        Caller must use reclaim() to free the memory of argv.
//        All pointers inside argv are originally from cmd. So cmd should not
//        be freed when argv is being used.
// Returns 0 if and only if no error happens.
// TODO: Support "" and ''. (Now spaces are always splitting words.)
int shell_split(char *cmd, int *pargc, char ***pargv) {
  int argmax = 8;
  int argc = 0;
  char **argv = alloc(argmax * sizeof(char *));
  while (*cmd) {
    while (*cmd && isspace(*cmd)) {
      *cmd++ = '\0';
    }
    if (*cmd) {
      if (argc >= argmax) {
        argmax <<= 1;
        char **newargv = alloc(argmax * sizeof(char *));
        memcpy(newargv, argv, argc * sizeof(char *));
        reclaim(argv);
        argv = newargv;
      }
      argv[argc++] = cmd;
      while (*cmd && !isspace(*cmd)) {
        cmd++;
      }
    }
  }
  *pargc = argc;
  *pargv = argv;
  return 0;
}

// Try executing cmd. If it succeeded, a dynamically allocated string is
// returned (should be freed by reclaim()). Otherwise, NULL is returned.
const char *shell_exec(const char *command) {
  int argc; 
  char **argv;
  char *result = NULL;
  char *cmd = strdup(command);
  if (shell_split(cmd, &argc, &argv) == 0) {
    if (argc == 0) {
      result = alloc(1);
      *result = '\0';
    } else if (argc > 0 && strcmp("echo", argv[0]) == 0) {
      result = echo(argc, argv);
    }
    reclaim(argv);
  }
  reclaim(cmd);
  return result;
}
