#ifndef CYNIDE_AST_H
#define CYNIDE_AST_H

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

class ASTNode {
public:
  ASTNode() = default;
  virtual ~ASTNode() = default;

  virtual std::string toString(int indent = 0) const = 0;
};

class ExprNode : public ASTNode {};

class StmtNode : public ASTNode {};

// Literals
class IntLiteral : public ExprNode {
public:
  explicit IntLiteral(int64_t val) : value(val) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "IntLiteral(" << value << ")";
    return os.str();
  }

public:
  int64_t value = 0;
};

class FloatLiteral : public ExprNode {
public:
  explicit FloatLiteral(double val) : value(val) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "FloatLiteral(" << value << ")";
    return os.str();
  }

public:
  double value = 0.0;
};

class StringLiteral : public ExprNode {
public:
  explicit StringLiteral(std::string val) : name(std::move(val)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "StringLiteral(" << name << ")";
    return os.str();
  }

public:
  std::string name = "";
};

// Operations
class BinaryExpr : public ExprNode {
public:
  BinaryExpr(std::string op, std::unique_ptr<ExprNode> lhs,
             std::unique_ptr<ExprNode> rhs)
      : op(std::move(op)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "BinaryExpr(" << op << ")\n";
    if (lhs)
      os << lhs->toString(indent + 2) << "\n";
    if (rhs)
      os << rhs->toString(indent + 2);
    return os.str();
  }

public:
  std::string op;
  std::unique_ptr<ExprNode> lhs;
  std::unique_ptr<ExprNode> rhs;
};

class UnaryExpr : public ExprNode {
public:
  UnaryExpr(std::string op, std::unique_ptr<ExprNode> expr)
      : op(std::move(op)), operand(std::move(expr)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "UnaryExpr(" << op << ")\n";
    if (operand)
      os << operand->toString(indent + 2);
    return os.str();
  }

public:
  std::string op;
  std::unique_ptr<ExprNode> operand;
};

// Function calls
class FunctionCall : public ExprNode {
public:
  FunctionCall(std::string callee, std::vector<std::unique_ptr<ExprNode>> args)
      : callee(std::move(callee)), arguments(std::move(args)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "FuncCall(" << callee << ")\n";
    for (auto &arg : arguments) {
      os << arg->toString(indent + 2) << "\n";
    }
    return os.str();
  }

public:
  std::string callee;
  std::vector<std::unique_ptr<ExprNode>> arguments;
};

class Block;

// Declaration / Assignment
class VariableDecl : public StmtNode {
public:
  VariableDecl(std::string name, std::string type,
               std::unique_ptr<ExprNode> expr)
      : name(std::move(name)), type(std::move(type)),
        initializer(std::move(expr)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "VarDecl(" << name;
    if (!typeAnnotation.empty())
      os << "t:" << typeAnnotation;
    os << ")";
    if (initializer)
      os << "\n" << expr->toString(indent + 2);
    return os.str();
  }

public:
  std::string name;
  std::string typeAnnotation;
  std::unique_ptr<ExprNode> initializer;
};

class FunctionDef : public StmtNode {
public:
  using Param = std::pair<std::string, std::string>;
  FunctionDef(std::string name, std::vector<Param> params, std::string retType,
              std::unique_ptr<Block> body)
      : name(std::move(name)), params(std::move(params)),
        retType(std::move(retType)), body(std::move(body)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "FuncDef(" << name << "(";
    for (size_t i = 0; i < params.size(); ++i) {
      if (i > 0)
        os << ", ";
      os << params[i].first << " " << params[i].second;
    }
    os << ") -> " << returnType << ")";
    if (body)
      os << "\n" << body->toString(indent + 2);
    return os.str();
  }

public:
  std::string name;
  std::vector<Param> params;
  std::string returnType;
  std::unique_ptr<Block> body;
};

class Assignment : public StmtNode {
public:
  Assignment(std::string name, std::unique_ptr<ExprNode> expr)
      : name(std::move(name)), value(std::move(expr)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "Assign(" << name << ")";
    if (value)
      os << "\n" << value->toString(indent + 2);
    return os.str();
  }

public:
  std::string name;
  std::unique_ptr<ExprNode> value;
};

