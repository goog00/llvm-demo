#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;

// 1.生成基本块：
//创建了三个基本块：条件判断块（CondBB）、循环体块（LoopBB）和循环后的块（AfterBB）。
// 2.插入phi指令：
//在条件判断块中插入了一个phi指令，初始化时只有一个前驱块，即EntryBB。
// 3.生成条件判断和循环体：
//在条件判断块中，生成了判断条件，并根据条件跳转到循环体或循环后的基本块。在循环体中，生成了变量的更新操作，并跳转回条件判断块。
// 4.更新phi指令：
//在生成循环体块之后，更新phi指令，添加来自循环体块的前驱信息。

int  main() {

    LLVMContext Context;
    Module *TheModule = new Module("while_loop",Context);
    IRBuilder<> Builder(Context);

    //创建函数
    FunctionType *FuncType = FunctionType::get(Type::getVoidTy(Context),false);
    Function *TheFunction = Function::Create(FuncType,Function::ExternalLinkage,"main",TheModule);

    //创建入口基本块
    BasicBlock *EntryBB = BasicBlock::Create(Context,"entry",TheFunction);
    Builder.SetInsertPoint(EntryBB);

    //创建 i 变量并初始化为0
    AllocaInst *Alloca = Builder.CreateAlloca(Type::getInt32Ty(Context),nullptr,"i");
    Builder.CreateStore(ConstantInt::get(Type::getInt32Ty(Context),0),Alloca);

    //创建基本块
    BasicBlock *CondBB = BasicBlock::Create(Context, "cond", TheFunction);
    BasicBlock *LoopBB = BasicBlock::Create(Context,"loop",TheFunction);
    BasicBlock *AfterBB = BasicBlock::Create(Context,"afterloop",TheFunction);


    //进入条件判断基本块
    Builder.CreateBr(CondBB);
    Builder.SetInsertPoint(CondBB);
    
    //插入一个空的phi指令
    PHINode *Phi = Builder.CreatePHI(Type::getInt32Ty(Context),2,"phi_i");
    
    //加载i的值
    LoadInst *LoadI = Builder.CreateLoad(Type::getInt32Ty(Context),Alloca,"i");
    Phi->addIncoming(LoadI,EntryBB);


    //生成i < 10的判断条件
    Value *Cond = Builder.CreateICmpSLT(Phi, ConstantInt::get(Type::getInt32Ty(Context),10),"cond");

    //根据判断结果跳转到LoopBB 或AfterBB
    Builder.SetInsertPoint(LoopBB);

    //加载phi 的值
    LoadInst *LoadI2 = Builder.CreateLoad(Type::getInt32Ty(Context),Alloca,"i");

    //生成i+1的值
    Value *Inc = Builder.CreateAdd(Phi,ConstantInt::get(Type::getInt32Ty(Context),1),"inc");

    //存储新的i值
    Builder.CreateStore(Inc,Alloca);


    // 调回基本块进行下一次判断
    Builder.CreateBr(CondBB);

    //更新phi指令的另一个输入
    Phi->addIncoming(Inc,LoopBB);

    //在循环后的基本块中添加后续代码
    Builder.SetInsertPoint(AfterBB);
    Builder.CreateRetVoid();

    //验证生成的代码
    verifyFunction(*TheFunction);
    TheModule->print(errs(),nullptr);

    delete TheModule;
    return 0;




}