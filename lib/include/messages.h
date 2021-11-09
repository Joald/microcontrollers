#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdbool.h>

#include "buttons.h"

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

#define OUT_BUF_SIZE 1024

bool shouldSend();
char getCharToSend();

typedef struct {
  const char* buf;
  int len;
} MessageBuffer;

MessageBuffer getBuf(ButtonID button, bool if_released);

void insertToBuf(int button, bool if_released);

#endif // MESSAGES_H