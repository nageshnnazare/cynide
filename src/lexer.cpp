#include <lexer.h>

#include <cctype>
#include <unordered_map>

Lexer::Lexer(std::string source) : _source(std::move(source)) {}

std::vector<Token> Lexer::tokenize() { std::vector<Token> tokens; }