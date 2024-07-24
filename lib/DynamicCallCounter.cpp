//动态的统计模块中函数调用次数

//如何查看经过pass处理后的IR？
//1.运行 opt 命令并保存输出
//  $LLVM_DIR/bin/opt -load-pass-plugin=libDynamicCallCounter.dylib -passes="dynamic-cc" input_for_cc.bc -o instrumented.bc
//2.使用 llvm-dis 将 bitcode 转换为 LLVM IR 汇编代码
//  $LLVM_DIR/bin/llvm-dis instrumented.bc -o instrumented.ll
//如下：
//  @CounterFor_foo = common global i32 0, align 4
//  @0 = private unnamed_addr constant [4 x i8] c"foo\00", align 1
//  @CounterFor_bar = common global i32 0, align 4
//  @1 = private unnamed_addr constant [4 x i8] c"bar\00", align 1
//  @CounterFor_fez = common global i32 0, align 4
//  @2 = private unnamed_addr constant [4 x i8] c"fez\00", align 1
//  @CounterFor_main = common global i32 0, align 4
//  @3 = private unnamed_addr constant [5 x i8] c"main\00", align 1
//  @ResultFormatStrIR = global [14 x i8] c"%-20s %-10lu\0A\00"
//  @ResultHeaderStrIR = global [160 x i8] c"============================\0ALLVM-TUTOR: dynamic analysis results\0A=============================\0AName           #N DIRECT CLALS\0A-------------------------------\0A\00"
//  @llvm.global_dtors = appending global [1 x { i32, ptr, ptr }] [{ i32, ptr, ptr } { i32 0, ptr @printf_wrapper, ptr null }]

// ; Function Attrs: noinline nounwind optnone ssp uwtable
//  define void @foo() #0 {
//       %1 = load i32, ptr @CounterFor_foo, align 4
//      %2 = add i32 1, %1
//      store i32 %2, ptr @CounterFor_foo, align 4
//       ret void
// }
//完整的IR在最下面

#include "DynamicCallCounter.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"


using namespace llvm;

#define DEBUG_TYPE "dynamic-cc"

Constant *CreateGlobalCounter(Module &M,StringRef GlobalVarName) {
    auto &CTX = M.getContext();
    

    Constant *NewGlobalVar = M.getOrInsertGlobal(GlobalVarName,IntegerType::getInt32Ty(CTX));

    GlobalVariable *NewGV = M.getNamedGlobal(GlobalVarName);

    NewGV->setLinkage(GlobalValue::CommonLinkage);
    NewGV->setAlignment(MaybeAlign(4));
    NewGV->setInitializer(llvm::ConstantInt::get(CTX,APInt(32,0)));

    return NewGlobalVar;
}



