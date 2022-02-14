#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>

#include <stm32.h>
#include <gpio.h>
#include <fonts.h>
#include <delay.h>

#include "lib/include/keyboard.h"
#include "lib/include/lcd.h"

#include "speaker.h"
#include "game.h"

// for debugging only
#include "lib/include/dma_uart.h"

void initLcd() {
  LCDconfigure();
  LCDsetFont(&font8x16);
  LCDclear();
  LCDgoto(0, 0);
}

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

bool fall_on = false;

atomic_int to_move = 0;

extern void TIM5_IRQHandler() {
  uint32_t it_status = TIM5->SR & TIM5->DIER;
  if (it_status & TIM_SR_UIF) {
    TIM5->SR = ~TIM_SR_UIF;

    if (fall_on) {
      atomic_fetch_add(&to_move, 1);
    }
  }
}

void loop() {
  KbKey key;

  // handle key press events
  while ((key = getNext()) != KB_NOKEY) {
    // 123A press frets
    // 7 resets song
    // * toggles note fall
    // others were previously used for debugging

    if (GET_ROW_NUM(key) == 1) { // Row 1; keys 1-4
      int col = GET_COL_NUM(key);
      handleFretPress(col);
    }

    if (key == KB_7) {
      DMA_DBG("Resetting...\n");
      resetGame();
    }

    if (key == KB_STAR) {
      fall_on = !fall_on;
      if (fall_on) {
        DMA_DBG("Fall on!\n");
      } else {
        DMA_DBG("Fall off!\n");
      }
    }
  }

  // check for fret key release
  for (int i = 1; i <= 4; ++i) {
    if (LCDisFretPressed(i) && !isKeyHeld(KB_ROW_KEY(1) | KB_COL_KEY(i))) {
      handleFretRelease(i);
    }
  }
  int moves = atomic_exchange(&to_move, 0);
  if (moves > 0) {
    handleTicks(moves);
  }
}

int main() {
  initDmaUart();
  initKb();
  initLcd();
  DMA_DBG("\n\nStarting Gietar Hiero!\n");

  LCDdrawBoard();

  updateScore();

  initGameTimer();
  initSpeakerTimer();

  while (true) {
    loop();
  }
}

/** TODOLIST:
 X Make note drawing/moving respect edges
 X figure out which y is ok for fret press
 X implement fret pressing
 X improve note edge visuals
 X add timer for moving and key for spawning notes
 X   (use keys to adjust speed)
 X   (find best speed?)
 X combine the two to play notes
   STRETCH:
 X add basic song storage
 X play basic sounds
 X add note pitch to the song storage and play the sounds when relevant
 X add basic scoring
 * maybe add loading screen, menu etc.
 */