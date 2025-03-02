#ifndef HAL_h
#define HAL_h

#include <stddef.h>
#include <stdint.h>

// This is really weird, I might just get rid of HAL.
// The original purpose was for testing on native, but
// tbh it's just easier to test with a teensy.
#ifdef ARDUINO
#include <Arduino.h>
#else
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define INPUT_PULLDOWN 3
#define OUTPUT_OPENDRAIN 4
#define INPUT_DISABLE 5
#define LSBFIRST 0
#define MSBFIRST 1
#define _BV(n) (1 << (n))
#define CHANGE 4
#define FALLING 2
#define RISING 3
#define LED_BUILTIN 13
#endif

void hal_pinMode(uint8_t pin, uint8_t mode);
uint8_t hal_digitalRead(uint8_t pin);
void hal_digitalWrite(uint8_t pin, uint8_t val);
void hal_delay(uint32_t msec);
uint32_t hal_millis();

void hal_print(const char s[]);
void hal_println(const char s[]);
void hal_printf(const char format[], ...);

#endif