bool DynamicCallCounter::runOnModule(Module &M) {
    bool Instructmented = false;

    llvm::StringMap<Constant *> CallCounterMap;
    llvm::StringMap<Constant *> FuncNameMap;

    auto &CTX = M.getContext();

    // step1: 遍历Module的每一个函数，插入调用次数的代码
    for(auto &F : M) {
        if(F.isDeclaration())
          continue;

        IRBuilder<> Builder(&*F.getEntryBlock().getFirstInsertionPt());

        std::string CounterName = "CounterFor_" + std::string(F.getName());
        Constant *Var = CreateGlobalCounter(M, CounterName);
        CallCounterMap[F.getName()] = Var;

        auto FuncName = Builder.CreateGlobalStringPtr(F.getName());
        FuncNameMap[F.getName()] = FuncName;

        //插入
        LoadInst *Load2 = Builder.CreateLoad(IntegerType::getInt32Ty(CTX),Var);
        Value *Inc2 = Builder.CreateAdd(Builder.getInt32(1),Load2);
        Builder.CreateStore(Inc2,Var);

        LLVM_DEBUG(dbgs() << " Instrumentsed : " << F.getName() << "\n");

        Instructmented = true;


    }


    if(false == Instructmented) {
        return Instructmented;
    }


    //step2: 插入printf 声明
    PointerType *PrintfArgTy = PointerType::getUnqual(Type::getInt8Ty(CTX));
    FunctionType *PrintfTy = FunctionType::get(IntegerType::getInt32Ty(CTX),PrintfArgTy,true);
    
    FunctionCallee Printf = M.getOrInsertFunction("printf",PrintfTy);

    Function *PrintfF = dyn_cast<Function>(Printf.getCallee());
    PrintfF->setDoesNotThrow();
    PrintfF->addParamAttr(0,Attribute::NoCapture);
    PrintfF->addParamAttr(0,Attribute::ReadOnly);


    //step3: 插入全局变量
    llvm::Constant *ResultFormatStr = llvm::ConstantDataArray::getString(CTX,"%-20s %-10lu\n");

    Constant *ResultFormatStrVar = M.getOrInsertGlobal("ResultFormatStrIR",ResultFormatStr->getType());

    dyn_cast<GlobalVariable>(ResultFormatStrVar)->setInitializer(ResultFormatStr);

    std::string out = "";

    out += "============================\n";
    out += "LLVM-TUTOR: dynamic analysis results\n";
    out += "=============================\n";
    out += "Name           #N DIRECT CLALS\n";
    out += "-------------------------------\n";

    llvm::Constant *ResultHeaderStr = llvm::ConstantDataArray::getString(CTX,out.c_str());

    Constant *ResultHeaderStrVar = M.getOrInsertGlobal("ResultHeaderStrIR", ResultHeaderStr->getType());
    dyn_cast<GlobalVariable>(ResultHeaderStrVar)->setInitializer(ResultHeaderStr);


    //step4: 定义打印包装用来打印结果
    FunctionType *PrintfWrapperTy = FunctionType::get(llvm::Type::getVoidTy(CTX),{},false);

    Function *PrintfWrapperF = dyn_cast<Function>(M.getOrInsertFunction("printf_wrapper",PrintfWrapperTy).getCallee());

    llvm::BasicBlock *RetBlock = llvm::BasicBlock::Create(CTX,"enter",PrintfWrapperF);
    IRBuilder<> Builder(RetBlock);
   
    //开始插入对printf的调用
    llvm::Value *ResultHeaderStrPtr = Builder.CreatePointerCast(ResultHeaderStrVar, PrintfArgTy);
    llvm::Value *ResultFormatStrPtr = Builder.CreatePointerCast(ResultFormatStrVar, PrintfArgTy);

    Builder.CreateCall(Printf, {ResultHeaderStrPtr});

    LoadInst *LoadCounter;
    for(auto &item : CallCounterMap) {
        LoadCounter = Builder.CreateLoad(IntegerType::getInt32Ty(CTX),item.second);
        Builder.CreateCall(Printf,{ResultFormatStrPtr,FuncNameMap[item.first()],LoadCounter});

    }

    Builder.CreateRetVoid();

    appendToGlobalDtors(M,PrintfWrapperF, 0);

    return true;

    
}



PreservedAnalyses DynamicCallCounter::run(llvm::Module &M, llvm::ModuleAnalysisManager &) {
    bool Changed = runOnModule(M);

    return (Changed ? llvm::PreservedAnalyses::none()
                    : llvm::PreservedAnalyses::all());
}


//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getDynamicCallCounterPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "dynamic-cc", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "dynamic-cc") {
                    MPM.addPass(DynamicCallCounter());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getDynamicCallCounterPluginInfo();
}



// ; ModuleID = 'instrumented.bc'
// source_filename = "../inputs/input_for_cc.c"
// target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
// target triple = "x86_64-apple-macosx14.0.0"

// @CounterFor_foo = common global i32 0, align 4
// @0 = private unnamed_addr constant [4 x i8] c"foo\00", align 1
// @CounterFor_bar = common global i32 0, align 4
// @1 = private unnamed_addr constant [4 x i8] c"bar\00", align 1
// @CounterFor_fez = common global i32 0, align 4
// @2 = private unnamed_addr constant [4 x i8] c"fez\00", align 1
// @CounterFor_main = common global i32 0, align 4
// @3 = private unnamed_addr constant [5 x i8] c"main\00", align 1
// @ResultFormatStrIR = global [14 x i8] c"%-20s %-10lu\0A\00"
// @ResultHeaderStrIR = global [160 x i8] c"============================\0ALLVM-TUTOR: dynamic analysis results\0A=============================\0AName           #N DIRECT CLALS\0A-------------------------------\0A\00"
// @llvm.global_dtors = appending global [1 x { i32, ptr, ptr }] [{ i32, ptr, ptr } { i32 0, ptr @printf_wrapper, ptr null }]

