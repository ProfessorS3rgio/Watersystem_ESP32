#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
class Print {
public:
  char buffer[32768] = {};
  size_t used = 0;
  size_t write(uint8_t value) { assert(used + 1 < sizeof(buffer)); buffer[used++] = value; buffer[used] = 0; return 1; }
  size_t write(const uint8_t* data, size_t size) { for (size_t i=0;i<size;++i) write(data[i]); return size; }
  void print(const char* value) { write(reinterpret_cast<const uint8_t*>(value), strlen(value)); }
  void print(int value) { char number[24]; snprintf(number, sizeof(number), "%d", value); print(number); }
  void println(char value) { write(value); write('\n'); }
  void println(const char* value) { print(value); write('\n'); }
};
inline void delay(int) {}
