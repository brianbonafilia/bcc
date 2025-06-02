#include <string.h>
#include "codegen.h"
#include "parser.h"
#include "arena.h"

#define ASM_PADDING 4
#define VAR_SIZE 8

void AllocInstr(Arena* arena, ArmFunction* arm_func) {
  if (arm_func->instructions == NULL) {
    arm_func->instructions = arena_alloc(arena, sizeof(Instruction));
    return;
  }
  arena_alloc(arena, sizeof(Instruction));
}

void AllocNumInstr(Arena* arena, ArmFunction* af, int num) {
  for (int i = 0; i < num; i++) {
    AllocInstr(arena, af);
  }
}

Operand TackyValToArmVal(TackyVal val) {
  Operand op;
  switch (val.type) {
    case TACKY_CONST:
      op.type = IMM;
      op.imm = val.const_val;
      break;
    case TACKY_VAR:
      op.type = PSEUDO;
      strcpy(op.identifier, val.identifier);
      break;
  }
  return op;
}

UnaryOperator TackyUnaryOpToArm(TackyUnaryOp op) {
  switch (op) {
    case TACKY_COMPLEMENT:
      return NOT;
    case TACKY_NEGATE:
      return NEG;
  }
}

BinaryOperator ToArmBinaryOp(TackyBinaryOp op) {
  switch (op) {
    case TACKY_DIVIDE:
      return A_DIVIDE;
    case TACKY_REMAINDER:
      fprintf(stderr, "Unexpected binary op, remainder found");
      exit(2);
    case TACKY_MULTIPLY:
      return A_MULTIPLY;
    case TACKY_SUBTRACT:
      return A_SUBTRACT;
    case TACKY_ADD:
      return A_ADD;
  }
}

void AppendArmUnary(Arena* arena, ArmFunction* af, TackyInstruction ti) {
  AllocInstr(arena, af);
  AllocInstr(arena, af);
  AllocInstr(arena, af);
  Mov mov;
  mov.src = TackyValToArmVal(ti.unary.src);
  mov.dst = (Operand) {.type = REGISTER, .reg = W11};
  af->instructions[af->length++] = (Instruction) {
      .type = MOV,
      .mov = mov
  };
  ArmUnary unary = (ArmUnary) {
      .op = TackyUnaryOpToArm(ti.unary.op),
      .reg = W11
  };
  af->instructions[af->length++] = (Instruction) {
      .type = UNARY,
      .unary = unary
  };
  mov.src = (Operand) {.type = REGISTER, .reg = W11};
  mov.dst = TackyValToArmVal(ti.unary.dst);
  af->instructions[af->length++] = (Instruction) {
      .type = MOV,
      .mov = mov
  };
}

void AppendArmRemainder(Arena* arena, ArmFunction* af, TackyInstruction ti) {
  AllocNumInstr(arena, af, 5);
  Mov mov;
  mov.src = TackyValToArmVal(ti.binary.left);
  mov.dst = (Operand) {
    .type = REGISTER,
    .reg = W11
  };
  af->instructions[af->length++] = (Instruction) {
    .type = MOV,
    .mov = mov
  };
  mov.src = TackyValToArmVal(ti.binary.right);
  mov.dst.reg = W12;
  af->instructions[af->length++] = (Instruction) {
    .type = MOV,
    .mov = mov
  };
  af->instructions[af->length++] = (Instruction) {
      .type = BINARY,
      .binary = (ArmBinary) {
          .op = A_DIVIDE,
          .left = W11,
          .right = W12,
          .dst = W13
      }
  };
  af->instructions[af->length++] = (Instruction) {
    .type = MSUB,
    .msub = (ArmMsub) {
      .dst = W13,
      .left = W11,
      .right = W12,
      .m = W13
    }
  };
  mov.src = (Operand) {.type = REGISTER, .reg = W13};
  mov.dst = TackyValToArmVal(ti.binary.dst);
  af->instructions[af->length++] = (Instruction) {
      .type = MOV,
      .mov = mov
  };
}

