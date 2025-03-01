/**
 * Assembly AST
 * program = Program(function_definition)
 * function_definition = Function(identifier_name, instruction* instructions)
 * instruction = Mov(operand src, operand dst) | Ret
 * operand = Imm(int) | Register
 */
#ifndef BCC_SRC_CODEGEN_H_
#define BCC_SRC_CODEGEN_H_

#include "arena.h"
#include "parser.h"

typedef enum {
  MOV,
  RET
} InstructionType;

typedef enum {
  REGISTER,
  IMM
} OperandType;

typedef struct {
  OperandType type;
  union {
    int imm;
  };
} Operand;

typedef struct {
  Operand src;
  Operand dst;
} Mov;

typedef struct {
  InstructionType type;
  union {
    Mov mov;
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
void WriteArmAssembly(ArmProgram* program, char* s_file);

#endif //BCC_SRC_CODEGEN_H_
