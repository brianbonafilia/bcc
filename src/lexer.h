#include <stdio.h>

#define BRK "{}();" 

enum TokenType {
  tInvalidToken,
  tIdentifier,
  tConstant,
  tInt,
  tReturn,
  tOpenParen,
  tCloseParen,
  tOpenBrace,
  tCloseBrace,
  tSemicolin,
  tEof
};

struct TokenList {
  struct Token* tokens;
  int length;
};

struct Token {
  enum TokenType type;
  // to be used for identifiers and constants
  // might need to rethink for digits.
  char value[120];
};

struct Token NextToken(FILE* fp);
struct TokenList Lex(FILE* fp);
