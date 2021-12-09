#include <stdbool.h>
#include <stm32.h>
#include <gpio.h>
#include <delay.h>

#include "keyboard.h"

//// <DEBUG>
// #include "dma_uart.h"
//// </DEBUG>

int key_col = 0;
int key_row = 0;


#define N_COLS 4
#define N_ROWS 4

#define KB_GPIO GPIOC

// All macros take row/col numbers from [1..4]

#define KB_COL_PIN_NUM(col_num) (col_num - 1)
#define KB_ROW_PIN_NUM(row_num) (row_num + 5)

#define KB_COL_PIN(col_num) (1 << KB_COL_PIN_NUM(col_num))
#define KB_ROW_PIN(col_num) (1 << KB_ROW_PIN_NUM(col_num))

#define KB_ROW_PIN_MASK (KB_ROW_PIN(1) | KB_ROW_PIN(2) | KB_ROW_PIN(3) | KB_ROW_PIN(4))

#define KB_SET_PIN(TYPE, num, val) \
 do { \
  KB_GPIO->BSRR = KB_##TYPE##_PIN(num) << (val ? 0 : 16); \
 } while (false)

// Except this one, which takes from [6..9]

#define KB_ROW_PR(row_num) (EXTI_PR_PR##row_num)

#define KB_ROW_PR_MASK \
  (KB_ROW_PR(6) | KB_ROW_PR(7) | KB_ROW_PR(8) | KB_ROW_PR(9))

static uint32_t kbRowPR(int row_num) {
  // have to manually type numbers to make use of 
  // preprocessor concatenation

  #define KB_CASE(i) case i: return KB_ROW_PR(i)
  switch (KB_ROW_PIN_NUM(row_num)) {
    KB_CASE(6);
    KB_CASE(7);
    KB_CASE(8);
    KB_CASE(9);
    default:
      // correct row_num will never reach here
      return -1;
  }
}

static void setAllColsTo(bool to) {
  // col bits are 0-3
  KB_GPIO->BSRR = 0b1111 << (to ? 0 : 16);
}

// handlers are extern to force a linker error 
// when compiling other *.c that uses the same ones


#define SEND_UINT(SIZE) \
  char state_buf##SIZE[SIZE + 1]; \
  bool can_send_state##SIZE = true; \
  void markSent##SIZE(char* buf) { \
    if (buf == state_buf##SIZE) { \
      can_send_state##SIZE = true; \
    } \
  } \
  void sendUnsignedInt##SIZE(uint##SIZE##_t to_send) { \
    _Static_assert(SIZE < 64, "Cannot send 64+bit ints"); \
    if (1 || can_send_state##SIZE) { \
      for (int64_t i = 31; i >= 0; --i) { \
        state_buf##SIZE[i] = '0' + (bool)(to_send & (1 << i)); \
      } \
      state_buf##SIZE[SIZE] = '\n'; \
      can_send_state##SIZE = false; \
      dmaSend(state_buf##SIZE, SIZE + 1); \
    } else {\
      /*DMA_SEND("Sending unsigned int" #SIZE " currently busy\n");*/ \
    } \
  } \

// SEND_UINT(16)
// SEND_UINT(32)

extern void EXTI9_5_IRQHandler() {
  // DMA_SEND("PIN IRQ HANDLER\n");
  EXTI->IMR &= ~KB_ROW_PR_MASK;

  uint32_t state = EXTI->PR;
  
  // ////assumes line 5 will not be used
  EXTI->PR = state;

  // sendUnsignedInt32(state);

  if (!(state & KB_ROW_PR_MASK)) {
    // DMA_SEND("ROW_MASK WRONG\n");
  }

  setAllColsTo(true);

  // reset the counter
  TIM3->CNT = 0;
  // enable the counter
  TIM3->CR1 |= TIM_CR1_CEN;
}

bool kb_scan();

extern void TIM3_IRQHandler() {
  uint32_t it_status = TIM3->SR & TIM3->DIER;
  if (it_status & TIM_SR_UIF) {
    TIM3->SR = ~TIM_SR_UIF;
    // DMA_SEND("TIM3 IRQ HANDLER\n");
    bool anything_pressed = kb_scan();

    if (!anything_pressed) {
      // DMA_SEND("nothing pressed\n");
      // disable counter
      TIM3->CR1 &= ~TIM_CR1_CEN;

      setAllColsTo(false);

      // zero out the interrupts
      EXTI->PR |= KB_ROW_PR_MASK;

      EXTI->IMR |= KB_ROW_PR_MASK;
    }
  }
}

