#ifndef CYNIDE_LEXER_H
#define CYNIDE_LEXER_H

#include <string>
#include <vector>

#include <stage.h>
#include <token.h>

/*
    CYNIDE LEXER with indentation handling logic.

    Indentation algorithm:
    - Maintain a stack of indentation widths (columns); start with {0};
    - Blank lines and lines that contain only spaces and/or #-comment are
      skipped entirely - they never change the indentation stack;
    - TODO: Tab indentation is not supported yet.
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

/**
 * @class Lexer
 * @brief Performs lexical analysis (tokenization) on Cynide source code.
 *
 * The Lexer reads raw source code characters, groups them into logical tokens,
 * and maintains track of indentation levels (using spaces) to emit INDENT and
 * DEDENT tokens, which define scope blocks in the language.
 */
class Lexer : public Stage {
public:
  /**
   * @brief Constructs a new Lexer instance.
   * @param source The raw Cynide source code string to tokenize.
   */
  explicit Lexer(std::string source) : _source(std::move(source)) {};
  ~Lexer() = default;

  /**
   * @brief Tokenizes the entire source file.
   *
   * Scans the source string character-by-character, processes indentation,
   * identifiers, keywords, literals, and punctuation.
   *
   * @return A vector of Token objects representing the source program.
   */
  std::vector<Token> tokenize();

  /**
   * @brief Lexes the remainder of the current line, appending tokens to the
   * output.
   * @param out The output vector of tokens to append to.
   */
  void tokenizeRestOfLine(std::vector<Token> &out);

  /**
   * @brief Checks if the lexer has encountered an error.
   * @return true if an error has occurred, false otherwise.
   */
  bool hasError() const override { return _error; };

  /**
   * @brief Retrieves the error message generated during tokenization.
   * @return A string representation of the lexical error.
   */
  std::string errorMessage() const override { return _errorMessage; };

  /**
   * @brief Utility function to print a list of tokens to stdout.
   * @param tokens The vector of tokens to print.
   */
  static void dumpTokens(const std::vector<Token> &tokens);

private:
  std::string _source;
  size_t _pos = 0;
  int _line = 1;
  int _column = 1;
  bool _error = false;
  std::string _errorMessage;

  /**
   * @brief Peeks at the character at the current position or slightly ahead.
   * @param ahead Number of characters ahead to peek.
   * @return The character at the peek position, or '\0' if out of bounds.
   */
  char peek(size_t ahead = 0) const;

  /**
   * @brief Advances the lexer position by one character and updates line/column
   * numbers.
   * @return The character that was advanced past.
   */
  char advanceChar();

  /**
   * @brief Flags a lexical error with a given message.
   * @param msg The descriptive error message.
   */
  void reportError(const std::string &msg);

  /**
   * @brief Skips characters until a newline or end-of-file is reached (comment
   * content).
   */
  void skipCommentLine();

  /**
   * @brief Skips blank lines and lines containing only comments.
   * @return true if it was able to process/skip or reached a code boundary,
   * false on error (e.g. tabs).
   */
  bool skipBlankAndCommentLine();

  /**
   * @brief Helper to construct a Token.
   * @param type The type of the token.
   * @param value The lexeme value.
   * @param line The line number.
   * @param col The column number.
   * @return A constructed Token object.
   */
  Token makeToken(TokenType type, std::string value, int line, int col);

  /**
   * @brief Determines if an identifier text matches a keyword or is a generic
   * identifier.
   * @param text The identifier string.
   * @return The matching keyword TokenType or TokenType::IDENTIFIER.
   */
  static TokenType keywordType(const std::string &text);
};

#endif // CYNIDE_LEXER_H