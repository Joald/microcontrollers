#include "buttons.h"
#include "dma_uart.h"
#include "messages.h"

#define BUTTON_HANDLER_NAME(button) button##_handler

// Generates handlers for buttons
#define BUTTON_HANDLER(button) \
  static void BUTTON_HANDLER_NAME(button)() { \
    bool if_released = !isButtonActive(button); \
    MessageBuffer buffer = getBuf(button, if_released); \
    dmaSend(buffer.buf, buffer.len); \
  }

BUTTON_HANDLER(B_LEFT)
BUTTON_HANDLER(B_RIGHT)
BUTTON_HANDLER(B_UP)
BUTTON_HANDLER(B_DOWN)
BUTTON_HANDLER(B_FIRE)
BUTTON_HANDLER(B_USER)
BUTTON_HANDLER(B_MODE)

static const ButtonHandler button_handlers[BUTTON_COUNT] = {
  BUTTON_HANDLER_NAME(B_LEFT),
  BUTTON_HANDLER_NAME(B_RIGHT),
  BUTTON_HANDLER_NAME(B_UP),
  BUTTON_HANDLER_NAME(B_DOWN),
  BUTTON_HANDLER_NAME(B_FIRE),
  BUTTON_HANDLER_NAME(B_USER),
  BUTTON_HANDLER_NAME(B_MODE),
};

int main() {
  initButtons();
  initButtonInterrupts();
  
  for (ButtonID button = 0; button < BUTTON_COUNT; ++button) {
    registerButtonHandler(button, button_handlers[button]);
  }
  
  initDmaUart();

  while (true) {}
}
