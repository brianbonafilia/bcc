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
 *  instruction = Return(val) | Unary(unary_operator, val src, val dst)
 *  val = Constant(int) | Var(identifier)
 *  unary_operator = Complement | Negate
 * 
 */

#include "arena.h"
#include "parser.h"

typedef enum {
  TACKY_CONST,
  TACKY_VAR
} TackyValType;

typedef enum {
  TACKY_COMPLEMENT,
  TACKY_NEGATE
} TackyUnaryOp;

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

typedef enum {
  TACKY_RETURN,
  TACKY_UNARY,
} TackyInstrType;

typedef struct {
  TackyInstrType type;
  union {
    TackyVal return_val;
    TackyUnary unary;
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
