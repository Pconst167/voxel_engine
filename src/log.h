#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>

void print_log(const char *message, ...)
  __attribute__((format(printf, 1, 2)));

#endif