#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "arena.h"
#include "parser.h"
#include "lexer.h"

Block* ParseBlock(Arena* arena, TokenList* list);
Exp* ParseFactor(Arena* arena, TokenList* list);
Exp* ParseExp(Arena* arena, TokenList* list, int min_precedence);
BlockItem* ParseBlockItem(Arena* arena, TokenList* list);
Declaration* ParseDeclaration(Arena* arena, TokenList* list);

bool IsUnaryAssignment(UnaryOp op) {
  switch (op) {
    case PRE_INCREMENT:
    case PRE_DECREMENT:
      return true;
    default:
      return false;
  }
}

void ExpectTokenType(Token token, TokenType type) {
  if (token.type != type) {
    fprintf(stderr, "Expected type %s but got type %s\n", TokenTypeStr(type),
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
    case tIncrement:
      return PRE_INCREMENT;
    case tDecrement:
      return PRE_DECREMENT;
    default:
      fprintf(stderr, "encountered bad unary op\n");
      exit(1);
  }
}

PostFixOp ToPostFixOp(TokenType t) {
  switch (t) {
    case tIncrement:
      return POST_INCREMENT;
    case tDecrement:
      return POST_DECREMENT;
    default:
      fprintf(stderr, "Invalid postfix op\n");
      exit(1);
  }
}

bool IsPostFixOp(TokenType t) {
  switch (t) {
    case tIncrement:
    case tDecrement:
      return true;
    default:
      return false;
  }
}

Exp* ParseFactorInner(Arena* arena, TokenList* list) {
  Token token = DequeueToken(list);
  Exp* e;
  switch (token.type) {
    case tTilde:
    case tMinus:
    case tLogicalNot:
    case tIncrement:
    case tDecrement:
      e = arena_alloc(arena, sizeof(Exp));
      e->type = eUnaryExp;
      e->unary_exp.op_type = GetOp(token.type);
      e->unary_exp.exp = ParseFactor(arena, list);
      return e;
    case tOpenParen:
      e = ParseExp(arena, list, 0);
      ExpectTokenType(DequeueToken(list), tCloseParen);
      break;
    case tConstant:
      // TODO: use more proper parsing here. Or store const ints as ints.
      e = arena_alloc(arena, sizeof(Exp));
      e->const_val = atoi(token.value);
      break;
    case tIdentifier:
      e = arena_alloc(arena, sizeof(Exp));
      e->type = eVar;
      e->identifier = arena_alloc(arena, strlen(token.value) + 1);
      strcpy(e->identifier, token.value);
      break;
    default:
      fprintf(stderr, "Unexpected token encountered parsing a factor: %s\n",
              TokenTypeStr(token.type));
      exit(2);
  }
  Token* next_token = &list->tokens[0];
  while (IsPostFixOp(next_token->type)) {
    Exp* next = arena_alloc(arena, sizeof(Exp));
    next->type = ePostFix;
    next->post_fix = (PostFix) {
        .exp = e,
        .op = ToPostFixOp(next_token->type)
    };
    e = next;
    DequeueToken(list);
    next_token = &list->tokens[0];
  }
  return e;
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
    case tAndEquals:
    case tOrEquals:
    case tXorEquals:
    case tLShiftEquals:
    case tRShiftEquals:
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
    case tQuestion:
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
    case tQuestion:
      return 3;
    case tPlusEquals:
    case tMinusEquals:
    case tMultiplyEquals:
    case tDividesEquals:
    case tModuloEquals:
    case tAndEquals:
    case tOrEquals:
    case tXorEquals:
    case tLShiftEquals:
    case tRShiftEquals:
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
    case tAndEquals:
    case tOrEquals:
    case tXorEquals:
    case tLShiftEquals:
    case tRShiftEquals:
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
    case tAndEquals:
      return A_AND_EQUALS;
    case tOrEquals:
      return A_OR_EQUALS;
    case tXorEquals:
      return A_XOR_EQUALS;
    case tLShiftEquals:
      return A_L_SHIFT_EQUALS;
    case tRShiftEquals:
      return A_R_SHIFT_EQUALS;
    case tAssignment:
      return A_EQUALS;
    default:
      fprintf(stderr, "Unexpected assignment type\n");
      exit(1);
  }
}

