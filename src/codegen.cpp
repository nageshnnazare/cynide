#include <cstdlib>

#include <llvm/ADT/APFloat.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <codegen.h>

namespace {

llvm::PointerType *makeCStringPtrTy(llvm::LLVMContext &ctx) {
  return llvm::PointerType::getUnqual(ctx);
}

llvm::Constant *castToCStringPtr(llvm::Constant *raw,
                                 llvm::PointerType *ptrTy) {
  if (raw->getType() == ptrTy)
    return raw;
  return llvm::ConstantExpr::getBitCast(raw, ptrTy);
}

llvm::Constant *globalFmtString(llvm::LLVMContext &ctx, llvm::Module *mod,
                                llvm::PointerType *ptrTy,
                                const std::string &s) {
  llvm::IRBuilder<> tmp(ctx);
  llvm::Constant *raw;
  raw = tmp.CreateGlobalString(s, "cynide_fmt", 0, mod);
  return castToCStringPtr(raw, ptrTy);
}

llvm::Value *boolToCString(llvm::IRBuilder<> &builder, llvm::LLVMContext &ctx,
                           llvm::Module *mod, llvm::PointerType *ptrTy,
                           llvm::Value *condI1) {
  llvm::Constant *t = globalFmtString(ctx, mod, ptrTy, "true\n");
  llvm::Constant *f = globalFmtString(ctx, mod, ptrTy, "false\n");
  return builder.CreateSelect(condI1, t, f);
}

} // namespace

Codegen::Codegen()
    : _module(std::make_unique<llvm::Module>("blaze_module", _context)),
      _builder(_context) {
  _voidTy = llvm::Type::getVoidTy(_context);
  _int64Ty = llvm::Type::getInt64Ty(_context);
  _int32Ty = llvm::Type::getInt32Ty(_context);
  _doubleTy = llvm::Type::getDoubleTy(_context);
  _boolTy = llvm::Type::getInt1Ty(_context);
  _ptrTy = makeCStringPtrTy(_context);
}

void Codegen::reportError(const std::string &msg) {
  _error = true;
  _errorMessage = msg;
}

void Codegen::declarePrintf() {
  llvm::FunctionType *printfTy =
      llvm::FunctionType::get(_int32Ty, _ptrTy, true);
  llvm::Function::Create(printfTy, llvm::Function::ExternalLinkage, "printf",
                         _module.get());
}

void Codegen::pushScope() { _scopeStack.emplace_back(); }

void Codegen::popScope() {
  if (!_scopeStack.empty())
    _scopeStack.pop_back();
}

void Codegen::bindVariable(const std::string &name, llvm::AllocaInst *alloc,
                           const std::string &dataType) {
  if (_scopeStack.empty())
    pushScope();
  _scopeStack.back()[name] = Binding{alloc, dataType};
}

Codegen::Binding *Codegen::lookupBinding(const std::string &name) {
  for (auto it = _scopeStack.rbegin(); it != _scopeStack.rend(); ++it) {
    auto j = it->find(name);
    if (j != it->end())
      return &j->second;
  }
  return nullptr;
}

llvm::Type *Codegen::getLLVMType(const std::string &typeName) {
  if (typeName == "int")
    return _int64Ty;
  if (typeName == "float")
    return _doubleTy;
  if (typeName == "bool")
    return _boolTy;
  if (typeName == "string")
    return _ptrTy;
  if (typeName == "void")
    return _voidTy;
  reportError("Unknown Cynide type: " + typeName);
  return _int64Ty;
}

llvm::AllocaInst *Codegen::createEntryBlockAlloca(llvm::Function *fn,
                                                  const std::string &name,
                                                  llvm::Type *type) {
  llvm::IRBuilder<> tmp(&fn->getEntryBlock(), fn->getEntryBlock().begin());
  return tmp.CreateAlloca(type, nullptr, name);
}

llvm::Constant *Codegen::getConstantString(const std::string &s) {
  return globalFmtString(_context, _module.get(), _ptrTy, s);
}

