#include <stdio.h>
#include "arena.h"
#include "ir_gen.h"
#include "parser.h"

int tmp_count = 0;

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
    default:
      fprintf(stderr, "Unexpected binary op type: %d", op);
  }
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

