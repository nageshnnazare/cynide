/**
 * @file sema.cpp
 * @brief Semantic analysis: type inference, scope checking, and sema logging.
 */

#include "ast.h"
#include <iostream>

#include <sema.h>

void Sema::dumpSema() {
  for (const auto &entry : _logs) {
    for (int i = 0; i < entry.depth; ++i)
      std::cout << "  ";
    std::cout << entry.msg << "\n";
  }
}

void Sema::analyze(Program &program) {
  _error = false;
  _errorMessage.clear();
  _scopeStack.clear();
  _functions.clear();
  _logs.clear();
  _depth = 0;

  pushScope();

  for (auto &st : program.statements) {
    if (auto *fd = dynamic_cast<FunctionDef *>(st.get())) {
      FuncInfo fi;
      fi.returnType = fd->returnType;
      fi.params = fd->params;
      if (_functions.count(fd->name))
        reportError("Duplicate function '" + fd->name + "'.");
      _functions[fd->name] = fi;
    }
  }

  emit("=== Semantic Analysis ===");
  if (!_functions.empty()) {
    emit("");
    emit("--- Function Signatures ---");
    for (auto &[name, fi] : _functions) {
      std::string sig = "fn " + name + "(";
      for (size_t i = 0; i < fi.params.size(); ++i) {
        if (i > 0)
          sig += ", ";
        sig += fi.params[i].first + ": " + fi.params[i].second;
      }
      sig += ") -> " + fi.returnType;
      emit(sig);
    }
    emit("");
  }

  emit("--- Analysis ---");

  for (auto &st : program.statements)
    analyzeStatement(st.get());

  popScope();

  emit("");
  if (_error)
    emit("Result: ERRORS found (see above).");
  else
    emit("Result: OK \xe2\x80\x94 all types check out.");
}

std::string Sema::analyzeExpr(ExprNode *expr) {
  if (!expr) {
    reportError("Internal: null expression.");
    return "<error>";
  }

  if (auto *lit = dynamic_cast<IntLiteral *>(expr)) {
    (void)lit;
    return "int";
  }
  if (auto *lit = dynamic_cast<FloatLiteral *>(expr)) {
    (void)lit;
    return "float";
  }
  if (dynamic_cast<StringLiteral *>(expr))
    return "string";
  if (dynamic_cast<BoolLiteral *>(expr))
    return "bool";

  if (auto *id = dynamic_cast<Identifier *>(expr)) {
    VarInfo *v = lookupVar(id->name);
    if (!v) {
      reportError("Use of undeclared variable '" + id->name + "'.");
      return "<error>";
    }
    return v->dataType;
  }

  if (auto *bin = dynamic_cast<BinaryExpr *>(expr)) {
    std::string lt = analyzeExpr(bin->lhs.get());
    std::string rt = analyzeExpr(bin->rhs.get());

    if (bin->op == "==" || bin->op == "!=") {
      if (lt == "string" && rt == "string")
        return "bool";
    }

    if ((bin->op == "+" || bin->op == "-" || bin->op == "*" || bin->op == "/" ||
         bin->op == "%") &&
        (lt == "string" || rt == "string")) {
      reportError("Arithmetic operator '" + bin->op +
                  "' not supported on strings.");
    }

    return inferBinaryResultType(bin->op, lt, rt);
  }

  if (auto *un = dynamic_cast<UnaryExpr *>(expr)) {
    std::string inner = analyzeExpr(un->operand.get());
    if (un->op == "not")
      return "bool";
    if (un->op == "-") {
      if (inner == "string" || inner == "bool")
        reportError("Unary '-' not supported on type '" + inner + "'.");
      return inner;
    }
    return inner;
  }

  if (auto *call = dynamic_cast<FunctionCall *>(expr)) {
    auto it = _functions.find(call->callee);
    if (it == _functions.end()) {
      reportError("Call to undeclared function '" + call->callee + "'.");
      return "<error>";
    }

    const FuncInfo &fi = it->second;
    if (call->arguments.size() != fi.params.size()) {
      reportError("Function '" + call->callee + "' expects " +
                  std::to_string(fi.params.size()) + " argument(s), got " +
                  std::to_string(call->arguments.size()) + ".");
    }

    for (size_t i = 0; i < call->arguments.size(); ++i) {
      std::string argTy = analyzeExpr(call->arguments[i].get());
      if (i < fi.params.size()) {
        const std::string &expect = fi.params[i].second;
        if (argTy != expect && argTy != "<error>" &&
            !(expect == "float" && argTy == "int") &&
            !(expect == "int" && argTy == "float")) {
          reportError("Argument " + std::to_string(i + 1) + " of '" +
                      call->callee + "': expected '" + expect + "', got '" +
                      argTy + "'.");
        }
      }
    }
    return fi.returnType;
  }

  reportError("Unknown expression kind in sema.");
  return "<error>";
}

