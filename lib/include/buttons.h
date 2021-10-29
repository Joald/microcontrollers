#ifndef BUTTONS_H
#define BUTTONS_H

#include <gpio.h>

////////////////////////// INPUT BUTTONS //////////////////////////

#define BUTTON_COUNT 7

typedef enum {
  B_LEFT, B_RIGHT, B_UP, B_DOWN, 
  B_FIRE, B_USER, B_MODE
} ButtonID;

static inline GPIO_TypeDef* buttonGpio(ButtonID button) {
  return button == B_USER ? GPIOC : 
    button == B_MODE ? GPIOA : GPIOB;
}

#define BUTTON_PREV_VAL(button) ((bool)(last_button_state & (1 << button)))

int getCurrentState();
void initButtons();

#endif // BUTTONS_H