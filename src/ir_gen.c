#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "arena.h"
#include "ir_gen.h"
#include "parser.h"

int tmp_count = 0;
int label_count = 0;

TackyVal EmitTacky(Arena* arena, Exp* exp, TackyFunction* tf);

// Assumes no allocations concurrently.
void AppendInstruction(Arena* arena, TackyFunction* tf, TackyInstruction instr) {
  if (tf->instr_length == 0) {
    tf->instructions = arena_alloc(arena, sizeof(TackyInstruction));
  } else {
    arena_alloc(arena, sizeof(TackyInstruction));
  }
  arena_alloc(arena, sizeof(TackyInstruction));
  tf->instructions[tf->instr_length++] = instr;
}

TackyUnaryOp ConvertOp(UnaryOp op) {
  switch (op) {
    case COMPLEMENT:
      return TACKY_COMPLEMENT;
    case NEGATE:
      return TACKY_NEGATE;
    case LOGICAL_NOT:
      return TACKY_L_NOT;
    default:
      fprintf(stderr, "bad unary op found\n");
      exit(2);
  }
}

TackyBinaryOp ConvertBinaryOp(BinaryOp op) {
  switch (op) {
    case ADD:
      return TACKY_ADD;
    case SUBTRACT:
      return TACKY_SUBTRACT;
    case MULTIPLY:
      return TACKY_MULTIPLY;
    case DIVIDE:
      return TACKY_DIVIDE;
    case REMAINDER:
      return TACKY_REMAINDER;
    case OR:
      return TACKY_OR;
    case XOR:
      return TACKY_XOR;
    case AND:
      return TACKY_AND;
    case RIGHT_SHIFT:
      return TACKY_RSHIFT;
    case LEFT_SHIFT:
      return TACKY_LSHIFT;
    case LESS_OR_EQUAL:
      return TACKY_LE_EQUAL;
    case LESS_THAN:
      return TACKY_LESS_THAN;
    case GREATER_THAN:
      return TACKY_GREATER_THAN;
    case GREATER_OR_EQUAL:
      return TACKY_GE_EQUAL;
    case EQUAL:
      return TACKY_EQUAL;
    case NOT_EQUAL:
      return TACKY_NOT_EQUAL;
    default:
      fprintf(stderr, "Unexpected binary op type: %d", op);
  }
}

bool ShouldExpandBinary(Exp* exp) {
  assert(exp->type == eBinaryExp);
  switch (exp->binary_exp.op) {
    case LOGICAL_AND:
    case LOGICAL_OR:
      return true;
    default:
      return false;
  }
}

void AssignLabel(BinaryOp op, char* dst) {
  switch (op) {
    case LOGICAL_AND:
      snprintf(dst, 20, "false_%d", label_count);
      return;
    case LOGICAL_OR:
      snprintf(dst, 20, "true_%d", label_count);
      return;
    default:
      fprintf(stderr, "bad label op code\n");
      exit(2);
  }
}

void BuildBinaryJmp(BinaryOp op, TackyInstruction* jmp) {
  if (op == LOGICAL_AND) {
    jmp->type = TACKY_JMP_Z;
    return;
  }
  if (op == LOGICAL_OR) {
    jmp->type = TACKY_JMP_NZ;
    return;
  }
  fprintf(stderr, "unexpected jmp op call\n");
  exit(2);
}

// this is for logical AND and OR, in which we need to potentially
// short circuit.
TackyVal ExpandBinaryExp(Arena* arena, BinaryExp exp, TackyFunction* tf) {
  // evaluate the expression on left and jump to end if false.
  TackyInstruction jmp;
  BuildBinaryJmp(exp.op, &jmp);
  jmp.jump_cond.val = EmitTacky(arena, exp.left, tf);
  AssignLabel(exp.op, jmp.jump_cond.target);
  label_count++;
  AppendInstruction(arena, tf, jmp);
  // do the same for the right. reusing jmp.
  jmp.jump_cond.val = EmitTacky(arena, exp.right, tf);
  AppendInstruction(arena, tf, jmp);
  // if we make it this far, mark as true if AND false if OR. and jump to end
  TackyInstruction copy = {
      .type = TACKY_COPY,
      .copy = (TackyCopy ) {
          .src = (TackyVal) {
              .type = TACKY_CONST,
              .const_val = exp.op == LOGICAL_AND ? 1 : 0,
          },
          .dst = (TackyVal) {
            .type = TACKY_VAR,
          }
      }
  };
  sprintf(copy.copy.dst.identifier, "tmp.%d", tmp_count++);
  AppendInstruction(arena, tf, copy);
  TackyInstruction endJump;
  endJump.type = TACKY_JMP;
  snprintf(endJump.jump_cond.target, 20, "end_%d", label_count++);
  AppendInstruction(arena, tf, endJump);

  TackyInstruction label;
  label.type = TACKY_LABEL;
  strcpy(label.label, jmp.label);
  AppendInstruction(arena, tf, label);
  // for the fail case mark as result as zero.
  copy.copy.src.const_val = exp.op == LOGICAL_AND ? 0 : 1;
  AppendInstruction(arena, tf, copy);
  strcpy(label.label, endJump.label);
  AppendInstruction(arena, tf, label);
  return copy.copy.dst;
}

TackyVal EmitTacky(Arena* arena, Exp* exp, TackyFunction* tf) {
  switch (exp->type) {
    case eConst: {
      TackyVal const_val = {.type = TACKY_CONST, .const_val =  exp->const_val};
      return const_val;
    }
    case eUnaryExp: {
      TackyVal src = EmitTacky(arena, exp->unary_exp.exp, tf);
      TackyVal dst = {.type = TACKY_VAR};
      sprintf(dst.identifier, "tmp.%d", tmp_count++);
      TackyUnaryOp op = ConvertOp(exp->unary_exp.op_type);
      TackyInstruction t_instr = {
          .type = TACKY_UNARY,
          .unary.op = op,
          .unary.src = src,
          .unary.dst = dst
      };
      AppendInstruction(arena, tf, t_instr);
      return dst;
    }
    case eBinaryExp: {
      if (ShouldExpandBinary(exp)) {
        return ExpandBinaryExp(arena, exp->binary_exp, tf);
      }
      TackyVal left = EmitTacky(arena, exp->binary_exp.left, tf);
      TackyVal right = EmitTacky(arena, exp->binary_exp.right, tf);
      TackyVal dst = {.type = TACKY_VAR};
      sprintf(dst.identifier, "tmp.%d", tmp_count++);
      TackyBinaryOp op = ConvertBinaryOp(exp->binary_exp.op);
      TackyInstruction t_instr = {
          .type = TACKY_BINARY,
          .binary.op = op,
          .binary.left = left,
          .binary.right = right,
          .binary.dst = dst,
      };
      AppendInstruction(arena, tf, t_instr);
      return dst;
    }
  }
}

TackyFunction* EmitTackyFunction(Arena* arena, Function* func) {
  TackyFunction* t_func = arena_alloc(arena, sizeof(TackyFunction));
  t_func->identifier = func->name;
  t_func->instr_length = 0;
  TackyVal src = EmitTacky(arena, func->statement->exp, t_func);
  TackyInstruction return_instr = {
      .type = TACKY_RETURN,
      .return_val = src
  };
  AppendInstruction(arena, t_func, return_instr);
  return t_func;
}

TackyProgram* EmitTackyProgram(Arena* arena, Program* program) {
  TackyProgram* pgrm = arena_alloc(arena, sizeof(TackyProgram));
  pgrm->function_def = EmitTackyFunction(arena, program->function);
  return pgrm;
} 

