#include <stdbool.h>

#include <stm32.h>
#include <gpio.h>

#include "uart_init.h"
#include "leds.h"
#include "buttons.h"

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


////////////////////////// MESSAGES //////////////////////////

#define MESSAGE_COUNT (BUTTON_COUNT * 2)

#define LEFT_PRESSED "LEFT PRESSED\n"
#define LEFT_RELEASED "LEFT RELEASED\n"
#define RIGHT_PRESSED "RIGHT PRESSED\n"
#define RIGHT_RELEASED "RIGHT RELEASED\n"
#define UP_PRESSED "UP PRESSED\n"
#define UP_RELEASED "UP RELEASED\n" 
#define DOWN_PRESSED "DOWN PRESSED\n"
#define DOWN_RELEASED "DOWN RELEASED\n"
#define FIRE_PRESSED "FIRE PRESSED\n" 
#define FIRE_RELEASED "FIRE RELEASED\n"
#define USER_PRESSED "USER PRESSED\n"
#define USER_RELEASED "USER RELEASED\n" 
#define MODE_PRESSED "MODE PRESSED\n"
#define MODE_RELEASED "MODE RELEASED\n"


const char* messages[MESSAGE_COUNT] = {
  LEFT_PRESSED, LEFT_RELEASED, 
  RIGHT_PRESSED, RIGHT_RELEASED, 
  UP_PRESSED, UP_RELEASED, 
  DOWN_PRESSED, DOWN_RELEASED, 
  FIRE_PRESSED, FIRE_RELEASED, 
  USER_PRESSED, USER_RELEASED, 
  MODE_PRESSED, MODE_RELEASED,
};

int message_lengths[MESSAGE_COUNT] = {
  sizeof(LEFT_PRESSED), sizeof(LEFT_RELEASED),
  sizeof(RIGHT_PRESSED), sizeof(RIGHT_RELEASED),
  sizeof(UP_PRESSED), sizeof(UP_RELEASED), 
  sizeof(DOWN_PRESSED), sizeof(DOWN_RELEASED), 
  sizeof(FIRE_PRESSED), sizeof(FIRE_RELEASED), 
  sizeof(USER_PRESSED), sizeof(USER_RELEASED), 
  sizeof(MODE_PRESSED), sizeof(MODE_RELEASED),
};

#define OUT_BUF_SIZE 1024

struct OutputBuffer {
  // buffer stores index into messages and message_lengths arrays
  int buf[OUT_BUF_SIZE];
  int start;
  int size;
  int offset;
} output_buffer;

static bool shouldSend() {
  return output_buffer.size != 0;
}

// extracts next character from output buffer
static char getCharToSend() {
  int message_index = output_buffer.buf[output_buffer.start];
  char to_send = messages[message_index][output_buffer.offset];
  output_buffer.offset++;

  // subtract one to avoid sending null terminator
  if (output_buffer.offset == message_lengths[message_index] - 1) {
    output_buffer.start = (output_buffer.start + 1) % OUT_BUF_SIZE;
    output_buffer.size--;
    output_buffer.offset = 0;
  }
    
  return to_send;
}

static int placeToInsert() {
  return (output_buffer.start + output_buffer.size) % OUT_BUF_SIZE;
}

static void insertToBuf(int button, bool if_released) {
  // assume output buffer can never fill up
  int buffer_index = placeToInsert();

  // when releasing we should add 1 to index
  output_buffer.buf[buffer_index] = 2 * button + if_released;
  output_buffer.size++;
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