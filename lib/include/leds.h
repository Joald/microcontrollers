#ifndef LEDS_H
#define LEDS_H

#include <stdbool.h>
#include <gpio.h>

////////////////////////// LED OPERATIONS //////////////////////////


#define RED_LED_GPIO GPIOA
#define GREEN_LED_GPIO GPIOA
#define BLUE_LED_GPIO GPIOB
#define GREEN2_LED_GPIO GPIOA
#define RED_LED_PIN 6
#define GREEN_LED_PIN 7
#define BLUE_LED_PIN 0
#define GREEN2_LED_PIN 5


#define RedLEDon() do { RED_LED_GPIO->BSRR = 1 << (RED_LED_PIN + 16); } while (false)
#define RedLEDoff() do { RED_LED_GPIO->BSRR = 1 << RED_LED_PIN; } while (false)
#define RedLEDread() (!(RED_LED_GPIO->ODR & (1 << RED_LED_PIN)))

#define GreenLEDon() do { GREEN_LED_GPIO->BSRR = 1 << (GREEN_LED_PIN + 16); } while (false)
#define GreenLEDoff() do { GREEN_LED_GPIO->BSRR = 1 << GREEN_LED_PIN; } while (false)
#define GreenLEDread() (!(GREEN_LED_GPIO->ODR & (1 << GREEN_LED_PIN)))

#define BlueLEDon() do { BLUE_LED_GPIO->BSRR = 1 << (BLUE_LED_PIN + 16); } while (false)
#define BlueLEDoff() do { BLUE_LED_GPIO->BSRR = 1 << BLUE_LED_PIN; } while (false)
#define BlueLEDread() (!(BLUE_LED_GPIO->ODR & (1 << BLUE_LED_PIN)))

#define Green2LEDon() do { GREEN2_LED_GPIO->BSRR = 1 << GREEN2_LED_PIN; } while (false)
#define Green2LEDoff() do { GREEN2_LED_GPIO->BSRR = 1 << (GREEN2_LED_PIN + 16); } while (false)
#define Green2LEDread() (GREEN2_LED_GPIO->ODR & (1 << GREEN2_LED_PIN))

void initLeds();

#define LED_OP_SIZE 3

#define LED_OP_COUNT 12

// color-based operations

static inline bool isValidColor(char color) {
  return color == 'R' || color == 'G' || color == 'B' || color == 'g';
}

static inline bool isValidOp(char op) {
  return op == '0' || op == '1' || op == 'T';
}

typedef enum {
  C_RED = 'R',
  C_GREEN = 'G',
  C_BLUE = 'B',
  C_GREEN2 = 'g',
} LedColor;

void diodeOn(LedColor color);
void diodeOff(LedColor color);
bool diodeRead(LedColor color);
void diodeToggle(LedColor color);

void processLedOp(LedColor color, char operation_type);

#endif // LEDS_H