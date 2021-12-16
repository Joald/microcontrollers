#include <stdbool.h>
#include <stm32.h>
#include <gpio.h>
#include <delay.h>
#include <assert.h>

#include "keyboard.h"

int key_col = 0;
int key_row = 0;

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


extern void EXTI9_5_IRQHandler() {
  EXTI->IMR &= ~KB_ROW_PR_MASK;

  EXTI->PR |= 0;

  setAllColsTo(true);

  // reset the counter
  TIM3->CNT = 0;
  // enable the counter
  TIM3->CR1 |= TIM_CR1_CEN;
}

bool scanKeys();

extern void TIM3_IRQHandler() {
  uint32_t it_status = TIM3->SR & TIM3->DIER;
  if (it_status & TIM_SR_UIF) {
    TIM3->SR = ~TIM_SR_UIF;
    bool anything_pressed = scanKeys();

    if (!anything_pressed) {
      // disable counter
      TIM3->CR1 &= ~TIM_CR1_CEN;

      setAllColsTo(false);

      // zero out the interrupts
      EXTI->PR |= KB_ROW_PR_MASK;

      EXTI->IMR |= KB_ROW_PR_MASK;
    }
  }
}

#define KEY_BUF_SIZE 128
static_assert(__builtin_popcount(KEY_BUF_SIZE) == 1, "key buf size must be a power of two");

typedef unsigned char buf_ind_t;
struct PressedKeyBuffer {
  KbKey keys[KEY_BUF_SIZE];
  uint32_t data;
  // data represents:
  // buf_ind_t padding[2];
  // buf_ind_t start;
  // buf_ind_t size;
  // it's stored inside single uint32_t to make concurrency easier 
  // and undefined behavior harder
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


// call from main "thread" only; can "block"
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
    static_assert((KEY_BUF_SIZE << 8) - 1 == 0x7fff, "mask calculation wrong");

    // data memory barrier instruction
    __DMB();

    // try to write
    if (0 == __STREXW(memory_val, memory)) {
      // data memory barrier instruction
      __DMB();
          
      // written, return success
      return rv;
    }
    // buffer changed between read and write, retry
    retries++;    
  }
  // retries ran out, better luck next time...
  return KB_NOKEY;
}

// only call from interrupt handler, so we don't need sync
static void storeKeyPress(KbKey key) {
  key_buf.keys[(GET_BUF_START + GET_BUF_SIZE) % KEY_BUF_SIZE] = key;
  // don't need synchronization as we can't get interrupted by getNext
  if (GET_BUF_SIZE == KEY_BUF_SIZE) {
    SET_BUF_START((GET_BUF_START + 1) % KEY_BUF_SIZE);
  } else {
    // key_buf.size++;
    SET_BUF_SIZE(GET_BUF_SIZE + 1);
  }
}

// 16 keys, 16 bits - 
uint16_t pressed_key_mask = 0;

#define MAKE_KEY_MASK(key) \
  (1u << (uint16_t)(4 * (GET_ROW_NUM(key) - 1) + GET_COL_NUM(key) - 1))

bool isKeyHeld(KbKey key) {
  // assume is valid key and not KB_NOKEY
  return pressed_key_mask & MAKE_KEY_MASK(key);
}

bool scanKeys() {
  uint16_t new_key_mask = 0;
  for (int i = 1; i <= N_COLS; ++i) {
    KB_SET_PIN(COL, i, false);
	  for (int x = 0; x < 10; x++) __NOP();
    unsigned int state = KB_GPIO->IDR; // TODO should it be ODR?
    KB_SET_PIN(COL, i, true);
    state = ~state;
    state &= KB_ROW_PIN_MASK;
    if (state) {
      int col = i;

      // - clz = *c*ount *l*eading *z*ero bits
      // - subtract it from total number of bits 
      //   to get the number of the last leading zero bit
      // - subtract 1 to get the number of the first set bit
      // - subtract 5 to put it into [1..4] interval
      int row = (8 * sizeof(unsigned int)) - __builtin_clz(state) - 1 - 5;

      // static asserts to test if the calculation works
      static_assert((8 * sizeof(unsigned int)) == 32, "bad uint size");
      static_assert(__builtin_clz(0b1111000000) == 32 - 10, "bad clz calc");
      static_assert(
        (8 * sizeof(unsigned int)) - __builtin_clz(0b1111000000) - 1 - 5 == 4, 
        "bad row calc"
      );

      KbKey key = KB_ROW_KEY(row) | KB_COL_KEY(col);

      uint16_t key_mask = MAKE_KEY_MASK(key);
      if (!(pressed_key_mask & key_mask)) {
        storeKeyPress(key);
      }
      new_key_mask |= key_mask;
    }
  }
  pressed_key_mask = new_key_mask;
  return pressed_key_mask != 0;
}

void initKb() {
  static_assert(KB_ROW_PIN_MASK == KB_ROW_PR_MASK, "Pin and PR masks different");
  static_assert(KB_ROW_PIN_MASK == 64+128+256+512, "Pin mask is not exactly bits 6-9");
  
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

  // enable kb timing
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

  // enable timer3 timing
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

  // configure timer
  TIM3->CR1 = TIM_CR1_URS; // counting up, interrupts only on overflow
  TIM3->PSC = 0;
  TIM3->ARR = 160000; // 10 ms / (1 / 16 MHz) = 160 000
  TIM3->EGR = TIM_EGR_UG;

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