void AppendArmBinary(Arena* arena, ArmFunction* af, TackyInstruction ti) {
  if (ti.binary.op == TACKY_REMAINDER) {
    AppendArmRemainder(arena, af, ti);
    return;
  }
  AllocNumInstr(arena, af, 4);
  Mov mov;
  mov.src = TackyValToArmVal(ti.binary.left);
  mov.dst = (Operand) {.type = REGISTER, .reg = W11};
  af->instructions[af->length++] = (Instruction) {
      .type = MOV,
      .mov = mov
  };
  mov.src = TackyValToArmVal(ti.binary.right);
  mov.dst.reg = W12;
  af->instructions[af->length++] = (Instruction) {
      .type = MOV,
      .mov = mov
  };
  af->instructions[af->length++] = (Instruction) {
    .type = BINARY,
    .binary = (ArmBinary) {
      .op = ToArmBinaryOp(ti.binary.op),
      .left = W11,
      .right = W12,
      .dst = W12
    }
  };
  mov.src = (Operand) {.type = REGISTER, .reg = W12};
  mov.dst = TackyValToArmVal(ti.binary.dst);
  af->instructions[af->length++] = (Instruction) {
    .type = MOV,
    .mov = mov
  };
}

void AppendArmInstruction(Arena* arena, ArmFunction* arm_func, TackyInstruction t_instr) {
  switch (t_instr.type) {
    case TACKY_RETURN: {
      AllocInstr(arena, arm_func);
      AllocInstr(arena, arm_func);
      Mov mov;
      mov.src = TackyValToArmVal(t_instr.return_val);
      mov.dst = (Operand) {.type = REGISTER, .reg = W0};
      arm_func->instructions[arm_func->length++] = (Instruction) {
          .type = MOV,
          .mov = mov
      };
      arm_func->instructions[arm_func->length++] = (Instruction) {
          .type = RET
      };
      return;
    }
    case TACKY_UNARY:
      AppendArmUnary(arena, arm_func, t_instr);
      return;
    case TACKY_BINARY:
      AppendArmBinary(arena, arm_func, t_instr);
      return;
  }
}

void ToArmFunctionFromTacky(Arena* arena, TackyProgram* tacky_program,
                            ArmProgram* arm_program) {
  arm_program->function_def = arena_alloc(arena, sizeof(ArmFunction));
  arm_program->function_def->name = tacky_program->function_def->identifier;
  for (int i = 0; i < tacky_program->function_def->instr_length; ++i) {
    AppendArmInstruction(arena, arm_program->function_def,
                         tacky_program->function_def->instructions[i]);
  }
}

ArmProgram* TranslateTacky(Arena* arena, TackyProgram* tacky_program) {
  ArmProgram* arm_program = arena_alloc(arena, sizeof(ArmProgram));
  ToArmFunctionFromTacky(arena, tacky_program, arm_program);
  return arm_program;
}

int GetVarNum(Arena* scratch, char** identifiers, int* size, char* match) {
  for (int i = 0; i < *size; ++i) {
    if (strcmp(identifiers[i], match) == 0) {
      return i;
    }
  }
  if (*size == 100) {
    fprintf(stderr, "too many variables in a function");
    exit(2);
  }
  char* copy = arena_alloc(scratch, sizeof(char) * strlen(match) + 1);
  identifiers[(*size)++] = strcpy(copy, match);
  return *size - 1;
}

