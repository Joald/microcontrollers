#include <stdbool.h>

#include <stm32.h>
#include <gpio.h>

#include "uart_init.h"
#include "leds.h"
#include "buttons.h"
#include "messages.h"

////////////////////////// CYCLIC INPUT BUFFER //////////////////////////

// cyclic buffer of opcode size 

struct InputBuffer {
  char buf[LED_OP_SIZE]; 
  int start;
  int size;
} input_buf;

#define INPUT_BUF_NTH_VAL(n) (input_buf.buf[(input_buf.start + n) % LED_OP_SIZE])

static void cleanBuffer() {
  input_buf.start = 0;
  input_buf.size = 0;
}

static void shiftBuffer() {
  input_buf.start = (input_buf.start + 1) % LED_OP_SIZE;
  input_buf.size--;
}

// insert character to the buffer and parse command if necessary
static void processInputChar(char input_char) {
  INPUT_BUF_NTH_VAL(input_buf.size) = input_char;
  input_buf.size++;

  if (input_buf.size == LED_OP_SIZE) {
    char must_be_l = INPUT_BUF_NTH_VAL(0);
    char led_color = INPUT_BUF_NTH_VAL(1);
    char operation_type = INPUT_BUF_NTH_VAL(2);

    if (must_be_l == 'L' && isValidColor(led_color) && isValidOp(operation_type)) {
      processLedOp(led_color, operation_type);
      cleanBuffer();
    } else {
      // invalid opcode, remove one byte from buffer
      shiftBuffer();
    }
  }
}

// Button/message interaction

int last_button_state = 0;

static void updateButtonsState() {
  int state = getCurrentState();

  for (ButtonID button = 0; button < BUTTON_COUNT; ++button) {
    int button_mask = 1 << button;
    bool button_pressed = state & button_mask;
    bool prev_button_pressed = last_button_state & button_mask;
    if (button_pressed != prev_button_pressed) {
      insertToBuf(button, prev_button_pressed);
    }
  }
  last_button_state = state;
}

////////////////////////// MAIN LOOP //////////////////////////

int main() {
  initUart();
  initLeds();
  initButtons();

  while (true) {
    // check if receiving from USART is possible
    if (USART2->SR & USART_SR_RXNE) {
      processInputChar(USART2->DR);
    }
    
    // check if sending to USART is possible (and makes sense)
    if (shouldSend() && (USART2->SR & USART_SR_TXE)) {
      USART2->DR = getCharToSend();
    }

    updateButtonsState();
  }

}