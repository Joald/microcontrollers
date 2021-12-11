#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <assert.h>

// Public interface for 4x4 keyboard driver

// Key code definitions and utilities


// for space efficiency we make keys 1 byte in size
typedef uint8_t kb_num_t;

// the following take key num in 
#define KB_ROW_KEY(n) (kb_num_t)(1 << (n + 3))
#define KB_COL_KEY(n) (kb_num_t)(1 << (n - 1))

#define ROW(n) KB_ROW_KEY(n)
#define COL(n) KB_COL_KEY(n)

// lowest four bits is the row 
// highest four bits is the column 
typedef enum {
  KB_0     = ROW(4) | COL(2),
  KB_1     = ROW(1) | COL(1),
  KB_2     = ROW(1) | COL(2),
  KB_3     = ROW(1) | COL(3),
  KB_4     = ROW(2) | COL(1),
  KB_5     = ROW(2) | COL(2),
  KB_6     = ROW(2) | COL(3),
  KB_7     = ROW(3) | COL(1),
  KB_8     = ROW(3) | COL(2),
  KB_9     = ROW(3) | COL(3),
  KB_A     = ROW(1) | COL(4),
  KB_B     = ROW(2) | COL(4),
  KB_C     = ROW(3) | COL(4),
  KB_D     = ROW(4) | COL(4),
  KB_STAR  = ROW(4) | COL(1),
  KB_POUND = ROW(4) | COL(3),
  KB_NOKEY = (kb_num_t)0xff // all bits set
} KbKey;

static_assert(sizeof(KbKey) == sizeof(kb_num_t), "kbkey takes more than 1 byte");

#undef ROW // don't leak such short macro names
#undef COL 

#define GET_ROW(key) (key >> 4)
#define GET_COL(key) (key & (kb_num_t)0xf)

// unsigned int because that's what __buildin_clz needs
// there are no versions of it for smaller types
// and defining it as inline assembly could be bad for the compiler,
// as well as preventing the usage of the macro inside static_assert
#define GET_NUM(SRC, key) \
  ((8 * sizeof(unsigned int)) - __builtin_clz((unsigned int)GET_##SRC(key)))

// returns row/col number in [1..4]
#define GET_ROW_NUM(key) GET_NUM(ROW, key)
#define GET_COL_NUM(key) GET_NUM(COL, key)

static_assert(GET_ROW_NUM(KB_ROW_KEY(1)) == 1, "bad GET_ROW_NUM calc");



// Keyboard interaction methods

// Initalizes the keyboard (as in lecture slides)
void kb_init();

// Checks if key is held
bool isKeyHeld(KbKey key);

// Gets next key press to detect.
// Holding a key only generates one press
KbKey getNext();


#endif // KEYBOARD_H