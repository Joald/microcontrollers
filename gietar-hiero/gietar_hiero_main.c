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
  // enable timer2 timing
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;

  TIM5->CR1 = TIM_CR1_URS; // counting up, interrupts only on overflow
  TIM5->PSC = 1;
  TIM5->ARR = 80000; // chosen by trial and error
  TIM5->EGR = TIM_EGR_UG;

  // enable interrupt
  TIM5->SR = ~TIM_SR_UIF;
  TIM5->DIER = TIM_DIER_UIE;

  NVIC_EnableIRQ(TIM5_IRQn);

  // enable timer
  TIM5->CR1 |= TIM_CR1_CEN;
}

uint64_t post_counter = 0;
uint64_t post_counter_max = 1;// << 20;
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
  initKb();
  initLcd();  
  DMA_DBG("LCD INIT DONE\n");
  
  LCDdrawBoard();

  initGameTimer();
  DMA_DBG("GAME TIMER INIT DONE\n");  

  Delay(100000);

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
    }
    for (int i = 1; i <= 4; ++i) {
      if (LCDisFretPressed(i) && !isKeyHeld(KB_ROW_KEY(1) | KB_COL_KEY(i))) {
        handleFretRelease(i);
      }
    }
    int moves = atomic_exchange(&to_move, 0);
    for (int i = 0; i < moves; ++i) {
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