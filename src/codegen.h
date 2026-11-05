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
  // The default initial mov
  MOV,
  // Store register in mem
  STR,
  // load from mem to register
  LDR,
  RET,
  UNARY,
  BINARY,
  MSUB,
  ALLOC_STACK,
  DEALLOC_STACK,
  BRANCH,
  SET_CC,
  LABEL,
  CMP,
  CMP_BRANCH,
} InstructionType;

typedef enum {
  W0,
  W10,
  W11,
  W12,
  W13,
} Register;

typedef enum {
  REGISTER,
  IMM,
  PSEUDO,
  STACK
} OperandType;

typedef struct {
  OperandType type;
  union {
    // immediate value
    int imm;
    // stack location within frame
    int stack_location;
    // variable name, 31 char max as per C lang.
    char identifier[32];
    // self explanatory :)
    Register reg;
  };
} Operand;

typedef struct {
  Operand src;
  Operand dst;
} Mov;

typedef enum {
  NOT,
  NEG,
} UnaryOperator;

// remainder excluded, to be done in three steps...
// Divide -> multiply -> subtract
typedef enum {
  A_ADD,
  A_SUBTRACT,
  A_DIVIDE,
  A_MULTIPLY,
  A_OR,
  A_XOR,
  A_AND,
  A_RSHIFT,
  A_LSHIFT,
  A_CMP,
} BinaryOperator;

// no DST as we just output to input register.
typedef struct {
  UnaryOperator op;
  Register reg;
} ArmUnary;

// no dst register, outputs to right register.
typedef struct {
  BinaryOperator op;
  Register left;
  Register right;
  Register dst;
} ArmBinary;

typedef struct {
  // the Minuend
  Register left;
  // the Subtrahend
  Register right;
  // multiplies the subtrahend
  Register m;
  Register dst;
} ArmMsub;

typedef struct {
  int size;
} AllocStack;

typedef enum {
  // equal , not equal, greater, greater equal...
  B_E, B_NE, B_G, B_GE, B_L, B_LE, B_Z, B_NZ, B_NO_CC
} ArmCC;

typedef struct {
  ArmCC cc;
  labelStr label;
} Branch;

typedef struct {
  Branch branch;
  Register reg;
} CompareBranch;

typedef struct {
  ArmCC cc;
  Register reg;
} SetCC;

typedef struct {
  labelStr identifier;
} ArmLabel;

typedef struct {
  InstructionType type;
  union {
    Mov mov;
    ArmUnary unary;
    ArmBinary binary;
    ArmMsub msub;
    AllocStack alloc_stack;
    Branch branch;
    CompareBranch cmp_branch;
    ArmLabel label;
    SetCC set_cc;
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

ArmProgram* TranslateTacky(Arena* arena, TackyProgram* tacky_program);
void ReplacePseudoRegisters(Arena* scratch, ArmProgram* tacky_program);
void InstructionFixUp(Arena* arena, ArmProgram* tacky_program);
void WriteArmAssembly(ArmProgram* program, char* s_file);
char* GetCcStr(ArmCC cc);
char* GetRegisterStr(Register reg);
char* ToUnaryOpStr(UnaryOperator op);
char* ToBinaryOpStr(BinaryOperator op);
#endif //BCC_SRC_CODEGEN_H_