std::string Codegen::inferExprType(ExprNode *expr) {
  if (!expr)
    return "void";

  if (dynamic_cast<IntLiteral *>(expr))
    return "int";
  if (dynamic_cast<FloatLiteral *>(expr))
    return "float";
  if (dynamic_cast<StringLiteral *>(expr))
    return "string";
  if (dynamic_cast<BoolLiteral *>(expr))
    return "bool";

  if (auto *id = dynamic_cast<Identifier *>(expr)) {
    if (Binding *b = lookupBinding(id->name))
      return b->_dataType;
    reportError("Unknown identifier '" + id->name + "' in type inference.");
    return "int";
  }

  if (auto *call = dynamic_cast<FunctionCall *>(expr)) {
    auto it = _functionReturnTypes.find(call->callee);
    if (it != _functionReturnTypes.end())
      return it->second;
    reportError("Call to unknown function '" + call->callee + "'.");
    return "int";
  }

  if (auto *u = dynamic_cast<UnaryExpr *>(expr)) {
    if (u->op == "not")
      return "bool";
    return inferExprType(u->operand.get());
  }

  if (auto *b = dynamic_cast<BinaryExpr *>(expr)) {
    const std::string &op = b->op;
    if (op == "and" || op == "or" || op == "==" || op == "!=" || op == "<" ||
        op == ">" || op == "<=" || op == ">=")
      return "bool";

    std::string lt = inferExprType(b->lhs.get());
    std::string rt = inferExprType(b->rhs.get());
    if (op == "%")
      return "int";
    if (lt == "float" || rt == "float")
      return "float";
    return "int";
  }

  return "int";
}

llvm::Value *Codegen::generateExpression(ExprNode *expr) {
  if (!expr) {
    reportError("Internal error: null expression.");
    return nullptr;
  }

  if (auto *lit = dynamic_cast<IntLiteral *>(expr)) {
    return llvm::ConstantInt::get(_int64Ty, static_cast<uint64_t>(lit->value),
                                  true);
  }
  if (auto *lit = dynamic_cast<FloatLiteral *>(expr)) {
    return llvm::ConstantFP::get(_context, llvm::APFloat(lit->value));
  }
  if (auto *lit = dynamic_cast<StringLiteral *>(expr)) {
    llvm::IRBuilder<> tmp(_context);
    llvm::Constant *g;

    g = tmp.CreateGlobalString(lit->name.c_str(), "strlit", 0, _module.get());
    return castToCStringPtr(g, _ptrTy);
  }
  if (auto *lit = dynamic_cast<BoolLiteral *>(expr)) {
    return llvm::ConstantInt::get(_boolTy, lit->value ? 1 : 0);
  }

  if (auto *id = dynamic_cast<Identifier *>(expr)) {
    Binding *b = lookupBinding(id->name);
    if (!b) {
      reportError("Unknown variable '" + id->name + "'.");
      return llvm::ConstantInt::get(_int64Ty, 0);
    }
    llvm::Type *ty = getLLVMType(b->_dataType);
    return _builder.CreateLoad(ty, b->_allocaInst, id->name + ".val");
  }

  if (auto *be = dynamic_cast<BinaryExpr *>(expr))
    return generateBinaryExpr(be);
  if (auto *ue = dynamic_cast<UnaryExpr *>(expr))
    return generateUnaryExpr(ue);
  if (auto *call = dynamic_cast<FunctionCall *>(expr))
    return generateFunctionCall(call);

  reportError("Unsupported expression in codegen.");
  return nullptr;
}

llvm::Value *Codegen::generateUnaryExpr(UnaryExpr *node) {
  llvm::Value *inner = generateExpression(node->operand.get());
  if (!inner)
    return nullptr;

  if (node->op == "not") {
    llvm::Value *c = inner;
    if (c->getType() != _boolTy)
      c = _builder.CreateICmpNE(c, llvm::ConstantInt::get(c->getType(), 0),
                                "tobool");
    return _builder.CreateXor(c, llvm::ConstantInt::get(_boolTy, 1), "lnot");
  }

  if (node->op == "-") {
    std::string t = inferExprType(node->operand.get());
    if (t == "float")
      return _builder.CreateFNeg(inner, "fneg");
    return _builder.CreateNeg(inner, "neg");
  }

  reportError("Unknown unary operator '" + node->op + "'.");
  return inner;
}