// ; Function Attrs: noinline nounwind optnone ssp uwtable
// define void @foo() #0 {
//   %1 = load i32, ptr @CounterFor_foo, align 4
//   %2 = add i32 1, %1
//   store i32 %2, ptr @CounterFor_foo, align 4
//   ret void
// }

// ; Function Attrs: noinline nounwind optnone ssp uwtable
// define void @bar() #0 {
//   %1 = load i32, ptr @CounterFor_bar, align 4
//   %2 = add i32 1, %1
//   store i32 %2, ptr @CounterFor_bar, align 4
//   call void @foo()
//   ret void
// }

// ; Function Attrs: noinline nounwind optnone ssp uwtable
// define void @fez() #0 {
//   %1 = load i32, ptr @CounterFor_fez, align 4
//   %2 = add i32 1, %1
//   store i32 %2, ptr @CounterFor_fez, align 4
//   call void @bar()
//   ret void
// }

// ; Function Attrs: noinline nounwind optnone ssp uwtable
// define i32 @main() #0 {
//   %1 = load i32, ptr @CounterFor_main, align 4
//   %2 = add i32 1, %1
//   store i32 %2, ptr @CounterFor_main, align 4
//   %3 = alloca i32, align 4
//   %4 = alloca i32, align 4
//   store i32 0, ptr %3, align 4
//   call void @foo()
//   call void @bar()
//   call void @fez()
//   store i32 0, ptr %4, align 4
//   store i32 0, ptr %4, align 4
//   br label %5

// 5:                                                ; preds = %9, %0
//   %6 = load i32, ptr %4, align 4
//   %7 = icmp slt i32 %6, 10
//   br i1 %7, label %8, label %12

// 8:                                                ; preds = %5
//   call void @foo()
//   br label %9

// 9:                                                ; preds = %8
//   %10 = load i32, ptr %4, align 4
//   %11 = add nsw i32 %10, 1
//   store i32 %11, ptr %4, align 4
//   br label %5, !llvm.loop !5

// 12:                                               ; preds = %5
//   ret i32 0
// }

// ; Function Attrs: nounwind
// declare i32 @printf(ptr nocapture readonly, ...) #1

// define void @printf_wrapper() {
// enter:
//   %0 = call i32 (ptr, ...) @printf(ptr @ResultHeaderStrIR)
//   %1 = load i32, ptr @CounterFor_bar, align 4
//   %2 = call i32 (ptr, ...) @printf(ptr @ResultFormatStrIR, ptr @1, i32 %1)
//   %3 = load i32, ptr @CounterFor_main, align 4
//   %4 = call i32 (ptr, ...) @printf(ptr @ResultFormatStrIR, ptr @3, i32 %3)
//   %5 = load i32, ptr @CounterFor_foo, align 4
//   %6 = call i32 (ptr, ...) @printf(ptr @ResultFormatStrIR, ptr @0, i32 %5)
//   %7 = load i32, ptr @CounterFor_fez, align 4
//   %8 = call i32 (ptr, ...) @printf(ptr @ResultFormatStrIR, ptr @2, i32 %7)
//   ret void
// }

// attributes #0 = { noinline nounwind optnone ssp uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cmov,+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "tune-cpu"="generic" }
// attributes #1 = { nounwind }

// !llvm.module.flags = !{!0, !1, !2, !3}
// !llvm.ident = !{!4}

// !0 = !{i32 1, !"wchar_size", i32 4}
// !1 = !{i32 8, !"PIC Level", i32 2}
// !2 = !{i32 7, !"uwtable", i32 2}
// !3 = !{i32 7, !"frame-pointer", i32 2}
// !4 = !{!"Homebrew clang version 18.1.7"}
// !5 = distinct !{!5, !6}
// !6 = !{!"llvm.loop.mustprogress"}