void ReplacePseudoRegisters(Arena* scratch, ArmProgram* program) {
  char** identifiers = arena_alloc(scratch, sizeof(char*) * 100);
  int size = 0;
  for (int i = 0; i < program->function_def->length; ++i) {
    Instruction* instruction = program->function_def->instructions + i;
    if (instruction->type == MOV) {
      if (instruction->mov.src.type == PSEUDO) {
        int pos = GetVarNum(scratch, identifiers, &size, instruction->mov.src.identifier);
        instruction->mov.src = (Operand) {
            .type = STACK,
            .stack_location = pos
        };
      }
      if (instruction->mov.dst.type == PSEUDO) {
        int pos = GetVarNum(scratch, identifiers, &size, instruction->mov.dst.identifier);
        instruction->mov.dst = (Operand) {
            .type = STACK,
            .stack_location =  pos
        };
      }
    }
  }
}

int max(int a, int b) {
  if (a > b) {
    return a;
  }
  return b;
}

// Since you cannot mov between to stack addresses, use a scratch register as an
// intermediary. Also add the instruction to allocate stack and deallocate stack.
void UpdateInstructions(Arena* arena, ArmFunction* func) {
  Instruction* next_list_instr = arena_alloc(arena, sizeof(Instruction) * (func->length + 2));
  int pos = 1;
  int largest_stack = 0;
  for (int i = 0; i < func->length - 1; ++i) {
    Instruction next = func->instructions[i];
    if (next.type == MOV && next.mov.src.type == STACK && next.mov.dst.type == STACK) {
      next.type = STR;
      Instruction after;
      after.type = LDR;
      after.mov.src = (Operand) {
          .type = REGISTER,
          .reg = W10
      };
      after.mov.dst = next.mov.dst;
      next.mov.dst = (Operand) {
          .type = REGISTER,
          .reg = W10
      };
      next_list_instr[pos++] = next;
      next_list_instr[pos++] = after;
      arena_alloc(arena, sizeof(Instruction));
      ++func->length;
      largest_stack = max(next.mov.src.stack_location, largest_stack);
      largest_stack = max(after.mov.dst.stack_location, largest_stack);
    } else if (next.type == MOV) {
      if (next.mov.src.type == STACK) {
        next.type = LDR;
        largest_stack = max(next.mov.src.stack_location, largest_stack);
      } else if (next.mov.dst.type == STACK) {
        next.type = STR;
        largest_stack = max(next.mov.dst.stack_location, largest_stack);
      }
      next_list_instr[pos++] = next;
    } else {
      next_list_instr[pos++] = next;
    }
  }
  next_list_instr[0] = (Instruction) {
      .type = ALLOC_STACK, .alloc_stack = (AllocStack) {.size = largest_stack}
  };
  next_list_instr[pos++] = (Instruction) {
      .type = DEALLOC_STACK, .alloc_stack = (AllocStack) {.size = largest_stack}
  };
  next_list_instr[pos] = (Instruction) { .type = RET };
  func->instructions = next_list_instr;
  func->length += 2;
}

void InstructionFixUp(Arena* arena, ArmProgram* arm_program) {
  UpdateInstructions(arena, arm_program->function_def);
}

void WriteRegister(Register reg, FILE* asm_f) {
  switch(reg) {
    case W0:
      fprintf(asm_f, "W0");
      return;
    case W11:
      fprintf(asm_f, "W11");
      return;
    case W12:
      fprintf(asm_f, "W12");
      return;
    case W13:
      fprintf(asm_f, "W13");
      return;
    case W10:
      fprintf(asm_f, "W10");
      return;
  }
}

void WriteOperand(Operand op, FILE* asm_f) {
  switch (op.type) {
    case REGISTER:
      WriteRegister(op.reg, asm_f);
      return;
    case IMM:
      fprintf(asm_f, "#%d", op.imm);
      return;
    case STACK:
      fprintf(asm_f, "[sp, #%d]", op.stack_location);
      return;
  }
}

void WriteTwoOperands(Operand src, Operand dst, FILE* f) {
  WriteOperand(dst, f);
  fprintf(f, ",    ");
  WriteOperand(src, f);
  fprintf(f, "\n");
}

int RoundStackSize(int size) {
  if (size % 2 == 1) {
    ++size;
  }
  return size;
}

