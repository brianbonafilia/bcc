/*
 * Parses a list of tokens using recursive descent.
 *
 * Initially scoped down to a small grammar, which is as follows
 * <program> ::= <function>
 * <function> ::= "int" <identifier> "(" "void" ")" "{" { <block_item> } "}"
 * <block_item> ::= <statement> | <declaration>
 * <block> ::= "{" { <block_item> } "}"
 * <declaration> ::= "int" <identifier> [ "=" <exp> ] ";"
 * <for_init> ::= <declaration> | [ <exp> ] ";"
 * <statement> ::= "return" <exp> ";"
 *      | <exp> ";"
 *      | <block>
 *      | "if" "(" <exp> ")" <statement> [ "else" <statement> ]
 *      | "goto" <identifier> ":"
 *      | <identifier> ":" <statement>
 *      | "break" ";"
 *      | "continue" ";"
 *      | "do" <statement> "while" "(" <exp> ")" ";"
 *      | "while" "(" <exp> ")" <statement>
 *      | "for" "(" <for_init> [ <exp> ] ";" [ <exp> ] ")" <statement>
 *      | ";"
 * <exp> ::= <factor> | <exp> <binop> <exp> | <exp> "?" <exp> ":" <exp>
 * <factor> ::= <int> | <identifier> | <unop> <factor> | "(" <exp> ")"
 * <unop> ::= "-" | "~" | "!" | "++" | "--"
 * <postfix> ::= "++" | "--"
 * <binop> ::= "-" | "-" | "*" | "/" | "%" | "&&" | "||"
 *      | "==" | "!=" | "<" | "<=" | ">" | ">=" | "|" | "&" | "<<"
 *      | ">>" | "^"
 * <identifier> ::= ? identifier ?
 * <int> ::= ? constant ?
 *
 * With the Abstract Syntax Tree Defined as
 * program = Program(function_definition()
 * function_definition = Function(identifier name, block body)
 * block_item = S(statement) | D(declaration)
 * block = Block(block_item*)
 * for_init = InitDecl(declaration) | InitExp(exp?)
 * statement = Return(exp)
 *    | Expression(exp)
 *    | If(exp condition, statement then, statement? else)
 *    | Goto(identifier label)
 *    | GotoLabel(identifier label, statement next)
 *    | Compound(block)
 *    | Break(identifier label)
 *    | Continue(identifier label)
 *    | While(exp condition, statement body, identifier label)
 *    | DoWhile(statement body, exp condition, identifier label)
 *    | For(for_init init, exp? condition, exp? post, statement body, identifier label)
 *    | null
 * exp = Constant(int)
 *    | Var(identifier)
 *    | Unary(unary_operator, exp)
 *    | PostFix(exp, postfix_operator)
 *    | Binary(binary_operator, exp, exp)
 *    | Assignment(exp, exp)
 *    | Conditional(exp condition, exp, exp)
 * unary_operator = Complement | Negate | Logical Not | PreIncrement | PreDecrement
 * postfix_operator = PostIncrement | PostDecrement
 * binary_operator = Subtract | Add | Multiply | Divide | Remainder | Logical And
 *    | Logical Or | Not Equals | Equals | Less Than | Greater Than | Greater
 *    Equals | Lesser Equals | Or | And | XOr | Left Shift | Right Shift
 *
 */
#ifndef BCC_SRC_PARSER_H
#define BCC_SRC_PARSER_H
#include <stdbool.h>
#include "arena.h"
#include "lexer.h"

typedef struct Block Block;
typedef struct Exp Exp;
typedef struct ConditionalExp ConditionalExp;
typedef struct Statement Statement;
typedef struct IfStatement IfStatement;
typedef struct While While;
typedef struct DoWhile DoWhile;
typedef struct For For;
typedef struct ForInit ForInit;
typedef struct GotoLabel GotoLabel;
typedef struct Declaration Declaration;
typedef struct Assignment Assignment;
typedef struct BlockItem BlockItem;
typedef struct PostFix PostFix;

typedef enum {
  eConst,
  eUnaryExp,
  eBinaryExp,
  eVar,
  ePostFix,
  eAssignment,
  eConditional
} ExpType;

typedef enum {
  B_STATEMENT,
  B_DECLARATION
} BlockItemType;

typedef enum {
  COMPLEMENT,
  NEGATE,
  LOGICAL_NOT,
  PRE_INCREMENT,
  PRE_DECREMENT
} UnaryOp;

typedef enum {
  POST_INCREMENT,
  POST_DECREMENT
} PostFixOp;

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
  I_JMP_ELSE,
  I_JMP_END
} IfJumpType;

typedef enum {
  S_RETURN,
  S_EXP,
  S_IF,
  S_NULL,
  S_GOTO,
  S_GOTO_LABEL,
  S_BLOCK,
  S_BREAK,
  S_CONTINUE,
  S_FOR,
  S_WHILE,
  S_DO_WHILE
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
  A_AND_EQUALS,
  A_OR_EQUALS,
  A_XOR_EQUALS,
  A_L_SHIFT_EQUALS,
  A_R_SHIFT_EQUALS,
} AssignmentType;

struct Assignment {
  Exp* left;
  Exp* right;
  AssignmentType type;
};

struct PostFix {
  Exp* exp;
  PostFixOp op;
};

struct ConditionalExp {
  Exp* cond;
  Exp* left;
  Exp* right;
};

struct Exp {
  ExpType type;
  union {
    int const_val;
    char* identifier;
    Assignment assignment;
    UnaryExp unary_exp;
    BinaryExp binary_exp;
    PostFix post_fix;
    ConditionalExp conditional_exp;
  };
};

struct IfStatement {
  Exp* cond;
  Statement* statement;
  bool has_else;
  Statement* else_statement;
};

struct GotoLabel {
  char* identifier;
  Statement* statement;
};

struct While {
  Exp* cond;
  Statement* body;
  char* identifier;
};

typedef enum {
  FI_DECLARATION,
  FI_EXP,
} ForInitType;

struct ForInit {
  ForInitType type;
  Declaration* dec;
  Exp* exp;
};

struct For {
  ForInit* for_init;
  Exp* cond;
  Exp* post;
};

struct Statement {
  StatementType type;
  union {
    Block* block;
    Exp* exp;
    IfStatement* if_statement;
    While* s_while;
    For* s_for;
    char* label_identifier;
    GotoLabel* goto_label;
  };
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
  Block* body;
} Function;

struct BlockItem {
  BlockItemType type;
  union {
    Statement* statement;
    Declaration* declaration;
  };
};

struct Block {
  BlockItemList* list;
};

typedef struct {
  Function* function;
} Program;

bool IsUnaryAssignment(UnaryOp op);
Program* ParseTokens(Arena* arena, TokenList list);
#endif // BCC_SRC_PARSER_H
