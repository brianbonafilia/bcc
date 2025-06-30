#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver.h"

#define MIN_ARGUMENTS 2

int main(int argc, char **argv) {

  if (argc < MIN_ARGUMENTS) {
    fprintf(stderr, "Incorrect number of arguments, got %d, expected "
                    "at least %d",
            argc, MIN_ARGUMENTS);
    exit(1);
  }
  Mode mode = FULL;
  if (argc == 3) {
    char* opt = argv[1];
    if (strcmp(opt, "--lex") == 0) {
      mode = LEX;
    } else if (strcmp(opt, "--parse") == 0) {
      mode = PARSE;
    } else if (strcmp(opt, "--tacky") == 0) {
      mode = TACKY;
    } else if (strcmp(opt, "--codegen") == 0) {
        mode = CODEGEN;
    } else {
      fprintf(stderr, "Invalid option not know: %s", opt);
      exit(1);
    }
    ++argv;
  }
  Compile(argv[1], mode);
  return 0;
}
