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
      fprintf(stderr, "encountered bad unary op");
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
    default:
      e = arena_alloc(arena, sizeof(Exp));
      ExpectTokenType(token, tConstant);
      // TODO: use more proper parsing here. Or store const ints as ints.
      e->const_val = atoi(token.value);
      return e;
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
    default:
      fprintf(stderr, "Expected +,-,/,%%, or * got type: %s\n",
              TokenTypeStr(t));
      exit(2);
  }
}

Exp* ParseExp(Arena* arena, TokenList* list, int min_precedence) {
  Exp* left = ParseFactor(arena, list);
  Token* next_token = &list->tokens[0];
  while (IsBinaryOp(next_token->type) &&
      Precedence(next_token->type) >= min_precedence) {
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
    next_token = &list->tokens[0];
  }
  return left;
}

Statement* ParseStatement(Arena* arena, TokenList* list) {
  Statement* s = arena_alloc(arena, sizeof(Statement));
  s->type = S_RETURN;
  ExpectTokenType(DequeueToken(list), tReturn);
  s->exp = ParseExp(arena, list, 0);
  ExpectTokenType(DequeueToken(list), tSemicolin);
  return s;
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
  f->statement = ParseStatement(arena, list);
  ExpectTokenType(DequeueToken(list), tCloseBrace);
  return f;
}

Program* ParseTokens(Arena* arena, TokenList list) {
  Program* program = arena_alloc(arena, sizeof(program));
  program->function = ParseFunction(arena, &list);
  ExpectTokenType(DequeueToken(&list), tEof);
  return program;
}
