#include "pretty_print.h"

#include <stdio.h>
#include "parser.h"
#include "ir_gen.h"
#include "codegen.h"

void PrintExpression(Exp* exp, int padding);

char* BinaryOpStr(BinaryOp op) {
  switch (op) {
    case ADD:
      return "Add";
    case MULTIPLY:
      return "Multiply";
    case DIVIDE:
      return "Divide";
    case REMAINDER:
      return "remainder";
    case SUBTRACT:
      return "Subtract";
    case OR:
      return "Or";
    case AND:
      return "And";
    case XOR:
      return "Xor";
    case RIGHT_SHIFT:
      return "RightShift";
    case LEFT_SHIFT:
      return "LeftShift";
    case LOGICAL_AND:
      return "LogicalAnd";
    case LOGICAL_OR:
      return "LogicalOr";
    case EQUAL:
      return "Equals";
    case NOT_EQUAL:
      return "NotEquals";
    case GREATER_THAN:
      return "GreaterThan";
    case GREATER_OR_EQUAL:
      return "GreaterOrEqual";
    case LESS_THAN:
      return "LessThan";
    case LESS_OR_EQUAL:
      return "LessOrEqual";
    default:
      fprintf(stderr, "uh oh, unexpected binary op, code:%d", op);
      exit(2);
  }
}

void PrintBinary(BinaryExp exp, int padding) {
  char* op = BinaryOpStr(exp.op);
  printf("%*s%s, \n", padding, "", op);
  PrintExpression(exp.left, padding);
  PrintExpression(exp.right, padding);
}

void PrintUnary(UnaryExp unary_exp, int padding) {
  char* op;
  switch (unary_exp.op_type) {
    case COMPLEMENT:
      op = "Complement";
      break;
    case NEGATE:
      op = "Negate";
      break;
    case LOGICAL_NOT:
      op = "Not";
      break;
  }
  printf("%*s%s,\n", padding, "", op);
  PrintExpression(unary_exp.exp, padding);
}

