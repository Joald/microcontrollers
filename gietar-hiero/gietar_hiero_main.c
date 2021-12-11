#include <stdbool.h>
#include <stm32.h>

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
  
  // LCD_PRINT("Row: ");
  
  // LCDputcharWrap('-');
  // LCD_PRINT(", Col: ");
  // LCDputcharWrap('-');

  // LCDgoto(1, 0);
  // LCD_PRINT("B===D");
  

  bool hash_displayed = false;  

  LCDdrawNote(4, 30, N_BLUE);
  LCDdrawNote(3, 40, N_GREEN);
  LCDdrawNote(2, 50, N_YELLOW);
  LCDdrawNote(1, 60, N_RED);

  int red_note_y = 60;  

  while (true) {
    KbKey key;
    while ((key = getNext()) != KB_NOKEY) {
      if (key == KB_1) {
        DMA_DBG("Found key 1!\n");
      }
      if (GET_ROW_NUM(key) == 1) {
        DMA_DBG("ROW_NUM = 1!\n");
      }
      if (GET_COL_NUM(key) == 1) {
        DMA_DBG("COL_NUM = 1!\n");
      }
      if (key == KB_2) {
        LCDmoveNoteVertical(1, red_note_y, true, N_RED);
        red_note_y--;
      }
      if (key == KB_8) {
        LCDmoveNoteVertical(1, red_note_y, false, N_RED);
        red_note_y++;
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

    Delay(1000000);
  }
}

/** TODO:
 X Make note drawing/moving respect edges
 * figure out which y is ok for fret press
 * improve note edge visuals
 * implement fret pressing
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