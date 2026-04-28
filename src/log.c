#include "log.h"
#include "options.h"

// print out terminal log if log options enabled
void print_log(const char *message, ...){
  va_list args;

  if(options.log){
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fputc('\n', stderr);
  }
}