#ifndef BCC_SRC_DRIVER_H
#define BCC_SRC_DRIVER_H

typedef enum {
  LEX,
  PARSE,
  CODEGEN,
  FULL
} Mode;

void Compile(char* file_name, Mode mode);

#endif // BCC_SRC_DRIVER_H
