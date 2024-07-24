//pass 实现8-bit整数加指令add 替换为Mixed Boolean-Airthmetic（MBA）
//即： a + b == (((a ^ b) + 2 * (a & b)) * 39 + 23) * 151 + 111

#include "MBAAdd.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <random>

using namespace llvm;

//DEBUG_TYPE：定义了调试类型，便于在调试信息中标识和筛选相关输出。
#define DEBUG_TYPE "mba-add"

// AddstCount：这是一个统计变量，用于统计被替换的指令数量。
STATISTIC(AddstCount,"The # of Addstituted instructions");

//实现

bool MBAAdd::runOnBasicBlock(BasicBlock &BB) {
    bool Changed = false;


    for(auto Inst = BB.begin(), IE = BB.end(); Inst != IE; ++Inst) {

        //尝试将当前指令 Inst 转换为二元运算符类型。如果转换成功，则 BinOp 不为空；否则，跳过该指令。
        auto *BinOp = dyn_cast<BinaryOperator>(Inst);
        if(!BinOp) 
          continue;

        if(BinOp->getOpcode() != Instruction::Add)
        continue;

        //检查指令的结果类型是否是 8 位宽的整数。如果不是，则跳过该指令。
        if(!BinOp->getType()->isIntegerTy() || 
            !(BinOp->getType()->getIntegerBitWidth() == 8))

           continue;

        IRBuilder<> Builder(BinOp);

        auto Val39 = ConstantInt::get(BinOp->getType(),39);
        auto Val151 = ConstantInt::get(BinOp->getType(),151);
        auto Val23 = ConstantInt::get(BinOp->getType(),23);
        auto Val2 = ConstantInt::get(BinOp->getType(),2);
        auto Val111  = ConstantInt::get(BinOp->getType(),111);

        //创建指令表示： (((a ^ b) + 2 * (a & b)) * 39 + 23) *  151 + 111
        //
        Instruction *NewInst = 
                //E = e5 + 111
                BinaryOperator::CreateAdd(
                    Val111,
                    //e5 = e4 * 151
                    Builder.CreateMul(
                        Val151,
                        //e4 = e2 + 23
                        Builder.CreateAdd(
                            Val23,
                           //e3 = e2 * 39
                            Builder.CreateMul(
                                Val39,
                                 //e2 = e0 + e1 
                                Builder.CreateAdd(
                                    //e0 = a^b
                                    Builder.CreateXor(BinOp->getOperand(0),
                                                      BinOp->getOperand(1)),
                                    //e1 = 2 * (a & b)
                                    Builder.CreateMul(Val2,Builder.CreateAnd(BinOp->getOperand(0),
                                                                             BinOp->getOperand(1)))                  
                                )
                            )
                        )
                    )
                );
        LLVM_DEBUG(dbgs() << *BinOp << " -> " << *NewInst << "\n");

        ReplaceInstWithInst(&BB, Inst, NewInst);
        Changed = true;

        ++AddstCount;
             
    }

    return Changed;
}

PreservedAnalyses MBAAdd::run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &) {

    bool Changed = false;
    for(auto &BB : F) {
        Changed |= runOnBasicBlock(BB);
    }   

    return (Changed ? llvm::PreservedAnalyses::none()
                    : llvm::PreservedAnalyses::all());                         

}


/-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getMBAAddPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "mba-add", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "mba-add") {
                    FPM.addPass(MBAAdd());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getMBAAddPluginInfo();
}