static llvm::Value *promoteToFP(llvm::IRBuilder<> &_builder, llvm::Value *v,
                                llvm::Type *_int64Ty, llvm::Type *_doubleTy,
                                bool isFloat) {
  if (!isFloat)
    return v;
  if (v->getType() == _doubleTy)
    return v;
  return _builder.CreateSIToFP(v, _doubleTy, "sitofp");
}

llvm::Value *Codegen::generateBinaryExpr(BinaryExpr *node) {
  const std::string &op = node->op;

  if (op == "and" || op == "or") {
    llvm::Value *L = generateExpression(node->lhs.get());
    llvm::Value *R = generateExpression(node->rhs.get());
    if (L->getType() != _boolTy)
      L = _builder.CreateICmpNE(L, llvm::ConstantInt::get(L->getType(), 0),
                                "lb");
    if (R->getType() != _boolTy)
      R = _builder.CreateICmpNE(R, llvm::ConstantInt::get(R->getType(), 0),
                                "rb");
    if (op == "and")
      return _builder.CreateAnd(L, R);
    return _builder.CreateOr(L, R);
  }

  llvm::Value *L = generateExpression(node->lhs.get());
  llvm::Value *R = generateExpression(node->rhs.get());
  if (!L || !R)
    return nullptr;

  std::string lt = inferExprType(node->lhs.get());
  std::string rt = inferExprType(node->rhs.get());
  bool fp = (lt == "float" || rt == "float");

  if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" ||
      op == ">=") {
    if (lt == "string" && rt == "string") {
      llvm::Function *strcmpFn = _module->getFunction("strcmp");
      if (!strcmpFn) {
        llvm::FunctionType *fty =
            llvm::FunctionType::get(_int32Ty, {_ptrTy, _ptrTy}, false);
        strcmpFn = llvm::Function::Create(fty, llvm::Function::ExternalLinkage,
                                          "strcmp", _module.get());
      }
      llvm::Value *cmp = _builder.CreateCall(strcmpFn, {L, R}, "strcmp.call");
      llvm::Value *eq =
          _builder.CreateICmpEQ(cmp, llvm::ConstantInt::get(_int32Ty, 0));
      if (op == "==")
        return eq;
      if (op == "!=")
        return _builder.CreateXor(eq, llvm::ConstantInt::get(_boolTy, 1));
      reportError("Only == and != are supported for strings.");
      return eq;
    }

    if (fp) {
      L = promoteToFP(_builder, L, _int64Ty, _doubleTy, fp);
      R = promoteToFP(_builder, R, _int64Ty, _doubleTy, fp);
      llvm::CmpInst::Predicate pred;
      if (op == "==")
        pred = llvm::CmpInst::FCMP_OEQ;
      else if (op == "!=")
        pred = llvm::CmpInst::FCMP_ONE;
      else if (op == "<")
        pred = llvm::CmpInst::FCMP_OLT;
      else if (op == ">")
        pred = llvm::CmpInst::FCMP_OGT;
      else if (op == "<=")
        pred = llvm::CmpInst::FCMP_OLE;
      else
        pred = llvm::CmpInst::FCMP_OGE;
      return _builder.CreateFCmp(pred, L, R);
    }

    llvm::CmpInst::Predicate pred;
    if (op == "==")
      pred = llvm::CmpInst::ICMP_EQ;
    else if (op == "!=")
      pred = llvm::CmpInst::ICMP_NE;
    else if (op == "<")
      pred = llvm::CmpInst::ICMP_SLT;
    else if (op == ">")
      pred = llvm::CmpInst::ICMP_SGT;
    else if (op == "<=")
      pred = llvm::CmpInst::ICMP_SLE;
    else
      pred = llvm::CmpInst::ICMP_SGE;
    return _builder.CreateICmp(pred, L, R);
  }

  if (op == "%") {
    return _builder.CreateSRem(L, R, "srem");
  }

  L = promoteToFP(_builder, L, _int64Ty, _doubleTy, fp);
  R = promoteToFP(_builder, R, _int64Ty, _doubleTy, fp);

  if (fp) {
    if (op == "+")
      return _builder.CreateFAdd(L, R, "fadd");
    if (op == "-")
      return _builder.CreateFSub(L, R, "fsub");
    if (op == "*")
      return _builder.CreateFMul(L, R, "fmul");
    if (op == "/")
      return _builder.CreateFDiv(L, R, "fdiv");
  } else {
    if (op == "+")
      return _builder.CreateNSWAdd(L, R, "add");
    if (op == "-")
      return _builder.CreateNSWSub(L, R, "sub");
    if (op == "*")
      return _builder.CreateNSWMul(L, R, "mul");
    if (op == "/")
      return _builder.CreateSDiv(L, R, "sdiv");
  }

  reportError("Unknown binary operator '" + op + "'.");
  return nullptr;
}

