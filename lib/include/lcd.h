#ifndef GUITAR_HERO_LCD_H
#define GUITAR_HERO_LCD_H

// Adapted from the basic LCD driver provided by Marcin Peczarski and Marcin Engel

#include <fonts.h>

// functions exported by the original driver
void LCDconfigure(void);
void LCDclear(void);
void LCDgoto(int textLine, int charPos);
void LCDputchar(char c);
void LCDputcharWrap(char c);

// additional functions from the basic driver, now exported
void LCDsetFont(const font_t *font);

// new functionality for drawing the game

typedef enum {
  N_BLUE,
  N_GREEN,
  N_YELLOW,
  N_RED
} NoteColor;


void LCDdrawBoard();
void LCDdrawNote(int col, int y, NoteColor color);
void LCDdrawNoteXY(int x, int y, NoteColor color);
void LCDmoveNoteVertical(int col, int oldy, bool up, NoteColor color);

#endif // GUITAR_HERO_LCD_H
