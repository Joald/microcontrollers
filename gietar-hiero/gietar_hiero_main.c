#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>

#include <stm32.h>
#include <gpio.h>
#include <fonts.h>
#include <delay.h>

#include "dma_uart.h"
#include "keyboard.h"

// quotation marks include to use the modified driver
#include "lcd.h"

#include "game.h"


void initLcd() {
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

void setTim5Params(int prescaler, int max_value) {
  TIM5->CR1 |= TIM_CR1_UDIS;
  TIM5->PSC = prescaler;
  TIM5->ARR = max_value;
  TIM5->CR1 &= ~TIM_CR1_UDIS;
}

#define LCD_PRINT(str_literal) \
  do { \
    const char* buf = str_literal; \
    for (unsigned i = 0; i < sizeof(str_literal) - 1; ++i) { \
      LCDputchar(buf[i]); \
    } \
  } while (false)

// void updateRedNotePos(int y) {
//   LCDgoto(0, sizeof("Red note y = ") - 1);
//   int divisor = 100;
//   if (y < 0) {
//     divisor /= 10;
//     y = -y;
//     LCDputchar('-');
//   }
  
//   while (divisor > 0) {
//     div_t divv = div(y, divisor);
//     LCDputchar('0' + divv.quot);
//     y = divv.rem;
//     divisor /= 10;
//   }
// }


void initGameTimer() {

  // DMA_DBG("????\n");
  // enable timer2 timing
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
  // DMA_DBG("timing enabled\n");

  TIM5->CR1 = TIM_CR1_URS; // counting up, interrupts only on overflow
  TIM5->PSC = 1; //TODO: 160000 - 1;
  TIM5->ARR = 80000; // 10 ms / (1 / 16 MHz) = 160 000
  TIM5->EGR = TIM_EGR_UG;
  // DMA_DBG("CR1, PSC, ARR, EGR set\n");


  TIM5->SR = ~TIM_SR_UIF;
  TIM5->DIER = TIM_DIER_UIE;
  // DMA_DBG("SR, DIER set\n");

  NVIC_EnableIRQ(TIM5_IRQn);
  // DMA_DBG("NVIC enabled irq\n");
  TIM5->CR1 |= TIM_CR1_CEN;
  // DMA_DBG("timer enabled\n");
}

uint64_t post_counter = 0;
uint64_t post_counter_max = 1;// << 20;
// int red_note_y = 60;
bool fall_on = false;

atomic_int to_move = 0;

extern void TIM5_IRQHandler() {
  uint32_t it_status = TIM5->SR & TIM5->DIER;
  if (it_status & TIM_SR_UIF) {
    TIM5->SR = ~TIM_SR_UIF;
    static bool first_handler = false;
    if (!first_handler) {
      DMA_DBG("First HANDLER!\n");
      first_handler = true;
    }
    if (fall_on && post_counter == 0) {
      // DMA_DBG("Fall on!\n");
      atomic_fetch_add(&to_move, 1);
    }
    if (fall_on) {
      post_counter++;
      post_counter &= post_counter_max - 1;
    }
  }
}

int main() {
  initDmaUart();
  // DMA_DBG("DMA INIT DONE\n");

  // DMA_DBG("KB INIT START\n");
  initKb();
  // DMA_DBG("KB INIT DONE\n");

  // DMA_DBG("LCD INIT\n");
  initLcd();  
  DMA_DBG("LCD INIT DONE\n");
  
  LCDdrawBoard();

  // DMA_DBG("GAME TIMER INIT START\n");  
  initGameTimer();
  DMA_DBG("GAME TIMER INIT DONE\n");  

  Delay(100000);

  // return 0;
  
  // LCD_PRINT("Red note y = 060");
  
  // LCDputcharWrap('-');
  // LCD_PRINT(", Col: ");
  // LCDputcharWrap('-');

  // LCDgoto(1, 0);
  // LCD_PRINT("B===D");
  

  // bool hash_displayed = false;  

  // LCDdrawNote(4, 30);
  // LCDdrawNote(3, 40);
  // LCDdrawNote(2, 50);
  // LCDdrawNote(1, 60);
  spawnNote(1);
  spawnNote(2);
  spawnNote(3);
  spawnNote(4);

  while (true) {
    KbKey key;
    while ((key = getNext()) != KB_NOKEY) {
      if (GET_ROW_NUM(key) == 1) {
        int col = GET_COL_NUM(key);
        handleFretPress(col);
      }
      // if (key == KB_0) {
      //   LCDdrawBoard();
      // }
      if (key == KB_5) {
        fall_on = !fall_on;
        if (fall_on) {
          DMA_DBG("Fall on!\n");
        } else {
          DMA_DBG("Fall off!\n");
        }
      }
      if (key == KB_C && post_counter_max > 1) {
        post_counter_max >>= 1;
        DMA_DBG("Speeding up!\n");
      }
      if (key == KB_D && post_counter_max < 1ull << 62ull) {
        post_counter_max <<= 1;
        DMA_DBG("Slowing down!\n");
      }
      // if (key == KB_8) {
      //   LCDmoveNoteVertical(1, red_note_y, true);
      //   red_note_y--;
      //   updateRedNotePos(red_note_y);
      // }
      // if (key == KB_0) {
      //   LCDmoveNoteVertical(1, red_note_y, false);
      //   red_note_y++;
      //   updateRedNotePos(red_note_y);
      // }
      // if (key == KB_POUND && !hash_displayed) {
      //   LCDgoto(3, 7);
      //   LCDputchar('#');
      //   hash_displayed = true;
      // }
      // LCDgoto(0, 5);
      // LCDputchar('0' + GET_ROW_NUM(key));
      // LCDgoto(0, 13);
      // LCDputchar('0' + GET_COL_NUM(key));
    } 
    // if (!isKeyHeld(KB_1) && hash_displayed) {
    //   LCDgoto(3, 7);
    //   LCDputchar(' ');        
    //   hash_displayed = false;
    // }
    for (int i = 1; i <= 4; ++i) {
      if (LCDisFretPressed(i) && !isKeyHeld(KB_ROW_KEY(1) | KB_COL_KEY(i))) {
        handleFretRelease(i);
      }
    }
    int moves = atomic_exchange(&to_move, 0);
    for (int i = 0; i < moves; ++i) {
      // LCDmoveNoteVertical(1, red_note_y, false);
      // red_note_y++;
      // if (red_note_y == LCD_PIXEL_HEIGHT + 5) {
      //   red_note_y = -30;
      // }
      // updateRedNotePos(red_note_y);
      moveNotes();
    }
    Delay(10000);
  }
}

/** TODO:
 X Make note drawing/moving respect edges
 X figure out which y is ok for fret press
 X implement fret pressing
 X improve note edge visuals
 X add timer for moving and key for spawning notes
 X   (use keys to adjust speed)
 *   (find best speed?)
 ? combine the two to play notes
 * then communicate with laptop to get some actual "music"
 * and add basic scoring
 * STRETCH:
 * maybe add loading screen, menu etc.
 * use received data from computer to "play" sounds to a diode
 * get a speaker to 
 */