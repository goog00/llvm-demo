#ifndef LLVM_TUTOR_DUPLICATE_BB_H
#define LLVM_TUTOR_DUPLICATE_BB_H

#include "RIV.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Pass.h"


#include <map>
#include <memory>

namespace llvm {
    class RandomNumberGenerator;
}

struct DuplicateBB : public llvm::PassInfoMixin<DuplicateBB> {
    llvm::PreservedAnalyses run(llvm::Function &F,
                                llvm::FunctionAnalysisManager &);


    using BBToSingleRIVMap = 
            std::vector<std::tuple<llvm::BasicBlock *,llvm::Value * >>;

    using ValueToPhiMap = std::map<llvm::Value *, llvm::Value *>;

    BBToSingleRIVMap findBBSToDuplicate(llvm::Function &F, 
                                        const RIV::Result &RIVResult);

    void cloneBB(llvm::BasicBlock &BB, llvm::Value *ContextValue, ValueToPhiMap &ReMapper);


    unsigned DuplicateBBCount = 0;

    static bool isRequired() { return true;}

    std::unique_ptr<llvm::RandomNumberGenerator> pRNG;
};



#endif