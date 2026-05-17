#ifndef CYNIDE_TOKEN_H
#define CYNIDE_TOKEN_H

#include <string>

enum class TokenType {
  IDENTIFIER,

  // Keywords
  KW_IF,
  KW_ELIF,
  KW_ELSE,

  KW_WHILE,
  KW_FOR,

  KW_FN,
  KW_RETURN,

  KW_PRINT,

  KW_TRUE,
  KW_FALSE,

  KW_AND,
  KW_OR,
  KW_NOT,

  KW_INT,
  KW_FLOAT,
  KW_BOOL,
  KW_STRING,
  KW_VOID,

  // Operators
  OP_PLUS,
  OP_MINUS,
  OP_STAR,
  OP_SLASH,
  OP_PERCENT,
  OP_EQ,
  OP_NEQ,
  OP_LT,
  OP_GT,
  OP_LTE,
  OP_GTE,

  // Literals
  LIT_STRING,

  // Punctuation
  LPAREN,
  RPAREN,
  COLON,
  COMMA,
  ARROW,

  // Structure
  INDENT,
  DEDENT,
  NEWLINE,
  EOF_TOKEN,

  UNKNOWN,
};

struct Token {
  TokenType type = TokenType::UNKNOWN;
  std::string value;
  int line = 1;
  int column = 1;
};

std::string tokenTypeToString(TokenType type);

#endif // CYNIDE_TOKEN_H