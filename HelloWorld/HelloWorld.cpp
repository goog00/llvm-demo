#include "llvm/ADT/StringMap.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace
{

    // Pass 具体实现
    void visitor(Function &F)
    {
        errs() << "(llvm-demo) Hello from: " << F.getName() << "\n";
        errs() << "(llvm-demo) number of arguments: " << F.arg_size() << "\n";
    }

    struct HelloWorld : PassInfoMixin<HelloWorld>
    {

        PreservedAnalyses run(Function &F, FunctionAnalysisManager &)
        {
            visitor(F);
            return PreservedAnalyses::all();
        }

        static bool isRequired() { return true; }
    };

} // namespace

// registration

llvm::PassPluginLibraryInfo getHelloWorldPluginInfo()
{
    return {LLVM_PLUGIN_API_VERSION, "HelloWorld", LLVM_VERSION_STRING,
            [](PassBuilder &PB)
            {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>)
                    {
                        if (Name == "hello-world")
                        {
                            FPM.addPass(HelloWorld());
                            return true;
                        }
                        return false;
                    });
            }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
    return getHelloWorldPluginInfo();
}