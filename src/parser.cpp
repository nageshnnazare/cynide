/**
 * @file parser.cpp
 * @brief Recursive-descent parser implementation for the Cynide grammar.
 */

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include <ast.h>
#include <parser.h>
#include <token.h>

void Parser::dumpAst(const std::unique_ptr<Program> &program) {
  std::cout << program->toString() << "\n";
}

std::unique_ptr<Program> Parser::parseProgram() {
  std::vector<std::unique_ptr<StmtNode>> stmts;
  while (!check(TokenType::EOF_TOKEN) && !_error) {
    auto stmt = parseStatement();
    if (!stmt)
      break;
    stmts.push_back(std::move(stmt));
    skipNewLines();
  }
  if (!_error)
    consume(TokenType::EOF_TOKEN, "Expected end of file.");
  return std::make_unique<Program>(std::move(stmts));
}

std::unique_ptr<StmtNode> Parser::parseStatement() {
  skipNewLines();

  if (check(TokenType::EOF_TOKEN) || check(TokenType::DEDENT))
    return nullptr;

  if (match(TokenType::KW_LET))
    return parseVariableDecl();

  if (match(TokenType::KW_FN))
    return parseFunctionDef();

  if (match(TokenType::KW_IF))
    return parseIfElseStmt();

  if (match(TokenType::KW_WHILE))
    return parseWhileStmt();

  if (match(TokenType::KW_FOR))
    return parseForStmt();

  if (match(TokenType::KW_RETURN))
    return parseReturnStmt();

  if (match(TokenType::KW_PRINT))
    return parsePrintStmt();

  return parseAssignOrExprStmt();
}

std::unique_ptr<VariableDecl> Parser::parseVariableDecl() {
  // Syntax: let <name> [: <type>] = <expr>
  Token name =
      consume(TokenType::IDENTIFIER, "Expected variable name after 'let'.");

  std::string typeAnn;
  // Check for optional type annotation (e.g. ": int")
  if (match(TokenType::COLON))
    typeAnn = parseTypeAnnotation();

  consume(TokenType::ASSIGN, "Expected '=' after variable declaration.");

  std::unique_ptr<ExprNode> expr = parseExpr();
  return std::make_unique<VariableDecl>(name.value, typeAnn, std::move(expr));
}

std::unique_ptr<FunctionDef> Parser::parseFunctionDef() {
  // Syntax: fn <name>([param: type, ...]) -> <retType>: <body>
  Token name =
      consume(TokenType::IDENTIFIER, "Expected function name after 'fn'.");
  consume(TokenType::LPAREN, "Expected '(' after function name.");

  std::vector<FunctionDef::Param> params;
  // Parse parameter list until the closing parenthesis
  while (!check(TokenType::RPAREN)) {
    do {
      Token pname = consume(TokenType::IDENTIFIER, "Expected parameter name");
      consume(TokenType::COLON, "Expected ':' after parameter name");
      std::string typeAnn = parseTypeAnnotation();
      params.emplace_back(pname.value, typeAnn);
    } while (match(TokenType::COMMA));
  }
  consume(TokenType::RPAREN, "Expected ')' after parameters.");
  consume(TokenType::ARROW, "Expected '->' before return type.");
  std::string retType = parseTypeAnnotation();
  consume(TokenType::COLON, "Expected ':' after function signature.");
  skipNewLines();

  std::unique_ptr<Block> body = parseBlock();
  return std::make_unique<FunctionDef>(name.value, std::move(params), retType,
                                       std::move(body));
}

