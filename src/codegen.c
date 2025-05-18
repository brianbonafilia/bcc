#include <string.h>
#include "codegen.h"
#include "parser.h"
#include "arena.h"

#define ASM_PADDING 4
#define VAR_SIZE 8

void ToArmInstruction(Arena* arena, Statement* s, ArmFunction* f) {
  switch (s->exp->type) {
    case eConst:
      f->instructions = arena_alloc(arena, sizeof(Instruction) * 2);
      f->instructions[0] = (Instruction) {
          .type = MOV,
          .mov = {
              .src = {.type = IMM, .imm = s->exp->const_val},
              .dst = {.type = REGISTER}
          }
      };
      f->instructions[1] = (Instruction) {
          .type = RET
      };
      f->length = 2;
      return;
  }
}

void ToArmFunction(Arena* arena, Program* program, ArmProgram* arm_program) {
  arm_program->function_def = arena_alloc(arena, sizeof(ArmFunction));
  arm_program->function_def->name = program->function->name;
  Statement* statement = program->function->statement;
  ToArmInstruction(arena, statement, arm_program->function_def);
}

ArmProgram* Translate(Arena* arena, Program* program) {
  ArmProgram* arm_program = arena_alloc(arena, sizeof(ArmProgram));
  ToArmFunction(arena, program, arm_program);
  return arm_program;
}

void AllocInstr(Arena* arena, ArmFunction* arm_func) {
  if (arm_func->length == 0) {
    arm_func->instructions = arena_alloc(arena, sizeof(Instruction));
    return;
  }
  arena_alloc(arena, sizeof(Instruction));
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
    case TACKY_UNARY: {
      AllocInstr(arena, arm_func);
      AllocInstr(arena, arm_func);
      AllocInstr(arena, arm_func);
      Mov mov;
      mov.src = TackyValToArmVal(t_instr.unary.src);
      mov.dst = (Operand) {.type = REGISTER, .reg = W11};
      arm_func->instructions[arm_func->length++] = (Instruction) {
          .type = MOV,
          .mov = mov
      };
      ArmUnary unary = (ArmUnary) {
          .op = TackyUnaryOpToArm(t_instr.unary.op),
          .reg = W11
      };
      arm_func->instructions[arm_func->length++] = (Instruction) {
          .type = UNARY,
          .unary = unary
      };
      mov.src = (Operand) {.type = REGISTER, .reg = W11};
      mov.dst = TackyValToArmVal(t_instr.unary.dst);
      arm_func->instructions[arm_func->length++] = (Instruction) {
          .type = MOV,
          .mov = mov
      };
      return;
    }
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
  char* copy = arena_alloc(scratch, sizeof(char) * strlen(match));
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

void WriteUnary(ArmUnary unary, FILE* asm_f) {
  switch (unary.op) {
    case NEG:
      fprintf(asm_f, "%*sNEG  ", ASM_PADDING, "");
      WriteRegister(unary.reg, asm_f);
      fprintf(asm_f, ",    ");
      WriteRegister(unary.reg, asm_f);
      return;
    case NOT:
      fprintf(asm_f, "%*sMVN  ", ASM_PADDING, "");
      WriteRegister(unary.reg, asm_f);
      fprintf(asm_f, ",    ");
      WriteRegister(unary.reg, asm_f);
      return;
  }
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
