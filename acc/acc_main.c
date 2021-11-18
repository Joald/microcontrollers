#include <stdbool.h>
#include <stdint.h>
#include <stm32.h>

#include "leds.h"

#define COUNTER_SIZE 749

#define DIODE_HIGH 499
#define DIODE_MID 249
#define DIODE_LOW 49

#define CTRL_REG1 0x20

#define OUT_X 0x29
#define OUT_Y 0x2b
#define OUT_Z 0x2d

#define LIS35DE_ADDR 0x1c

void writeToAccelerometer(int reg, int value) {
  // Zainicjuj transmisję sygnału START
  I2C1->CR1 |= I2C_CR1_START;
  // Czekaj na zakończenie transmisji bitu START, co jest
  // sygnalizowane ustawieniem bitu SB (ang. start bit) w rejestrze
  // SR1, czyli czekaj na spełnienie warunku
  while (!(I2C1->SR1 & I2C_SR1_SB)) {}
  // Zainicjuj wysyłanie 7-bitowego adresu slave’a, tryb MT
  I2C1->DR = LIS35DE_ADDR << 1;
  // Czekaj na zakończenie transmisji adresu, ustawienie bitu ADDR
  // (ang. address sent) w rejestrze SR1, warunek
  while (!(I2C1->SR1 & I2C_SR1_ADDR)) {}
  // Skasuj bit ADDR przez odczytanie rejestru SR2 po odczytaniu
  // rejestru SR1
  I2C1->SR2;
  // Zainicjuj wysyłanie 8-bitowego numeru rejestru slave’a
  I2C1->DR = reg;
  // Czekaj na opróżnienie kolejki nadawczej, czyli na ustawienie
  // bitu TXE (ang. transmitter data register empty ) w rejestrze
  // SR1, warunek
  while (!(I2C1->SR1 & I2C_SR1_TXE)) {}
  // Wstaw do kolejki nadawczej 8-bitową wartość zapisywaną do
  // rejestru slave’a
  I2C1->DR = value;
  // Czekaj na zakończenie transmisji, czyli na ustawienie bitu BTF
  // (ang. byte transfer finished) w rejestrze SR1, warunek
  while (!(I2C1->SR1 & I2C_SR1_BTF)) {}
  // Zainicjuj transmisję sygnału STOP
  I2C1->CR1 |= I2C_CR1_STOP;
}

int8_t readFromAccelerometer(int reg) {
  // Zainicjuj transmisję sygnału START
  I2C1->CR1 |= I2C_CR1_START;
  // Czekaj na ustawienie bitu SB w rejestrze SR1, warunek
  while (!(I2C1->SR1 & I2C_SR1_SB)) {}
  // Zainicjuj wysyłanie 7-bitowego adresu slave’a, tryb MT
  I2C1->DR = LIS35DE_ADDR << 1;
  // Czekaj na zakończenie transmisji adresu, warunek
  while (!(I2C1->SR1 & I2C_SR1_ADDR)) {}
  // Skasuj bit ADDR
  I2C1->SR2;
  // Zainicjuj wysyłanie numeru rejestru slave’a
  I2C1->DR = reg;
  // Czekaj na zakończenie transmisji, czyli na ustawienie bitu BTF
  // (ang. byte transfer finished) w rejestrze SR1, czyli na
  // spełnienie warunku
  while (!(I2C1->SR1 & I2C_SR1_BTF)) {}
  // Zainicjuj transmisję sygnału REPEATED START
  I2C1->CR1 |= I2C_CR1_START;
  // Czekaj na ustawienie bitu SB w rejestrze SR1, warunek
  while (!(I2C1->SR1 & I2C_SR1_SB)) {}
  // Zainicjuj wysyłanie 7-bitowego adresu slave’a, tryb MR
  I2C1->DR = (LIS35DE_ADDR << 1) | 1;
  // Ustaw, czy po odebraniu pierwszego bajtu ma być wysłany
  // sygnał ACK czy NACK.
  // Ponieważ ma być odebrany tylko jeden bajt, ustaw wysłanie
  // sygnału NACK, zerując bit ACK
  I2C1->CR1 &= ~I2C_CR1_ACK;
  // Czekaj na zakończenie transmisji adresu, warunek
  while (!(I2C1->SR1 & I2C_SR1_ADDR)) {}
  // Skasuj bit ADDR
  I2C1->SR2;
  // Zainicjuj transmisję sygnału STOP, aby został wysłany po
  // odebraniu ostatniego (w tym przypadku jedynego) bajtu
  I2C1->CR1 |= I2C_CR1_STOP;
  // Czekaj na ustawienie bitu RXNE (ang. receiver data register
  // not empty) w rejestrze SR1, warunek
  while (!(I2C1->SR1 & I2C_SR1_RXNE)) {}
  // Odczytaj odebraną 8-bitową wartość
  return (int8_t) I2C1->DR;
}

