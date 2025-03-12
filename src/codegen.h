/**
 * Assembly AST
 * program = Program(function_definition)
 * function_definition = Function(identifier_name, instruction* instructions)
 * instruction = Mov(operand src, operand dst)
 *              | Unary(unary_operator, operand)
 *              | AllocateStack(int)
 *              | Ret
 * unary_operator = Neg | Not
 * operand = Imm(int) | Reg(reg) | Pseudo(identifier) | Stack(int)
 * reg = W0 | W10
 */
#ifndef BCC_SRC_CODEGEN_H_
#define BCC_SRC_CODEGEN_H_

#include "arena.h"
#include "ir_gen.h"
#include "parser.h"

typedef enum {
  MOV,
  RET,
  UNARY,
  ALLOC_STACK
} InstructionType;

typedef enum {
  W0,
  W10
} Register;

typedef enum {
  REGISTER,
  IMM
} OperandType;

typedef struct {
  OperandType type;
  union {
    int imm;
    Register register;
  };
} Operand;

typedef struct {
  Operand src;
  Operand dst;
} Mov;

typedef enum {
  NOT,
  NEG
} UnaryOperator;

typedef struct {
  UnaryOperator op;
  Operand operand;
} ArmUnary;

typedef struct {
  int size;
} AllocStack;

typedef struct {
  InstructionType type;
  union {
    Mov mov;
    ArmUnary;
    AllocStack;
  };
} Instruction;

typedef struct {
  char* name;
  Instruction* instructions;
  int length;
} ArmFunction;

typedef struct {
  ArmFunction* function_def;
} ArmProgram;

ArmProgram* Translate(Arena* arena, Program* program);
ArmProgram* TranslateTacky(Arena* arena, TackyProgram tacky_program);
void WriteArmAssembly(ArmProgram* program, char* s_file);

#endif //BCC_SRC_CODEGEN_H_
