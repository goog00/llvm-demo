
// RIV 是一个分析过程，对于输入函数中的每个基本块 BB 计算集合可达整数值，即 BB 中可见（即可使用）的整数值


// #### RIV 分析过程

// 1. **定义与初始化**：
//    - `DefValMapTy DefinedValuesMap`：用于存储每个基本块定义的整数值。
//    - `ResultMap`：用于存储每个基本块可达的整数值。

// 2. **计算每个基本块定义的整数值**：
//    - 遍历函数的每个基本块，并找到所有整数类型的指令。

// 3. **计算入口基本块的RIVs**：
//    - 入口基本块的RIVs包括全局变量和输入参数。

// 4. **遍历控制流图（CFG）**：
//    - 使用广度优先搜索遍历控制流图，从根节点开始。
//    - 对每个基本块，将其父基本块的定义值和RIVs加入到它的RIVs中。

// ### 输出结果解析

// 根据给定的`.ll`文件内容和实现的RIV分析，输出结果如下：

// #### BB %3

// - 该基本块是入口基本块。
// - 可达的整数值：
//   - `i32 %0`（函数参数）
//   - `i32 %1`（函数参数）
//   - `i32 %2`（函数参数）

// #### BB %6

// - 从基本块%3跳转到%6。
// - 可达的整数值：
//   - `%4 = add nsw i32 %0, 123`
//   - `%5 = icmp sgt i32 %0, 0`
//   - `i32 %0`（函数参数）
//   - `i32 %1`（函数参数）
//   - `i32 %2`（函数参数）

// #### BB %17

// - 从基本块%3、%10和%14跳转到%17。
// - 可达的整数值：
//   - `%4 = add nsw i32 %0, 123`
//   - `%5 = icmp sgt i32 %0, 0`
//   - `i32 %0`（函数参数）
//   - `i32 %1`（函数参数）
//   - `i32 %2`（函数参数）

// #### BB %10

// - 从基本块%6跳转到%10。
// - 可达的整数值：
//   - `%7 = mul nsw i32 %1, %0`
//   - `%8 = sdiv i32 %1, %2`
//   - `%9 = icmp eq i32 %7, %8`
//   - `%4 = add nsw i32 %0, 123`
//   - `%5 = icmp sgt i32 %0, 0`
//   - `i32 %0`（函数参数）
//   - `i32 %1`（函数参数）
//   - `i32 %2`（函数参数）

// #### BB %14

// - 从基本块%6跳转到%14。
// - 可达的整数值：
//   - `%7 = mul nsw i32 %1, %0`
//   - `%8 = sdiv i32 %1, %2`
//   - `%9 = icmp eq i32 %7, %8`
//   - `%4 = add nsw i32 %0, 123`
//   - `%5 = icmp sgt i32 %0, 0`
//   - `i32 %0`（函数参数）
//   - `i32 %1`（函数参数）
//   - `i32 %2`（函数参数）

// ### 总结

// 从输出结果可以看出：
// - 每个基本块中的可达整数值包括其父基本块中定义的所有整数值。
// - 入口基本块包含全局变量和输入参数。
// - 对于其他基本块，可达的整数值包括：
//   - 父基本块中定义的整数值。
//   - 该基本块中定义的整数值。

// 这些信息对于理解控制流图中数据流动的分析非常重要，可以帮助进一步进行优化和改进。例如，了解哪些整数值在每个基本块中是可达的，可以用于数据依赖分析、代码优化以及并行化等。


#include "RIV.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Format.h"

#include <deque>

using namespace llvm;

using NodeTy = DomTreeNodeBase<llvm::BasicBlock> *;

using DefValMapTy = RIV::Result;

static void printRIVResult(llvm::raw_ostream &OutS, const RIV::Result &RIVMap);

