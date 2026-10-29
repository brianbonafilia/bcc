//
// Created by Brian Bonafilia on 5/25/26.
//

#ifndef BCC_SRC_SEMANTIC_ANALYSIS_H_
#define BCC_SRC_SEMANTIC_ANALYSIS_H_

#include "parser.h"

typedef struct {
  char* key;
  char* val;
} VariableKeyVal;

typedef struct {
  // array of keyVals, subject to address change.
  VariableKeyVal* variablesMap;
  int size;
  int capacity;
  double load_factor;
} VariableMap;

VariableMap* MakeVariableMap();

// Gets a variable from the hash map, if no variable found, returns NULL.
char* VariableMapGet(VariableMap* m, char* key);
void VariableMapPut(VariableMap* m, char* key, char* val);
void FreeMap(VariableMap* m);

void AnalyzeSemantics(Program* program);
void ResolveDeclaration(VariableMap* m, Declaration* dec);
void ResolveStatement(VariableMap* m, Statement* statement);
void ResolveExp(VariableMap* m, Exp* exp);



#endif //BCC_SRC_SEMANTIC_ANALYSIS_H_
