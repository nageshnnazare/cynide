#ifndef CYNIDE_LEXER_H
#define CYNIDE_LEXER_H

#include <string>
#include <vector>

#include <token.h>

/*
    CYNIDE LEXER with indentation handling logic.

    Indentation algorithm:
    - Maintina a stack of indentation widths (columns); start with {0};
    - Blank lines and lines that contain only spaces/ tabs and/or #-comment are
      skipped entirely - they never change the indentation stack;
    - At the beginning og each non-skipped lines, count only the leading space
   chars (tabs are rejected);
    - Let `cur` be that count and `top` = stack.back();
    - If `cur` > `top`, push `cur` onto the stack and emit an INDENT token;
    - If `cur` < `top`, pop while `stack.back()` > `cur` and emit a DEDENT
   token;
    - After pops we must have `stack.back()` == `cur`; if not, report an error.
    - If `cur` == `top`, emit nothing.
    - After lexing the whole file, pop down to 0 emitting DEDENT, the EOF_TOKEN;
*/

class Lexer {
public:
  explicit Lexer(std::string source);

  std::vector<Token> tokenize();
  bool has_error() const { return _error; };
  std::string error_message() const { return _error_message; };
  ~Lexer();

private:
  std::string _source;
  size_t _pos = 0;
  int _line = 1;
  int _column = 1;
  bool _error = false;
  std::string _error_message;
};

#endif // CYNIDE_LEXER_H