llvm::Value *Codegen::generateFunctionCall(FunctionCall *node) {
  llvm::Function *callee = _module->getFunction(node->callee);
  if (!callee) {
    reportError("Unknown function '" + node->callee + "' in call.");
    return llvm::ConstantInt::get(_int64Ty, 0);
  }

  std::vector<llvm::Value *> args;
  unsigned idx = 0;
  for (auto &param : callee->args()) {
    if (idx >= node->arguments.size())
      break;
    llvm::Value *v = generateExpression(node->arguments[idx].get());
    llvm::Type *expect = param.getType();
    if (v->getType() != expect) {
      if (expect == _doubleTy && v->getType() == _int64Ty)
        v = _builder.CreateSIToFP(v, _doubleTy, "arg.fp");
      else if (expect == _int64Ty && v->getType() == _doubleTy)
        v = _builder.CreateFPToSI(v, _int64Ty, "arg.i64");
    }
    args.push_back(v);
    ++idx;
  }

  llvm::Type *retTy = callee->getReturnType();
  if (retTy->isVoidTy()) {
    _builder.CreateCall(callee, args);
    return nullptr;
  }
  return _builder.CreateCall(callee, args, "calltmp");
}

void Codegen::generateVariableDecl(VariableDecl *node) {
  std::string ty = node->typeAnnotation.empty()
                       ? inferExprType(node->initializer.get())
                       : node->typeAnnotation;

  llvm::Type *llvmTy = getLLVMType(ty);
  llvm::AllocaInst *alloc =
      createEntryBlockAlloca(_currentFunction, node->name, llvmTy);

  llvm::Value *init = generateExpression(node->initializer.get());
  if (!init)
    return;

  if (init->getType() != llvmTy) {
    if (llvmTy == _doubleTy && init->getType() == _int64Ty)
      init = _builder.CreateSIToFP(init, _doubleTy, "init.fp");
    else if (llvmTy == _int64Ty && init->getType() == _doubleTy)
      init = _builder.CreateFPToSI(init, _int64Ty, "init.i64");
  }

  _builder.CreateStore(init, alloc);
  bindVariable(node->name, alloc, ty);
}

void Codegen::generateAssignment(AssignStmt *node) {
  Binding *b = lookupBinding(node->name);
  if (!b) {
    reportError("Assignment to unknown variable '" + node->name + "'.");
    return;
  }
  llvm::Type *ty = getLLVMType(b->_dataType);
  llvm::Value *rhs = generateExpression(node->value.get());
  if (!rhs)
    return;
  if (rhs->getType() != ty) {
    if (ty == _doubleTy && rhs->getType() == _int64Ty)
      rhs = _builder.CreateSIToFP(rhs, _doubleTy, "asgn.fp");
    else if (ty == _int64Ty && rhs->getType() == _doubleTy)
      rhs = _builder.CreateFPToSI(rhs, _int64Ty, "asgn.i64");
  }
  _builder.CreateStore(rhs, b->_allocaInst);
}

void Codegen::generateExprStmt(ExprStmt *node) {
  llvm::Value *v = generateExpression(node->expr.get());
  (void)v;
}

