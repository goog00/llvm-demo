//主要实现：
//从向量中提取数据

//执行命令：
//clang++  AdvancedIRVectorExtract.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core` -fno-rtti  -o toy
// ./toy
// ; ModuleID = 'my_module'
// source_filename = "my_module"

// define i32 @foo(<4 x i32> %a) {
// entry:
//   %0 = extractelement <4 x i32> %a, i32 0
//   %1 = extractelement <4 x i32> %a, i32 1
//   %2 = extractelement <4 x i32> %a, i32 2
//   %3 = extractelement <4 x i32> %a, i32 3
//   %multmp = mul i32 %0, %1
//   %multmp1 = mul i32 %multmp, %2
//   %multmp2 = mul i32 %multmp1, %3
//   ret i32 %multmp2
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
  // Type *ptrTy = vecTy->getPointerTo(0);

  FunctionType *funcType =
      FunctionType::get(Builder.getInt32Ty(), vecTy, false);

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
  


  //--- start extractelement
  Value *Vec = fooFunc->arg_begin();
  SmallVector<Value *, 4> V(4);
  for (unsigned int i = 0; i < 4; i++){
    V[i] = getExtractElement(Builder, Vec, Builder.getInt32(i));
  }
  Value *add1 = createArith(Builder, V[0], V[1]);
  Value *add2 = createArith(Builder, add1, V[2]);
  Value *add = createArith(Builder, add2, V[3]);
  //--- end extractelement
  
  Builder.CreateRet(add);
  verifyFunction(*fooFunc);

  TheModule->print(llvm::outs(), nullptr);

  return 0;
}