std::unique_ptr<IfElseStmt> Parser::parseIfElseStmt() {
  std::unique_ptr<ExprNode> cond = parseExpr();
  consume(TokenType::COLON, "Expected ':' after if condition.");
  skipNewLines();
  std::unique_ptr<Block> ifBranch = parseBlock();

  std::vector<IfElseStmt::ElseIf> elifs;
  skipNewLines();
  while (match(TokenType::KW_ELIF)) {
    std::unique_ptr<ExprNode> ec = parseExpr();
    consume(TokenType::COLON, "Expected ':' after elif condition.");
    skipNewLines();
    std::unique_ptr<Block> eb = parseBlock();
    elifs.emplace_back(std::move(ec), std::move(eb));
    skipNewLines();
  }
  std::unique_ptr<Block> elseBranch;
  if (match(TokenType::KW_ELSE)) {
    consume(TokenType::COLON, "Expected ':' after else.");
    skipNewLines();
    elseBranch = parseBlock();
  }

  return std::make_unique<IfElseStmt>(std::move(cond), std::move(ifBranch),
                                      std::move(elifs), std::move(elseBranch));
}

std::unique_ptr<WhileStmt> Parser::parseWhileStmt() {
  std::unique_ptr<ExprNode> cond = parseExpr();
  consume(TokenType::COLON, "Expected ':' after while condition.");
  skipNewLines();
  std::unique_ptr<Block> body = parseBlock();
  return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

std::unique_ptr<ForStmt> Parser::parseForStmt() {
  Token name =
      consume(TokenType::IDENTIFIER, "Expected variable name after 'for'.");
  consume(TokenType::KW_IN, "Expected 'in' after for-loop.");
  consume(TokenType::KW_RANGE, "Expected 'range' after 'in'.");
  consume(TokenType::LPAREN, "Expected '(' after 'range'.");

  std::unique_ptr<ExprNode> start = parseExpr();
  std::unique_ptr<ExprNode> end;
  if (match(TokenType::COMMA))
    end = parseExpr();

  consume(TokenType::RPAREN, "Expected ')' after range args.");
  consume(TokenType::COLON, "Expected ':' after for-loop header.");
  skipNewLines();

  std::unique_ptr<Block> body = parseBlock();
  return std::make_unique<ForStmt>(name.value, std::move(start), std::move(end),
                                   std::move(body));
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
  skipNewLines();
  if (check(TokenType::DEDENT) || check(TokenType::EOF_TOKEN))
    return std::make_unique<ReturnStmt>(nullptr);

  std::unique_ptr<ExprNode> expr = parseExpr();
  return std::make_unique<ReturnStmt>(std::move(expr));
}

std::unique_ptr<PrintStmt> Parser::parsePrintStmt() {
  consume(TokenType::LPAREN, "Expected '(' after 'print'.");
  std::vector<std::unique_ptr<ExprNode>> args;

  if (!check(TokenType::RPAREN)) {
    do {
      args.push_back(parseExpr());
    } while (match(TokenType::COMMA));
  }
  consume(TokenType::RPAREN, "Expected ')' after print arguments.");
  return std::make_unique<PrintStmt>(std::move(args));
}

std::unique_ptr<StmtNode> Parser::parseAssignOrExprStmt() {
  size_t saved = _pos;

  if (check(TokenType::IDENTIFIER) && _pos + 1 < _tokens.size() &&
      _tokens[_pos + 1].type == TokenType::ASSIGN) {
    std::string name = advance().value;
    advance();

    std::unique_ptr<ExprNode> rhs = parseExpr();
    return std::make_unique<AssignStmt>(name, std::move(rhs));
  }

  _pos = saved;
  std::unique_ptr<ExprNode> expr = parseExpr();
  return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<Block> Parser::parseBlock() {
  consume(TokenType::INDENT, "Expected indented block.");

  std::vector<std::unique_ptr<StmtNode>> stmts;
  skipNewLines();

  while (!check(TokenType::DEDENT) && !check(TokenType::EOF_TOKEN) && !_error) {
    std::unique_ptr<StmtNode> st = parseStatement();
    if (!st)
      break;

    stmts.push_back(std::move(st));
    skipNewLines();
  }

  consume(TokenType::DEDENT, "Expected end of indented block.");
  return std::make_unique<Block>(std::move(stmts));
}

std::unique_ptr<ExprNode> Parser::parseExpr() { return parseOr(); }

std::unique_ptr<ExprNode> Parser::parseOr() {
  std::unique_ptr<ExprNode> expr = parseAnd();
  while (match(TokenType::KW_OR)) {
    std::string op = tokenLexForBinary(TokenType::KW_OR);
    std::unique_ptr<ExprNode> rhs = parseAnd();
    expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(rhs));
  }
  return expr;
}

std::unique_ptr<ExprNode> Parser::parseAnd() {
  std::unique_ptr<ExprNode> expr = parseEquality();
  while (match(TokenType::KW_AND)) {
    std::string op = tokenLexForBinary(TokenType::KW_AND);
    std::unique_ptr<ExprNode> rhs = parseEquality();
    expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(rhs));
  }
  return expr;
}

