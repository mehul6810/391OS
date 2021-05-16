#include "keyboard.h"

#include "../interrupts/i8259.h"
#include "terminal.h"

/* State variables */
static int capslock = 0;
static int numlock = 0;
static int shift = 0;
static int ctrl = 0;
static int alt = 0;

// Interesting note: if both shifts are down, the first to go up
//  will NOT cause a RELEASE signal! Only the second one up will.
//  For this reason, we cannot have separate states lshift and rshift

/** keyboard_init
 * DESCRIPTION: Initializes PS/2 keyboard
 */
void keyboard_init(){
    terminal_init();

    // We were told not to worry about sending a reset signal
    // Simply enable interrupts for this device and we are good!
    enable_irq(KEYBOARD_IRQ);
}

/* keyboard_irq
 * DESCRIPTION: Interrupt handler for PS/2 keyboard
 * INPUTS:      context -- the interrupt context (used for terminal switching)
 * RESULT: prints character to screen
 */
void keyboard_irq(int_regs_t context){
    // Read scancode from PS/2 port
    scancode1_t scancode = inb(PS2_DATA);

    // From http://www.osdever.net/bkerndev/Docs/keyboard.htm
    /* If the top bit of the byte we read from the keyboard is
    *  set, that means that a key has just been released */
    if (scancode & 0x80)
    {
        /* You can use this one to see if the user released the
        *  shift, alt, or control keys... */
        switch(scancode & (~0x80)){
            case CODE_LSHIFT:
                shift = 0;
                return;
            case CODE_RSHIFT:
                shift = 0;
                return;
            case CODE_CTRL:
                ctrl = 0;
                return;
            case CODE_ALT:
                alt = 0;
            default:
                return;
        }
    }
    else
    {
        /* Here, a key was just pressed. Please note that if you
        *  hold a key down, you will get repeated key press
        *  interrupts. */

        // Update state if applicable (if so, we are done)
        switch(scancode){
		    case CODE_CAPSLOCK:
                capslock = !capslock;
                return;
            case CODE_NUMLOCK:
                numlock = !numlock;
                return;
            case CODE_LSHIFT:
                shift = 1;
                return;
            case CODE_RSHIFT:
                shift = 1;
                return;
            case CODE_CTRL:
                ctrl = 1;
                return;
            case CODE_ALT:
                alt = 1;
                return;
            default:
                break;
        }

        // Switch terminals on ALT+F#
        if(alt && (scancode == CODE_F1 || scancode == CODE_F2 || scancode == CODE_F3)){
            // EOI is sent within focus_terminal
            focus_terminal(scancode - CODE_F1, context);
            return; // Only reaches here if already focused on this terminal
        }

        // If this is a normal character...
        char c = keymap_US[scancode];

        // Clear screen on CTRL+L
        if(c == 'l' && ctrl){
            terminal_clear_screen();
            return;
        }

        // Tab is a special case
        if(c == '\t'){
            // TODO: print 4 spaces? do autocomplete?
            c = ' ';
        }

        // Print other characters
        if(shift && !capslock){
            c = shift_char(c);
        }
        else if(capslock && !shift){
            c = caps_char(c);
        }

        // Send character to terminal driver
        if(c != '\0'){
            terminal_input(c);
        }
    }
}

/** shift_char
 * DESCRIPTION returns the character to be printed if shift is down
 * INPUT: c -- un-shifted character
 * RETURN VALUE: the character to print
 */
char shift_char(char c){
    if(c >= 'a' && c <= 'z'){
        return c - 32;
    }

    switch(c){
        case '`': return '~';
        case '1': return '!';
        case '2': return '@';
        case '3': return '#';
        case '4': return '$';
        case '5': return '%';
        case '6': return '^';
        case '7': return '&';
        case '8': return '*';
        case '9': return '(';
        case '0': return ')';
        case '-': return '_';
        case '=': return '+';
        case '[': return '{';
        case ']': return '}';
        case '\\': return '|';
        case ';': return ':';
        case '\'': return '\"';
        case ',': return '<';
        case '.': return '>';
        case '/': return '?';
    }

    return c;
}

/** caps_char
 * DESCRIPTION returns the character to be printed if CAPSLOCK is on
 * INPUT: c -- lower-case character
 * RETURN VALUE: the character to print
 */
char caps_char(char c){
    if(c >= 'a' && c <= 'z'){
        return c - 32;
    }

    return c;
}

// From http://www.osdever.net/bkerndev/Docs/keyboard.htm
char keymap_US[N_KEYS] =
{
    0,  0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', /* <-- Tab */
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, /* <-- control key */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};