// rescales value from 0..256 to 0..COUNTER_SIZE
int normalize(int8_t acc_value) {
  // Normally, to rescale value, we would first divide by
  // old max value, then multiply by new max value.
  // On integers we need to do it in reverse to 
  // avoid precision loss.
  
  int value = acc_value;
  if (value < 0) {
    value = -value;
  }
  value *= COUNTER_SIZE;
  value /= 1 << 8; // max size
  if (value > COUNTER_SIZE) {
    value = COUNTER_SIZE;
  }
  return value;
}

void initTimer() {
  // TIMER PWM CONFIG

  // enable timing
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
  

  // configure timer pins
  GPIOafConfigure(
    RED_LED_GPIO,
    RED_LED_PIN,
    GPIO_OType_PP,
    GPIO_Low_Speed,
    GPIO_PuPd_NOPULL,
    GPIO_AF_TIM3);
  GPIOafConfigure(
    GREEN_LED_GPIO,
    GREEN_LED_PIN,
    GPIO_OType_PP,
    GPIO_Low_Speed,
    GPIO_PuPd_NOPULL,
    GPIO_AF_TIM3);
  
  GPIOafConfigure(
    BLUE_LED_GPIO,
    BLUE_LED_PIN,
    GPIO_OType_PP,
    GPIO_Low_Speed,
    GPIO_PuPd_NOPULL,
    GPIO_AF_TIM3);

  // configure timer
  TIM3->PSC = 199;
  TIM3->ARR = COUNTER_SIZE;
  TIM3->EGR = TIM_EGR_UG;
  TIM3->CCR1 = DIODE_MID;
  TIM3->CCR2 = DIODE_MID;
  TIM3->CCR3 = DIODE_MID;

  // configure clock modes
  TIM3->CCMR1 =
      TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2
    | TIM_CCMR1_OC1PE
    | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2 
    | TIM_CCMR1_OC2PE;
  TIM3->CCMR2 = 
      TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2
    | TIM_CCMR2_OC3PE;

  // connect outputs
  TIM3->CCER = 
      TIM_CCER_CC1E | TIM_CCER_CC1P 
    | TIM_CCER_CC2E | TIM_CCER_CC2P
    | TIM_CCER_CC3E | TIM_CCER_CC3P;

  // enable counter with ARR buffering
  TIM3->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
}

int main() {
  initLeds();

  initTimer();

  //// I2C ACCELEROMETER CONFIG

  // enable timing
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
  RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

  // configure pins
  GPIOafConfigure(GPIOB, 8, GPIO_OType_OD,
    GPIO_Low_Speed, GPIO_PuPd_NOPULL,
    GPIO_AF_I2C1);
  GPIOafConfigure(GPIOB, 9, GPIO_OType_OD,
    GPIO_Low_Speed, GPIO_PuPd_NOPULL,
    GPIO_AF_I2C1);

  // basic bus config
  I2C1->CR1 = 0;

  // configure bus speed
  #define I2C_SPEED_HZ 100000
  #define PCLK1_MHZ 16
  I2C1->CCR = (PCLK1_MHZ * 1000000) /
    (I2C_SPEED_HZ << 1);
  I2C1->CR2 = PCLK1_MHZ;
  I2C1->TRISE = PCLK1_MHZ + 1;

  // enable interface
  I2C1->CR1 |= I2C_CR1_PE;

  // set ctrl reg to power active
  writeToAccelerometer(CTRL_REG1, 0b01000111);

  while (true) {
    TIM3->CCR1 = normalize(readFromAccelerometer(OUT_X));
    TIM3->CCR2 = normalize(readFromAccelerometer(OUT_Y));
    TIM3->CCR3 = normalize(readFromAccelerometer(OUT_Z));
  }


  
}