// Control flow
class IfElseStmt : public StmtNode {
public:
  using ElseIf = std::pair<std::unique_ptr<ExprNode>, std::unique_ptr<Block>>;
  IfElseStmt(std::unique_ptr<ExprNode> cond, std::unique_ptr<Block> ifB,
             std::vector<ElseIf> elifs, std::unique_ptr<Block> elseB)
      : condition(std::move(cond)), ifBody(std::move(ifB)),
        elseIfBody(std::move(elifs)), elseBody(std::move(elseB)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "IfStmt";
    if (condition)
      os << "\n"
         << std::string(indent + 2, ' ') << "Cond:\n"
         << condition->toString(indent + 4);
    if (ifBody)
      os << "\n"
         << std::string(indent + 2, ' ') << "Then:\n"
         << ifBody->toString(indent + 4);
    for (auto &elif : elseIfBody) {
      os << "\n"
         << std::string(indent + 2, ' ') << "Elif:\n"
         << elif.first->toString(indent + 4) << "\n"
         << elif.second->toString(indent + 4);
    }
    if (elseBody)
      os << "\n"
         << std::string(indent + 2, ' ') << "Else:\n"
         << elseBody->toString(indent + 4);
    return os.str();
  }

public:
  std::unique_ptr<ExprNode> condition;
  std::unique_ptr<Block> ifBody;
  std::vector<ElseIf> elseIfBody;
  std::unique_ptr<Block> elseBody;
};

class WhileStmt : public StmtNode {
public:
  WhileStmt(std::unique_ptr<ExprNode> cond, std::unique_ptr<Block> body)
      : condition(std::move(cond)), body(std::move(body)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "WhileStmt";
    if (condition)
      os << "\n"
         << std::string(indent + 2, ' ') << "Cond:\n"
         << condition->toString(indent + 4);
    if (body)
      os << "\n"
         << std::string(indent + 2, ' ') << "Body:\n"
         << body->toString(indent + 4);
    return os.str();
  }

public:
  std::unique_ptr<ExprNode> condition;
  std::unique_ptr<Block> body;
};

class ForStmt : public StmtNode {
public:
  ForStmt(std::string var, std::unique_ptr<ExprNode> startEx,
          std::unique_ptr<ExprNode> endEx, std::unique_ptr<ExprNode> stepEx,
          std::unique_ptr<Block> body)
      : var(std::move(var)), startExpr(std::move(startEx)),
        endExpr(std::move(endEx)), stepExpr(std::move(stepEx)),
        body(std::move(body)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "ForStmt(";
    if (loopVar)
      os << "Var: " << loopVar;
    os << ")";
    if (startExpr)
      os << "\n"
         << std::string(indent + 2, ' ') << "Start:\n"
         << startExpr->toString(indent + 4);
    if (endExpr)
      os << "\n"
         << std::string(indent + 2, ' ') << "End:\n"
         << endExpr->toString(indent + 4);
    if (body)
      os << "\n"
         << std::string(indent + 2, ' ') << "Body:\n"
         << body->toString(indent + 4);
    return os.str();
  }

public:
  std::string loopVar;
  std::unique_ptr<ExprNode> startExpr;
  std::unique_ptr<ExprNode> endExpr;
  std::unique_ptr<Block> body;
};

class ReturnStmt : public StmtNode {
public:
  explicit ReturnStmt(std::unique_ptr<ExprNode> expr) : expr(std::move(expr)) {}

  std::string toString(int indent = 0) const override;

public:
  std::unique_ptr<ExprNode> expr;
};

class PrintStmt : public StmtNode {
public:
  explicit PrintStmt(std::vector<std::unique_ptr<ExprNode>> args)
      : arguments(std::move(args)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "PrintStmt";
    for (auto &arg : arguments) {
      os << "\n" << arg->toString(indent + 2);
    }
    return os.str();
  }

public:
  std::vector<std::unique_ptr<ExprNode>> arguments;
};

class ExprStmt : public StmtNode {
public:
  explicit ExprStmt(std::unique_ptr<ExprNode> expr) : expr(std::move(expr)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "ExprStmt";
    if (expr)
      os << "\n" << expr->toString(indent + 2);
    return os.str();
  }

public:
  std::unique_ptr<ExprNode> expr;
};

class Block : public StmtNode {
public:
  explicit Block(std::vector<std::unique_ptr<StmtNode>> stmts)
      : statements(std::move(stmts)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "Block\n";
    for (auto &stmt : statements) {
      os << stmt->toString(indent + 2) << "\n";
    }
    return os.str();
  }

public:
  std::vector<std::unique_ptr<StmtNode>> statements;
};

class Program : public ASTNode {
public:
  explicit Program(std::vector<std::unique_ptr<StmtNode>> stmts)
      : statements(std::move(stmts)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "Program\n";
    for (auto &stmt : statements) {
      os << stmt->toString(indent + 2) << "\n";
    }
    return os.str();
  }

public:
  std::vector<std::unique_ptr<StmtNode>> statements;
};

#endif // CYNIDE_AST_H