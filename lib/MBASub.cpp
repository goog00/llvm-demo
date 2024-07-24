//pass实现整数减指令替换为Mixed Boolean Arithmetic（MBA)
//即：a - b == (a + ~b) + 1

#include "MBASub.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <random>

using namespace llvm;

#define DEBUG_TYPE "mba-sub"

STATISTIC(SubstCount, "the # of substituted instructions");

// 实现

bool MBASub::runOnBasicBlock(BasicBlock &BB) {
    bool Changed = false;

    for(auto Inst = BB.begin(), IE = BB.end(); Inst != IE; ++Inst) {
         
         //跳过非二进制指令
        auto *BinOp = dyn_cast<BinaryOperator>(Inst);
        if(!BinOp)
          continue;

        unsigned Opcode = BinOp->getOpcode();
        if(Opcode != Instruction::Sub || !BinOp->getType()->isIntegerTy())
          continue;

        IRBuilder<> Builder(BinOp);

        //创建表示（a + ~b) + 1的指令
        Instruction *NewValue = BinaryOperator::CreateAdd(
            Builder.CreateAdd(BinOp->getOperand(0),
            Builder.CreateNot(BinOp->getOperand(1))),
            ConstantInt::get(BinOp->getType(),1)
        );  

        LLVM_DEBUG(dbgs() << *BinOp << " -> " << *NewValue << "\n");

        //用新指令 （a + ~b) + 1 替换 （a - b)
        ReplaceInstWithInst(&BB, Inst, NewValue);

        Changed = true;

        ++SubstCount;

        return Changed;

    }
}

PreservedAnalyses MBASub::run(llvm::Function &F, llvm::FunctionAnalysisManager &) {
    bool Changed = false;  // 初始化 Changed 变量为 false

    // 遍历函数中的每一个基本块（BasicBlock）
    for (auto &BB : F) {
        // 调用 runOnBasicBlock 并将结果与 Changed 进行按位或操作，并将结果赋值给 Changed
        Changed |= runOnBasicBlock(BB);
    }

    // 如果 Changed 为 true，则返回 llvm::PreservedAnalyses::none()，
    // 表示分析结果没有被保存，因为有变化。
    // 否则返回 llvm::PreservedAnalyses::all()，表示分析结果全部被保存。
    return (Changed ? llvm::PreservedAnalyses::none()
                    : llvm::PreservedAnalyses::all());
}


//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getMBASubPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "mba-sub", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "mba-sub") {
                    FPM.addPass(MBASub());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getMBASubPluginInfo();
}
