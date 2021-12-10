#include <stdbool.h>
#include <stm32.h>
#include <gpio.h>
#include <delay.h>
#include <assert.h>

#include "keyboard.h"

//// <DEBUG>
#include "dma_uart.h"
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
    static_assert(SIZE < 64, "Cannot send 64+bit ints"); \
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

#define KEY_BUF_SIZE 128
static_assert(__builtin_popcount(KEY_BUF_SIZE) == 1, "key buf size must be a power of two");

typedef unsigned char buf_ind_t;
struct PressedKeyBuffer {
  KbKey keys[KEY_BUF_SIZE];
  uint32_t data;
  // equivalent to:
  // buf_ind_t padding[2];
  // buf_ind_t start;
  // buf_ind_t size;
} key_buf;

#define GET_START(where) ((where) >> (8))
#define GET_SIZE(where) ((where) & 0xff)


#define GET_BUF_START GET_START(key_buf.data)
#define GET_BUF_SIZE GET_SIZE(key_buf.data)


#define SET_START(where, what) \
  do { \
    uint32_t tmp = what; \
    where &= ~(0xff00); \
    where |= tmp << 8; \
  } while (false)

#define SET_SIZE(where, what) \
  do { \
    uint32_t tmp = what; \
    where &= ~0xff; \
    where |= tmp; \
  } while (false)

#define SET_BUF_START(what) SET_START(key_buf.data, what)
#define SET_BUF_SIZE(what) SET_SIZE(key_buf.data, what)
// static_assert(
//      &key_buf.padding[0] + 2 == &key_buf.start
//   && &key_buf.start + 1 == &key_buf.size,
//   "Key buffer insides not aligned correctly"
// );


// call from main "thread" only; can block
KbKey getNext() {
  uint32_t* memory = &key_buf.data;
  int retries = 0;

  // inspired by http://www.atakansarioglu.com/simple-semaphore-mutex-arm-cortex-m-implementation/
	while (retries < 32) { // 32 times because it's a nice round number
    if (GET_BUF_SIZE == 0) {
      return KB_NOKEY;
    }

		// read the current buffer stats
		uint32_t memory_val = __LDREXW(memory);

    // read key at read value
    KbKey rv = key_buf.keys[GET_START(memory_val)]; // indexing at key_buf.start

		// modify value, equivalent to:
    // key_buf.start = (key_buf.start + 1) % KEY_BUF_SIZE;
    // key_buf.size--; 
    SET_START(memory_val, (GET_START(memory_val) + 1) % KEY_BUF_SIZE);
    SET_SIZE(memory_val, GET_SIZE(memory_val) - 1);
    static_assert((1 << 8) == 256, "math doesn't work");
    static_assert((KEY_BUF_SIZE << 8) - 1 == 0x7fff, "expr is not correct mask");

    // data memory barrier instruction
    __DMB();

    // try to write
    if (0 == __STREXW(memory_val, memory)) {
      // data memory barrier instruction
      __DMB();
      // DMA_DBG("Success!\n");
      // switch (GET_BUF_SIZE) {
      //   case 0: DMA_DBG("Emptied!\n"); break;
      //   case 1: DMA_DBG("Almost emptied!\n"); break;
      //   case 2: DMA_DBG("Not almost emptied!\n"); break;
      //   default: DMA_DBG("Not at all emptied!\n"); break;
      // }
          
      // written, return success
      return rv;
    }
    // buffer changed between read and write, retry
    DMA_DBG("Retrying...\n");
    retries++;    
  } 
  DMA_DBG("Timeout!\n");
  // retries ran out, better luck next time...
  return KB_NOKEY;
}

// only call from interrupt handler, so we don't need sync
static void storeKeyPress(KbKey key) {
  key_buf.keys[(GET_BUF_START + GET_BUF_SIZE) % KEY_BUF_SIZE] = key;
  // don't need synchronization as we can't get interrupted by getNext
	// __DMB();
  if (GET_BUF_SIZE == KEY_BUF_SIZE) {
    SET_BUF_START((GET_BUF_START + 1) % KEY_BUF_SIZE);
  } else {
    SET_BUF_SIZE(GET_BUF_SIZE + 1);
    // key_buf.size++;
  }
}

uint16_t pressed_key_mask = 0;

#define MAKE_KEY_MASK(key) \
  (1u << (uint16_t)(4 * (GET_ROW_NUM(key) - 1) + GET_COL_NUM(key) - 1))

bool isKeyHeld(KbKey key) {
  // assume is valid key and not KB_NOKEY
  return pressed_key_mask & MAKE_KEY_MASK(key);
}

bool kb_scan() {
  // DMA_DBG("kb_scan\n");
  uint16_t new_key_mask = 0;
  for (int i = 1; i <= N_COLS; ++i) {
    KB_SET_PIN(COL, i, false);
	  for (int x = 0; x < 10; x++) __NOP();
    unsigned int state = KB_GPIO->IDR;
    KB_SET_PIN(COL, i, true);
    state = ~state;
    state &= KB_ROW_PIN_MASK;
    if (state) {
      int col = i;

      // - clz is number of leading zero bits
      // - subtract it from total number of bits 
      //   to get the number of the last leading zero bit
      // - subtract 1 to get the number of the first set bit
      // - subtract 5 to put it into [1..4] interval
      int row = (8 * sizeof(unsigned int)) - __builtin_clz(state) - 1 - 5;

      // static asserts to test if the calculation works
      static_assert((8 * sizeof(unsigned int)) == 32, "weird uint size");
      static_assert(__builtin_clz(0b1111000000) == 32 - 10, "weird clz calc");
      static_assert(
        (8 * sizeof(unsigned int)) - __builtin_clz(0b1111000000) - 1 - 5 == 4, 
        "bad row calc"
      );

      KbKey key = KB_ROW_KEY(row) | KB_COL_KEY(col);

      uint16_t key_mask = MAKE_KEY_MASK(key);
      if (pressed_key_mask & key_mask) {
        // ???
      } else {
        storeKeyPress(key);
        DMA_DBG("Stored!\n");
      }
      new_key_mask |= key_mask;

     
      // if (!found) {
      //   buf[sizeof("Found nonempty on col=") - 1] = '0' + key_col;
      //   buf[sizeof("Found nonempty on col=_, row=") - 1] = '0' + key_row;
      //   can_send = false;
      //   dmaSend(buf, sizeof(MSG));
      // } else {
      //   DMA_SEND("Another column...\n");
      // }
    }
    // sendUnsignedInt16(state);
  }
  pressed_key_mask = new_key_mask;
  return pressed_key_mask != 0;
}

void kb_init() {
  static_assert(KB_ROW_PIN_MASK == KB_ROW_PR_MASK, "Pin and PR masks different");
  static_assert(KB_ROW_PIN_MASK == 64+128+256+512, "Pin mask is not exactly bits 6-9");
  

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
