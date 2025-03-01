#include "codegen.h"
#include "parser.h"
#include "arena.h"

void ToArmInstruction(Arena* arena, Statement* s, ArmFunction* f) {
  switch(s->exp->type) {
    case eConst:
      f->instructions = arena_alloc(arena, sizeof(Instruction) * 2);
      f->instructions[0] =  (Instruction) {
          .type = MOV,
          .mov = {
              .src = {.type = IMM, .imm = s->exp->constValue},
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

ArmProgram* Translate(Arena* arena, Program* program){
  ArmProgram* arm_program = arena_alloc(arena, sizeof(ArmProgram));
  ToArmFunction(arena, program, arm_program);
  return arm_program;
}

void WriteOperand(Operand op, FILE* asm_f) {
  switch(op.type) {
    case REGISTER:
      fprintf(asm_f, "w0");
      return;
    case IMM:
      fprintf(asm_f, "#%d", op.imm);
      return;
  }
}

void WriteInstruction(Instruction* instruction, FILE* asm_f) {
  switch (instruction->type) {
    case MOV:
      fprintf(asm_f, "MOV   ");
      WriteOperand(instruction->mov.dst, asm_f);
      fprintf(asm_f, ",    ");
      WriteOperand(instruction->mov.src, asm_f);
      fprintf(asm_f, "\n");
      return;
    case RET:
      fprintf(asm_f, "RET\n");
      return;
  }
}

void WriteFunctionDef(ArmFunction* function, FILE* asm_f) {
  fprintf(asm_f, "        .globl _%s\n", function->name);
  fprintf(asm_f, "_%s:\n", function->name );
  for (int i = 0; i < function->length; ++i) {
    WriteInstruction(&function->instructions[i], asm_f);
  }
}
void WriteArmAssembly(ArmProgram* program, char* s_file) {
  FILE* asm_f = fopen(s_file, "w");
  WriteFunctionDef(program->function_def, asm_f);
  fclose(asm_f);
}
