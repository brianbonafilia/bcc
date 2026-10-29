//
// Created by Brian Bonafilia on 6/3/26.
//
#include <string.h>
#include "semantic_analysis.h"
#include "stdbool.h"
#include "stdio.h"

#define RESULT(testPassed) !testPassed;

int main() {
  bool testPassed = true;

  VariableMap* map = MakeVariableMap();
  char* got = VariableMapGet(map, "toot");
  if (got != NULL) {
    FreeMap(map);
    return RESULT(false)
  }
  VariableMapPut(map, "test", "val");
  got = VariableMapGet(map, "test");
  if (got == NULL) {
    testPassed = false;
    fprintf(stderr, "Did not find expected value \"test\" from map");
  }
  if (strcmp(got, "val") != 0) {
    testPassed = false;
    fprintf(stderr, "Did not find expected value \"test\" from map");
  }

  FreeMap(map);
  return RESULT(testPassed);
}