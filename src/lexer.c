#include "lexer.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_TOKENS 100

bool IsBreak(char c) {
  if (isspace(c)) {
    return true;
  }
  if (c == EOF) {
    return true;
  }
  char *brk = BRK;
  while (*brk != '\0') {
    if (c == *brk) {
      return true;
    }
    brk++;
  }
  return false;
}

struct Token GetAlphaToken(FILE *fp) {
  struct Token result;
  int index = 0;
  char c = fgetc(fp);
  while (isalpha(c)) {
    result.value[index++] = c;
    c = fgetc(fp);
  }

  ungetc(c, fp);
  result.value[index] = '\0';
  if (IsBreak(c)) {
    if (strcmp(result.value, "int") == 0) {
      result.type = tInt;
      return result;
    }
    if (strcmp(result.value, "return") == 0) {
      result.type = tReturn;
      return result;
    }
    result.type = tIdentifier;
    return result;
  }
  result.type = tInvalidToken;
  return result;
}

struct Token GetConstantToken(FILE *fp) {
  struct Token result;
  int index = 0;
  char c = fgetc(fp);
  while (isdigit(c)) {
    result.value[index++] = c;
    c = fgetc(fp);
  }

  ungetc(c, fp);
  result.value[index] = '\0';
  if (IsBreak(c)) {
    result.type = tConstant;
    return result;
  }
  result.type = tInvalidToken;
  return result;
}

struct Token NextToken(FILE *fp) {
  struct Token result;
  char c = fgetc(fp);
  // trim whitespace before next token.
  while (isspace(c)) {
    c = fgetc(fp);
  }
  if (c == EOF) {
    result.type = tEof;
    return result;
  }
  switch (c) {
    case '{':result.type = tOpenBrace;
      return result;
    case '}':result.type = tCloseBrace;
      return result;
    case '(':result.type = tOpenParen;
      return result;
    case ')':result.type = tCloseParen;
      return result;
    case ';':result.type = tSemicolin;
      return result;
    case 'a' ... 'z':
    case 'A' ... 'Z':ungetc(c, fp);
      return GetAlphaToken(fp);
    case '0' ... '9':ungetc(c, fp);
      return GetConstantToken(fp);
    default:result.type = tInvalidToken;
      result.value[0] = c;
      result.value[1] = '\0';
      return result;
  }
}

struct TokenList Lex(FILE *fp) {
  struct TokenList token_list;
  struct Token *tokens = malloc(sizeof(struct Token) * MAX_TOKENS);
  token_list.tokens = tokens;
  int index = 0;
  struct Token next_token = NextToken(fp);
  while (next_token.type != tInvalidToken && next_token.type != tEof) {
    tokens[index++] = next_token;
    next_token = NextToken(fp);
  }
  tokens[index++] = next_token;
  token_list.length = index;
  return token_list;
}

