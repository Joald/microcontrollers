#include "messages.h"

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

static int getIndex(int button, bool if_released) {
  // when releasing we should add 1 to index
  return 2 * button + if_released;
}

#ifdef USE_MSG_BUFFER

struct OutputBuffer {
  // buffer stores index into messages and message_lengths arrays
  int buf[OUT_BUF_SIZE];
  int start;
  int size;
  int offset;
} output_buffer;

bool shouldSend() {
  return output_buffer.size != 0;
}

// extracts next character from output buffer
char getCharToSend() {
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

void insertToBuf(int button, bool if_released) {
  // assume output buffer can never fill up
  int buffer_index = placeToInsert();

  output_buffer.buf[buffer_index] = getIndex(button, if_released);
  output_buffer.size++;
}

#endif

MessageBuffer getBuf(ButtonID button, bool if_released) {
  // assert output_buffer.offset == 0
  int index = getIndex(button, if_released);
  MessageBuffer buf = {.buf = messages[index], .len = message_lengths[index]};
  return buf;
}