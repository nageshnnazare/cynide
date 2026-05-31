#ifndef CYNIDE_AST_H
#define CYNIDE_AST_H

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

/**
 * @class ASTNode
 * @brief Base class for all nodes in the Cynide Abstract Syntax Tree (AST).
 *
 * Provides a virtual interface for debugging/formatting the tree as a string.
 */
class ASTNode {
public:
  ASTNode() = default;
  virtual ~ASTNode() = default;

  /**
   * @brief Generates a string representation of the AST node.
   * @param indent The indentation level for pretty-printing.
   * @return A string showing the node type and its contents.
   */
  virtual std::string toString(int indent = 0) const = 0;
};

/**
 * @class ExprNode
 * @brief Base class for all expression AST nodes.
 */
class ExprNode : public ASTNode {};

/**
 * @class StmtNode
 * @brief Base class for all statement AST nodes.
 */
class StmtNode : public ASTNode {};

/**
 * @class Program
 * @brief Root AST node representing a complete Cynide program.
 *
 * A program consists of a sequence of statements.
 */
class Program : public ASTNode {
public:
  /**
   * @brief Constructs a Program node with a sequence of statements.
   * @param stmts The list of statements comprising the program.
   */
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

/**
 * @class Block
 * @brief Represents a block statement containing a sequence of statements.
 *
 * Typically used as the body of functions or control-flow constructs.
 */
class Block : public StmtNode {
public:
  /**
   * @brief Constructs a Block node.
   * @param stmts The statements inside the block.
   */
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

/**
 * @class IntLiteral
 * @brief Represents an integer literal expression.
 */
class IntLiteral : public ExprNode {
public:
  /**
   * @brief Constructs an integer literal expression.
   * @param val The 64-bit integer value.
   */
  explicit IntLiteral(int64_t val) : value(val) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "IntLiteral(" << value << ")";
    return os.str();
  }

public:
  int64_t value = 0;
};

/**
 * @class FloatLiteral
 * @brief Represents a floating-point literal expression.
 */
class FloatLiteral : public ExprNode {
public:
  /**
   * @brief Constructs a float literal expression.
   * @param val The double-precision float value.
   */
  explicit FloatLiteral(double val) : value(val) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "FloatLiteral(" << value << ")";
    return os.str();
  }

public:
  double value = 0.0;
};

/**
 * @class StringLiteral
 * @brief Represents a string literal expression.
 */
class StringLiteral : public ExprNode {
public:
  /**
   * @brief Constructs a string literal expression.
   * @param val The string value.
   */
  explicit StringLiteral(std::string val) : name(std::move(val)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "StringLiteral(" << name << ")";
    return os.str();
  }

public:
  std::string name = "";
};

/**
 * @class BoolLiteral
 * @brief Represents a boolean literal expression (`true` or `false`).
 */
class BoolLiteral : public ExprNode {
public:
  /**
   * @brief Constructs a boolean literal expression node.
   * @param val The boolean value.
   */
  explicit BoolLiteral(bool val) : value(std::move(val)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "BoolLiteral("
       << (value ? "true" : "false") << ")";
    return os.str();
  }

public:
  bool value = false;
};

/**
 * @class Identifier
 * @brief Represents an identifier expression (variable name or function name).
 */
class Identifier : public ExprNode {
public:
  /**
   * @brief Constructs an identifier expression node.
   * @param name The identifier name string.
   */
  explicit Identifier(std::string name) : name(std::move(name)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "Identifier(" << name << ")";
    return os.str();
  }

public:
  std::string name = "";
};

/**
 * @class BinaryExpr
 * @brief Represents an expression with a binary operator.
 *
 * Example: `lhs + rhs`.
 */
class BinaryExpr : public ExprNode {
public:
  /**
   * @brief Constructs a BinaryExpr.
   * @param op The operator symbol (e.g. "+", "*").
   * @param lhs The left-hand side operand.
   * @param rhs The right-hand side operand.
   */
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

/**
 * @class UnaryExpr
 * @brief Represents an expression with a unary operator.
 *
 * Example: `-operand`, `not operand`.
 */
class UnaryExpr : public ExprNode {
public:
  /**
   * @brief Constructs a UnaryExpr.
   * @param op The unary operator symbol.
   * @param expr The operand expression.
   */
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

/**
 * @class ExprStmt
 * @brief Represents a statement wrapping an expression.
 */
class ExprStmt : public StmtNode {
public:
  /**
   * @brief Constructs an ExprStmt.
   * @param expr The expression being evaluated as a statement.
   */
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

/**
 * @class FunctionCall
 * @brief Represents a function call expression.
 */
class FunctionCall : public ExprNode {
public:
  /**
   * @brief Constructs a FunctionCall.
   * @param callee The name of the function to call.
   * @param args The arguments to pass.
   */
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

/**
 * @class FunctionDef
 * @brief Represents a function definition.
 */
class FunctionDef : public StmtNode {
public:
  using Param = std::pair<std::string, std::string>;