char* ToUnaryOpStr(UnaryOperator op) {
  switch (op) {
    case NEG:
      return "NEG";
    case NOT:
      return "MVN";
  }
}

void WriteUnary(ArmUnary unary, FILE* asm_f) {
    fprintf(asm_f, "%*s%s  ", ASM_PADDING, "", ToUnaryOpStr(unary.op));
    WriteRegister(unary.reg, asm_f);
    fprintf(asm_f, ",    ");
    WriteRegister(unary.reg, asm_f);
}

char* ToBinaryOpStr(BinaryOperator op) {
  switch (op) {
    case A_ADD:
      return "ADD";
    case A_SUBTRACT:
      return "SUB";
    case A_DIVIDE:
      return "SDIV";
    case A_MULTIPLY:
      return "MUL";
  }
}

void WriteBinary(ArmBinary binary, FILE* asm_f) {
  fprintf(asm_f, "%*s%s  ", ASM_PADDING, "", ToBinaryOpStr(binary.op));
  WriteRegister(binary.dst, asm_f);
  fprintf(asm_f, ",    ");
  WriteRegister(binary.left, asm_f);
  fprintf(asm_f, ",    ");
  WriteRegister(binary.right, asm_f);
}

void WriteMsub(ArmMsub msub, FILE* asm_f) {
  fprintf(asm_f, "%*sMSUB  ", ASM_PADDING, "");
  WriteRegister(msub.dst, asm_f);
  fprintf(asm_f, ",    ");
  WriteRegister(msub.right, asm_f);
  fprintf(asm_f, ",    ");
  WriteRegister(msub.m, asm_f);
  fprintf(asm_f, ",    ");
  WriteRegister(msub.left, asm_f);
}

void WriteInstruction(Instruction* instruction, FILE* asm_f) {
  switch (instruction->type) {
    case ALLOC_STACK:
      fprintf(asm_f, "%*sSUB  sp, sp, #%d\n", ASM_PADDING, "", RoundStackSize(instruction->alloc_stack.size) * VAR_SIZE);
      return;
    case DEALLOC_STACK:
      fprintf(asm_f, "%*sADD  sp, sp, #%d\n", ASM_PADDING, "", RoundStackSize(instruction->alloc_stack.size) * VAR_SIZE);
      return;
    case MOV:
      fprintf(asm_f, "%*sMOV  ", ASM_PADDING, "");
      WriteTwoOperands(instruction->mov.src, instruction->mov.dst, asm_f);
      return;
    case LDR:
      fprintf(asm_f, "%*sLDR  ", ASM_PADDING, "");
      WriteTwoOperands(instruction->mov.src, instruction->mov.dst, asm_f);
      return;
    case STR:
      fprintf(asm_f, "%*sSTR  ", ASM_PADDING, "");
      WriteTwoOperands(instruction->mov.dst, instruction->mov.src, asm_f);
      return;
    case UNARY:
      WriteUnary(instruction->unary, asm_f);
      fprintf(asm_f, "\n");
      return;
    case BINARY:
      WriteBinary(instruction->binary, asm_f);
      fprintf(asm_f, "\n");
      return;
    case MSUB:
      WriteMsub(instruction->msub, asm_f);
      fprintf(asm_f, "\n");
      return;
    case RET:
      fprintf(asm_f, "%*sRET\n", ASM_PADDING, "");
      return;
  }
}

void WriteFunctionDef(ArmFunction* function, FILE* asm_f) {
  fprintf(asm_f, "        .globl _%s\n", function->name);
  fprintf(asm_f, "_%s:\n", function->name);
  for (int i = 0; i < function->length; ++i) {
    WriteInstruction(&function->instructions[i], asm_f);
  }
}

void WriteArmAssembly(ArmProgram* program, char* s_file) {
  FILE* asm_f = fopen(s_file, "w");
  WriteFunctionDef(program->function_def, asm_f);
  fclose(asm_f);
}
