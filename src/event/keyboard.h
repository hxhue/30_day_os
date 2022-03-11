#ifndef KEYBOARD_H
#define KEYBOARD_H

#if (defined(__cplusplus))
extern "C" {
#endif

#include <event/event.h>

extern event_queue_t g_keyboard_event_queue;
extern const char g_keycode_table[0x54];
extern const char g_keycode_shift_table[0x54];

void emit_keyboard_event(unsigned data);
void init_keyboard_event_queue();
void wait_kbdc_ready();
void init_keyboard();

#define KEY_ESCAPE          0X01
#define KEY_BACKSPACE       0X0E
#define KEY_TAB             0X0F
#define KEY_ENTER           0X1C
#define KEY_LEFTCTRL        0X1D
#define KEY_LEFTSHIFT       0X2A
#define KEY_RIGHTSHIFT      0X36
#define KEY_LEFTALT         0X38
#define KEY_CAPSLOCK        0X3A
#define KEY_F1              0X3B
#define KEY_F2              0X3C
#define KEY_F3              0X3D
#define KEY_F4              0X3E
#define KEY_F5              0X3F
#define KEY_F6              0X40
#define KEY_F7              0X41
#define KEY_F8              0X42
#define KEY_F9              0X43
#define KEY_F10             0X44
#define KEY_NUMLOCK         0X45
#define KEY_SCROLLLOCK      0X46
#define KEY_F11             0X57
#define KEY_F12             0X58
#define KEY_RIGHTALT        0XE038
#define KEY_RIGHTCTRL       0XE01D
#define KEY_INSERT          0XE052
#define KEY_DELETE          0XE053
#define KEY_LEFTARROW       0XE04B
#define KEY_HOME            0XE047
#define KEY_END             0XE04F
#define KEY_UPARROW         0XE048
#define KEY_DOWNARROW       0XE050
#define KEY_PAGEUP          0XE049
#define KEY_PAGEDOWN        0XE051
#define KEY_RIGHTARROW      0XE04D
#define KEY_KP_SLASH        0XE035 // KP is for keypad
#define KEY_KP_ENTER        0XE01C

#define KEY_PRTSC           0XE02A37
#define KEY_PAUSE           0XE20000

// Higher 8 bits are for flags.
// Lower 24 bits are for key identities.
#define KEY_RELEASE_FLAG    0X80000000
#define KEY_CAPITAL_FLAG    0X40000000
#define KEY_SHIFT_FLAG      0X20000000

// Processes the new byte and returns the result if there is one.
// Since many keys are composed of multiple keycodes, process_keycode() may not
// instantly returns a key, so 0 is returned instead. If the key is released, 
// sign bit (31th bit of an int value) of the return value is set. 
// This function is only used by PS/2 keyboard interruption handler.
int process_keycode(int keycode);

static inline int is_plain_key(int key) {
  key &= 0x00ffffff;
  return key >= 0 && key < 0x54 && g_keycode_table[key];
}

// Returns the visible char represented by key when is_plain_key(key) == true.
// Otherwise, 0 is returned. During translatoin, A capital flag toggles a
// letter's case. A shift flag toggles the character's case when it's a letter
// and replaces the character by looking up in the backup keycode table
// otherwise. Other flags are ignored.
char to_plain_char(int key);

static inline int is_released_key(int key) {
  return key & KEY_RELEASE_FLAG;
}

static inline int is_pressed_key(int key) {
  return !is_released_key(key);
}

// Ignores all flags and only compares keys' identities.
static inline int key_equal(int k1, int k2) {
  return (k1 & 0x00ffffff) == (k2 & 0x00ffffff);
}

#if (defined(__cplusplus))
}
#endif

#endif

