#include <stdbool.h>
#include <gpio.h>
#include "buttons.h"

const int button_pin[BUTTON_COUNT] = {3, 4, 5, 6, 10, 13, 0};

static bool isButtonActive(ButtonID button) {
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
