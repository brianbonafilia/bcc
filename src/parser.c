#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "arena.h"
#include "parser.h"
#include "lexer.h"

Exp* ParseFactor(Arena* arena, TokenList* list);
Exp* ParseExp(Arena* arena, TokenList* list, int min_precedence);

void ExpectTokenType(Token token, TokenType type) {
  if (token.type != type) {
    fprintf(stderr, "Expected type %s but got type %s", TokenTypeStr(type),
            TokenTypeStr(token.type));
    exit(2);
  }
}

UnaryOp GetOp(TokenType type) {
  switch (type) {
    case tTilde:
      return COMPLEMENT;
    case tMinus:
      return NEGATE;
    case tLogicalNot:
      return LOGICAL_NOT;
    default:
      fprintf(stderr, "encountered bad unary op\n");
      exit(1);
  }
}

Exp* ParseFactorInner(Arena* arena, TokenList* list) {
  Token token = DequeueToken(list);
  Exp* e;
  switch (token.type) {
    case tTilde:
    case tMinus:
    case tLogicalNot:
      e = arena_alloc(arena, sizeof(Exp));
      e->type = eUnaryExp;
      e->unary_exp.op_type = GetOp(token.type);
      e->unary_exp.exp = ParseFactor(arena, list);
      return e;
    case tOpenParen:
      e = ParseExp(arena, list, 0);
      ExpectTokenType(DequeueToken(list), tCloseParen);
      return e;
    case tConstant:
      // TODO: use more proper parsing here. Or store const ints as ints.
      e = arena_alloc(arena, sizeof(Exp));
      e->const_val = atoi(token.value);
      return e;
    case tIdentifier:
      e = arena_alloc(arena, sizeof(Exp));
      e->type = eVar;
      e->identifier = arena_alloc(arena, strlen(token.value) + 1);
      strcpy(e->identifier, token.value);
      return e;
    default:
      fprintf(stderr, "Unexpected token encountered parsing a factor: %s\n",
              TokenTypeStr(token.type));
      exit(2);
  }
}

Exp* ParseFactor(Arena* arena, TokenList* list) {
  return ParseFactorInner(arena, list);
}

BinaryOp ParseBinop(Token token) {
  switch (token.type) {
    case tPlus:
      return ADD;
    case tMinus:
      return SUBTRACT;
    case tAsterik:
      return MULTIPLY;
    case tForSlash:
      return DIVIDE;
    case tModulo:
      return REMAINDER;
    case tOr:
      return OR;
    case tAnd:
      return AND;
    case tXor:
      return XOR;
    case tRightShift:
      return RIGHT_SHIFT;
    case tLeftShift:
      return LEFT_SHIFT;
    case tLogicalAnd:
      return LOGICAL_AND;
    case tLogicalOr:
      return LOGICAL_OR;
    case tEqual:
      return EQUAL;
    case tNotEqual:
      return NOT_EQUAL;
    case tLessThan:
      return LESS_THAN;
    case tGreaterThan:
      return GREATER_THAN;
    case tLessOrEqual:
      return LESS_OR_EQUAL;
    case tGreaterOrEqual:
      return GREATER_OR_EQUAL;
    default:
      fprintf(stderr, "Expected binary op or got type: %s\n",
              TokenTypeStr(token.type));
      exit(2);
  }
}

// Works for now, probably could be done more efficiently.
bool IsBinaryOp(TokenType t) {
  switch (t) {
    case tAssignment:
    case tPlusEquals:
    case tMinusEquals:
    case tMultiplyEquals:
    case tDividesEquals:
    case tModuloEquals:
    case tPlus:
    case tMinus:
    case tAsterik:
    case tForSlash:
    case tModulo:
    case tOr:
    case tAnd:
    case tXor:
    case tRightShift:
    case tLeftShift:
    case tLogicalAnd:
    case tLogicalOr:
    case tEqual:
    case tNotEqual:
    case tLessThan:
    case tGreaterThan:
    case tLessOrEqual:
    case tGreaterOrEqual:
      return true;
    default:
      return false;
  }
}
int Precedence(TokenType t) {
  switch (t) {
    case tForSlash:
    case tModulo:
    case tAsterik:
      return 50;
    case tPlus:
    case tMinus:
      return 45;
    case tRightShift:
    case tLeftShift:
      return 40;
    case tLessOrEqual:
    case tLessThan:
    case tGreaterThan:
    case tGreaterOrEqual:
      return 35;
    case tEqual:
    case tNotEqual:
      return 30;
    case tAnd:
      return 25;
    case tXor:
      return 20;
    case tOr:
      return 15;
    case tLogicalAnd:
      return 10;
    case tLogicalOr:
      return 5;
    case tPlusEquals:
    case tMinusEquals:
    case tMultiplyEquals:
    case tDividesEquals:
    case tModuloEquals:
    case tAssignment:
      return 1;
    default:
      fprintf(stderr, "Expected +,-,/,%%,=, or * got type: %s\n",
              TokenTypeStr(t));
      exit(2);
  }
}

bool IsAssignmentToken(TokenType t) {
  switch (t) {
    case tPlusEquals:
    case tMinusEquals:
    case tMultiplyEquals:
    case tDividesEquals:
    case tModuloEquals:
    case tAssignment:
      return true;
    default:
      return false;
  }
}

