#include <string.h>
#include "arena.h"
#include "parser.h"
#include "lexer.h"

void ExpectTokenType(Token token, TokenType type) {
  if (token.type != type) {
    fprintf(stderr, "Expected type %s but got type %s", TokenTypeStr(type),
            TokenTypeStr(token.type));
    exit(2);
  }
}

Exp* ParseExp(Arena* arena, TokenList* list) {
  Exp* e = arena_alloc(arena, sizeof(e));
  Token token = DequeueToken(list);
  ExpectTokenType(token, tConstant);
  // TODO: use more proper parsing here. Or store const ints as ints.
  e->constValue = atoi(token.value);
  return e;
}

Statement* ParseStatement(Arena* arena, TokenList* list) {
  Statement* s = arena_alloc(arena, sizeof(Statement));
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
  f->name = arena_alloc(arena, strlen(token.value));
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