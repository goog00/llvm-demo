
#include "InjectFuncCall.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

#define DEBUG_TYPE "inject-func-call"

//通过注入printf 来检验Module


//pass 实现

bool InjectFuncCall::runOnModule(Module &M) {
    bool InsertedAtLeastOnePrintf = false;

    auto &CTX = M.getContext();
    PointerType *PrintfArgTy =  PointerType::getUnqual(Type::getInt8Ty(CTX));

    //1.声明printf
    FunctionType *PrintTy = FunctionType::get(IntegerType::getInt32Ty(CTX),PrintfArgTy, true);

    FunctionCallee Printf = M.getOrInsertFunction("printf",PrintTy);

    Function *PrintF = dyn_cast<Function>(Printf.getCallee());
    PrintF->setDoesNotThrow();
    PrintF->addParamAttr(0,Attribute::NoCapture);
    PrintF->addParamAttr(0,Attribute::ReadOnly);

    //2.定义一个全局变量，让它表示打印的格式
    llvm::Constant *PrintfFormatStr = llvm::ConstantDataArray::getString(CTX,"(llvm-tutor) Hello from :%s\n(llvm-tutor) number of arguments: %d\n");

    Constant *PrintfFormatStrVar = M.getOrInsertGlobal("PrintfFormatStr",PrintfFormatStr->getType());

    dyn_cast<GlobalVariable>(PrintfFormatStrVar)->setInitializer(PrintfFormatStr);


    //3.遍历Module中函数，并插入printf调用
    for(auto &F : M) {
        if(F.isDeclaration()) {
            continue;
        }
        //构造Builder,在function的顶部插入点
        IRBuilder<> Builder(&*F.getEntryBlock().getFirstInsertionPt());

        auto FuncName = Builder.CreateGlobalStringPtr(F.getName());

        llvm::Value *FormatStrPtr = Builder.CreatePointerCast(PrintfFormatStrVar,PrintfArgTy,"formatStr");
         
        LLVM_DEBUG(dbgs() << " Injecting call to printf inside " << F.getName()
                      << "\n");

        Builder.CreateCall(Printf, {FormatStrPtr, FuncName, Builder.getInt32(F.arg_size())});

        InsertedAtLeastOnePrintf = true;

    }

    return InsertedAtLeastOnePrintf;


}


PreservedAnalyses InjectFuncCall::run(llvm::Module &M, llvm::ModuleAnalysisManager &) {
    bool Changed = runOnModule(M);

    return (Changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
}



//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getInjectFuncCallPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "inject-func-call", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "inject-func-call") {
                    MPM.addPass(InjectFuncCall());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getInjectFuncCallPluginInfo();
}
