#ifndef LLVM_TUTOR_INSTRUMENT_BASIC_H
#define LLVM_TUTOR_INSTRUMENT_BASIC_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

struct DynamicCallCounter : public llvm::PassInfoMixin<DynamicCallCounter> {
    llvm::PreservedAnalyses run(llvm::Module &M,llvm::ModuleAnalysisManager &);

    bool runOnModule(llvm::Module &M);

    static bool isRequired() { return true; }    
};

#endif