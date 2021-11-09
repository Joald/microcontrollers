#include "leds.h"


void initLeds() {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;

  __NOP();
  RedLEDoff();
  GreenLEDoff();
  BlueLEDoff();
  Green2LEDoff();
  
  GPIOoutConfigure(RED_LED_GPIO,
      RED_LED_PIN,
      GPIO_OType_PP,
      GPIO_Low_Speed,
      GPIO_PuPd_NOPULL);
  
  GPIOoutConfigure(GREEN_LED_GPIO,
      GREEN_LED_PIN,
      GPIO_OType_PP,
      GPIO_Low_Speed,
      GPIO_PuPd_NOPULL);    

  GPIOoutConfigure(BLUE_LED_GPIO,
      BLUE_LED_PIN,
      GPIO_OType_PP,
      GPIO_Low_Speed,
      GPIO_PuPd_NOPULL);    

  GPIOoutConfigure(GREEN2_LED_GPIO,
      GREEN2_LED_PIN,
      GPIO_OType_PP,
      GPIO_Low_Speed,
      GPIO_PuPd_NOPULL);
}

// color-based operations

void diodeOn(LedColor color) {
  if (color == 'R' || color == 'A') {
    RedLEDon();
  } 
  if (color == 'G' || color == 'A') {
    GreenLEDon();
  }
  if (color == 'B' || color == 'A') {
    BlueLEDon();
  }
  if (color == 'g' || color == 'A') {
    Green2LEDon();
  }
}

void diodeOff(LedColor color) {
  if (color == 'R' || color == 'A') {
    RedLEDoff();
  } 
  if (color == 'G' || color == 'A') {
    GreenLEDoff();
  }
  if (color == 'B' || color == 'A') {
    BlueLEDoff();
  }
  if (color == 'g' || color == 'A') {
    Green2LEDoff();
  }
}

bool diodeRead(LedColor color) {
  if (color == 'R') {
    return RedLEDread();
  } else if (color == 'G') {
    return GreenLEDread();
  } else if (color == 'B') {
    return BlueLEDread();
  } else if (color == 'g') {
    return Green2LEDread();
  } else {
    // should never happen
    return false;
  }
}

void diodeToggle(LedColor color) {
  if (diodeRead(color)) {
    diodeOff(color);
  } else {
    diodeOn(color);
  }
}

void processLedOp(LedColor color, char operation_type) {
  if (operation_type == '0') {
    diodeOff(color);
  } else if (operation_type == '1') {
    diodeOn(color);
  } else if (operation_type == 'T') {
    diodeToggle(color);
  }
}

