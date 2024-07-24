#ifndef LLVM_TUTOR_STATICCALLCOUNTER_H
#define LLVM_TUTOR_STATICCALLCOUNTER_H

#include "llvm/ADT/MapVector.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

//使用using创建类型别名
//这里的 unsigned 类型通常用于表示计数、索引或标志等无符号整数值。
using ResultStaticCC = llvm::MapVector<const llvm::Function *, unsigned>;

struct StaticCallCounter : public llvm::AnalysisInfoMixin<StaticCallCounter> {
    using Result = ResultStaticCC;
    Result run(llvm::Module &M, llvm::ModuleAnalysisManager &);
    Result runOnModule(llvm::Module &M);

    static bool isRequired() { return true;}

private:
    static llvm::AnalysisKey Key;
    friend struct llvm::AnalysisInfoMixin<StaticCallCounter>;

};


class StaticCallCounterPrinter
    : public llvm::PassInfoMixin<StaticCallCounterPrinter> {
public:
  explicit StaticCallCounterPrinter(llvm::raw_ostream &OutS) : OS(OutS) {}
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
  // Part of the official API:
  //  https://llvm.org/docs/WritingAnLLVMNewPMPass.html#required-passes
  static bool isRequired() { return true; }

private:
  llvm::raw_ostream &OS;
};


#endif