Exp* ParseConditionalMiddle(Arena* a, TokenList* list) {
  ExpectTokenType(DequeueToken(list), tQuestion);
  Exp* e = ParseExp(a, list, 0);
  ExpectTokenType(DequeueToken(list), tColon);
  return e;
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
    } else if (next_token->type == tQuestion) {
      Exp* middle = ParseConditionalMiddle(arena, list);
      // Since it is right associative, we set the precedence to next_token, rather than
      // adding 1 like we do elsewhere.
      Exp* right = ParseExp(arena, list, Precedence(next_token->type));
      Exp* new_left = arena_alloc(arena, sizeof(Exp));
      *new_left = (Exp) {
          .type = eConditional,
          .conditional_exp = (ConditionalExp) {
              .cond = left,
              .left = middle,
              .right = right
          },
      };
      left = new_left;
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

Exp* ParseMaybeExp(Arena* arena, TokenList* list, TokenType end) {
  if (list->tokens[0].type == end) {
    DequeueToken(list);
    return NULL;
  }
  Exp* exp = ParseExp(arena, list, 0);
  ExpectTokenType(DequeueToken(list), end);
  return exp;
}

ForInit* ParseForInit(Arena* arena, TokenList* list) {
  ForInit* init = arena_alloc(arena, sizeof(ForInit));
  if (list->tokens[0].type == tInt) {
    init->type = FI_DECLARATION;
    init->dec = ParseDeclaration(arena, list);
    return init;
  }
  init->type = FI_EXP;
  init->exp = ParseMaybeExp(arena, list, tSemicolin);
  return init;
}

Statement* ParseStatement(Arena* arena, TokenList* list) {
  Statement* s = arena_alloc(arena, sizeof(Statement));
  Token token = DequeueToken(list);
  switch (token.type) {
    case tBreak:
      s->type = S_BREAK;
      ExpectTokenType(DequeueToken(list), tSemicolin);
      return s;
    case tContinue:
      s->type = S_CONTINUE;
      ExpectTokenType(DequeueToken(list), tSemicolin);
      return s;
    case tWhile:
      s->type = S_WHILE;
      ExpectTokenType(DequeueToken(list), tOpenParen);
      s->s_while = arena_alloc(arena, sizeof(While));
      s->s_while->cond = ParseExp(arena, list, 0);
      ExpectTokenType(DequeueToken(list), tCloseParen);
      s->s_while->body = ParseStatement(arena, list);
      return s;
    case tDo:
      s->type = S_DO_WHILE;
      ParseStatement(arena, list);
      ExpectTokenType(DequeueToken(list), tWhile);
      ExpectTokenType(DequeueToken(list), tOpenParen);
      s->s_while = arena_alloc(arena, sizeof(While));
      s->s_while->cond = ParseExp(arena, list, 0);
      ExpectTokenType(DequeueToken(list), tCloseParen);
      ExpectTokenType(DequeueToken(list), tSemicolin);
      return s;
    case tFor:
      s->type = S_FOR;
      ExpectTokenType(DequeueToken(list), tOpenParen);
      s->s_for = arena_alloc(arena, sizeof(For));
      s->s_for->for_init = ParseForInit(arena, list);
      s->s_for->cond = ParseMaybeExp(arena, list, tSemicolin);
      s->s_for->post = ParseMaybeExp(arena, list, tCloseParen);
      return s;
    case tReturn:
      s->type = S_RETURN;
      s->exp = ParseExp(arena, list, /*min_precedence=*/0);
      ExpectTokenType(DequeueToken(list), tSemicolin);
      return s;
    case tSemicolin:
      s->type = S_NULL;
      return s;
    case tGoto: {
      s->type = S_GOTO;
      Token t = DequeueToken(list);
      ExpectTokenType(t, tIdentifier);
      s->label_identifier = arena_alloc(arena, strlen(t.value) + 1);
      strcpy(s->label_identifier, t.value);
      ExpectTokenType(DequeueToken(list), tSemicolin);
      return s;
    }
      // Add special handling for possible goto labels
    case tIdentifier: {
      if (DequeueToken(list).type != tColon) {
        list->tokens--;
        goto handle_default;
      }
      s->type = S_GOTO_LABEL;
      s->goto_label = arena_alloc(arena, sizeof(GotoLabel));
      s->goto_label->identifier = arena_alloc(arena, strlen(token.value) + 1);
      strcpy(s->goto_label->identifier, token.value);
      s->goto_label->statement = ParseStatement(arena, list);
      return s;
    }
    case tIf:
      s->type = S_IF;
      ExpectTokenType(DequeueToken(list), tOpenParen);
      s->if_statement = arena_alloc(arena, sizeof(IfStatement));
      s->if_statement->cond = ParseExp(arena, list, /*min_precedence=*/0);
      ExpectTokenType(DequeueToken(list), tCloseParen);
      s->if_statement->statement = ParseStatement(arena, list);
      if (DequeueToken(list).type == tElse) {
        s->if_statement->has_else = true;
        s->if_statement->else_statement = ParseStatement(arena, list);
      } else {
        list->tokens--;
      }
      return s;
    case tOpenBrace:
      s->type = S_BLOCK;
      s->block = ParseBlock(arena, list);
      return s;
    default:
    handle_default:
      s->type = S_EXP;
      list->tokens--;
      s->exp = ParseExp(arena, list, /*min_precedence=*/0);
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

Block* ParseBlock(Arena* arena, TokenList* list) {
   Block* block = arena_alloc(arena, sizeof(Block));
  block->list = arena_alloc(arena, sizeof(BlockItemList));
  block->list->items = arena_alloc(arena, sizeof(BlockItem) * 10);
  // TODO: update to use a more proper allocation strategy...
  // arenas seeming a not great strategy... perhaps I can make it work though. could
  // even be lazy and just malloc and skip free.
  while (list->tokens[0].type != tCloseBrace) {
    block->list->items[block->list->length++] = ParseBlockItem(arena, list);
  }
  ExpectTokenType(DequeueToken(list), tCloseBrace);
  return block;
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
  f->body = ParseBlock(arena, list);
  return f;
}

Program* ParseTokens(Arena* arena, TokenList list) {
  Program* program = arena_alloc(arena, sizeof(program));
  program->function = ParseFunction(arena, &list);
  ExpectTokenType(DequeueToken(&list), tEof);
  return program;
}
