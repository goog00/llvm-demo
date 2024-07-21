#include "OpcodeCounter.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

// 定义两个pass: OpcodeCounter 和OpcodeCounterPrinter
// OpcodeCounter：统计llvm IR opcode 的个数
// 和OpcodeCounterPrinter：因为OpcodeCounter是分析Pass，不改变原始llvm IR ,但是我们又想打印输出分析结果，
// 所以，同时定义打印pass: OpcodeCounterPrinter

//打印分析的结果
static void printOpcodeCounterResult(llvm::raw_ostream &, const ResultOpcodeCounter &OC);


//pass 具体实现

llvm::AnalysisKey OpcodeCounter::Key;

OpcodeCounter::Result OpcodeCounter::generateOpcodeMap(llvm::Function &Func) {
    OpcodeCounter::Result OpcodeMap;

    for(auto &BB : Func) {
        for(auto &Inst : BB) {
            StringRef Name = Inst.getOpcodeName();

            if (OpcodeMap.find(Name) == OpcodeMap.end())
            {
               OpcodeMap[Inst.getOpcodeName()] =1;
            } else {
                OpcodeMap[Inst.getOpcodeName()]++;
            }
            
        }
    }

    return OpcodeMap;
}

OpcodeCounter::Result OpcodeCounter::run(llvm::Function &Func,
                                         llvm::FunctionAnalysisManager &) {
  return generateOpcodeMap(Func);
}

PreservedAnalyses OpcodeCounterPrinter::run(Function &Func,
                                            FunctionAnalysisManager &FAM) {

        auto &OpcodeMap = FAM.getResult<OpcodeCounter>(Func);

        OS << "Printing analysis 'OpcodeCounter Pass' for function '" << Func.getName() << "':\n";

       printOpcodeCounterResult(OS,OpcodeMap);

       return PreservedAnalyses::all(); 

}



//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getOpcodeCounterPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "OpcodeCounter", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
          // #1 REGISTRATION FOR "opt -passes=print<opcode-counter>"
          // Register OpcodeCounterPrinter so that it can be used when
          // specifying pass pipelines with `-passes=`.
          PB.registerPipelineParsingCallback(
              [&](StringRef Name, FunctionPassManager &FPM,
                  ArrayRef<PassBuilder::PipelineElement>) {
                if (Name == "print<opcode-counter>") {
                  FPM.addPass(OpcodeCounterPrinter(llvm::errs()));
                  return true;
                }
                return false;
              });
          // #2 REGISTRATION FOR "-O{1|2|3|s}"
          // Register OpcodeCounterPrinter as a step of an existing pipeline.
          // The insertion point is specified by using the
          // 'registerVectorizerStartEPCallback' callback. To be more precise,
          // using this callback means that OpcodeCounterPrinter will be called
          // whenever the vectoriser is used (i.e. when using '-O{1|2|3|s}'.
          PB.registerVectorizerStartEPCallback(
              [](llvm::FunctionPassManager &PM,
                 llvm::OptimizationLevel Level) {
                PM.addPass(OpcodeCounterPrinter(llvm::errs()));
              });
          // #3 REGISTRATION FOR "FAM.getResult<OpcodeCounter>(Func)"
          // Register OpcodeCounter as an analysis pass. This is required so that
          // OpcodeCounterPrinter (or any other pass) can request the results
          // of OpcodeCounter.
          PB.registerAnalysisRegistrationCallback(
              [](FunctionAnalysisManager &FAM) {
                FAM.registerPass([&] { return OpcodeCounter(); });
              });
          }
        };
}


extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getOpcodeCounterPluginInfo();
}


//------------------------------------------------------------------------------
// Helper functions - implementation
//------------------------------------------------------------------------------
static void printOpcodeCounterResult(raw_ostream &OutS,
                                     const ResultOpcodeCounter &OpcodeMap) {
  OutS << "================================================="
               << "\n";
  OutS << "LLVM-TUTOR: OpcodeCounter results\n";
  OutS << "=================================================\n";
  const char *str1 = "OPCODE";
  const char *str2 = "#TIMES USED";
  OutS << format("%-20s %-10s\n", str1, str2);
  OutS << "-------------------------------------------------"
               << "\n";
  for (auto &Inst : OpcodeMap) {
    OutS << format("%-20s %-10lu\n", Inst.first().str().c_str(),
                           Inst.second);
  }
  OutS << "-------------------------------------------------"
               << "\n\n";
}
