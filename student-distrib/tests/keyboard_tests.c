#include "keyboard_tests.h"
#include "tests.h"

#include "../devices/keyboard.h"
#include "../lib/lib.h"

/* Keyboard Scancode Tests
 *
 * Asserts that scancode translation works
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: Keyboard interrupt handler
 * Files: keyboard.h/c
 */
int scancode_test(){
	TEST_HEADER();

	// Test some letters
	if(keymap_US[CODE_A] != 'a') return FAIL;
	if(keymap_US[CODE_B] != 'b') return FAIL;
	if(keymap_US[CODE_C] != 'c') return FAIL;
	if(keymap_US[CODE_D] != 'd') return FAIL;
	if(keymap_US[CODE_E] != 'e') return FAIL;

	// Test some numbers
	if(keymap_US[CODE_9] != '9') return FAIL;
	if(keymap_US[CODE_8] != '8') return FAIL;
	if(keymap_US[CODE_7] != '7') return FAIL;
	if(keymap_US[CODE_2] != '2') return FAIL;
	if(keymap_US[CODE_1] != '1') return FAIL;
	if(keymap_US[CODE_0] != '0') return FAIL;

	// Test some special chars
	if(keymap_US[CODE_MINUS] != '-') return FAIL;
	if(keymap_US[CODE_PERIOD] != '.') return FAIL;
	if(keymap_US[CODE_FSLASH] != '/') return FAIL;
	if(keymap_US[CODE_ASTERISK] != '*') return FAIL;

	// Test some non-printed codes
	if(keymap_US[CODE_CTRL] != 0) return FAIL;
	if(keymap_US[CODE_RSHIFT] != 0) return FAIL;
	if(keymap_US[CODE_CAPSLOCK] != 0) return FAIL;
	if(keymap_US[CODE_NUMLOCK] != 0) return FAIL;

	return PASS;
}
