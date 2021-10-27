#include <stdio.h>
#include <stdbool.h>

#define BUTTON_COUNT 7
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

bool shouldSend() {
  return output_buffer.size != 0;
}

// extracts next character from output buffer
char getCharToSend() {
  int message_index = output_buffer.buf[output_buffer.start];
  char to_send = messages[message_index][output_buffer.offset++];

  if (output_buffer.offset >= message_lengths[message_index]) {
    output_buffer.start = (output_buffer.start + 1) % OUT_BUF_SIZE;
    output_buffer.size--;
    output_buffer.offset = 0;
  }
    
  return to_send;
}

int placeToInsert() {
  return (output_buffer.start + output_buffer.size) % OUT_BUF_SIZE;
}

void updateButtonsState() {
    int button = 7;
    int button_mask = 1 << 7;
    // assume output buffer can never fill up
    int buffer_index = placeToInsert();
    // when releasing we should add 1 to index
    output_buffer.buf[buffer_index] = 2 * button;
}

int main() {
    updateButtonsState();
    char buf[11];
    for (int i = 0; i < 10; ++i) {
buf[i] = getCharToSend();
    }
    buf[10] = '\0';
    printf("%s", buf);
}