void Codegen::generatePrintStatement(PrintStmt *node) {
  llvm::Function *printfFn = _module->getFunction("printf");
  if (!printfFn) {
    reportError("printf not declared.");
    return;
  }

  std::string fmt;
  std::vector<llvm::Value *> argsVals;

  bool first = true;
  for (auto &arg : node->arguments) {
    if (!first)
      fmt.push_back(' ');
    first = false;

    std::string ty = inferExprType(arg.get());
    llvm::Value *v = generateExpression(arg.get());
    if (!v)
      return;

    if (ty == "int") {
      fmt += "%lld";
      argsVals.push_back(v);
    } else if (ty == "float") {
      fmt += "%f";
      llvm::Value *d = v;
      if (d->getType() != _doubleTy)
        d = _builder.CreateSIToFP(d, _doubleTy, "print.fp");
      argsVals.push_back(d);
    } else if (ty == "bool") {
      fmt += "%s";
      llvm::Value *c = v;
      if (c->getType() != _boolTy)
        c = _builder.CreateICmpNE(c, llvm::ConstantInt::get(c->getType(), 0),
                                  "pb");
      llvm::Value *sel =
          boolToCString(_builder, _context, _module.get(), _ptrTy, c);
      argsVals.push_back(sel);
    } else if (ty == "string") {
      fmt += "%s";
      argsVals.push_back(v);
    } else {
      reportError("Unsupported type in print.");
      return;
    }
  }

  fmt.push_back('\n');
  llvm::Constant *fmtPtr = getConstantString(fmt);

  std::vector<llvm::Value *> callArgs;
  callArgs.push_back(fmtPtr);
  callArgs.insert(callArgs.end(), argsVals.begin(), argsVals.end());
  _builder.CreateCall(printfFn, callArgs);
}

void Codegen::generateReturnStatement(ReturnStmt *node) {
  llvm::Function *fn = _builder.GetInsertBlock()->getParent();
  llvm::Type *retTy = fn->getReturnType();

  if (retTy->isVoidTy()) {
    if (node->expr)
      reportError("Value returned in void function.");
    _builder.CreateRetVoid();
    return;
  }

  if (!node->expr) {
    reportError("Missing return value.");
    _builder.CreateRet(llvm::Constant::getNullValue(retTy));
    return;
  }

  llvm::Value *v = generateExpression(node->expr.get());
  if (!v)
    return;
  if (v->getType() != retTy) {
    if (retTy == _doubleTy && v->getType() == _int64Ty)
      v = _builder.CreateSIToFP(v, _doubleTy, "ret.fp");
    else if (retTy == _int64Ty && v->getType() == _doubleTy)
      v = _builder.CreateFPToSI(v, _int64Ty, "ret.i64");
  }
  _builder.CreateRet(v);
}

void Codegen::generateIfStatement(IfElseStmt *node) {
  llvm::Function *fn = _builder.GetInsertBlock()->getParent();
  llvm::Value *condVal = generateExpression(node->condition.get());
  if (!condVal)
    return;
  if (condVal->getType() != _boolTy)
    condVal = _builder.CreateICmpNE(
        condVal, llvm::ConstantInt::get(condVal->getType(), 0), "ifcond");

  llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(_context, "then", fn);
  llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(_context, "else", fn);
  llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(_context, "ifcont", fn);

  _builder.CreateCondBr(condVal, thenBB, elseBB);

  _builder.SetInsertPoint(thenBB);
  generateBlock(node->ifBody.get());
  if (!_builder.GetInsertBlock()->getTerminator())
    _builder.CreateBr(mergeBB);

  _builder.SetInsertPoint(elseBB);

  for (auto &elif : node->elseIfBody) {
    llvm::BasicBlock *elifThen =
        llvm::BasicBlock::Create(_context, "elif.then", fn);
    llvm::BasicBlock *elifElse =
        llvm::BasicBlock::Create(_context, "elif.else", fn);

    llvm::Value *ec = generateExpression(elif.first.get());
    if (!ec)
      return;
    if (ec->getType() != _boolTy)
      ec = _builder.CreateICmpNE(ec, llvm::ConstantInt::get(ec->getType(), 0),
                                 "elifcond");

    _builder.CreateCondBr(ec, elifThen, elifElse);

    _builder.SetInsertPoint(elifThen);
    generateBlock(elif.second.get());
    if (!_builder.GetInsertBlock()->getTerminator())
      _builder.CreateBr(mergeBB);

    _builder.SetInsertPoint(elifElse);
  }

  if (node->elseBody) {
    generateBlock(node->elseBody.get());
    if (!_builder.GetInsertBlock()->getTerminator())
      _builder.CreateBr(mergeBB);
  }

  _builder.SetInsertPoint(mergeBB);
}

