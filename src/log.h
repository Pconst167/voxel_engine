#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

typedef struct console_t {
  bool is_open;
  char input_buffer[256];
  int input_length;
  int cursor_position;
} console_t;

extern console_t console;

void print_log(const char *message, ...)
  __attribute__((format(printf, 1, 2)));

#endif