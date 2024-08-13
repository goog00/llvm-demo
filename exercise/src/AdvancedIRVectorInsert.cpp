//主要实现：
// 把数据插入向量中

//执行命令：
//clang++  AdvancedIRVectorInsert.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core` -fno-rtti  -o toy
// ./toy
// ; ModuleID = 'my_module'
// source_filename = "my_module"

// define i32 @foo(ptr %a) {
// entry:
//   %0 = insertelement ptr %a, i32 10, i32 0
//   %1 = insertelement ptr %a, i32 20, i32 1
//   %2 = insertelement ptr %a, i32 30, i32 2
//   %3 = insertelement ptr %a, i32 40, i32 3
//   ret i32 0
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
  Type *vecTy = FixedVectorType::get(u32Ty,4);
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

Value *getInsertElement(IRBuilder<> &Builder, Value *Vec, Value *Val,
                        Value *Index) {
  return Builder.CreateInsertElement(Vec, Val, Index);                    
}

Value *getExtractElement(IRBuilder<> &Builder, Value *Vec, Value *Index) {
  return Builder.CreateExtractElement(Vec, Index);
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
  
  //--- start InsertElement
  Value *Vec = fooFunc->arg_begin();
  for (unsigned int i = 0; i < 4; i++) {
    Value *V = getInsertElement(Builder, Vec, 
                                Builder.getInt32((i + 1)*10), 
                                Builder.getInt32(i));
  }

  Builder.CreateRet(Builder.getInt32(0));
  verifyFunction(*fooFunc);

  TheModule->print(llvm::outs(), nullptr);

  return 0;
}