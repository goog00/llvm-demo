
// 浮点数相等比较的重要性
// 浮点数相等比较操作（例如 ==）在浮点数运算中可能会导致问题，因为浮点数在计算机中通常无法精确表示。由于舍入误差，两个看似相等的浮点数实际上可能并不完全相等。
// 这可能导致比较结果与预期不符，进而导致程序逻辑错误。
// 通过查找所有直接比较浮点数相等的操作，开发人员可以识别潜在的错误或不稳定性，并改进代码以避免这些问题。例如，开发人员可以使用某种容差范围来比较浮点数，而不是直接使用相等比较。

// FindFCmpEq 过程查找函数中所有浮点数相等比较操作，并打印它们。这样做的目的是识别和解决由于浮点数运算中的舍入误差而可能导致的问题。了解并处理这些问题有助于提高代码的稳定性和正确性。

#include "FindFCmpEq.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

using namespace llvm;

static void printFCmpEqInstructions(raw_ostream &OS, Function &Func, 
                                    const FindFCmpEq::Result &FCmpEqInsts) noexcept {

    if(FCmpEqInsts.empty())
    return;

    OS << "Floating-point equality comparisons in \"" << Func.getName() 
    << "\":\n";

    ModuleSlotTracker Tracker(Func.getParent());

    for(FCmpInst *FCmpEq : FCmpEqInsts) {
        FCmpEq->print(OS,Tracker);
        OS << '\n';
    }

}

static constexpr char PassArg[] = "find-fcmp-eq";
static constexpr char PassName[] = "Floating-point equality comparisons locator";
static constexpr char PluginName[] = "FindCFmpEq";

FindFCmpEq::Result FindFCmpEq::run(Function &Func, FunctionAnalysisManager &FAM) {
    return run(Func);
}

FindFCmpEq::Result FindFCmpEq::run(Function &Func) {
    Result Comparisons;

    for(Instruction &Inst : instructions(Func)) {
        if(auto *FCmp = dyn_cast<FCmpInst>(&Inst)){
            if(FCmp->isEquality()){
                Comparisons.push_back(FCmp);
            }
        }
    }

    return Comparisons;
}


PreservedAnalyses FindFCmpEqPrinter::run(Function &Func, FunctionAnalysisManager &FAM) {
    
    auto &Comparisons = FAM.getResult<FindFCmpEq>(Func);
    printFCmpEqInstructions(OS, Func, Comparisons);

    return PreservedAnalyses::all();
}



//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::AnalysisKey FindFCmpEq::Key;

PassPluginLibraryInfo getFindFCmpEqPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, PluginName, LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // #1 REGISTRATION FOR "FAM.getResult<FindFCmpEq>(Function)"
            PB.registerAnalysisRegistrationCallback(
                [](FunctionAnalysisManager &FAM) {
                  FAM.registerPass([&] { return FindFCmpEq(); });
                });
            // #2 REGISTRATION FOR "opt -passes=print<find-fcmp-eq>"
            // Printing passes format their pipeline element argument to the
            // pattern `print<pass-name>`. This is the pattern we're checking
            // for here.
            PB.registerPipelineParsingCallback(
                [&](StringRef Name, FunctionPassManager &FPM,
                    ArrayRef<PassBuilder::PipelineElement>) {
                  std::string PrinterPassElement =
                      formatv("print<{0}>", PassArg);
                  if (Name.equals(PrinterPassElement)) {
                    FPM.addPass(FindFCmpEqPrinter(llvm::outs()));
                    return true;
                  }

                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getFindFCmpEqPluginInfo();
}
