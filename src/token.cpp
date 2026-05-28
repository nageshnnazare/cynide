#include <token.h>

std::string tokenTypeToString(TokenType type) {
  switch (type) {
  case TokenType::IDENTIFIER:
    return "IDENTIFIER";

  // Keywords
  case TokenType::KW_IF:
    return "KW_IF";
  case TokenType::KW_ELIF:
    return "KW_ELIF";
  case TokenType::KW_ELSE:
    return "KW_ELSE";

  case TokenType::KW_WHILE:
    return "KW_WHILE";
  case TokenType::KW_FOR:
    return "KW_FOR";
  case TokenType::KW_IN:
    return "KW_IN";
  case TokenType::KW_RANGE:
    return "KW_RANGE";

  case TokenType::KW_FN:
    return "KW_FN";
  case TokenType::KW_RETURN:
    return "KW_RETURN";

  case TokenType::KW_PRINT:
    return "KW_PRINT";
  case TokenType::KW_LET:
    return "KW_LET";

  case TokenType::KW_TRUE:
    return "KW_TRUE";
  case TokenType::KW_FALSE:
    return "KW_FALSE";

  case TokenType::KW_AND:
    return "KW_AND";
  case TokenType::KW_OR:
    return "KW_OR";
  case TokenType::KW_NOT:
    return "KW_NOT";

  case TokenType::KW_INT:
    return "KW_INT";
  case TokenType::KW_FLOAT:
    return "KW_FLOAT";
  case TokenType::KW_BOOL:
    return "KW_BOOL";
  case TokenType::KW_STRING:
    return "KW_STRING";
  case TokenType::KW_VOID:
    return "KW_VOID";

  // Operators
  case TokenType::OP_PLUS:
    return "OP_PLUS";
  case TokenType::OP_MINUS:
    return "OP_MINUS";
  case TokenType::OP_STAR:
    return "OP_STAR";
  case TokenType::OP_SLASH:
    return "OP_SLASH";
  case TokenType::OP_PERCENT:
    return "OP_PERCENT";
  case TokenType::OP_EQ:
    return "OP_EQ";
  case TokenType::OP_NEQ:
    return "OP_NEQ";
  case TokenType::OP_LT:
    return "OP_LT";
  case TokenType::OP_GT:
    return "OP_GT";
  case TokenType::OP_LTE:
    return "OP_LTE";
  case TokenType::OP_GTE:
    return "OP_GTE";

  // Literals
  case TokenType::LIT_STRING:
    return "LIT_STRING";
  case TokenType::LIT_INT:
    return "LIT_INT";
  case TokenType::LIT_FLOAT:
    return "LIT_FLOAT";

  // Punctuation
  case TokenType::LPAREN:
    return "LPAREN";
  case TokenType::RPAREN:
    return "RPAREN";
  case TokenType::COLON:
    return "COLON";
  case TokenType::COMMA:
    return "COMMA";
  case TokenType::ARROW:
    return "ARROW";
  case TokenType::ASSIGN:
    return "ASSIGN";

  // Structure
  case TokenType::INDENT:
    return "INDENT";
  case TokenType::DEDENT:
    return "DEDENT";
  case TokenType::NEWLINE:
    return "NEWLINE";
  case TokenType::EOF_TOKEN:
    return "EOF_TOKEN";

  case TokenType::UNKNOWN:
  default:
    return "UNKNOWN";
  }
}
