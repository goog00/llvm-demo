
//主要实现：
//1.创建module， Function，函数的参数，返回值，basicblock,算术运算，全局变量，
// if-else ,loop
//执行脚本：
//clang++ -O3 CreateModule.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core` -o toy
// ./toy
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
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
  std::vector<Type *> Integers(FunArgs.size(), Type::getInt32Ty(Context));
  FunctionType *FuncType =
      FunctionType::get(Builder.getInt32Ty(), Integers, false);
  Function *FooFunc = Function::Create(
      FuncType, llvm::Function::ExternalLinkage, Name, TheModule.get());
  return FooFunc;
}

// 对函数设置参数
void setFuncArgs(Function *FooFunc, std::vector<std::string> FunArgs) {
  unsigned Idx = 0;
  Function::arg_iterator AI, AE;
  for (AI = FooFunc->arg_begin(), AE = FooFunc->arg_end(); AI != AE;
       ++AI, ++Idx) {
    AI->setName(FunArgs[Idx]);
  }
}

// 创建基本块
BasicBlock *createBB(Function *FooFunc, std::string Name) {
  return BasicBlock::Create(Context, Name, FooFunc);
}

// 创建全局变量
GlobalVariable *createGlob(IRBuilder<> &Builder, std::string Name) {
  TheModule->getOrInsertGlobal(Name, Builder.getInt32Ty());
  GlobalVariable *gVar = TheModule->getNamedGlobal(Name);
  gVar->setLinkage(GlobalValue::CommonLinkage);
  // 设置对齐方式
  gVar->setAlignment(Align(4));
  return gVar;
}

// 在block中，添加算术运算的指令
Value *createArith(IRBuilder<> &Builder, Value *L, Value *R) {
  return Builder.CreateMul(L, R, "multmp");
}

// 创建if-else的IR
Value *createIfElse(IRBuilder<> &Builder, BBList List, ValList VL) {
  Value *Condtn = VL[0];
  Value *Arg1 = VL[1];
  BasicBlock *ThenBB = List[0];
  BasicBlock *ElseBB = List[1];
  BasicBlock *MergeBB = List[2];
  Builder.CreateCondBr(Condtn, ThenBB, ElseBB);

  Builder.SetInsertPoint(ThenBB);
  Value *ThenVal = Builder.CreateAdd(Arg1, Builder.getInt32(1), "thenaddtmp");

  Builder.CreateBr(MergeBB);

  Builder.SetInsertPoint(ElseBB);
  Value *ElseVal = Builder.CreateAdd(Arg1, Builder.getInt32(2), "elseaddtmp");
  Builder.CreateBr(MergeBB);

  unsigned PhiBBSize = List.size() - 1;
  Builder.SetInsertPoint(MergeBB);
  PHINode *Phi =
      Builder.CreatePHI(Type::getInt32Ty(Context), PhiBBSize, "iftmp");
  Phi->addIncoming(ThenVal, ThenBB);
  Phi->addIncoming(ElseVal, ElseBB);

  return Phi;
}

// 创建loop的IR
Value *createLoop(IRBuilder<> &Builder, BBList List, ValList VL,
                  Value *StartVal, Value *EndVal) {
  BasicBlock *PreheaderBB = Builder.GetInsertBlock();
  Value *val = VL[0];
  BasicBlock *LoopBB = List[0];
  Builder.CreateBr(LoopBB);
  Builder.SetInsertPoint(LoopBB);
  PHINode *IndVar = Builder.CreatePHI(Type::getInt32Ty(Context), 2, "i");
  IndVar->addIncoming(StartVal, PreheaderBB);

  Value *Add = Builder.CreateAdd(val, Builder.getInt32(5), "addtmp");
  Value *StepVal = Builder.getInt32(1);
  Value *NextVal = Builder.CreateAdd(IndVar, StepVal, "nextval");
  Value *EndCond = Builder.CreateICmpULT(IndVar, EndVal, "endcond");
  EndCond = Builder.CreateICmpNE(EndCond, Builder.getInt1(false), "loopcond");
  BasicBlock *LoopEndBB = Builder.GetInsertBlock();
  BasicBlock *AfterBB = List[1];

  Builder.CreateCondBr(EndCond, LoopBB, AfterBB);
  Builder.SetInsertPoint(AfterBB);
  IndVar->addIncoming(NextVal, LoopEndBB);
  return Add;
}

int main() {

  FunArgs.push_back("a");
  FunArgs.push_back("b");

  llvm::IRBuilder<> Builder(Context);

  GlobalVariable *gVar = createGlob(Builder, "x");

  // 创建函数
  Function *FooFunc = createFunc(Builder, "foo");
  setFuncArgs(FooFunc, FunArgs);

  // 创建基本块
  llvm::BasicBlock *EntryBB = createBB(FooFunc, "entry");

  Builder.SetInsertPoint(EntryBB);

  // //----- start arith
  // Value *Arg1 = FooFunc->arg_begin();
  // Value *constant = Builder.getInt32(16);
  // Value *val = createArith(Builder, Arg1, constant);
  // //----- end arith

  // //----- start phi
  // Value *val2 = Builder.getInt32(100);

  // // 检查操作数类型是否一致
  // // llvm::errs() << "val type: ";
  // // val->getType()->print(llvm::errs());
  // // llvm::errs() << "\nval2 type: ";
  // // val2->getType()->print(llvm::errs() );
  // // llvm::errs() << "\n";

  // Value *Compare = Builder.CreateICmpULT(val, val2, "cmptmp");

  // // ICmpULT 需要两个相同类型的整数操作数，
  // // 如果 val 和 val2 的类型不一致，就会导致断言错误。
  // // ICmpNE 也需要两个相同类型的操作数，
  // // 所以要确保 Compare 和 Builder.getInt32(0) 的类型一致。
  // // 将 Compare 与布尔类型值比较，而不是 i32
  // Value *Condtn = Builder.CreateICmpNE(Compare, Builder.getInt1(false),
  // "ifcond");

  // ValList VL;
  // VL.push_back(Condtn);
  // VL.push_back(Arg1);

  // BasicBlock *ThenBB = createBB(FooFunc, "then");
  // BasicBlock *ElseBB = createBB(FooFunc, "else");
  // BasicBlock *MergeBB = createBB(FooFunc, "ifcont");

  // BBList List;
  // List.push_back(ThenBB);
  // List.push_back(ElseBB);
  // List.push_back(MergeBB);

  // Value *v = createIfElse(Builder, List, VL);

  // //----- end phi

  Function::arg_iterator AI = FooFunc->arg_begin();
  Value *Arg1 = AI++;
  Value *Arg2 = AI;
  Value *constant = Builder.getInt32(16);

  Value *val = createArith(Builder, Arg1, constant);
  ValList VL;
  VL.push_back(Arg1);

  BBList List;
  BasicBlock *LoopBB = createBB(FooFunc, "loop");
  BasicBlock *AfterBB = createBB(FooFunc, "afterloop");
  List.push_back(LoopBB);
  List.push_back(AfterBB);

  Value *StartVal = Builder.getInt32(1);
  Value *Res = createLoop(Builder, List, VL, StartVal, Arg2);

  Builder.CreateRet(Res);

  TheModule->print(llvm::outs(), nullptr);

  return 0;
}