RIV::Result RIV::buildRIV(Function &F,NodeTy CFGRoot) {
    Result ResultMap;

    std::deque<NodeTy> BBsToProcess;

    BBsToProcess.push_back(CFGRoot);


    //step1：为每一个基本块计算一组定义的integer值
    DefValMapTy DefinedValuesMap;
    for(BasicBlock &BB : F) {
        auto &Values = DefinedValuesMap[&BB];
        for(Instruction &Inst : BB){
            if(Inst.getType()->isIntegerTy())
               Values.insert(&Inst);
        }
    }

    //step2: 为Entry BB 计算rivs. 这将包含全局变量和输入参数。
    auto &EntryBBValues = ResultMap[&F.getEntryBlock()];

    for(auto &Global : F.getParent()->globals())
       if(Global.getValueType()->isIntegerTy())
          EntryBBValues.insert(&Global);

    for(Argument &Arg : F.args()) 
       if (Arg.getType()->isIntegerTy())
       {
          EntryBBValues.insert(&Arg);
       }

    //step3：遍历CFG，对其中的每个BB计算RIVs
    while (!BBsToProcess.empty()){
      auto *Parent = BBsToProcess.back();
      BBsToProcess.pop_back();


      auto &ParentDefs = DefinedValuesMap[Parent->getBlock()];

      llvm::SmallPtrSet<llvm::Value *,8> ParentRIVs = ResultMap[Parent->getBlock()];

      for(NodeTy Child : *Parent) {
        BBsToProcess.push_back(Child);
        auto ChildBB = Child->getBlock();

        ResultMap[ChildBB].insert(ParentDefs.begin(),ParentDefs.end());

        ResultMap[ChildBB].insert(ParentRIVs.begin(),ParentRIVs.end());

      }

    }

    return ResultMap;

}


//RIV pass
RIV::Result RIV::run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM) {
    DominatorTree *DT = &FAM.getResult<DominatorTreeAnalysis>(F);
    Result Res = buildRIV(F, DT->getRootNode());

    return Res;
}


//打印pass 
PreservedAnalyses RIVPrinter::run(Function &Func, FunctionAnalysisManager &FAM) {
    
    auto RIVMap = FAM.getResult<RIV>(Func);

    printRIVResult(OS,RIVMap);

    return PreservedAnalyses::all();
}


//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
AnalysisKey RIV::Key;

llvm::PassPluginLibraryInfo getRIVPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "riv", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // #1 REGISTRATION FOR "opt -passes=print<riv>"
            PB.registerPipelineParsingCallback(
                [&](StringRef Name, FunctionPassManager &FPM,
                    ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "print<riv>") {
                    FPM.addPass(RIVPrinter(llvm::errs()));
                    return true;
                  }
                  return false;
                });
            // #2 REGISTRATION FOR "FAM.getResult<RIV>(Function)"
            PB.registerAnalysisRegistrationCallback(
                [](FunctionAnalysisManager &FAM) {
                  FAM.registerPass([&] { return RIV(); });
                });
          }};
};

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getRIVPluginInfo();
}

 

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------
static void printRIVResult(raw_ostream &OutS, const RIV::Result &RIVMap) {
  OutS << "=================================================\n";
  OutS << "LLVM-TUTOR: RIV analysis results\n";
  OutS << "=================================================\n";

  const char *Str1 = "BB id";
  const char *Str2 = "Reachable Ineger Values";
  OutS << format("%-10s %-30s\n", Str1, Str2);
  OutS << "-------------------------------------------------\n";

  const char *EmptyStr = "";

  for (auto const &KV : RIVMap) {
    std::string DummyStr;
    raw_string_ostream BBIdStream(DummyStr);
    KV.first->printAsOperand(BBIdStream, false);
    OutS << format("BB %-12s %-30s\n", BBIdStream.str().c_str(), EmptyStr);
    for (auto const *IntegerValue : KV.second) {
      std::string DummyStr;
      raw_string_ostream InstrStr(DummyStr);
      IntegerValue->print(InstrStr);
      OutS << format("%-12s %-30s\n", EmptyStr, InstrStr.str().c_str());
    }
  }

  OutS << "\n\n";
}