  /**
   * @brief Constructs a FunctionDef.
   * @param name The name of the function.
   * @param params The formal parameter list.
   * @param retType The return type annotation.
   * @param body The body block of the function.
   */
  FunctionDef(std::string name, std::vector<Param> params, std::string retType,
              std::unique_ptr<Block> body)
      : name(std::move(name)), params(std::move(params)),
        returnType(std::move(retType)), body(std::move(body)) {}

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

/**
 * @class VariableDecl
 * @brief Represents a variable declaration statement.
 *
 * Example: `let x: int = 42`.
 */
class VariableDecl : public StmtNode {
public:
  /**
   * @brief Constructs a VariableDecl.
   * @param name The name of the declared variable.
   * @param type The type annotation string (optional).
   * @param expr The initializer expression.
   */
  VariableDecl(std::string name, std::string type,
               std::unique_ptr<ExprNode> expr)
      : name(std::move(name)), typeAnnotation(std::move(type)),
        initializer(std::move(expr)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "VarDecl(" << name;
    if (!typeAnnotation.empty())
      os << "t:" << typeAnnotation;
    os << ")";
    if (initializer)
      os << "\n" << initializer->toString(indent + 2);
    return os.str();
  }

public:
  std::string name;
  std::string typeAnnotation;
  std::unique_ptr<ExprNode> initializer;
};

/**
 * @class AssignStmt
 * @brief Represents a variable assignment statement.
 *
 * Example: `x = 42`.
 */
class AssignStmt : public StmtNode {
public:
  /**
   * @brief Constructs an Assignment statement.
   * @param name The target variable name.
   * @param expr The value expression to assign.
   */
  AssignStmt(std::string name, std::unique_ptr<ExprNode> expr)
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

/**
 * @class IfElseStmt
 * @brief Represents an if-elif-else conditional statement.
 */
class IfElseStmt : public StmtNode {
public:
  using ElseIf = std::pair<std::unique_ptr<ExprNode>, std::unique_ptr<Block>>;

  /**
   * @brief Constructs an IfElseStmt.
   * @param cond The if condition.
   * @param ifB The if body block.
   * @param elifs The list of elif clauses.
   * @param elseB The else body block.
   */
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

/**
 * @class WhileStmt
 * @brief Represents a while loop statement.
 */
class WhileStmt : public StmtNode {
public:
  /**
   * @brief Constructs a WhileStmt.
   * @param cond The loop termination condition expression.
   * @param body The loop body block.
   */
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

/**
 * @class ForStmt
 * @brief Represents a for loop over `range(...)`.
 *
 * Examples:
 *   - `for i in range(10):`  — startExpr is 10, endExpr is null (start 0).
 *   - `for i in range(3, 10):` — startExpr is 3, endExpr is 10.
 */
class ForStmt : public StmtNode {
public:
  /**
   * @brief Constructs a ForStmt.
   * @param var The loop iterator variable name.
   * @param startEx Lower bound, or sole argument for `range(n)`.
   * @param endEx Upper bound; null for single-argument `range(n)`.
   * @param body The loop body block.
   */
  ForStmt(std::string var, std::unique_ptr<ExprNode> startEx,
          std::unique_ptr<ExprNode> endEx, std::unique_ptr<Block> body)
      : loopVar(std::move(var)), startExpr(std::move(startEx)),
        endExpr(std::move(endEx)), body(std::move(body)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "ForStmt(" << "Var: " << loopVar << ")";
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

/**
 * @class ReturnStmt
 * @brief Represents a return statement.
 */
class ReturnStmt : public StmtNode {
public:
  /**
   * @brief Constructs a ReturnStmt.
   * @param expr The expression to return (optional, can be nullptr).
   */
  explicit ReturnStmt(std::unique_ptr<ExprNode> expr) : expr(std::move(expr)) {}

  std::string toString(int indent = 0) const override {
    std::ostringstream os;
    os << std::string(indent, ' ') << "ReturnStmt";
    if (expr)
      os << "\n" << expr->toString(indent + 2);
    return os.str();
  }

public:
  std::unique_ptr<ExprNode> expr;
};

/**
 * @class PrintStmt
 * @brief Represents a built-in print statement.
 */
class PrintStmt : public StmtNode {
public:
  /**
   * @brief Constructs a PrintStmt.
   * @param args The list of expressions to print.
   */
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

#endif // CYNIDE_AST_H