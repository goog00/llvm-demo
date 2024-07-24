//统计输入的模块中的静态函数访问。静态引用

#include "StaticCallCounter.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"




using namespace llvm;

static void printStaticCCResult(llvm::raw_ostream &OutS, const ResultStaticCC &DirectCalls);


StaticCallCounter::Result StaticCallCounter::runOnModule(Module &M) {

    llvm::MapVector<const llvm::Function*, unsigned> Res;

    for(auto &Func : M) {
        for(auto &BB : Func) {
            for(auto &Ins : BB){
                
                //如果是调用指令 CB 不是null
                auto *CB = dyn_cast<CallBase>(&Ins);
                if(nullptr == CB){
                    continue;
                }

                //返回指向被调用函数的指针，如果调用是间接的或者函数签名不匹配，则返回 nullptr
                auto DirectInvoc = CB->getCalledFunction();
                if(nullptr == DirectInvoc){
                    continue;
                }


                auto CallCount = Res.find(DirectInvoc);
                if(Res.end() == CallCount){
                    CallCount = Res.insert(std::make_pair(DirectInvoc,0)).first;
                }

                ++CallCount->second;
            }
        }
    }

    return Res;
}


PreservedAnalyses
StaticCallCounterPrinter::run(Module &M,
                              ModuleAnalysisManager &MAM) {

  auto DirectCalls = MAM.getResult<StaticCallCounter>(M);

  printStaticCCResult(OS, DirectCalls);
  return PreservedAnalyses::all();
}

StaticCallCounter::Result
StaticCallCounter::run(llvm::Module &M, llvm::ModuleAnalysisManager &) {
  return runOnModule(M);
}

//------------------------------------------------------------------------------
// New PM Registration
//------------------------------------------------------------------------------
AnalysisKey StaticCallCounter::Key;

llvm::PassPluginLibraryInfo getStaticCallCounterPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "static-cc", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // #1 REGISTRATION FOR "opt -passes=print<static-cc>"
            PB.registerPipelineParsingCallback(
                [&](StringRef Name, ModulePassManager &MPM,
                    ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "print<static-cc>") {
                    MPM.addPass(StaticCallCounterPrinter(llvm::errs()));
                    return true;
                  }
                  return false;
                });
            // #2 REGISTRATION FOR "MAM.getResult<StaticCallCounter>(Module)"
            PB.registerAnalysisRegistrationCallback(
                [](ModuleAnalysisManager &MAM) {
                  MAM.registerPass([&] { return StaticCallCounter(); });
                });
          }};
};

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getStaticCallCounterPluginInfo();
}

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------
static void printStaticCCResult(raw_ostream &OutS,
                                const ResultStaticCC &DirectCalls) {
  OutS << "================================================="
       << "\n";
  OutS << "LLVM-TUTOR: static analysis results\n";
  OutS << "=================================================\n";
  const char *str1 = "NAME";
  const char *str2 = "#N DIRECT CALLS";
  OutS << format("%-20s %-10s\n", str1, str2);
  OutS << "-------------------------------------------------"
       << "\n";

  for (auto &CallCount : DirectCalls) {
    OutS << format("%-20s %-10lu\n", CallCount.first->getName().str().c_str(),
                   CallCount.second);
  }

  OutS << "-------------------------------------------------"
       << "\n\n";
}
