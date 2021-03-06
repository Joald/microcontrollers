#include <assert.h>
#include <stm32.h>
#include <gpio.h>
#include "speaker.h"
#include "dma_uart.h"

#define SPEAKER_GPIO GPIOB
#define SPEAKER_PIN 7

int fakeWaveLen = 18261; // Middle A, 440 Hz

void initSpeakerTimer() {
  // enable timing
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

  // configure timer pins
  GPIOafConfigure(
    SPEAKER_GPIO,
    SPEAKER_PIN,
    GPIO_OType_PP,
    GPIO_Low_Speed,
    GPIO_PuPd_NOPULL,
    GPIO_AF_TIM4);
  
  // configure timer
  TIM4->PSC = 0;
  TIM4->ARR = fakeWaveLen;
  TIM4->EGR = TIM_EGR_UG;

  // configure clock modes
  TIM4->CCMR1 =
      TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_0
    | TIM_CCMR1_OC2PE;

  // connect outputs
  TIM4->CCER = 
    TIM_CCER_CC2E | TIM_CCER_CC2P;

  // enable counter with ARR buffering
  TIM4->CR1 = TIM_CR1_ARPE;
}

void speakerOn() {
  TIM4->CR1 |= TIM_CR1_CEN;
}

void speakerOff() {
  TIM4->CR1 &= ~TIM_CR1_CEN;
}

void toggleSpeaker() {
  if (TIM4->CR1 & TIM_CR1_CEN) {
    speakerOff();
  } else {
    speakerOn();
  }
}

static void updateFreq() {
  TIM4->ARR = fakeWaveLen;
  TIM4->CCR2 = fakeWaveLen * 99 / 100;
}

void changeWaveLen(int by) {
  fakeWaveLen += by;
  updateFreq();
  char msg[] = "Freq changed to ______\n";
  int first = sizeof("Freq changed to ") - 1;
  int digit = 100000;
  for (int i = 0; i < 6; ++i) {
    msg[first + i] = '0' + (fakeWaveLen % (digit * 10)) / digit;
    digit /= 10;
  }
  dmaSendWithCopy(msg, sizeof(msg) - 1);
}

// Lookup table for note wavelengths generated by manually finding a couple of notes 
// using a software pitch detector (tuned to A4=440), and from that, a simple linear regression
// calculated the rest. Precise wavelengths taken from https://pages.mtu.edu/~suits/notefreqs.html.
// All of the above was done in IPython, so no record of it exists, but it's very simple to replicate
// with just basic knowledge of numpy and scikit-learn.

const int note_lengths[] = { 
#include "note_lengths.txt"
};

int getNoteLength(Note note) {
  // on 16-bit timer B2 is the lowest possible note, we need TIM2 or TIM5 to play every note
  assert(note.octave > 2 || note.octave == 2 && note.letter == 12);
  return note_lengths[note.octave * 12 + note.letter - 1];
}

void setNote(Note note) {
  char msg[] = "Setting note with octave=_ and letter=__\n";
  msg[sizeof("Setting note with octave=") - 1] = note.octave + '0';
  msg[sizeof("Setting note with octave=_ and letter=") - 1] = (note.letter / 10) + '0';
  msg[sizeof("Setting note with octave=_ and letter=")] = (note.letter % 10) + '0';
  dmaSendWithCopy(msg, sizeof(msg) - 1);
  changeWaveLen(getNoteLength(note) - fakeWaveLen);
}