// #define MSG "Found nonempty on col=_, row=_;\n"

// char buf[sizeof(MSG)] = MSG;
// volatile bool can_send = true;

// void markBufSent(char* buff) {
//   if (buff == buf) { // TODO: what the fuck is this
//     can_send = true;
//   }
// }

bool kb_scan() {
  // DMA_SEND("kb_scan\n");
  bool found = false;
  for (int i = 1; i <= N_COLS; ++i) {
    KB_SET_PIN(COL, i, false);
	  for (int x = 0; x < 10; x++) __NOP();
    unsigned int state = KB_GPIO->IDR;
    KB_SET_PIN(COL, i, true);
    state = ~state;
    state &= KB_ROW_PIN_MASK;
    if (state) {
      key_col = i;
      key_row = (8 * sizeof(unsigned int)) - __builtin_clz(state) - 1 - 5;
      _Static_assert((8 * sizeof(unsigned int)) == 32, "werid uint size");
      _Static_assert(__builtin_clz(0b1111000000) == 32 - 10, "weird clz calc");
      _Static_assert((8 * sizeof(unsigned int)) - __builtin_clz(0b1111000000) - 1 - 5 == 4, "bad clz calc");
      // if (!found) {
      //   buf[sizeof("Found nonempty on col=") - 1] = '0' + key_col;
      //   buf[sizeof("Found nonempty on col=_, row=") - 1] = '0' + key_row;
      //   can_send = false;
      //   dmaSend(buf, sizeof(MSG));
      // } else {
      //   DMA_SEND("Another column...\n");
      // }
      found = true;
    }
    // sendUnsignedInt16(state);
  }
  return found;
}

void kb_init() {
  _Static_assert(KB_ROW_PIN_MASK == KB_ROW_PR_MASK, "Pin and PR masks different");
  _Static_assert(KB_ROW_PIN_MASK == 64+128+256+512, "Pin mask is not exactly bits 6-9");
  

  // registerDmaUartHandler(H_DMA_SEND_FINISH, markSent16);
  // registerDmaUartHandler(H_DMA_SEND_FINISH, markSent32);
  // registerDmaUartHandler(H_DMA_SEND_FINISH, markBufSent);

  // TODO: not sure if needed
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

  // enable kb timing
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

  // enable timer3 timing
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

  // configure timer
  TIM3->CR1 = TIM_CR1_URS; // counting up, interrupts only on overflow
  TIM3->PSC = 0; //TODO: 160000 - 1;
  TIM3->ARR = 160000; // 10 ms / (1 / 16 MHz) = 160 000
  TIM3->EGR = TIM_EGR_UG;
  // TIM3->CCR1 = DIODE_MID;
  // TIM3->CCR2 = DIODE_MID;
  // TIM3->CCR3 = DIODE_MID;

  // enable update interrupt
  TIM3->SR = ~TIM_SR_UIF;
  TIM3->DIER = TIM_DIER_UIE;

  NVIC_EnableIRQ(TIM3_IRQn);

  // set all columns to low
  for (int i = 1; i <= N_COLS; ++i) {
    KB_SET_PIN(COL, i, false);
  }

  // configure column pins
  for (int i = 1; i <= N_COLS; ++i) {
    GPIOoutConfigure(
      KB_GPIO,
      KB_COL_PIN_NUM(i),
      GPIO_OType_PP,
      GPIO_High_Speed,
      GPIO_PuPd_NOPULL
    );
  }


  // configure row pins
  uint32_t prs_to_zero = 0;
  for (int i = 1; i <= N_ROWS; ++i) {
    GPIOinConfigure(
      KB_GPIO,
      KB_ROW_PIN_NUM(i),
      GPIO_PuPd_UP,
      EXTI_Mode_Interrupt,
      EXTI_Trigger_Falling
    );

    prs_to_zero |= kbRowPR(i);
  }

  // zero appropriate interrupt bits by writing 1's to EXTI->PR
  EXTI->PR = prs_to_zero;

  NVIC_EnableIRQ(EXTI9_5_IRQn);

  RCC->APB2ENR &= ~RCC_APB2ENR_SYSCFGEN;

}