void Codegen::generateWhileStatement(WhileStmt *node) {
  llvm::Function *fn = _builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *condBB =
      llvm::BasicBlock::Create(_context, "while.cond", fn);
  llvm::BasicBlock *bodyBB =
      llvm::BasicBlock::Create(_context, "while.body", fn);
  llvm::BasicBlock *afterBB =
      llvm::BasicBlock::Create(_context, "while.end", fn);

  _builder.CreateBr(condBB);
  _builder.SetInsertPoint(condBB);
  llvm::Value *c = generateExpression(node->condition.get());
  if (!c)
    return;
  if (c->getType() != _boolTy)
    c = _builder.CreateICmpNE(c, llvm::ConstantInt::get(c->getType(), 0),
                              "while.cond.bool");
  _builder.CreateCondBr(c, bodyBB, afterBB);

  _builder.SetInsertPoint(bodyBB);
  generateBlock(node->body.get());
  if (!_builder.GetInsertBlock()->getTerminator())
    _builder.CreateBr(condBB);

  _builder.SetInsertPoint(afterBB);
}

void Codegen::generateForStatement(ForStmt *node) {
  llvm::Function *fn = _builder.GetInsertBlock()->getParent();

  llvm::AllocaInst *loopAlloc =
      createEntryBlockAlloca(fn, node->loopVar, _int64Ty);

  llvm::Value *startV = generateExpression(node->startExpr.get());
  llvm::Value *endV = generateExpression(node->endExpr.get());
  if (!startV || !endV)
    return;

  _builder.CreateStore(startV, loopAlloc);

  llvm::BasicBlock *condBB = llvm::BasicBlock::Create(_context, "for.cond", fn);
  llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(_context, "for.body", fn);
  llvm::BasicBlock *stepBB = llvm::BasicBlock::Create(_context, "for.step", fn);
  llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(_context, "for.end", fn);

  _builder.CreateBr(condBB);
  _builder.SetInsertPoint(condBB);
  llvm::Value *cur = _builder.CreateLoad(_int64Ty, loopAlloc, "for.iv");
  llvm::Value *cond = _builder.CreateICmpSLT(cur, endV, "for.cmp");
  _builder.CreateCondBr(cond, bodyBB, afterBB);

  _builder.SetInsertPoint(bodyBB);
  pushScope();
  bindVariable(node->loopVar, loopAlloc, "int");
  generateBlock(node->body.get());
  popScope();
  if (!_builder.GetInsertBlock()->getTerminator())
    _builder.CreateBr(stepBB);

  _builder.SetInsertPoint(stepBB);
  llvm::Value *curReload =
      _builder.CreateLoad(_int64Ty, loopAlloc, "for.iv.next");
  llvm::Value *nxt = _builder.CreateNSWAdd(
      curReload, llvm::ConstantInt::get(_int64Ty, 1), "for.inc");
  _builder.CreateStore(nxt, loopAlloc);
  _builder.CreateBr(condBB);

  _builder.SetInsertPoint(afterBB);
}

void Codegen::generateBlock(Block *node) {
  pushScope();
  for (auto &st : node->statements) {
    generateStatement(st.get());
    if (_error)
      break;
    if (_builder.GetInsertBlock()->getTerminator())
      break;
  }
  popScope();
}

