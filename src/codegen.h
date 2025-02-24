/**
 * Assembly AST
 * program = Program(function_definition)
 * function_definition = Function(identifier_name, instruction* instructions)
 * instruction = Mov(operand src, operand dst) | Ret
 * operand = Imm(int) | Register
 */
#ifndef BCC_SRC_CODEGEN_H_
#define BCC_SRC_CODEGEN_H_

typedef enum {
  MOV,
  RET
} InstructionType;

typedef struct

typedef struct {

} Mov;

typedef struct {

  union {

  };
} Instruction;

typedef struct {

  int length;
} ArmFunction;

typedef struct {
  char* name;
  ArmFunction* function_def;
} ArmProgram;

#endif //BCC_SRC_CODEGEN_H_
