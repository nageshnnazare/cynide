#ifndef CYNIDE_TOKEN_H
#define CYNIDE_TOKEN_H

#include <string>

/**
 * @brief Enum representing all valid token categories in Cynide language.
 *
 * This includes identifiers, keywords (control flow, functions, types,
 * literals), operators (arithmetic, comparison, logical), literal constants,
 * punctuation, and structural markers like indentation change or file
 * terminator.
 */
enum class TokenType {
  IDENTIFIER,

  // Keywords
  KW_IF,
  KW_ELIF,
  KW_ELSE,

  KW_WHILE,
  KW_FOR,
  KW_IN,
  KW_RANGE,

  KW_FN,
  KW_RETURN,

  KW_PRINT,
  KW_LET,

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
  LIT_INT,
  LIT_FLOAT,

  // Punctuation
  LPAREN,
  RPAREN,
  COLON,
  COMMA,
  ARROW,
  ASSIGN,

  // Structure
  INDENT,
  DEDENT,
  NEWLINE,
  EOF_TOKEN,

  UNKNOWN,
};

/**
 * @brief Represents a lexical token parsed from the source code.
 *
 * Contains the token class, the raw string lexeme/value extracted,
 * and the exact line and column coordinates in the source file.
 */
struct Token {
  TokenType type = TokenType::UNKNOWN;
  std::string value;
  int line = 1;
  int column = 1;
};

/**
 * @brief Converts a TokenType enum value to its corresponding string
 * representation.
 * @param type The TokenType value to convert.
 * @return A string description of the token type (e.g. "IDENTIFIER", "KW_IF").
 */
std::string tokenTypeToString(TokenType type);

#endif // CYNIDE_TOKEN_H