void Sema::analyzeVariableDecl(VariableDecl *node) {
  std::string initType = analyzeExpr(node->initializer.get());
  std::string resolvedType =
      node->typeAnnotation.empty() ? initType : node->typeAnnotation;

  if (!node->typeAnnotation.empty() && initType != "<error>" &&
      initType != resolvedType) {
    if (!((resolvedType == "float" && initType == "int") ||
          (resolvedType == "int" && initType == "float"))) {
      reportError("Variable '" + node->name + "' declared as '" + resolvedType +
                  "' but initializer has type '" + initType + "'.");
    }
  }

  declareVar(node->name, resolvedType);
  emit("let " + node->name + " : " + resolvedType + "  (init: " + initType +
       ")");
}

void Sema::analyzeAssignment(AssignStmt *node) {
  VarInfo *v = lookupVar(node->name);
  if (!v) {
    reportError("Assignment to undeclared variable '" + node->name + "'.");
    return;
  }
  std::string rhsTy = analyzeExpr(node->value.get());
  emit(node->name + " = <" + rhsTy + ">  (declared: " + v->dataType + ")");

  if (rhsTy != "<error>" && rhsTy != v->dataType) {
    if (!((v->dataType == "float" && rhsTy == "int") ||
          (v->dataType == "int" && rhsTy == "float"))) {
      reportError("Cannot assign '" + rhsTy + "' to variable '" + node->name +
                  "' of type '" + v->dataType + "'.");
    }
  }
}

void Sema::analyzeFunctionDef(FunctionDef *node) {
  emit("fn " + node->name + "(...) -> " + node->returnType + " {");
  ++_depth;

  pushScope();
  for (auto &p : node->params) {
    declareVar(p.first, p.second);
    emit("param " + p.first + " : " + p.second);
  }

  analyzeBlock(node->body.get());
  popScope();

  --_depth;
  emit("}");
}

void Sema::analyzeIfStatement(IfElseStmt *node) {
  std::string condTy = analyzeExpr(node->condition.get());
  emit("if <" + condTy + "> {");
  ++_depth;
  analyzeBlock(node->ifBody.get());
  --_depth;

  for (auto &elif : node->elseIfBody) {
    std::string eTy = analyzeExpr(elif.first.get());
    emit("} elif <" + eTy + "> {");
    ++_depth;
    analyzeBlock(elif.second.get());
    --_depth;
  }

  if (node->elseBody) {
    emit("} else {");
    ++_depth;
    analyzeBlock(node->elseBody.get());
    --_depth;
  }
  emit("}");
}

void Sema::analyzeWhileStatement(WhileStmt *node) {
  std::string condTy = analyzeExpr(node->condition.get());
  emit("while <" + condTy + "> {");
  ++_depth;
  analyzeBlock(node->body.get());
  --_depth;
  emit("}");
}

