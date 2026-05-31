#ifndef CYNIDE_SEMA_H
#define CYNIDE_SEMA_H

#include <map>
#include <string>
#include <vector>

#include <ast.h>
#include <stage.h>

/**
 * @class Sema
 * @brief Semantic analysis pass over the AST.
 *
 * Walks the program before codegen to resolve types, check declarations,
 * and validate assignments, calls, and control-flow conditions.
 *
 * Use `analyze()` to run the pass, then `hasError()` / `errorMessage()` for
 * status. Call `dumpSema()` to print the type-annotated trace (e.g. with
 * `--emit-sema`).
 */
class Sema : public Stage {
public:
  Sema() = default;

  /**
   * @brief Type-check the whole program and populate the sema log.
   * @param program Root AST from the parser.
   */
  void analyze(Program &program);

  bool hasError() const override { return _error; }
  std::string errorMessage() const override { return _errorMessage; }

  /** @brief Print the semantic analysis log to stdout. */
  void dumpSema();

private:
  /** @brief Variable binding: inferred or declared type and mutability. */
  struct VarInfo {
    std::string dataType;
    bool isMutable = true;
  };

  /** @brief Registered function signature for call checking. */
  struct FuncInfo {
    std::string returnType;
    std::vector<FunctionDef::Param> params;
  };

  /** @brief One indented line in the sema report. */
  struct LogEntry {
    int depth = 0;
    std::string msg;
  };

private:
  bool _error = false;
  std::string _errorMessage;

  std::vector<std::map<std::string, VarInfo>> _scopeStack;
  std::map<std::string, FuncInfo> _functions;

  std::vector<LogEntry> _logs;
  int _depth = 0;

  void reportError(const std::string &msg);
  /** @brief Append an indented line to the sema log. */
  inline void emit(const std::string &text);

  inline void pushScope();
  inline void popScope();
  /** @brief Bind @p name to @p type in the innermost scope. */
  void declareVar(const std::string &name, const std::string &type);
  /** @brief Resolve @p name through nested scopes, or nullptr if missing. */
  VarInfo *lookupVar(const std::string &name);

  void analyzeStatement(StmtNode *stmt);
  void analyzeBlock(Block *block);
  void analyzeVariableDecl(VariableDecl *node);
  void analyzeAssignment(AssignStmt *node);
  void analyzeFunctionDef(FunctionDef *node);
  void analyzeIfStatement(IfElseStmt *node);
  void analyzeWhileStatement(WhileStmt *node);
  /** @brief Handles `range(n)` (implicit 0..n) and `range(a, b)`. */
  void analyzeForStatement(ForStmt *node);
  void analyzeReturnStatement(ReturnStmt *node);
  void analyzePrintStatement(PrintStmt *node);
  void analyzeExprStmt(ExprStmt *node);

  /** @return Inferred type name for an expression (e.g. "int", "bool"). */
  std::string analyzeExpr(ExprNode *expr);

  /** @brief Result type of a binary operator given operand types. */
  std::string inferBinaryResultType(const std::string &op,
                                    const std::string &lhs,
                                    const std::string &rhs);
};

#endif // CYNIDE_SEMA_H
