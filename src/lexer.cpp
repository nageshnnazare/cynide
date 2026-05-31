/**
 * @file lexer.cpp
 * @brief Lexical analysis: tokens, keywords, literals, and INDENT/DEDENT.
 */

#include <cctype>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <lexer.h>
#include <token.h>

void Lexer::dumpTokens(const std::vector<Token> &tokens) {
  std::cout << std::left << std::setw(6) << "LINE" << std::setw(6) << "COL"
            << std::setw(16) << "TOK TYPE"
            << "TOK VALUE\n";
  std::cout << std::string(50, '-') << "\n";

  for (const auto &tok : tokens) {
    std::string typeStr = tokenTypeToString(tok.type);
    std::cout << std::left << std::setw(6) << tok.line << std::setw(6)
              << tok.column << std::setw(16) << typeStr;

    if (tok.type == TokenType::LIT_STRING)
      std::cout << "\"" << tok.value << "\"";
    else if (tok.type == TokenType::NEWLINE)
      std::cout << "\\n";
    else if (tok.type == TokenType::INDENT)
      std::cout << ">>";
    else if (tok.type == TokenType::DEDENT)
      std::cout << "<<";
    else if (tok.type == TokenType::EOF_TOKEN)
      std::cout << "<eof>";
    else
      std::cout << tok.value;

    std::cout << "\n";
  }
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  _pos = 0;
  _line = 1;
  _column = 1;
  _error = false;
  _errorMessage.clear();

  std::vector<int> indentStack;
  indentStack.push_back(0);

  while (!_error) {
    if (!skipBlankAndCommentLine())
      break;
    if (peek() == '\0')
      break;

    int indentColLine = _line;
    int indentColCol = _column;

    int indent = 0;
    while (peek() == ' ') {
      ++indent;
      advanceChar();
    }

    if (peek() == '\t') {
      reportError(std::to_string(indentColLine) + ":" +
                  std::to_string(indentColCol) +
                  " Tab chars are not allowed for indentation; use spaces.");
      break;
    }
    if (peek() == '\n') {
      advanceChar();
      continue;
    }
    if (peek() == '#') {
      skipCommentLine();
      if (peek() == '\n')
        advanceChar();
      continue;
    }

    int top = indentStack.back();
    if (indent > top) {
      indentStack.push_back(indent);
      tokens.push_back(
          makeToken(TokenType::INDENT, "", indentColLine, indentColCol));
    } else if (indent < top) {
      while (!indentStack.empty() && indentStack.back() > indent) {
        indentStack.pop_back();
        tokens.push_back(
            makeToken(TokenType::DEDENT, "", indentColLine, indentColCol));
      }
      if (indentStack.empty() || indentStack.back() != indent) {
        reportError(
            std::to_string(indentColLine) + ":" + std::to_string(indentColCol) +
            " Inconsistent dedent: indentation does not match any out block.");
        break;
      }
    }

    tokenizeRestOfLine(tokens);
    if (_error)
      break;

    tokens.push_back(makeToken(TokenType::NEWLINE, "", _line, _column));
  }
  if (!_error) {
    while (indentStack.size() > 1) {
      indentStack.pop_back();
      tokens.push_back(makeToken(TokenType::DEDENT, "", _line, _column));
    }
    tokens.push_back(makeToken(TokenType::EOF_TOKEN, "", _line, _column));
  }

  return tokens;
}

