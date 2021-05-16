#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../lib/lib.h"
#include "../interrupts/interrupts.h"

/* Functions */
void keyboard_irq(int_regs_t context);
void keyboard_init(void);
char shift_char(char c);
char caps_char(char c);

#define KEYBOARD_IRQ 1

/* Scancode translation */
#define N_KEYS 128

/* Keymaps for different keyboard layouts */
extern char keymap_US [N_KEYS];

/* PS2 Constants */
#define PS2_DATA 0x60
#define PS2_CONTROL 0x64

/* PS2 Command/Response codes from https://wiki.osdev.org/PS/2_Keyboard */
#define PS2_ACK 0xFA
#define PS2_RESEND 0xFE
#define PS2_TEST_SUCCESS 0xAA

#define PS2_COMMAND_LED 0xED
#define PS2_COMMAND_SCAN_CODE 0xF0
#define PS2_COMMAND_SET_DEFAULTS 0xF6
#define PS2_COMMAND_RESET 0xFF

/* Scan codes for set 1 from https://wiki.osdev.org/PS/2_Keyboard*/
typedef enum {
    CODE_ESC = 1,
    CODE_1,
    CODE_2,
    CODE_3,
    CODE_4,
    CODE_5,
    CODE_6,
    CODE_7,
    CODE_8,
    CODE_9,
    CODE_0,
    CODE_MINUS,
    CODE_EQUALS,
    CODE_BACKSPACE,
    CODE_TAB,
    CODE_Q,
    CODE_W,
    CODE_E,
    CODE_R,
    CODE_T,
    CODE_Y,
    CODE_U,
    CODE_I,
    CODE_O,
    CODE_P,
    CODE_OBRACKET,
    CODE_CBRACKET,
    CODE_ENTER,
    CODE_CTRL, // CODE_LCTRL and CODE_RCTRL are the same
    CODE_A,
    CODE_S,
    CODE_D,
    CODE_F,
    CODE_G,
    CODE_H,
    CODE_J,
    CODE_K,
    CODE_L,
    CODE_SEMI,
    CODE_APOS,
    CODE_BACKTICK,
    CODE_LSHIFT,
    CODE_BSLASH,
    CODE_Z,
    CODE_X,
    CODE_C,
    CODE_V,
    CODE_B,
    CODE_N,
    CODE_M,
    CODE_COMMA,
    CODE_PERIOD,
    CODE_FSLASH,
    CODE_RSHIFT,
    CODE_ASTERISK,
    CODE_ALT, // CODE_LALT and CODE_RALT are the same
    CODE_SPACE,
    CODE_CAPSLOCK,
    CODE_F1,
    CODE_F2,
    CODE_F3,
    CODE_F4,
    CODE_F5,
    CODE_F6,
    CODE_F7,
    CODE_F8,
    CODE_F9,
    CODE_F10,
    CODE_NUMLOCK,
    CODE_SCROLLLOCK,
    CODE_KEYPAD_7,
    CODE_KEYPAD_8,
    CODE_KEYPAD_9,
    CODE_KEYPAD_MINUS,
    CODE_KEYPAD_4,
    CODE_KEYPAD_5,
    CODE_KEYPAD_6,
    CODE_KEYPAD_PLUS,
    CODE_KEYPAD_1,
    CODE_KEYPAD_2,
    CODE_KEYPAD_3,
    CODE_KEYPAD_0,
    CODE_KEYPAD_PERIOD,
    CODE_KEYPAD_F11 = 0x57,
    CODE_KEYPAD_F12 = 0x58
} scancode1_t;

#endif /* KEYBOARD_H */
