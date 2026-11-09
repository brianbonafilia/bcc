//
// Created by Brian Bonafilia on 5/25/26.
//
#include "semantic_analysis.h"

#include <stdlib.h>
#include <string.h>
#include "parser.h"

const int default_capacity = 32;
int variableCount = 0;
int labelCount = 0;

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

char* AssignGotoLabel(char* dst, char* name) {
  sprintf(dst, "L%s%d", name, labelCount);
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
    case S_IF:
      ResolveExp(m, statement->if_statement->cond);
      ResolveStatement(m, statement->if_statement->statement);
      if (statement->if_statement->has_else) {
        ResolveStatement(m, statement->if_statement->else_statement);
      }
      return;
    case S_GOTO_LABEL:
      ResolveStatement(m, statement->goto_label->statement);
      return;
    case S_GOTO:
      return;
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
        fprintf(stderr, "Invalid lvalue\n");
        exit(2);
      }
      ResolveExp(m, exp->assignment.left);
      ResolveExp(m, exp->assignment.right);
      return;
    case eVar:
      v = VariableMapGet(m, exp->identifier);
      if (v == NULL) {
        fprintf(stderr, "Undeclared variable! %s\n", exp->identifier);
        exit(2);
      }
      exp->identifier = v;
      return;
    case eBinaryExp:
      ResolveExp(m, exp->binary_exp.left);
      ResolveExp(m, exp->binary_exp.right);
      return;
    case ePostFix:
      // TODO: eventually we will have postfix operators which
      // behave differently.
      if (exp->post_fix.exp->type != eVar) {
        fprintf(stderr, "expected lvalue\n");
        exit(1);
      }
      ResolveExp(m, exp->post_fix.exp);
      return;
    case eUnaryExp:
      if (IsUnaryAssignment(exp->unary_exp.op_type)) {
        if (exp->unary_exp.exp->type != eVar) {
          fprintf(stderr, "expected lvalue\n");
          exit(1);
        }
      }
      ResolveExp(m, exp->unary_exp.exp);
      return;
    case eConditional:
      ResolveExp(m, exp->conditional_exp.left);
      ResolveExp(m, exp->conditional_exp.cond);
      ResolveExp(m, exp->conditional_exp.right);
      return;
    case eConst:
      return;
    default:
      fprintf(stderr, "unknown expression type\n");
      exit(1);
  }
}

void AddStatementGotoLabels(VariableMap* m, Statement* s) {
  if (s->type == S_IF) {
    AddStatementGotoLabels(m, s->if_statement->statement);
    if (s->if_statement->has_else) {
      AddStatementGotoLabels(m, s->if_statement->else_statement);
    }
  } else if (s->type == S_GOTO_LABEL) {
    char* got = VariableMapGet(m, s->goto_label->identifier);
    if (got != NULL) {
      fprintf(stderr, "repeated goto label in the same function: %s\n",
              s->goto_label->identifier);
      exit(1);
    }
    char* v = malloc(sizeof(char) * 50);
    // To prevent name collisions with functions we will fiddle with the label name.
    // This is because function and labels can have the same name.
    VariableMapPut(m, s->goto_label->identifier, AssignGotoLabel(v, s->goto_label->identifier));
    s->goto_label->identifier = v;
    AddStatementGotoLabels(m, s->goto_label->statement);
  }
}

void CheckStatementGotoLabels(VariableMap* m, Statement* s) {
  if (s->type == S_IF) {
    CheckStatementGotoLabels(m, s->if_statement->statement);
    if (s->if_statement->has_else) {
      CheckStatementGotoLabels(m, s->if_statement->else_statement);
    }
  } else if (s->type == S_GOTO_LABEL) {
    CheckStatementGotoLabels(m, s->goto_label->statement);
  } if (s->type == S_GOTO) {
    char* got = VariableMapGet(m, s->goto_identifier);
    if (got == NULL) {
      fprintf(stderr, "unknown jump label: %s\n", s->goto_identifier);
      exit(1);
    }
    s->goto_identifier = got;
  }
}

void ValidateGotoLabels(Function* f) {
  VariableMap* map = MakeVariableMap();
  // check for duplicate labels
  for (int i = 0; i < f->item_list.length; ++i) {
    BlockItem* item = f->item_list.items[i];
    if (item->type == B_STATEMENT) {
      AddStatementGotoLabels(map, item->statement);
    }
  }
  // validate that goto jumps are to existing labels.
  for (int i = 0; i < f->item_list.length; ++i) {
    BlockItem* item = f->item_list.items[i];
    if (item->type == B_STATEMENT) {
      CheckStatementGotoLabels(map, item->statement);
    }
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
  ValidateGotoLabels(program->function);
}
