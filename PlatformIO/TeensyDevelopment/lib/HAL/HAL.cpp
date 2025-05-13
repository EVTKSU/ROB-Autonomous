#include "HAL.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <TeensyThreads.h>
#include <stdio.h>

void hal_pinMode(uint8_t pin, uint8_t mode) { pinMode(pin, mode); }

uint8_t hal_digitalRead(uint8_t pin) { return digitalRead(pin); }

void hal_digitalWrite(uint8_t pin, uint8_t val) { digitalWrite(pin, val); }

void hal_delay(uint32_t msec) { threads.delay(msec); }

uint32_t hal_millis() { return millis(); }

void hal_print(const char s[]) { Serial.print(s); }

void hal_println(const char s[]) { Serial.println(s); }

// Don't print more than 256 chars at a time
void hal_printf(const char format[], ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, format);
  vsprintf(buf, format, ap);
  Serial.print(buf);
  va_end(ap);
}

#else
// TODO implement (we only care about unix)

#include <stdio.h>

void hal_pinMode(uint8_t pin, uint8_t mode) {}

uint8_t hal_digitalRead(uint8_t pin) {}

void hal_digitalWrite(uint8_t pin, uint8_t val) {}

void hal_delay(uint32_t msec) {}

uint32_t hal_millis() {}

void hal_print(const char s[]) { printf(s); }

void hal_println(const char s[]) { printf("%s\n", s); }

void hal_printf(const char format[], ...) {
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
}

#endif
