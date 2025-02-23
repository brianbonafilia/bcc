/*
 * Parses a list of tokens using recursive descent.
 *
 * Initially scoped down to a small grammar, which is as follows
 * <program> ::= <function>
 * <function> ::= "int" <identifier> "(" "void" ")" "{" <statement> "}"
 * <statement> ::= "return" <exp> ";"
 * <exp> ::= <int>
 * <identifier> ::= ? identifier ?
 * <int> ::= ? constant ?
 *
 * With the Abstract Syntax Tree Defined as
 * program = Program(function_definition()
 * function_definition = Function(identifier name, statement body)
 * statement = Return(exp)
 * exp = Constant(int)
 */
#ifndef SRC_PARSER_H
#define SRC_PARSER_H
#include "lexer.h"

typedef enum {
  eConst,
} ExpType;

typedef struct {
  ExpType type;
  union {
    int constValue;
  };
} Exp;

typedef struct {
  Exp* exp;
} Statement;

typedef struct {
  char* name;
  Statement* statement;
} Function;

typedef struct {
  Function* function;
} Program;

Program* ParseTokens(Arena* arena, TokenList list);
#endif