void Lexer::tokenizeRestOfLine(std::vector<Token> &out) {
  while (true) {
    char c = peek();
    if (c == ' ' || c == '\t') {
      advanceChar();
      continue;
    }
    if (c == '#') {
      skipCommentLine();
      continue;
    }
    if (c == '\n') {
      advanceChar();
      break;
    }

    int tokLine = _line;
    int tokCol = _column;

    if (c == '-' && peek(1) == '>') {
      // return type
      advanceChar();
      advanceChar();
      out.push_back(makeToken(TokenType::ARROW, "->", tokLine, tokCol));
      continue;
    }
    if (c == '=' && peek(1) == '=') {
      // equality operator
      advanceChar();
      advanceChar();
      out.push_back(makeToken(TokenType::OP_EQ, "==", tokLine, tokCol));
      continue;
    }
    if (c == '!' && peek(1) == '=') {
      // non-equality operator
      advanceChar();
      advanceChar();
      out.push_back(makeToken(TokenType::OP_NEQ, "!=", tokLine, tokCol));
      continue;
    }
    if (c == '<' && peek(1) == '=') {
      // less than or equal to operator
      advanceChar();
      advanceChar();
      out.push_back(makeToken(TokenType::OP_LTE, "<=", tokLine, tokCol));
      continue;
    }
    if (c == '>' && peek(1) == '=') {
      // greater than or equal to operator
      advanceChar();
      advanceChar();
      out.push_back(makeToken(TokenType::OP_GTE, ">=", tokLine, tokCol));
      continue;
    }

    // single char operators
    switch (c) {
    case '+': {
      advanceChar();
      out.push_back(makeToken(TokenType::OP_PLUS, "+", tokLine, tokCol));
      continue;
    }
    case '-': {
      advanceChar();
      out.push_back(makeToken(TokenType::OP_MINUS, "-", tokLine, tokCol));
      continue;
    }
    case '*': {
      advanceChar();
      out.push_back(makeToken(TokenType::OP_STAR, "*", tokLine, tokCol));
      continue;
    }
    case '/': {
      advanceChar();
      out.push_back(makeToken(TokenType::OP_SLASH, "/", tokLine, tokCol));
      continue;
    }
    case '%': {
      advanceChar();
      out.push_back(makeToken(TokenType::OP_PERCENT, "%", tokLine, tokCol));
      continue;
    }
    case '=': {
      advanceChar();
      out.push_back(makeToken(TokenType::ASSIGN, "=", tokLine, tokCol));
      continue;
    }
    case '<': {
      advanceChar();
      out.push_back(makeToken(TokenType::OP_LT, "<", tokLine, tokCol));
      continue;
    }
    case '>': {
      advanceChar();
      out.push_back(makeToken(TokenType::OP_GT, ">", tokLine, tokCol));
      continue;
    }
    case '(': {
      advanceChar();
      out.push_back(makeToken(TokenType::LPAREN, "(", tokLine, tokCol));
      continue;
    }
    case ')': {
      advanceChar();
      out.push_back(makeToken(TokenType::RPAREN, ")", tokLine, tokCol));
      continue;
    }
    case ':': {
      advanceChar();
      out.push_back(makeToken(TokenType::COLON, ":", tokLine, tokCol));
      continue;
    }
    case ',': {
      advanceChar();
      out.push_back(makeToken(TokenType::COMMA, ",", tokLine, tokCol));
      continue;
    }
    default:
      break;
    }

    // string literals
    if (c == '"') {
      advanceChar();
      std::string s;
      while (true) {
        char q = peek();
        if (q == '\0' || q == '\n') {
          reportError(std::to_string(tokLine) + ":" + std::to_string(tokCol) +
                      " Unterminated string literal.");
          return;
        }
        if (q == '"') {
          advanceChar();
          break;
        }
        if (q == '\\') {
          advanceChar();
          char esc = peek();
          switch (esc) {
          case 'n': {
            advanceChar();
            s.push_back('\n');
            break;
          }
          case 't': {
            advanceChar();
            s.push_back('\t');
            break;
          }
          case 'r': {
            advanceChar();
            s.push_back('\r');
            break;
          }
          case '\\': {
            advanceChar();
            s.push_back('\\');
            break;
          }
          case '"': {
            advanceChar();
            s.push_back('"');
            break;
          }
          default: {
            reportError(std::to_string(tokLine) + ":" + std::to_string(tokCol) +
                        std::string(" Unknown escape sequence : '\\") + esc +
                        "'.");
            return;
          }
          }
          continue;
        }
        s.push_back(advanceChar());
      }
      out.push_back(makeToken(TokenType::LIT_STRING, s, tokLine, tokCol));
      continue;
    }

    // numerical literals
    if (std::isdigit(static_cast<unsigned char>(c))) {
      std::string num;
      while (std::isdigit(static_cast<unsigned char>(peek()))) {
        num.push_back(advanceChar());
      }
      if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
        num.push_back(advanceChar());
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
          num.push_back(advanceChar());
        }
        out.push_back(makeToken(TokenType::LIT_FLOAT, num, tokLine, tokCol));
      } else {
        out.push_back(makeToken(TokenType::LIT_INT, num, tokLine, tokCol));
      }
      continue;
    }

    // Keywords
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
      std::string id;
      while (std::isalnum(static_cast<unsigned char>(peek())) ||
             peek() == '_') {
        id.push_back(advanceChar());
      }
      TokenType tt = keywordType(id);
      out.push_back(makeToken(tt, id, tokLine, tokCol));
      continue;
    }

    if (c == '\0') {
      reportError(std::to_string(tokLine) + ":" + std::to_string(tokCol) +
                  std::string(" Expected new line at the end of the file."));
    } else {
      reportError(std::to_string(tokLine) + ":" + std::to_string(tokCol) +
                  std::string(" Unexpected character '") + c + "'.");
    }
    return;
  }
}

