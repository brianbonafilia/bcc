#include "pretty_print.h"

#include <stdio.h>
#include "parser.h"
#include "ir_gen.h"
#include "codegen.h"

void PrintExpression(Exp* exp, int padding);

void PrintUnary(UnaryExp unary_exp, int padding) {
  char* op;
  switch (unary_exp.op_type) {
    case COMPLEMENT:
      op = "Complement";
      break;
    case NEGATE:
      op = "Negate";
  }
  printf("%*s%s,\n", padding, "", op);
  PrintExpression(unary_exp.exp, padding);
}

void PrintExpression(Exp* exp, int padding) {
  switch(exp->type) {
    case eConst:
      printf("%*sConstant(%d)\n", padding, "", exp->const_val);
      return;
    case eUnaryExp: {
      printf("%*sUnary(\n", padding, "");
      PrintUnary(exp->unary_exp, padding + 2);
      printf("%*s)\n", padding, "");
    }
  }
}

void PrintStatement(Statement* statement, int padding) {
  PrintExpression(statement->exp, padding);
}

void PrintFunction(Function* function, int padding) {
  printf("%*sname = \"%s\"\n", padding, "", function->name);
  printf("%*sbody = Return(\n", padding, "");
  PrintStatement(function->statement, padding + 2);
  printf("%*s)\n", padding, "");
}

void PrettyPrintAST(Program* program) {
  printf("Program(\n");
  printf("  Function(\n");
  PrintFunction(program->function, 4);
  printf("  )\n)\n");
}

void PrintTackyVal(TackyVal val) {
  switch (val.type) {
    case TACKY_CONST:
      printf("%d", val.const_val);
      return;
    case TACKY_VAR:
      printf("%s", val.identifier);
      return;
  }
}

void PrintTackyReturn(TackyVal val, int padding) {
  switch(val.type) {
    case TACKY_CONST:
      printf("%*sReturn(%d),\n", padding, "", val.const_val);
      return;
    case TACKY_VAR:
      printf("%*sReturn(%s),\n", padding, "", val.identifier);
      return;
  }
}

void PrintTackyUnary(TackyUnary unary, int padding) {
  switch (unary.op) {
    case TACKY_COMPLEMENT:
      printf("%*sUnary(Complement, ", padding, "");
      break;
    case TACKY_NEGATE:
      printf("%*sUnary(Negate, ", padding, "");
      break;
  }
  PrintTackyVal(unary.src);
  printf(", ");
  PrintTackyVal(unary.dst);
  printf("),\n");
}

void PrintTackyInstruction(TackyInstruction* instruction, int padding) {
  switch(instruction->type) {
    case TACKY_RETURN:
      PrintTackyReturn(instruction->return_val, padding);
      return;
    case TACKY_UNARY:
      PrintTackyUnary(instruction->unary, padding);
      return;
  }
}

void PrintTackyFunction(TackyFunction* tf, int padding) {
  printf("%*sidentifier =  \"%s\"\n", padding, "", tf->identifier);
  printf("%*sinstructions = [\n", padding, "");
  padding += 2;
  for (int i = 0; i < tf->instr_length; ++i) {
    PrintTackyInstruction(tf->instructions + i, padding);
  }
  padding -= 2;
  printf("%*s]\n", padding, "");
}

void PrettyPrintTacky(TackyProgram* tacky_program) {
  printf("Program(\n");
  printf("  Function(\n");
  PrintTackyFunction(tacky_program->function_def, 4);
  printf("  )\n)\n");
}

void PrintRegister(Register reg) {
  switch(reg) {
    case W0:
      printf("W0");
      return;
    case W10:
      printf("W10");
      return;
    case W11:
      printf("W11");
      return;
  }
}

void PrintOperand(Operand op) {
  switch (op.type) {
    case REGISTER:
      PrintRegister(op.reg);
      return;
    case IMM:
      printf("%d", op.imm);
      return;
    case PSEUDO:
      printf("%s", op.identifier);
      return;
    case STACK:
      printf("Stack(%d)", op.stack_location);
      return;
  }
}

void PrintArmUnaryOp(UnaryOperator op) {
  switch(op) {
    case NEG:
      printf("NEG");
      return;
    case NOT:
      printf("NOT");
      return;
  }
}

void PrintArmUnary(ArmUnary unary, int padding) {
  printf("%*sUnary(", padding, "");
  PrintArmUnaryOp(unary.op);
  printf(", ");
  PrintRegister(unary.reg);
  printf(")\n");
}

void PrintTwoAddress(Operand src, Operand dst) {
  PrintOperand(src);
  printf(", ");
  PrintOperand(dst);
  printf("),\n");
}

void PrintArmInstruction(Instruction* instr, int padding) {
  switch(instr->type) {
    case UNARY:
     PrintArmUnary(instr->unary, padding);
     return;
    case LDR:
      printf("%*sLDR(", padding, "");
      PrintTwoAddress(instr->mov.src, instr->mov.dst);
      return;
    case STR:
      printf("%*sSTR(", padding, "");
      PrintTwoAddress(instr->mov.src, instr->mov.dst);
      return;
    case MOV:
      printf("%*sMOV(", padding, "");
      PrintTwoAddress(instr->mov.src, instr->mov.dst);
      return;
    case RET:
      printf("%*sRET,\n", padding, "");
      return;
    case ALLOC_STACK:
      printf("%*sAllocStack(%d)\n", padding, "", instr->alloc_stack.size);
      return;
    case DEALLOC_STACK:
      printf("%*sDeallocStack(%d)\n", padding, "", instr->alloc_stack.size);
      return;

  }
}

void PrintArmFunc(ArmFunction* f, int padding) {
  printf("%*sidentifier = \"%s\"\n", padding, "", f->name);
  printf("%*sinstructions = [\n", padding, "");
  padding += 2;
  for (int i = 0; i < f->length; ++i) {
    PrintArmInstruction(f->instructions + i, padding);
  }
  padding -= 2;
  printf("%*s]\n", padding, "");
}

void PrettyPrintAssemblyAST(ArmProgram* arm_program) {
  printf("ArmProgram(\n");
  printf("  Function(\n");
  PrintArmFunc(arm_program->function_def, 4);
  printf("  )\n)\n");
}

