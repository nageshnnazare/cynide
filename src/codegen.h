#ifndef CYNIDE_CODEGEN_H
#define CYNIDE_CODEGEN_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <ast.h>
#include <stage.h>

namespace llvm {
class AllocaInst;
class Function;
class Type;
} // namespace llvm

/**
 * @class Codegen
 * @brief Emits LLVM IR for a Cynide program and can write textual IR or object
 * files.
 *
 * Key LLVM pieces used here:
 * - \c LLVMContext owns LLVM-wide state.
 * - \c Module is a translation unit (global constants, functions).
 * - \c IRBuilder inserts instructions into the current basic block.
 * - \c AllocaInst + \c Store / \c Load give mutable locals (SSA after mem2reg
 * in optimized pipelines; for this educational compiler we keep it simple).
 */
class Codegen : public Stage {
public:
  Codegen();
  ~Codegen() = default;

  bool hasError() const override { return _error; }
  std::string errorMessage() const override { return _errorMessage; }

  void generate(Program &program);
  void dumpIR();
  void writeIR(const std::string &filename);
  void compileToObject(const std::string &filename);

private:
  bool _error = false;
  std::string _errorMessage;

private:
  struct Binding {
    llvm::AllocaInst *_allocaInst = nullptr;
    std::string _dataType;
  };

  llvm::LLVMContext _context;
  std::unique_ptr<llvm::Module> _module;
  llvm::IRBuilder<> _builder;

  llvm::Function *_currentFunction = nullptr;

  std::vector<std::map<std::string, Binding>> _scopeStack;

  std::map<std::string, llvm::Function *> _functions;
  std::map<std::string, std::string> _functionReturnTypes;

  llvm::Type *_voidTy = nullptr;
  llvm::Type *_int32Ty = nullptr;
  llvm::Type *_int64Ty = nullptr;
  llvm::Type *_doubleTy = nullptr;
  llvm::Type *_boolTy = nullptr;
  llvm::PointerType *_ptrTy = nullptr;

  void reportError(const std::string &msg);

  void declarePrintf();
  void registerFunction(FunctionDef &fd);
  void generateStatement(StmtNode *stmt);
  llvm::Value *generateExpression(ExprNode *expr);

  void generateVariableDecl(VariableDecl *node);
  void generateAssignment(AssignStmt *node);
  void generateFunctionDef(FunctionDef *node);
  void generateIfStatement(IfElseStmt *node);
  void generateWhileStatement(WhileStmt *node);
  void generateForStatement(ForStmt *node);
  void generateReturnStatement(ReturnStmt *node);
  void generatePrintStatement(PrintStmt *node);
  void generateExprStmt(ExprStmt *node);
  void generateBlock(Block *node);

  llvm::Value *generateFunctionCall(FunctionCall *node);
  llvm::Value *generateBinaryExpr(BinaryExpr *node);
  llvm::Value *generateUnaryExpr(UnaryExpr *node);

  llvm::Type *getLLVMType(const std::string &typeName);
  llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *fn,
                                           const std::string &name,
                                           llvm::Type *type);

  void pushScope();
  void popScope();
  void bindVariable(const std::string &name, llvm::AllocaInst *alloc,
                    const std::string &blazeTy);
  Binding *lookupBinding(const std::string &name);

  std::string inferExprType(ExprNode *expr);

  llvm::Constant *getConstantString(const std::string &s);
};

#endif // CYNIDE_CODEGEN_H