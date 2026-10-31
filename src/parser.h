/*
 * Parses a list of tokens using recursive descent.
 *
 * Initially scoped down to a small grammar, which is as follows
 * <program> ::= <function>
 * <function> ::= "int" <identifier> "(" "void" ")" "{" { <block_item> } "}"
 * <block_item> ::= <statement> | <declaration>
 * <declaration> ::= "int" <identifier> [ "=" <exp> ] ";"
 * <statement> ::= "return" <exp> ";" | <exp> ";" | ";"
 * <exp> ::= <factor> | <exp> <binop> <exp>
 * <factor> ::= <int> | <identifier> | <unop> <factor> | "(" <exp> ")"
 * <unop> ::= "-" | "~" | "!"
 * <binop> ::= "-" | "-" | "*" | "/" | "%" | "&&" | "||"
 *      | "==" | "!=" | "<" | "<=" | ">" | ">=" | "|" | "&" | "<<"
 *      | ">>" | "^"
 * <identifier> ::= ? identifier ?
 * <int> ::= ? constant ?
 *
 * With the Abstract Syntax Tree Defined as
 * program = Program(function_definition()
 * function_definition = Function(identifier name, block_item* body)
 * block_item = S(statement) | D(declaration)
 * statement = Return(exp) | Expression(exp) | null
 * exp = Constant(int)
 *    | Var(identifier)
 *    | Unary(unary_operator, exp)
 *    | Binary(binary_operator, exp, exp)
 *    | Assignment(exp, exp)
 * unary_operator = Complement | Negate | Logical Not
 * binary_operator = Subtract | Add | Multiply | Divide | Remainder | Logical And
 *    | Logical Or | Not Equals | Equals | Less Than | Greater Than | Greater
 *    Equals | Lesser Equals | Or | And | XOr | Left Shift | Right Shift
 *
 */
#ifndef BCC_SRC_PARSER_H
#define BCC_SRC_PARSER_H
#include "arena.h"
#include "lexer.h"

typedef struct Exp Exp;
typedef struct Statement Statement;
typedef struct Declaration Declaration;
typedef struct Assignment Assignment;
typedef struct BlockItem BlockItem;

typedef enum {
  eConst,
  eUnaryExp,
  eBinaryExp,
  eVar,
  eAssignment,
} ExpType;

typedef enum {
  B_STATEMENT,
  B_DECLARATION
} BlockItemType;

typedef enum {
  COMPLEMENT,
  NEGATE,
  LOGICAL_NOT,
} UnaryOp;

typedef enum {
  ADD,
  SUBTRACT,
  MULTIPLY,
  DIVIDE,
  REMAINDER,
  OR,
  XOR,
  AND,
  RIGHT_SHIFT,
  LEFT_SHIFT,
  LOGICAL_AND,
  LOGICAL_OR,
  EQUAL,
  NOT_EQUAL,
  GREATER_THAN,
  GREATER_OR_EQUAL,
  LESS_THAN,
  LESS_OR_EQUAL,
} BinaryOp;

typedef enum {
  S_RETURN,
  S_EXP,
  S_NULL
} StatementType;

typedef struct {
  UnaryOp op_type;
  Exp* exp;
} UnaryExp;

typedef struct {
  BinaryOp op;
  Exp* left;
  Exp* right;
} BinaryExp;

typedef enum {
  A_EQUALS,
  A_PLUS_EQUALS,
  A_MINUS_EQUALS,
  A_MULTIPLY_EQUALS,
  A_DIVIDE_EQUALS,
  A_MODULO_EQUALS,
} AssignmentType;

struct Assignment {
  Exp* left;
  Exp* right;
  AssignmentType type;
};

struct Exp {
  ExpType type;
  union {
    int const_val;
    char* identifier;
    Assignment assignment;
    UnaryExp unary_exp;
    BinaryExp binary_exp;
  };
};

struct BlockItem {
  BlockItemType type;
  union {
    Statement* statement;
    Declaration* declaration;
  };
};

struct Statement {
  StatementType type;
  Exp* exp;
};

struct Declaration {
  char* identifier;
  Exp* init;
};

typedef struct {
  BlockItem** items;
  int length;
} BlockItemList;

typedef struct {
  char* name;
  BlockItemList item_list;
} Function;

typedef struct {
  Function* function;
} Program;

Program* ParseTokens(Arena* arena, TokenList list);
#endif // BCC_SRC_PARSER_H
