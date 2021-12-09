#ifndef KEYBOARD_H
#define KEYBOARD_H

// Public interface for 4x4 keyboard driver

// Key codes
typedef enum {
  KB_0,
  KB_1,
  KB_2,
  KB_3,
  KB_4,
  KB_5,
  KB_6,
  KB_7,
  KB_8,
  KB_9,
  KB_A,
  KB_B,
  KB_C,
  KB_D,
  KB_STAR,
  KB_POUND
} KbKey;


// Initalizes the keyboard (as in lecture slides)
void kb_init();


extern int key_col;
extern int key_row;



#endif // KEYBOARD_H