void Codegen::registerFunction(FunctionDef &fd) {
  if (_functions.count(fd.name)) {
    reportError("Duplicate function definition '" + fd.name + "'.");
    return;
  }

  std::vector<llvm::Type *> paramTys;
  for (auto &p : fd.params)
    paramTys.push_back(getLLVMType(p.second));

  llvm::Type *retTy = getLLVMType(fd.returnType);
  llvm::FunctionType *fty = llvm::FunctionType::get(retTy, paramTys, false);

  llvm::Function *fn = llvm::Function::Create(
      fty, llvm::Function::ExternalLinkage, fd.name, _module.get());

  unsigned idx = 0;
  for (auto &arg : fn->args()) {
    arg.setName(fd.params[idx].first);
    ++idx;
  }

  _functions[fd.name] = fn;
  _functionReturnTypes[fd.name] = fd.returnType;
}

void Codegen::generateFunctionDef(FunctionDef *node) {
  llvm::Function *fn = _module->getFunction(node->name);
  if (!fn) {
    reportError("Function '" + node->name + "' not registered.");
    return;
  }

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(_context, "entry", fn);
  _builder.SetInsertPoint(entry);

  llvm::Function *prevFn = _currentFunction;
  _currentFunction = fn;
  pushScope();

  unsigned idx = 0;
  for (auto &arg : fn->args()) {
    const std::string &argName = node->params[idx].first;
    const std::string &argTyBlaze = node->params[idx].second;
    llvm::AllocaInst *alloc =
        createEntryBlockAlloca(fn, argName, arg.getType());
    _builder.CreateStore(&arg, alloc);
    bindVariable(argName, alloc, argTyBlaze);
    ++idx;
  }

  generateBlock(node->body.get());

  if (!_builder.GetInsertBlock()->getTerminator()) {
    llvm::Type *retTy = fn->getReturnType();
    if (retTy->isVoidTy())
      _builder.CreateRetVoid();
    else if (retTy == _int64Ty)
      _builder.CreateRet(llvm::ConstantInt::get(_int64Ty, 0));
    else if (retTy == _doubleTy)
      _builder.CreateRet(llvm::ConstantFP::get(_doubleTy, 0.0));
    else if (retTy == _boolTy)
      _builder.CreateRet(llvm::ConstantInt::get(_boolTy, 0));
    else if (retTy == _ptrTy)
      _builder.CreateRet(llvm::ConstantPointerNull::get(_ptrTy));
    else
      _builder.CreateRet(llvm::Constant::getNullValue(retTy));
  }

  popScope();
  _currentFunction = prevFn;

  llvm::verifyFunction(*fn);
}

void Codegen::generateStatement(StmtNode *stmt) {
  if (!stmt)
    return;

  if (auto *v = dynamic_cast<VariableDecl *>(stmt))
    generateVariableDecl(v);
  else if (auto *a = dynamic_cast<AssignStmt *>(stmt))
    generateAssignment(a);
  else if (auto *p = dynamic_cast<PrintStmt *>(stmt))
    generatePrintStatement(p);
  else if (auto *r = dynamic_cast<ReturnStmt *>(stmt))
    generateReturnStatement(r);
  else if (auto *w = dynamic_cast<WhileStmt *>(stmt))
    generateWhileStatement(w);
  else if (auto *f = dynamic_cast<ForStmt *>(stmt))
    generateForStatement(f);
  else if (auto *iff = dynamic_cast<IfElseStmt *>(stmt))
    generateIfStatement(iff);
  else if (auto *e = dynamic_cast<ExprStmt *>(stmt))
    generateExprStmt(e);
  else if (auto *f = dynamic_cast<FunctionDef *>(stmt))
    generateFunctionDef(f);
  else if (auto *b = dynamic_cast<Block *>(stmt))
    generateBlock(b);
  else {
    // Definitions are handled separately before main.
    reportError("Unsupported statement kind in codegen.");
  }
}

