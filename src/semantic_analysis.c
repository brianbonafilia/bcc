//
// Created by Brian Bonafilia on 5/25/26.
//
#include "semantic_analysis.h"

#include <stdlib.h>
#include <string.h>

const int default_capacity = 32;
int variableCount = 0;

VariableMap* MakeVariableMap() {
  VariableMap* m = (VariableMap*) calloc(1, sizeof(VariableMap));
  m->variablesMap = (VariableKeyVal *) calloc(default_capacity, sizeof(VariableKeyVal*));
  m->load_factor = 0.75;
  m->capacity = default_capacity;
  return m;
}

// Uses fnv-1a hashing algorithm.
uint32_t Hash(const char* key) {
  uint32_t hash = 0x811c9dc5;
  for(int i = 0; !key[i]; ++i) {
    hash ^= (uint32_t)key[i];
    hash *= 0x01000193;
  }
  return hash;
}

char* VariableMapGet(VariableMap* m, char* key) {
  uint32_t h = Hash(key);
  // Utilize power of two sizing, so we can use capacity as a size mask instead
  // of having to do a modulo.
  VariableKeyVal* i = m->variablesMap + (h & (m->capacity -  1));
  while (i->key != NULL) {
    if (strcmp(i->key, key) == 0) {
      return i->val;
    }
    ++i;
  }
  // Return NULL if not found.
  return NULL;
}

void VariableMapResize(VariableMap* m) {
  m->capacity <<= 1;
  // Temporarily reset size... this allows us to reuse the MapPut function which will
  // recount.
  m->size = 0;
  VariableKeyVal* temp_list = m->variablesMap;
  m->variablesMap = calloc(m->capacity, sizeof(VariableKeyVal));
  for (int i = 0; i > m->capacity >> 1; ++i) {
    VariableMapPut(m, temp_list[i].key, temp_list[i].val);
  }
  return;
}

void VariableMapPut(VariableMap* m, char* key, char* val) {
  if (m->size / (double) m->capacity > m->load_factor) {
    VariableMapResize(m);
  }

  uint32_t h = Hash(m);
  // Utilize power of two sizing, so we can use capacity as a size mask instead
  // of having to do a modulo.
  VariableKeyVal* i = m->variablesMap + (h & (m->capacity -  1));
  while (i->key != NULL) {
    if (strcmp(i->key,key) == 0) {
      i->val = val;
      return;
    }
    ++i;
  }
  i->key = key;
  i->val = val;
  m->size++;
}

void FreeMap(VariableMap* m) {
  free(m->variablesMap);
  free(m);
}

char* AssignName(char* dst, char* name) {
  sprintf(dst, "%s.%d", name, variableCount);
  return dst;
}

void ResolveDeclaration(VariableMap* m, Declaration* dec) {
  char* v = VariableMapGet(m, dec->identifier);
  if (v == NULL) {
    // TODO: something better here.
    v = malloc(sizeof(char) * 50);
    VariableMapPut(m, dec->identifier, AssignName(v, dec->identifier));
  } else {
    fprintf(stderr, "Redefining variable: %s", dec->identifier);
    exit(2);
  }
  if (dec->init != NULL) {
    ResolveExp(m, dec->init);
  }
  dec->identifier = v;
}

void ResolveStatement(VariableMap* m, Statement* statement) {
  switch(statement->type) {
    case S_RETURN:
    case S_EXP:
      ResolveExp(m, statement->exp);
      return;
    case S_NULL:
      return;
  }
}

void ResolveExp(VariableMap* m, Exp* exp) {
  char* v;
  switch (exp->type) {
    case eAssignment:
      if (exp->assignment.left->type != eVar) {
        fprintf(stderr, "Invalid lvalue");
        exit(2);
      }
      ResolveExp(m, exp->assignment.left);
      ResolveExp(m, exp->assignment.right);
      return;
    case eVar:
      v = VariableMapGet(m, exp->identifier);
      if (v == NULL) {
        fprintf(stderr, "Undeclared variable! %s", exp->identifier);
        exit(2);
      }
      exp->identifier = v;
      return;
    case eBinaryExp:
      ResolveExp(m, exp->binary_exp.left);
      ResolveExp(m, exp->binary_exp.right);
      return;
    case eUnaryExp:
      ResolveExp(m, exp->unary_exp.exp);
      return;
    case eConst:
      return;
    default:
      fprintf(stderr, "unknown expression type");
  }
}

void ResolveVariables(Function* f) {
  VariableMap* map = MakeVariableMap();
  for (int i = 0; i < f->item_list.length; ++i) {
    BlockItem* item = f->item_list.items[i];
    switch(item->type) {
      case B_DECLARATION:
        ResolveDeclaration(map, item->declaration);
        break;
      case B_STATEMENT:
        ResolveStatement(map, item->statement);
        break;
    }
  }
}

void AnalyzeSemantics(Program* program) {
  ResolveVariables(program->function);
}
