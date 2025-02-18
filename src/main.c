#include <stdio.h>
#include <stdlib.h>
#include "driver.h"

#define NUM_ARGUMENTS 2

int main(int argc, char** argv) {

  if (argc != NUM_ARGUMENTS) {
    fprintf(stderr, "Incorrect number of arguments, got %d, expected %d",
        argc, NUM_ARGUMENTS);
    exit(1); 
  }
  Compile(argv[1]);
  return 0;
}
