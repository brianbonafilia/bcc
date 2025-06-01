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

Token GetAlphaToken(FILE *fp) {
  Token result;
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
    if (strcmp(result.value, "void") == 0) {
      result.type = tVoid;
      return result;
    }
    result.type = tIdentifier;
    return result;
  }
  result.type = tInvalidToken;
  return result;
}

Token GetConstantToken(FILE *fp) {
  Token result;
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

Token NextToken(FILE *fp) {
  Token result;
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
    case '{':
      result.type = tOpenBrace;
      return result;
    case '}':
      result.type = tCloseBrace;
      return result;
    case '(':
      result.type = tOpenParen;
      return result;
    case ')':
      result.type = tCloseParen;
      return result;
    case ';':
      result.type = tSemicolin;
      return result;
    case '~':
      result.type = tTilde;
      return result;
    case '-':
      c = fgetc(fp);
      if (c == '-') {
        result.type = tInvalidToken;
        strcpy("--", result.value);
        return result;
      }
      ungetc(c, fp);
      result.type = tMinus;
      return result;
    case '+':
      c = fgetc(fp);
      if (c == '+') {
        result.type = tInvalidToken;
        strcpy("++", result.value);
        return result;
      }
      ungetc(c, fp);
      result.type = tPlus;
      return result;
    case '/':
      c = fgetc(fp);
      if (c == '/') {
        result.type = tInvalidToken;
        strcpy("//", result.value);
        return result;
      }
      ungetc(c, fp);
      result.type = tForSlash;
      return result;
    case '*':
      c = fgetc(fp);
      if (c == '*') {
        result.type = tInvalidToken;
        strcpy("**", result.value);
        return result;
      }
      ungetc(c, fp);
      result.type = tAsterik;
      return result;
    case '%':
      result.type = tModulo;
      return result;
    case 'a' ... 'z':
    case 'A' ... 'Z':
      ungetc(c, fp);
      return GetAlphaToken(fp);
    case '0' ... '9':
      ungetc(c, fp);
      return GetConstantToken(fp);
    default:
      result.type = tInvalidToken;
      result.value[0] = c;
      result.value[1] = '\0';
      return result;
  }
}

TokenList Lex(FILE *fp) {
  TokenList token_list;
  Token *tokens = malloc(sizeof(Token) * MAX_TOKENS);
  token_list.tokens = tokens;
  int index = 0;
  Token next_token = NextToken(fp);
  while (next_token.type != tInvalidToken && next_token.type != tEof) {
    tokens[index++] = next_token;
    next_token = NextToken(fp);
  }
  tokens[index++] = next_token;
  token_list.length = index;
  return token_list;
}

Token DequeueToken(TokenList *token_list) {
  token_list->length--;
  return *token_list->tokens++;
}

const char* TokenTypeStr(TokenType type) {
  return TypeStr[type];
}