std::unique_ptr<ExprNode> Parser::parseEquality() {
  std::unique_ptr<ExprNode> expr = parseComparison();
  while (true) {
    if (match(TokenType::OP_EQ)) {
      std::unique_ptr<ExprNode> rhs = parseComparison();
      expr =
          std::make_unique<BinaryExpr>("==", std::move(expr), std::move(rhs));
    } else if (match(TokenType::OP_NEQ)) {
      std::unique_ptr<ExprNode> rhs = parseComparison();
      expr =
          std::make_unique<BinaryExpr>("!=", std::move(expr), std::move(rhs));
    } else {
      break;
    }
  }
  return expr;
}

std::unique_ptr<ExprNode> Parser::parseComparison() {
  std::unique_ptr<ExprNode> expr = parseAddSub();
  while (true) {
    TokenType tok = peek().type;
    if (tok == TokenType::OP_LT || tok == TokenType::OP_GT ||
        tok == TokenType::OP_LTE || tok == TokenType::OP_GTE) {
      advance();
      std::string op = tokenLexForBinary(tok);
      std::unique_ptr<ExprNode> rhs = parseAddSub();
      expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(rhs));
    } else {
      break;
    }
  }
  return expr;
}

std::unique_ptr<ExprNode> Parser::parseAddSub() {
  std::unique_ptr<ExprNode> expr = parseMulDivMod();
  while (true) {
    if (match(TokenType::OP_PLUS)) {
      std::unique_ptr<ExprNode> rhs = parseMulDivMod();
      expr = std::make_unique<BinaryExpr>("+", std::move(expr), std::move(rhs));
    } else if (match(TokenType::OP_MINUS)) {
      std::unique_ptr<ExprNode> rhs = parseMulDivMod();
      expr = std::make_unique<BinaryExpr>("-", std::move(expr), std::move(rhs));
    } else {
      break;
    }
  }
  return expr;
}
std::unique_ptr<ExprNode> Parser::parseMulDivMod() {
  std::unique_ptr<ExprNode> expr = parseUnary();
  while (true) {
    if (match(TokenType::OP_STAR)) {
      std::unique_ptr<ExprNode> rhs = parseUnary();
      expr = std::make_unique<BinaryExpr>("*", std::move(expr), std::move(rhs));
    } else if (match(TokenType::OP_SLASH)) {
      std::unique_ptr<ExprNode> rhs = parseUnary();
      expr = std::make_unique<BinaryExpr>("/", std::move(expr), std::move(rhs));
    } else if (match(TokenType::OP_PERCENT)) {
      std::unique_ptr<ExprNode> rhs = parseUnary();
      expr = std::make_unique<BinaryExpr>("%", std::move(expr), std::move(rhs));
    } else {
      break;
    }
  }
  return expr;
}

std::unique_ptr<ExprNode> Parser::parseUnary() {
  if (match(TokenType::OP_MINUS)) {
    std::unique_ptr<ExprNode> rhs = parseUnary();
    return std::make_unique<UnaryExpr>("-", std::move(rhs));
  } else if (match(TokenType::KW_NOT)) {
    std::unique_ptr<ExprNode> rhs = parseUnary();
    return std::make_unique<UnaryExpr>("not", std::move(rhs));
  }
  return parsePrimary();
}