void PrintExpression(Exp* exp, int padding) {
  switch (exp->type) {
    case eConst:
      printf("%*sConstant(%d)\n", padding, "", exp->const_val);
      return;
    case eUnaryExp: {
      printf("%*sUnary(\n", padding, "");
      PrintUnary(exp->unary_exp, padding + 2);
      printf("%*s)\n", padding, "");
      return;
    }
    case eBinaryExp:
      printf("%*sBinary(\n", padding, "");
      PrintBinary(exp->binary_exp, padding + 2);
      printf("%*s)\n", padding, "");
      return;
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
  switch (val.type) {
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
    case TACKY_L_NOT:
      printf("%*sUnary(LogicalNot, ", padding, "");
      break;
    default:
      fprintf(stderr, "Invalid Unary Op\n");
      exit(2);
  }
  PrintTackyVal(unary.src);
  printf(", ");
  PrintTackyVal(unary.dst);
  printf("),\n");
}

char* GetBinaryOpStr(TackyBinaryOp op) {
  switch (op) {
    case TACKY_ADD:
      return "Add";
    case TACKY_SUBTRACT:
      return "Subtract";
    case TACKY_MULTIPLY:
      return "Multiply";
    case TACKY_DIVIDE:
      return "Divide";
    case TACKY_REMAINDER:
      return "Remainder";
    case TACKY_OR:
      return "Or";
    case TACKY_AND:
      return "And";
    case TACKY_XOR:
      return "Xor";
    case TACKY_LSHIFT:
      return "LShift";
    case TACKY_RSHIFT:
      return "RShift";
    case TACKY_NOT_EQUAL:
      return "NotEquals";
    case TACKY_EQUAL:
      return "Equals";
    case TACKY_GREATER_THAN:
      return "GreaterThan";
    case TACKY_LESS_THAN:
      return "LessThan";
    case TACKY_LE_EQUAL:
      return "LessOrEqual";
    case TACKY_GE_EQUAL:
      return "GreaterOrEqual";
    default:
      fprintf(stderr, "encounterd unexpected binary op");
      exit(2);
  }
}

void PrintTackyBinary(TackyBinary binary, int padding) {
  char* op = GetBinaryOpStr(binary.op);

  printf("%*sBinary(%s, ", padding, "", op);
  PrintTackyVal(binary.left);
  printf(", ");
  PrintTackyVal(binary.right);
  printf(", ");
  PrintTackyVal(binary.dst);
  printf("),\n");
}

void PrintTackyJmpCC(JumpCond jc, int padding) {
  PrintTackyVal(jc.val);
  printf(", %s)\n", jc.target);
}

void PrintTackyInstruction(TackyInstruction* instr, int padding) {
  switch (instr->type) {
    case TACKY_RETURN:
      PrintTackyReturn(instr->return_val, padding);
      return;
    case TACKY_UNARY:
      PrintTackyUnary(instr->unary, padding);
      return;
    case TACKY_BINARY:
      PrintTackyBinary(instr->binary, padding);
      return;
    case TACKY_LABEL:
      printf("%*sLabel(%s)\n", padding, "", instr->label);
      return;
    case TACKY_JMP:
      printf("%*sJMP(%s)\n", padding, "", instr->jump_cond.target);
      return;
    case TACKY_JMP_NZ:
      printf("%*sJMP_NZ(", padding, "");
      PrintTackyJmpCC(instr->jump_cond, padding);
      return;
    case TACKY_JMP_Z:
       printf("%*sJMP_Z(", padding, "");
      PrintTackyJmpCC(instr->jump_cond, padding);
      return;
    case TACKY_COPY:
      printf("%*sCOPY(");
      PrintTackyVal(instr->copy.src);
      printf(" , ");
      PrintTackyVal(instr->copy.dst);
      printf(")\n");
      return;
    default:
      fprintf(stderr, "Encountered unexpected tacky instr type");
      exit(2);
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
  switch (reg) {
    case W0:
      printf("W0");
      return;
    case W10:
      printf("W10");
      return;
    case W11:
      printf("W11");
      return;
    case W12:
      printf("W12");
      return;
    case W13:
      printf("W13");
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
  switch (op) {
    case NEG:
      printf("NEG");
      return;
    case NOT:
      printf("NOT");
      return;
  }
}

void PrintArmBinaryOp(BinaryOperator op) {
  switch (op) {
    case A_ADD:
      printf("ADD");
      return;
    case A_SUBTRACT:
      printf("SUB");
      return;
    case A_MULTIPLY:
      printf("MUL");
      return;
    case A_DIVIDE:
      printf("DIV");
      return;
    case A_OR:
      printf("OR");
      return;
    case A_AND:
      printf("AND");
      return;
    case A_XOR:
      printf("XOR");
      return;
    case A_RSHIFT:
      printf("ASR");
      return;
    case A_LSHIFT:
      printf("LSL");
      return;
    case A_CMP:
      printf("CMP");
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

void PrintArmBinary(ArmBinary binary, int padding) {
  printf("%*sBinary(%s, %s, %s, %s)\n", padding, "",
         ToBinaryOpStr(binary.op),
         GetRegisterStr(binary.left), GetRegisterStr(binary.right),
         GetRegisterStr(binary.dst));
}

void PrintTwoAddress(Operand src, Operand dst) {
  PrintOperand(src);
  printf(", ");
  PrintOperand(dst);
  printf("),\n");
}

void PrintArmMsub(ArmMsub arm_msub, int padding) {
  printf("%*sMsub(", padding, "");
  PrintRegister(arm_msub.left);
  printf(", ");
  PrintRegister(arm_msub.right);
  printf(", ");
  PrintRegister(arm_msub.m);
  printf(", ");
  PrintRegister(arm_msub.dst);
  printf(")\n");
}

void PrintArmInstruction(Instruction* instr, int padding) {
  switch (instr->type) {
    case UNARY:
      PrintArmUnary(instr->unary, padding);
      return;
    case BINARY:
      PrintArmBinary(instr->binary, padding);
      return;
    case MSUB:
      PrintArmMsub(instr->msub, padding);
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
    case SET_CC:
      printf("%*sCSET(%s, %s)\n", padding, "", GetCcStr(instr->set_cc.cc), GetRegisterStr(instr->set_cc.reg));
      return;
    case LABEL:
      printf("%*sLabel(%s)\n", padding, "", instr->label.identifier);
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

