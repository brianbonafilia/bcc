/*
 * Parses a list of tokens using recursive descent.
 *
 * Initially scoped down to a small grammar, which is as follows
 * <program> ::= <function>
 * <function> ::= "int" <identifier> "(" "void" ")" "{" <statement> "}"
 * <statement> ::= "return" <exp> ";"
 * <exp> ::= <int> | <unop> <exp> | "(" <exp> ")"
 * <unop> ::= "-" | "~"
 * <identifier> ::= ? identifier ?
 * <int> ::= ? constant ?
 *
 * With the Abstract Syntax Tree Defined as
 * program = Program(function_definition()
 * function_definition = Function(identifier name, statement body)
 * statement = Return(exp)
 * exp = Constant(int) | Unary(unary_operator, exp)
 * unary_operator = Complement | Negate
 */
#ifndef BCC_SRC_PARSER_H
#define BCC_SRC_PARSER_H
#include "arena.h"
#include "lexer.h"

typedef enum {
  eConst,
  eUnaryExp
} ExpType;

typedef enum {
  COMPLEMENT,
  NEGATE
} UnaryOp;

typedef enum {
  S_RETURN
} StatementType;

typedef struct Exp Exp;
typedef struct {
  UnaryOp op_type;
  Exp* exp;
} UnaryExp;

struct Exp {
  ExpType type;
  union {
    int const_val;
    UnaryExp unary_exp;
  };
};

typedef struct {
  StatementType type;
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
#endif // BCC_SRC_PARSER_H