std::unique_ptr<ExprNode> Parser::parsePrimary() {
  if (match(TokenType::LIT_INT)) {
    int64_t v = std::strtoll(previous().value.c_str(), nullptr, 10);
    return std::make_unique<IntLiteral>(v);
  }
  if (match(TokenType::LIT_FLOAT)) {
    double v = std::strtod(previous().value.c_str(), nullptr);
    return std::make_unique<FloatLiteral>(v);
  }
  if (match(TokenType::LIT_STRING)) {
    return std::make_unique<StringLiteral>(previous().value);
  }

  if (match(TokenType::KW_TRUE)) {
    return std::make_unique<BoolLiteral>(true);
  }
  if (match(TokenType::KW_FALSE)) {
    return std::make_unique<BoolLiteral>(false);
  }

  if (check(TokenType::IDENTIFIER)) {
    std::string name = advance().value;
    if (match(TokenType::LPAREN)) {
      std::vector<std::unique_ptr<ExprNode>> args;
      if (!check(TokenType::RPAREN)) {
        do {
          args.push_back(parseExpr());
        } while (match(TokenType::COMMA));
      }
      consume(TokenType::RPAREN, "Expected ')' after args.");
      return std::make_unique<FunctionCall>(name, std::move(args));
    }
    return std::make_unique<Identifier>(name);
  }

  if (match(TokenType::LPAREN)) {
    std::unique_ptr<ExprNode> expr = parseExpr();
    consume(TokenType::RPAREN, "Expected ')' after expression.");
    return expr;
  }

  reportError(peek(), "Expected expression.");
  return std::make_unique<IntLiteral>(0);
}

std::string Parser::parseTypeAnnotation() {
  Token tok = peek();
  switch (tok.type) {
  case TokenType::KW_INT:
    advance();
    return "int";
  case TokenType::KW_FLOAT:
    advance();
    return "float";
  case TokenType::KW_BOOL:
    advance();
    return "bool";
  case TokenType::KW_STRING:
    advance();
    return "string";
  case TokenType::KW_VOID:
    advance();
    return "void";
  default:
    reportError(tok, "Expected type name (int, float, bool, string, void).");
    return "";
  }
}

std::string Parser::tokenLexForBinary(TokenType type) const {
  switch (type) {
  case TokenType::OP_PLUS:
    return "+";
  case TokenType::OP_MINUS:
    return "-";
  case TokenType::OP_STAR:
    return "*";
  case TokenType::OP_SLASH:
    return "/";
  case TokenType::OP_PERCENT:
    return "%";
  case TokenType::OP_EQ:
    return "==";
  case TokenType::OP_NEQ:
    return "!=";
  case TokenType::OP_LT:
    return "<";
  case TokenType::OP_GT:
    return ">";
  case TokenType::OP_LTE:
    return "<=";
  case TokenType::OP_GTE:
    return ">=";
  case TokenType::KW_AND:
    return "and";
  case TokenType::KW_OR:
    return "or";
  default:
    return "?";
  }
}

// Helper functions
const Token &Parser::peek(size_t ahead) const {
  size_t i = _pos + ahead;
  if (i >= _tokens.size())
    return _tokens.back();
  return _tokens[i];
}

Token Parser::previous() const {
  if (_pos == 0)
    return _tokens.front();
  return _tokens[_pos - 1];
}

bool Parser::isAtEnd() const { return peek().type == TokenType::EOF_TOKEN; }

Token Parser::advance() {
  if (!isAtEnd())
    ++_pos;
  return previous();
}

bool Parser::check(TokenType type) const { return peek().type == type; }

bool Parser::match(TokenType type) {
  if (check(type)) {
    advance();
    return true;
  }
  return false;
}

Token Parser::consume(TokenType type, std::string msg) {
  if (check(type))
    return advance();

  reportError(peek(), msg);
  return peek();
}

void Parser::reportError(const Token &tok, const std::string &msg) {
  _error = true;
  _errorMessage = "Parse Error at " + std::to_string(tok.line) + ":" +
                  std::to_string(tok.column) + " " + msg;
}

void Parser::skipNewLines() {
  while (match(TokenType::NEWLINE)) {
  }
}
