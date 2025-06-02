#ifndef BCC_SRC_LEXER_H
#define BCC_SRC_LEXER_H

#include <stdio.h>

#define BRK "{}();~-+*/%|^&<>"

static const char *TypeStr[] = {
    "tInvalidToken",
    "tIdentifier",
    "tConstant",
    "tInt",
    "tVoid",
    "tReturn",
    "tOpenParen",
    "tCloseParen",
    "tOpenBrace",
    "tCloseBrace",
    "tSemicolin",
    "tMinus",
    "tPlus",
    "tAsterik",
    "tForSlash",
    "tModulo",
    "tIncrement",
    "tDecrementer",
    "tTilde",
    "tEof"
};

typedef enum {
  tInvalidToken,
  tIdentifier,
  tConstant,
  tInt,
  tVoid,
  tReturn,
  tOpenParen,
  tCloseParen,
  tOpenBrace,
  tCloseBrace,
  tSemicolin,
  tMinus,
  tPlus,
  tAsterik,
  tForSlash,
  tModulo,
  tIncrement,
  tDecrement,
  tAnd,
  tOr,
  tXor,
  tLeftShift,
  tRightShift,
  tTilde,
  tEof
} TokenType;

typedef struct {
  TokenType type;
  // to be used for identifiers and constants
  // might need to rethink for digits.
  char value[120];
} Token;

typedef struct {
  Token *tokens;
  int length;
} TokenList;

Token NextToken(FILE *fp);
TokenList Lex(FILE *fp);

Token DequeueToken(TokenList *token_list);

const char *TokenTypeStr(TokenType type);
#endif //BCC_SRC_LEXER_H
