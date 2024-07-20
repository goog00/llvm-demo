#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"

using namespace llvm;

// 实现内存数据对齐

int main() {

    LLVMContext Context;
    Module* M = new Module("my_module",Context);
    IRBuilder<> Builder(Context);

    //创建一个函数
    FunctionType *FuncType = FunctionType::get(Builder.getVoidTy(),false);
    Function* Func = Function::Create(FuncType, Function::ExternalLinkage,"my_function",M);

    //创建一个基本块
    BasicBlock* BB = BasicBlock::Create(Context,"entry",Func);
    Builder.SetInsertPoint(BB);

    //分配对齐的内存
    AllocaInst *Alloca = Builder.CreateAlloca(Builder.getInt32Ty(), nullptr, "aligned_var");
    Alloca->setAlignment(Align(16));

    //加载对齐的内存
    LoadInst* Load = Builder.CreateLoad(Type::getInt32Ty(Context),Alloca,"load");
    Load->setAlignment(Align(16));

    // 存储对齐的内存
    StoreInst* Store = Builder.CreateStore(Load, Alloca);
    Store->setAlignment(Align(16));

    //返回
    Builder.CreateRetVoid();

    //打印生成的LLVM IR
    M->print(llvm::errs(),nullptr);

    delete M;

    return 0;


}