//主要实现：
//1.getelementptr ：创建getelementptr，通过getelementptr获取地址
//2.load: 创建load， load 指令利用getelementptr获取的地址从内存中加载数据
//3.store: 创建store, store 指令把数据保存在getelementptr获取的地址中

//执行命令：
//clang++  AdvancedIROp.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core` -fno-rtti  -o toy
// ./toy
// ; ModuleID = 'my_module'
// source_filename = "my_module"

// define i32 @foo(ptr %a) {
// entry:
//   %a1 = getelementptr i32, ptr %a, i32 1
//   %load = load ptr, ptr %a1, align 8
//   ret ptr %load
// }

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>

using namespace llvm;

static LLVMContext Context;
static std::unique_ptr<llvm::Module> TheModule =
    std::make_unique<llvm::Module>("my_module", Context);

static std::vector<std::string> FunArgs;
typedef SmallVector<BasicBlock *, 16> BBList;
typedef SmallVector<Value *, 16> ValList;

// 创建函数
Function *createFunc(IRBuilder<> &Builder, std::string Name) {
  Type *u32Ty = Type::getInt32Ty(Context);
  Type *vecTy = FixedVectorType::get(u32Ty,2);
  Type *ptrTy = vecTy->getPointerTo(0);

  FunctionType *funcType =
      FunctionType::get(Builder.getInt32Ty(), ptrTy, false);

  Function *fooFunc = Function::Create(funcType, Function::ExternalLinkage,
                                       Name, TheModule.get());

  return fooFunc;
}

void setFuncArgs(Function *fooFunc, std::vector<std::string> FunArgs) {
  unsigned Idx = 0;
  Function::arg_iterator AI, AE;
  for (AI = fooFunc->arg_begin(), AE = fooFunc->arg_end(); AI != AE;
       ++AI, ++Idx)
    AI->setName(FunArgs[Idx]);
}

BasicBlock *createBB(Function *fooFunc, std::string Name) {
  return BasicBlock::Create(Context, Name, fooFunc);
}

// 在block中，添加算术运算的指令
Value *createArith(IRBuilder<> &Builder, Value *L, Value *R) {
  return Builder.CreateMul(L, R, "multmp");
}

// 获取地址
Value *getGEP(IRBuilder<> &Builder, Value *Base, Value *Offset) {
  return Builder.CreateGEP(Builder.getInt32Ty(), Base, Offset, "a1");
}

// 创建 load 指令
Value *getLoad(IRBuilder<> &Builder, Value *Address) {
    Type *loadType = Address->getType();
    return Builder.CreateLoad(loadType, Address, "load");
}


// build store ir
void getStore(IRBuilder<> &Builder, Value *Address, Value *V) {
    Builder.CreateStore(V, Address);
}

int main() {

  FunArgs.push_back("a");

  IRBuilder<> Builder(Context);

  // 创建函数
  Function *fooFunc = createFunc(Builder, "foo");
  setFuncArgs(fooFunc, FunArgs);

  Value *Base = fooFunc->arg_begin();

  // 创建基本块
  BasicBlock *entry = createBB(fooFunc, "entry");
  Builder.SetInsertPoint(entry);

  Value *gep = getGEP(Builder, Base, Builder.getInt32(1));
  Value *load = getLoad(Builder, gep);
  Value *constant = Builder.getInt32(16);
  Value *val = createArith(Builder, load, constant);
  getStore(Builder, gep, val);
  Builder.CreateRet(load);
  verifyFunction(*fooFunc);

  TheModule->print(llvm::outs(), nullptr);

  return 0;
}