void Codegen::generate(Program &program) {
  _error = false;
  _errorMessage.clear();

  declarePrintf();

  for (auto &st : program.statements) {
    if (auto *fd = dynamic_cast<FunctionDef *>(st.get()))
      registerFunction(*fd);
    if (_error)
      return;
  }

  for (auto &st : program.statements) {
    if (auto *fd = dynamic_cast<FunctionDef *>(st.get()))
      generateFunctionDef(fd);
    if (_error)
      return;
  }

  llvm::FunctionType *mainTy = llvm::FunctionType::get(_int32Ty, false);
  llvm::Function *mainFn = llvm::Function::Create(
      mainTy, llvm::Function::ExternalLinkage, "main", _module.get());

  llvm::BasicBlock *bb = llvm::BasicBlock::Create(_context, "entry", mainFn);
  _builder.SetInsertPoint(bb);

  _currentFunction = mainFn;
  pushScope();

  for (auto &st : program.statements) {
    if (dynamic_cast<FunctionDef *>(st.get()))
      continue;
    generateStatement(st.get());
    if (_error)
      break;
    if (_builder.GetInsertBlock()->getTerminator())
      break;
  }

  if (!_builder.GetInsertBlock()->getTerminator())
    _builder.CreateRet(llvm::ConstantInt::get(_int32Ty, 0));

  popScope();
  _currentFunction = nullptr;

  llvm::verifyFunction(*mainFn);
  llvm::verifyModule(*_module);
}

void Codegen::dumpIR() {
  if (_error)
    return;
  if (_module != nullptr)
    _module->print(llvm::outs(), nullptr);
}

void Codegen::writeIR(const std::string &filename) {
  std::error_code EC;
  llvm::raw_fd_ostream out(filename, EC, llvm::sys::fs::OF_None);
  if (EC) {
    reportError("Could not open IR file: " + EC.message());
    return;
  }
  _module->print(out, nullptr);
}

void Codegen::compileToObject(const std::string &filename) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

    std::string triple = llvm::sys::getDefaultTargetTriple();
    llvm::Triple tripleObj(triple);

  #if !defined(LLVM_VERSION_MAJOR)
  #include "llvm/Config/llvm-config.h"
  #endif

  #if defined(LLVM_VERSION_MAJOR) && LLVM_VERSION_MAJOR >= 10
    _module->setTargetTriple(tripleObj);

    std::string err;
    const llvm::Target *targetObj =
        llvm::TargetRegistry::lookupTarget(tripleObj, err);
  #else
    _module->setTargetTriple(triple);

    std::string err;
    const llvm::Target *targetObj =
        llvm::TargetRegistry::lookupTarget(triple, err);
  #endif
  if (!targetObj) {
    reportError("Could not lookup target: " + err);
    return;
  }

  llvm::TargetOptions opt;
  llvm::Reloc::Model RM = llvm::Reloc::PIC_;
  llvm::CodeModel::Model CM = llvm::CodeModel::Small;
#if defined(LLVM_VERSION_MAJOR) && LLVM_VERSION_MAJOR >= 10
  llvm::CodeGenOptLevel OL = llvm::CodeGenOptLevel::Default;
#else
  llvm::CodeGenOpt::Level OL = llvm::CodeGenOpt::Default;
#endif

  #if defined(LLVM_VERSION_MAJOR) && LLVM_VERSION_MAJOR >= 10
    llvm::TargetMachine *TM = targetObj->createTargetMachine(
        tripleObj, "generic", "", opt,
        std::optional<llvm::Reloc::Model>(RM),
        std::optional<llvm::CodeModel::Model>(CM), OL);
  #else
    llvm::TargetMachine *TM = targetObj->createTargetMachine(
        triple, "generic", "", opt,
        std::optional<llvm::Reloc::Model>(RM),
        std::optional<llvm::CodeModel::Model>(CM), OL);
  #endif

  if (!TM) {
    reportError("Could not create target machine.");
    return;
  }

  _module->setDataLayout(TM->createDataLayout());

  std::error_code EC;
  llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
  if (EC) {
    reportError("Could not open object file: " + EC.message());
    return;
  }

  llvm::legacy::PassManager pm;
#if defined(LLVM_VERSION_MAJOR) && LLVM_VERSION_MAJOR >= 10
  llvm::CodeGenFileType ft = llvm::CodeGenFileType::ObjectFile;

  if (TM->addPassesToEmitFile(pm, dest, nullptr, ft)) {
#else
  // older LLVM used CGFT_ObjectFile
  if (TM->addPassesToEmitFile(pm, dest, nullptr, llvm::CGFT_ObjectFile)) {
#endif
    reportError("TargetMachine cannot emit object files.");
    return;
  }

  pm.run(*_module);
}