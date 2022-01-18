#ifndef GUITAR_HERO_LCD_H
#define GUITAR_HERO_LCD_H

// Adapted from the basic LCD driver provided by Marcin Peczarski and Marcin Engel

#include <stdbool.h>
#include <fonts.h>

// symbols exported by the original driver
void LCDconfigure(void);
void LCDclear(void);
void LCDgoto(int textLine, int charPos);
void LCDputchar(char c);
void LCDputcharWrap(char c);

// additional symbols from the basic driver, now exported
void LCDsetFont(const font_t* font);

/* Screen size in pixels, left top corner has coordinates (0, 0). */

#define LCD_PIXEL_WIDTH   128
#define LCD_PIXEL_HEIGHT  160

// new functionality for drawing the game

#define FRET_PRESS_Y 134

typedef enum {
  N_BLUE,
  N_GREEN,
  N_YELLOW,
  N_RED
} NoteColor;

int LCDgetTextWidth();
void LCDsetFullRectangle();
void LCDdrawBoard();
void LCDdrawNote(int col, int y);
void LCDdrawNoteXY(int x, int y, NoteColor color);
void LCDmoveNoteVertical(int col, int oldy, int deltay);
void LCDremoveNote(int col, int y);
void LCDpressFret(int col);
void LCDreleaseFret(int col);
bool LCDisFretPressed(int col);

#endif // GUITAR_HERO_LCD_H
