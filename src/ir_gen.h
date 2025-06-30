#ifndef BCC_SRC_IR_GEN_H
#define BCC_SRC_IR_GEN_H

/*
 * Intermediate Representation generation
 * 
 * TAC (three address code) 
 * 
 * Generally this is two source operands and a destination
 * 
 * AST definition
 *  program = (function_definition)
 *  function_definition(identifier, instruction* body)
 *  instruction = Return(val)
 *    | Unary(unary_operator, val src, val dst)
 *    | Binary(binary_operator, val left, val right, val dst)
 *  val = Constant(int) | Var(identifier)
 *  unary_operator = Complement | Negate | NOT
 *  binary_operator = Add | Subtract | Multiply | Divide | Remainder
 *      | GreaterThan | GreaterOrEqual | LessThan | LessOrEqual
 *      | Equal | NotEqual
 */

#include "arena.h"
#include "parser.h"

typedef enum {
  TACKY_CONST,
  TACKY_VAR
} TackyValType;

typedef enum {
  TACKY_COMPLEMENT,
  TACKY_NEGATE,
  TACKY_L_NOT,
} TackyUnaryOp;

typedef enum {
  TACKY_ADD,
  TACKY_SUBTRACT,
  TACKY_MULTIPLY,
  TACKY_DIVIDE,
  TACKY_REMAINDER,
  TACKY_OR,
  TACKY_AND,
  TACKY_XOR,
  TACKY_RSHIFT,
  TACKY_LSHIFT,
  TACKY_EQUAL,
  TACKY_NOT_EQUAL,
  TACKY_GREATER_THAN,
  TACKY_GE_EQUAL,
  TACKY_LESS_THAN,
  TACKY_LE_EQUAL,
} TackyBinaryOp;

typedef struct {
  TackyValType type;
  union {
    int const_val;
    char identifier[32]; 
  };
} TackyVal;

typedef struct {
  TackyUnaryOp op;
  TackyVal src;
  TackyVal dst;  
} TackyUnary;

typedef char labelStr[20];

typedef struct {
  TackyBinaryOp op;
  TackyVal left;
  TackyVal right;
  TackyVal dst;
} TackyBinary;

typedef struct {
  labelStr target;
  TackyVal val;
} JumpCond;

typedef enum {
  TACKY_RETURN,
  TACKY_UNARY,
  TACKY_BINARY,
  TACKY_COPY,
  TACKY_JMP,
  TACKY_JMP_Z,
  TACKY_JMP_NZ,
  TACKY_LABEL,
} TackyInstrType;

typedef struct {
  TackyVal src;
  TackyVal dst;
} TackyCopy;

typedef struct {
  TackyInstrType type;
  union {
    TackyVal return_val;
    TackyUnary unary;
    TackyBinary binary;
    JumpCond jump_cond;
    labelStr label;
    TackyCopy copy;
  };
} TackyInstruction;

typedef struct {
  TackyInstruction* instructions;
  int instr_length;
  char* identifier;
} TackyFunction;

typedef struct {
  TackyFunction* function_def;
} TackyProgram;

TackyProgram* EmitTackyProgram(Arena* arena, Program* program);

#endif // BCC_SRC_IR_GEN_H