AssignmentType ToAssignmentType(TokenType t) {
  switch (t) {
    case tPlusEquals:
      return A_PLUS_EQUALS;
    case tMinusEquals:
      return A_MINUS_EQUALS;
    case tMultiplyEquals:
      return A_MULTIPLY_EQUALS;
    case tDividesEquals:
      return A_DIVIDE_EQUALS;
    case tModuloEquals:
      return A_MODULO_EQUALS;
    case tAssignment:
      return A_EQUALS;
    default:
      fprintf(stderr, "Unexpected assignment type\n");
      exit(1);
  }
}

Exp* ParseExp(Arena* arena, TokenList* list, int min_precedence) {
  Exp* left = ParseFactor(arena, list);
  Token* next_token = &list->tokens[0];
  while (IsBinaryOp(next_token->type) &&
      Precedence(next_token->type) >= min_precedence) {
    if (IsAssignmentToken(next_token->type)) {
      DequeueToken(list);
      Exp* right = ParseExp(arena, list, Precedence(next_token->type));
      Exp* a = arena_alloc(arena, sizeof(Exp));
      *a = (Exp) {
          .type = eAssignment,
          .assignment = (Assignment) {
              .left = left,
              .right = right,
              .type  = ToAssignmentType(next_token->type)
          }
      };
      left = a;
    } else {
      BinaryOp op = ParseBinop(DequeueToken(list));
      Exp* right = ParseExp(arena, list, Precedence(next_token->type) + 1);
      Exp* binexp = arena_alloc(arena, sizeof(Exp));
      *binexp = (Exp) {
          .type = eBinaryExp,
          .binary_exp = (BinaryExp) {
              .op = op,
              .left = left,
              .right = right,
          }
      };
      left = binexp;
    }
    next_token = &list->tokens[0];
  }
  return left;
}

Statement* ParseStatement(Arena* arena, TokenList* list) {
  Statement* s = arena_alloc(arena, sizeof(Statement));
  Token token = DequeueToken(list);
  switch (token.type) {
    case tReturn:
      s->type = S_RETURN;
      s->exp = ParseExp(arena, list, 0);
      ExpectTokenType(DequeueToken(list), tSemicolin);
      return s;
    case tSemicolin:
      s->type = S_NULL;
      return s;
    default:
      s->type = S_EXP;
      list->tokens--;
      s->exp = ParseExp(arena, list, 0);
      ExpectTokenType(DequeueToken(list), tSemicolin);
      return s;
  }
}

Declaration* ParseDeclaration(Arena* arena, TokenList* list) {
  Declaration* dec = arena_alloc(arena, sizeof(Declaration));
  ExpectTokenType(DequeueToken(list), tInt);
  Token token = DequeueToken(list);
  ExpectTokenType(token, tIdentifier);
  dec->identifier = arena_alloc(arena, strlen(token.value) + 1);
  strcpy(dec->identifier, token.value);
  token = DequeueToken(list);
  // No initialization
  if (token.type == tSemicolin) {
    return dec;
  }
  ExpectTokenType(token, tAssignment);
  dec->init = ParseExp(arena, list, 0);
  ExpectTokenType(DequeueToken(list), tSemicolin);
  return dec;
}

BlockItem* ParseBlockItem(Arena* arena, TokenList* list) {
  BlockItem* item = arena_alloc(arena, sizeof(BlockItem));
  if (list->tokens[0].type == tInt) {
    item->type = B_DECLARATION;
    item->declaration = ParseDeclaration(arena, list);
    return item;
  }
  item->type = B_STATEMENT;
  item->statement = ParseStatement(arena, list);
  return item;
}

// Expect <function> ::= "int" <identifier> "(" "void" ")" "{" <statement> "}"
Function* ParseFunction(Arena* arena, TokenList* list) {
  Function* f = arena_alloc(arena, sizeof(Function));
  Token token = DequeueToken(list);
  ExpectTokenType(token, tInt);
  token = DequeueToken(list);
  ExpectTokenType(token, tIdentifier);
  f->name = arena_alloc(arena, strlen(token.value) + 1);
  strcpy(f->name, token.value);
  ExpectTokenType(DequeueToken(list), tOpenParen);
  ExpectTokenType(DequeueToken(list), tVoid);
  ExpectTokenType(DequeueToken(list), tCloseParen);
  ExpectTokenType(DequeueToken(list), tOpenBrace);
  f->item_list.items = arena_alloc(arena, sizeof(BlockItem) * 100);
  // TODO: update to use a more proper allocation strategy...
  // arenas seeming a not great strategy... perhaps I can make it work though. could
  // even be lazy and just malloc and skip free.
  while (list->tokens[0].type != tCloseBrace) {
    f->item_list.items[f->item_list.length++] = ParseBlockItem(arena, list);
  }
  ExpectTokenType(DequeueToken(list), tCloseBrace);
  return f;
}

Program* ParseTokens(Arena* arena, TokenList list) {
  Program* program = arena_alloc(arena, sizeof(program));
  program->function = ParseFunction(arena, &list);
  ExpectTokenType(DequeueToken(&list), tEof);
  return program;
}
