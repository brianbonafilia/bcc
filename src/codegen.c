#include <string.h>
#include "codegen.h"
#include "parser.h"
#include "arena.h"

#define ASM_PADDING 4
#define VAR_SIZE 8

void AppendArmBinary(Arena* arena, ArmFunction* af, TackyInstruction ti);

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
    case TACKY_OR:
      return A_OR;
    case TACKY_XOR:
      return A_XOR;
    case TACKY_AND:
      return A_AND;
    case TACKY_RSHIFT:
      return A_RSHIFT;
    case TACKY_LSHIFT:
      return A_LSHIFT;
      /* behavior on comparitors is a bit odd, might have to consider
       * moving this logic out of here... */
    case TACKY_GE_EQUAL:
    case TACKY_LE_EQUAL:
    case TACKY_LESS_THAN:
    case TACKY_GREATER_THAN:
    case TACKY_EQUAL:
    case TACKY_NOT_EQUAL:
      return A_CMP;
    default:
      fprintf(stderr, "unexpected binary op\n");
      exit(2);
  }
}

void AppendArmUnary(Arena* arena, ArmFunction* af, TackyInstruction ti) {
  // somewhat hacky workaround for ARM missing a logical not instruction
  if (ti.unary.op == TACKY_L_NOT) {
    TackyUnary tu = ti.unary;
    ti.type = TACKY_BINARY;
    ti.binary = (TackyBinary) {
        .op = TACKY_EQUAL,
        .left = (TackyVal) {.type = TACKY_CONST, .const_val = 0},
        .right = tu.src,
        .dst = tu.dst,
    };
    AppendArmBinary(arena, af, ti);
    return;
  }
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

ArmCC GetArmCC(TackyBinaryOp op) {
  switch (op) {
    case TACKY_GE_EQUAL:
      return B_GE;
    case TACKY_LE_EQUAL:
      return B_LE;
    case TACKY_LESS_THAN:
      return B_L;
    case TACKY_GREATER_THAN:
      return B_G;
    case TACKY_EQUAL:
      return B_E;
    case TACKY_NOT_EQUAL:
      return B_NE;
    default:
      fprintf(stderr, "Invalid op for GetArmCC");
      exit(2);
  }
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
  /* For comparing operations, move results instructions to W12 */
  if (af->instructions[af->length - 1].binary.op == A_CMP) {
    AllocInstr(arena, af);
    af->instructions[af->length++] = (Instruction) {
        .type = SET_CC,
        .set_cc = (SetCC) {
            .reg = W12,
            .cc = GetArmCC(ti.binary.op),
        }
    };
  }
  mov.src = (Operand) {.type = REGISTER, .reg = W12};
  mov.dst = TackyValToArmVal(ti.binary.dst);
  af->instructions[af->length++] = (Instruction) {
      .type = MOV,
      .mov = mov
  };
}

void AppendTackyJmp(Arena* arena, ArmFunction* af, TackyInstruction ti) {
  AllocInstr(arena, af);
  Instruction instr = (Instruction) {
      .type = CMP_BRANCH,
  };
  switch (ti.type) {
    case TACKY_JMP:
      instr.type = BRANCH;
      instr.branch.cc = B_NO_CC;
      strcpy(instr.branch.label, ti.jump_cond.target);
      af->instructions[af->length++] = instr;
      return;
    case TACKY_JMP_Z:
      instr.cmp_branch = (CompareBranch) {
          .branch = (Branch) {
              .cc = B_Z,
          },
          .reg = W13,
      };
      strcpy(instr.cmp_branch.branch.label, ti.jump_cond.target);
      break;
    case TACKY_JMP_NZ:
      instr.cmp_branch = (CompareBranch) {
          .branch = (Branch) {
              .cc = B_NZ,
          },
          .reg = W13,
      };
      strcpy(instr.cmp_branch.branch.label, ti.jump_cond.target);
      break;
    default:
      fprintf(stderr, "unexpected jmp operation\n");
      exit(2);
  }
  // for conditional jumps alloc an additional instruction.
  AllocInstr(arena, af);
  // notably we throw the cmp val into a work register.
  af->instructions[af->length++] = (Instruction) {
      .type = MOV,
      .mov = (Mov) {
          .src = TackyValToArmVal(ti.jump_cond.val),
          .dst = (Operand) {
              .type = REGISTER,
              .reg = W13,
          },
      }
  };
  af->instructions[af->length++] = instr;
}

void AppendTackyCopy(Arena* arena, ArmFunction* af, TackyCopy copy) {
  AllocNumInstr(arena, af, 2);
  // have to write to a temp register first to ensure we write to
  // stack correctly.
  af->instructions[af->length++] = (Instruction) {
      .type = MOV,
      .mov = (Mov) {
          .src = TackyValToArmVal(copy.src),
          .dst = (Operand) {
              .reg = W13,
          }
      }
  };
  af->instructions[af->length++] = (Instruction) {
      .type = MOV,
      .mov = (Mov) {
          .src = (Operand) {
              .reg = W13,
          },
          .dst = TackyValToArmVal(copy.dst),
      }
  };
}

void AppendTackyLabel(Arena* arena, ArmFunction* af, labelStr label) {
  AllocInstr(arena, af);
  Instruction i = (Instruction) {
      .type = LABEL,
  };
  strcpy(i.label.identifier, label);
  af->instructions[af->length++] = i;
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
    case TACKY_COPY:
      AppendTackyCopy(arena, arm_func, t_instr.copy);
      return;
    case TACKY_JMP_Z:
    case TACKY_JMP_NZ:
    case TACKY_JMP:
      AppendTackyJmp(arena, arm_func, t_instr);
      return;
    case TACKY_LABEL:
      AppendTackyLabel(arena, arm_func, t_instr.label);
      return;
    default:
      fprintf(stderr, "unexpected tacky instruction conversion to ARM");
      exit(2);
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
  next_list_instr[pos] = (Instruction) {.type = RET};
  func->instructions = next_list_instr;
  func->length += 2;
}

void InstructionFixUp(Arena* arena, ArmProgram* arm_program) {
  UpdateInstructions(arena, arm_program->function_def);
}

char* GetRegisterStr(Register reg) {
  switch (reg) {
    case W0:
      return "W0";
    case W11:
      return "W11";
    case W12:
      return "W12";
    case W13:
      return "W13";
    case W10:
      return "W10";
    default:
      fprintf(stderr, "unexpected register?, crashing \n");
      exit(2);
  }
}

void WriteRegister(Register reg, FILE* asm_f) {
  switch (reg) {
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
      fprintf(asm_f, "[sp, #%d]", op.stack_location * 4);
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
    case A_OR:
      return "ORR";
    case A_XOR:
      return "EOR";
    case A_AND:
      return "AND";
    case A_LSHIFT:
      return "LSL";
    case A_RSHIFT:
      return "ASR";
    case A_CMP:
      return "CMP";
    default:
      fprintf(stderr, "unexpected arm binary op\n");
      exit(2);
  }
}

void WriteBinary(ArmBinary binary, FILE* asm_f) {
  fprintf(asm_f, "%*s%s  ", ASM_PADDING, "", ToBinaryOpStr(binary.op));
  if (binary.op != A_CMP) {
    WriteRegister(binary.dst, asm_f);
    fprintf(asm_f, ",    ");
  }
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

char* GetCcStr(ArmCC cc) {
  switch (cc) {
    case B_G:
      return "GT";
    case B_GE:
      return "GE";
    case B_E:
      return "EQ";
    case B_L:
      return "LT";
    case B_LE:
      return "LE";
    case B_NE:
      return "NE";
    case B_Z:
      return "EQ";
    case B_NZ:
      return "NE";
    case B_NO_CC:
      return "";
    default:
      fprintf(stderr, "unexpected CC, can't translate\n");
      exit(2);
  }
}

void WriteArmSetCC(FILE* asm_f, SetCC set_cc) {
  fprintf(asm_f, "%*sCSET %s, %s\n", ASM_PADDING, "",
          GetRegisterStr(set_cc.reg), GetCcStr(set_cc.cc));
}

void WriteArmBranch(FILE* asm_f, Branch branch) {
  if (branch.cc == B_NO_CC) {
    fprintf(asm_f, "%*sB _%s\n", ASM_PADDING, "", branch.label);
    return;
  }
  fprintf(asm_f, "%*sB.%s _%s\n", ASM_PADDING, "",
          GetCcStr(branch.cc), branch.label);
}

char* GetBranchCcStr(ArmCC arm_cc) {
  switch (arm_cc) {
    case B_Z:
      return "Z";
    case B_NZ:
      return "NZ";
  }
  fprintf(stderr, "Invalid Branch Condition code\n");
  exit(2);
}

void WriteCmpBranch(FILE* asm_f, CompareBranch c_branch) {
  fprintf(asm_f, "%*sCB%s  %s, _%s \n",
          ASM_PADDING, "",
          GetBranchCcStr(c_branch.branch.cc),
          GetRegisterStr(c_branch.reg), c_branch.branch.label);
}

void WriteInstruction(Instruction* instruction, FILE* asm_f) {
  switch (instruction->type) {
    case ALLOC_STACK:
      fprintf(asm_f,
              "%*sSUB  sp, sp, #%d\n",
              ASM_PADDING,
              "",
              RoundStackSize(instruction->alloc_stack.size) * VAR_SIZE);
      return;
    case DEALLOC_STACK:
      fprintf(asm_f,
              "%*sADD  sp, sp, #%d\n",
              ASM_PADDING,
              "",
              RoundStackSize(instruction->alloc_stack.size) * VAR_SIZE);
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
    case SET_CC:
      WriteArmSetCC(asm_f, instruction->set_cc);
      return;
    case BRANCH:
      WriteArmBranch(asm_f, instruction->branch);
      return;
    case LABEL:
      fprintf(asm_f, "_%s:\n", instruction->label.identifier);
      return;
    case CMP_BRANCH:
      WriteCmpBranch(asm_f, instruction->cmp_branch);
      return;
    default:
      fprintf(stderr, "failed to write instruction, unknown translation\n");
      exit(2);
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
