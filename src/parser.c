#include <string.h>
#include <assert.h>
#include "arena.h"
#include "parser.h"
#include "lexer.h"

Exp* ParseExp(Arena* arena, TokenList* list);

void ExpectTokenType(Token token, TokenType type) {
  if (token.type != type) {
    fprintf(stderr, "Expected type %s but got type %s", TokenTypeStr(type),
            TokenTypeStr(token.type));
    exit(2);
  }
}

UnaryOp GetOp(TokenType type) {
  switch(type) {
    case tTilde:
      return COMPLEMENT;
    case tMinus:
      return NEGATE;
    default:
      fprintf(stderr, "encountered bad unary op");
      exit(1);
  }
}

void ParseExpInner(Arena* arena, TokenList* list, Exp* e) {
  assert(e != NULL);
  Token token = DequeueToken(list);
  switch (token.type) {
    case tTilde:
    case tMinus:
      e->type = eUnaryExp;
      e->unary_exp.op_type = GetOp(token.type);
      e->unary_exp.exp = ParseExp(arena, list);
      return;
    case tOpenParen:
      ParseExpInner(arena, list, e);
      ExpectTokenType(DequeueToken(list), tCloseParen);
      return;
    default:
      ExpectTokenType(token, tConstant);
      // TODO: use more proper parsing here. Or store const ints as ints.
      e->const_val = atoi(token.value);
  }
}

Exp* ParseExp(Arena* arena, TokenList* list) {
  Exp* e = arena_alloc(arena, sizeof(Exp));
  ParseExpInner(arena, list, e);
  return e;
}

Statement* ParseStatement(Arena* arena, TokenList* list) {
  Statement* s = arena_alloc(arena, sizeof(Statement));
  s->type = S_RETURN;
  ExpectTokenType(DequeueToken(list), tReturn);
  s->exp = ParseExp(arena, list);
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