char Lexer::peek(size_t ahead) const {
  size_t i = _pos + ahead;
  if (i >= _source.size())
    return '\0';
  return _source[i];
}

char Lexer::advanceChar() {
  char c = peek();
  if (c == '\0')
    return '\0';
  if (c == '\n') {
    ++_line;
    _column = 1;
  } else {
    ++_column;
  }
  ++_pos;
  return c;
}

void Lexer::reportError(const std::string &msg) {
  _error = true;
  _errorMessage = msg;
}

void Lexer::skipCommentLine() {
  while (peek() != '\0' && peek() != '\n')
    advanceChar();
}

bool Lexer::skipBlankAndCommentLine() {
  while (true) {
    size_t savePos = _pos;
    int saveLine = _line;
    int saveCol = _column;

    while (peek() == ' ')
      advanceChar();

    if (peek() == '\t') {
      reportError(std::to_string(saveLine) + ":" + std::to_string(saveCol) +
                  "Tab chars are not allowed for indentation; use spaces.");
      return false;
    }

    if (peek() == '#') {
      skipCommentLine();
      if (peek() == '\n') {
        advanceChar();
        continue;
      }
      if (peek() == '\0')
        return true;
    }

    if (peek() == '\n') {
      advanceChar();
      continue;
    }

    if (peek() == '\0')
      return true;

    // save the line with code
    _pos = savePos;
    _line = saveLine;
    _column = saveCol;
    return true;
  }
}

Token Lexer::makeToken(TokenType type, std::string value, int line, int col) {
  return Token{
      .type = type, .value = std::move(value), .line = line, .column = col};
}

TokenType Lexer::keywordType(const std::string &text) {
  static const std::unordered_map<std::string, TokenType> kw = {
      {"if", TokenType::KW_IF},         {"elif", TokenType::KW_ELIF},
      {"else", TokenType::KW_ELSE},     {"while", TokenType::KW_WHILE},
      {"for", TokenType::KW_FOR},       {"in", TokenType::KW_IN},
      {"range", TokenType::KW_RANGE},   {"fn", TokenType::KW_FN},
      {"return", TokenType::KW_RETURN}, {"print", TokenType::KW_PRINT},
      {"let", TokenType::KW_LET},       {"true", TokenType::KW_TRUE},
      {"false", TokenType::KW_FALSE},   {"and", TokenType::KW_AND},
      {"or", TokenType::KW_OR},         {"not", TokenType::KW_NOT},
      {"int", TokenType::KW_INT},       {"float", TokenType::KW_FLOAT},
      {"bool", TokenType::KW_BOOL},     {"string", TokenType::KW_STRING},
      {"void", TokenType::KW_VOID},
  };
  auto it = kw.find(text);
  if (it == kw.end())
    return TokenType::IDENTIFIER;
  return it->second;
}