#include <stdbool.h>
#include <gpio.h>
#include <stm32.h>

#include "buttons.h"

static const int button_pin[BUTTON_COUNT] = {3, 4, 5, 6, 10, 13, 0};

#define BUTTON_PR(button) EXTI_PR_PR##button

static int buttonPR(ButtonID button) {
  #define BUTTON_PR_CASE(i) case i: return BUTTON_PR(i)
  switch (button_pin[button]) {
    BUTTON_PR_CASE(3);
    BUTTON_PR_CASE(4);
    BUTTON_PR_CASE(5);
    BUTTON_PR_CASE(6);
    BUTTON_PR_CASE(10);
    BUTTON_PR_CASE(13);
    BUTTON_PR_CASE(0);
    default: return -1;
  }
}

static ButtonID button_at_pin(int pin) {
  switch (pin) {
    case 3: return B_LEFT;
    case 4: return B_RIGHT;
    case 5: return B_UP;
    case 6: return B_DOWN;
    case 10: return B_FIRE;
    case 13: return B_USER;
    case 0: return B_MODE;
    default: return -1;
  }
}

bool isButtonActive(ButtonID button) {
  bool cond = 
    buttonGpio(button)->IDR & (1 << button_pin[button]);
  return button == B_MODE ? cond : !cond;
}

int getCurrentState() {
  int state = 0;
  for (ButtonID button = 0; button < BUTTON_COUNT; ++button) {
    state |= isButtonActive(button) << button;
  }
  return state;
}

void initButtons() {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
}

void initButtonInterrupts() {
  // assume initButtons() was already called
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

  bool enabled_9_to_5 = false;
  bool enabled_15_to_10 = false;

  int prs_to_zero = 0;

  for (ButtonID button = 0; button < BUTTON_COUNT; ++button) {
    GPIOinConfigure(
      buttonGpio(button), 
      button_pin[button], 
      GPIO_PuPd_UP,
      EXTI_Mode_Interrupt,
      EXTI_Trigger_Rising_Falling
    );

    prs_to_zero |= buttonPR(button);
  }

  EXTI->PR = prs_to_zero;

  for (ButtonID button = 0; button < BUTTON_COUNT; ++button) {
    const int pin = button_pin[button];  
    // assert(pin >= 0 && pin <= 15);

    if (pin <= 4) {
      NVIC_EnableIRQ(EXTI0_IRQn + pin);
    } else if (pin <= 8) {
      if (!enabled_9_to_5) {
        NVIC_EnableIRQ(EXTI9_5_IRQn);
        enabled_9_to_5 = true;
      }
    } else if (!enabled_15_to_10) {
      NVIC_EnableIRQ(EXTI15_10_IRQn);
      enabled_15_to_10 = true;
    }
  }

  // turn off SYSCFG as we only need it for GPIOinConfigures
  RCC->APB2ENR &= ~RCC_APB2ENR_SYSCFGEN;
}

static ButtonHandler handlers[BUTTON_COUNT];

void registerButtonHandler(ButtonID button, ButtonHandler handler) {
  handlers[button] = handler;
}

#define HANDLE_BUTTON(button) \
  do { \
    EXTI->PR = buttonPR(button); \
    ButtonHandler handler = handlers[button]; \
    if (handler) handler(); \
  } while (false)

#define SINGLE_PIN_HANDLER(pin) \
  void EXTI##pin##_IRQHandler(void) { \
    ButtonID button = button_at_pin(pin); \
    HANDLE_BUTTON(button); \
  }
// pins 1 and 2 are not button
SINGLE_PIN_HANDLER(0)
SINGLE_PIN_HANDLER(3)
SINGLE_PIN_HANDLER(4)

// TODO: generalize the below with macro

#define PINS_9_TO_5_COUNT 2

const int pins_9_to_5[PINS_9_TO_5_COUNT] = {5, 6};

void EXTI9_5_IRQHandler(void) {
  for (int i = 0; i < PINS_9_TO_5_COUNT; ++i) {
    const ButtonID button = button_at_pin(pins_9_to_5[i]);
    if (EXTI->PR & buttonPR(button)) {
      HANDLE_BUTTON(button);
    }
  }
}


#define PINS_15_TO_10 2

const int pins_15_to_10[PINS_15_TO_10] = {10, 13};

void EXTI15_10_IRQHandler(void) {
  for (int i = 0; i < PINS_15_TO_10; ++i) {
    const ButtonID button = button_at_pin(pins_15_to_10[i]);
    const int pr = buttonPR(button);
    if (EXTI->PR & pr) {
      EXTI->PR = pr;
      ButtonHandler handler = handlers[button];
      if (handler) handler();
    }
  }
}