void Sema::analyzeForStatement(ForStmt *node) {
  // Parser stores range(n) as startExpr=n, endExpr=null (implicit start 0).
  // range(a, b) uses startExpr=a and endExpr=b.
  std::string startTy;
  std::string endTy;
  if (node->endExpr) {
    startTy = analyzeExpr(node->startExpr.get());
    endTy = analyzeExpr(node->endExpr.get());
  } else {
    if (!node->startExpr) {
      reportError("Internal: for-loop range missing bounds.");
      return;
    }
    startTy = "int";
    endTy = analyzeExpr(node->startExpr.get());
  }
  emit("for " + node->loopVar + " : int  in range(<" + startTy + ">, <" +
       endTy + ">) {");
  ++_depth;

  pushScope();
  declareVar(node->loopVar, "int");
  analyzeBlock(node->body.get());
  popScope();

  --_depth;
  emit("}");
}

void Sema::analyzeReturnStatement(ReturnStmt *node) {
  if (node->expr) {
    std::string ty = analyzeExpr(node->expr.get());
    emit("return <" + ty + ">");
  } else {
    emit("return <void>");
  }
}

void Sema::analyzePrintStatement(PrintStmt *node) {
  std::string argTypes;
  for (size_t i = 0; i < node->arguments.size(); ++i) {
    if (i > 0)
      argTypes += ", ";
    argTypes += analyzeExpr(node->arguments[i].get());
  }
  emit("print(" + argTypes + ")");
}

void Sema::analyzeExprStmt(ExprStmt *node) {
  std::string ty = analyzeExpr(node->expr.get());
  emit("expr-stmt <" + ty + ">");
}

void Sema::analyzeBlock(Block *block) {
  if (!block)
    return;
  pushScope();
  for (auto &st : block->statements)
    analyzeStatement(st.get());
  popScope();
}

void Sema::analyzeStatement(StmtNode *stmt) {
  if (!stmt)
    return;

  if (auto *v = dynamic_cast<VariableDecl *>(stmt))
    analyzeVariableDecl(v);
  else if (auto *a = dynamic_cast<AssignStmt *>(stmt))
    analyzeAssignment(a);
  else if (auto *f = dynamic_cast<FunctionDef *>(stmt))
    analyzeFunctionDef(f);
  else if (auto *i = dynamic_cast<IfElseStmt *>(stmt))
    analyzeIfStatement(i);
  else if (auto *w = dynamic_cast<WhileStmt *>(stmt))
    analyzeWhileStatement(w);
  else if (auto *f = dynamic_cast<ForStmt *>(stmt))
    analyzeForStatement(f);
  else if (auto *r = dynamic_cast<ReturnStmt *>(stmt))
    analyzeReturnStatement(r);
  else if (auto *p = dynamic_cast<PrintStmt *>(stmt))
    analyzePrintStatement(p);
  else if (auto *e = dynamic_cast<ExprStmt *>(stmt))
    analyzeExprStmt(e);
  else if (auto *b = dynamic_cast<Block *>(stmt))
    analyzeBlock(b);
}

std::string Sema::inferBinaryResultType(const std::string &op,
                                        const std::string &lhs,
                                        const std::string &rhs) {
  if (op == "and" || op == "or" || op == "==" || op == "!=" || op == "<" ||
      op == ">" || op == "<=" || op == ">=")
    return "bool";

  if (op == "%")
    return "int";

  if (lhs == "float" || rhs == "float")
    return "float";
  return "int";
}

void Sema::reportError(const std::string &msg) {
  if (!_error) {
    _error = true;
    _errorMessage = msg;
  }
  emit("[ERROR] " + msg);
}

inline void Sema::emit(const std::string &msg) {
  _logs.push_back({_depth, msg});
}

inline void Sema::pushScope() { _scopeStack.emplace_back(); }

inline void Sema::popScope() {
  if (!_scopeStack.empty())
    _scopeStack.pop_back();
}

void Sema::declareVar(const std::string &name, const std::string &type) {
  if (_scopeStack.empty())
    pushScope();
  auto &top = _scopeStack.back();
  if (top.count(name))
    reportError("Redeclaration of variable '" + name + "' in same scope.");
  top[name] = VarInfo{type, true};
}

Sema::VarInfo *Sema::lookupVar(const std::string &name) {
  for (auto it = _scopeStack.rbegin(); it != _scopeStack.rend(); ++it) {
    auto j = it->find(name);
    if (j != it->end())
      return &j->second;
  }
  return nullptr;
}