#ifndef BCC_SRC_PRETTY_PRINT
#define BCC_SRC_PRETTY_PRINT

#include <stdio.h>
#include "parser.h"
#include "ir_gen.h"
#include "codegen.h"

void PrettyPrintAST(Program* program);

void PrettyPrintTacky(TackyProgram* program);

void PrettyPrintAssemblyAST(ArmProgram* program);

#endif // BCC_SRC_PRETTY_PRINT
