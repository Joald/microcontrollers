#include <stdbool.h>
#include <stm32.h>
#include <stdlib.h>

#include <gpio.h>
#include <fonts.h>
#include <delay.h>

#include "dma_uart.h"
#include "keyboard.h"

// quotation marks include to use the modified driver
#include "lcd.h"


void lcd_init() {
  LCDconfigure();
  LCDsetFont(&font8x16);
  LCDclear();
  LCDgoto(0, 0);
  // #define STR "Keyboard initialized!"
  // const char* buf = STR;
  // for (unsigned i = 0; i < sizeof(STR); ++i) {
  //   LCDputcharWrap(buf[i]);
  // }
}

void setTim3Params(int prescaler, int max_value) {
  TIM3->CR1 |= TIM_CR1_UDIS;
  TIM3->PSC = prescaler;
  TIM3->ARR = max_value;
  TIM3->CR1 &= ~TIM_CR1_UDIS;
}

#define LCD_PRINT(str_literal) \
  do { \
    const char* buf = str_literal; \
    for (unsigned i = 0; i < sizeof(str_literal) - 1; ++i) { \
      LCDputchar(buf[i]); \
    } \
  } while (false)

void updateRedNotePos(int red_note_y) {
  LCDgoto(0, sizeof("Red note y = ") - 1);
  int divisor = 100;
  for (int i = 0; i < 3; ++i) {
    div_t divv = div(red_note_y, divisor);
    LCDputcharWrap('0' + divv.quot);
    red_note_y = divv.rem;
    divisor /= 10;
  }
}

#define FRET_PRESS_Y 134

int main() {
  initDmaUart();
  DMA_DBG("DMA INIT DONE\n");

  DMA_DBG("KB INIT START\n");
  kb_init();
  DMA_DBG("KB INIT DONE\n");
  
  DMA_DBG("LCD INIT\n");
  lcd_init();  
  DMA_DBG("LCD INIT DONE\n");
  // Delay(100000);

  LCDdrawBoard();
  
  // LCD_PRINT("Red note y = 060");
  
  // LCDputcharWrap('-');
  // LCD_PRINT(", Col: ");
  // LCDputcharWrap('-');

  // LCDgoto(1, 0);
  // LCD_PRINT("B===D");
  

  bool hash_displayed = false;  

  LCDdrawNote(4, 30);
  LCDdrawNote(3, 40);
  LCDdrawNote(2, 50);
  LCDdrawNote(1, 60);

  int red_note_y = 60;

  bool col_pressed[5] = {};

  while (true) {
    KbKey key;
    while ((key = getNext()) != KB_NOKEY) {
      if (GET_ROW_NUM(key) == 1) {
        int col = GET_COL_NUM(key);
        col_pressed[col] = true;
        LCDdrawNote(col, FRET_PRESS_Y);
      }
      if (key == KB_C) {
        LCDmoveNoteVertical(1, red_note_y, true);
        red_note_y--;
        // updateRedNotePos(red_note_y);
      }
      if (key == KB_D) {
        LCDmoveNoteVertical(1, red_note_y, false);
        red_note_y++;
        // updateRedNotePos(red_note_y);
      }
      if (key == KB_POUND && !hash_displayed) {
        LCDgoto(3, 7);
        LCDputchar('#');
        hash_displayed = true;
      }
      // LCDgoto(0, 5);
      // LCDputchar('0' + GET_ROW_NUM(key));
      // LCDgoto(0, 13);
      // LCDputchar('0' + GET_COL_NUM(key));
    } 
    if (!isKeyHeld(KB_POUND) && hash_displayed) {
      LCDgoto(3, 7);
      LCDputchar(' ');        
      hash_displayed = false;
    }
    for (int i = 1; i <= 4; ++i) {
      if (col_pressed[i] && !isKeyHeld(KB_ROW_KEY(1) | KB_COL_KEY(i))) {
        LCDremoveNote(i, FRET_PRESS_Y);
      }
    }

    Delay(10000);
  }
}

/** TODO:
 X Make note drawing/moving respect edges
 X figure out which y is ok for fret press
 X implement fret pressing
 * improve note edge visuals
 * add timer for moving and key for spawning notes
 *   (use keys to adjust speed)
 * combine the two to play notes
 * then communicate with laptop to get some actual "music"
 * and add basic scoring
 * STRETCH:
 * maybe add loading screen, menu etc.
 * use received data from computer to "play" sounds to a diode
 * get a speaker to 
 */