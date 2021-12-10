#ifndef GUITAR_HERO_LCD_H
#define GUITAR_HERO_LCD_H

// Adapted from the basic LCD driver provided by Marcin Peczarski and Marcin Engel

#include <fonts.h>

void LCDconfigure(void);
void LCDclear(void);
void LCDgoto(int textLine, int charPos);
void LCDputchar(char c);
void LCDputcharWrap(char c);

// additional functions made available on top of the basic driver
void LCDsetFont(const font_t *font);

#endif // GUITAR_HERO_LCD_H
