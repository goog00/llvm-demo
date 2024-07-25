// 整体逻辑
// 1.检查并获取浮点比较指令。
// 2.根据比较谓词进行转换。
// 3.计算两个操作数的差值的绝对值。
// 4.修改比较指令，使其比较绝对值与一个非常小的浮点数（EpsilonValue）。
// 这个过程的目的是通过比较两个浮点数的差值的绝对值来替代直接的浮点等式比较，从而减少由于浮点运算精度问题可能导致的逻辑错误。

//命令：
// clang -emit-llvm -S -Xclang -disable-O0-optnone ../../inputs/input_for_fcmp_eq.c -o input_for_fcmp_eq.ll  
// /usr/local/llvm17/bin/opt  --load-pass-plugin  libFindFCmpEq.dylib  --load-pass-plugin  libConvertFCmpEq.dylib --passes="convert-fcmp-eq"  -S input_for_fcmp_eq.ll -o fcmp_eq_after_conversion.ll   
//结果：
// %11 = fcmp oeq double %9, %10
// 转换成
//   %11 = fsub double %9, %10
//   %12 = bitcast double %11 to i64
//   %13 = and i64 %12, 9223372036854775807
//   %14 = bitcast i64 %13 to double
//   %15 = fcmp olt double %14, 0x3CB0000000000000

#include "ConvertFCmpEq.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"

#include <cassert>

using namespace llvm;

static FCmpInst *convertFCmpEqInstruction(FCmpInst *FCmp) noexcept {
    // 确保传入的浮点比较指令不为空
    assert(FCmp && "the given fcmp instruction is null");

    // 如果传入的指令不是等式比较（== 或 !=），返回 nullptr
    if (!FCmp->isEquality()) {
        return nullptr;
    }

    // 获取浮点比较指令的左右操作数
    Value *LHS = FCmp->getOperand(0);
    Value *RHS = FCmp->getOperand(1);

    // 根据当前比较指令的谓词（Predicate），将其转换为新的谓词
    CmpInst::Predicate CmpPred = [FCmp] {
        switch (FCmp->getPredicate()) {
            case CmpInst::Predicate::FCMP_OEQ:
                // 将有序等于转换为有序小于
                return CmpInst::Predicate::FCMP_OLT;
            case CmpInst::Predicate::FCMP_UEQ:
                // 将无序等于转换为无序小于
                return CmpInst::Predicate::FCMP_ULT;
            case CmpInst::Predicate::FCMP_ONE:
                // 将有序不等转换为有序大于等于
                return CmpInst::Predicate::FCMP_OGE;
            case CmpInst::Predicate::FCMP_UNE:
                // 将无序不等转换为无序大于等于
                return CmpInst::Predicate::FCMP_UGE;
            default:
                // 遇到不支持的谓词时报错
                llvm_unreachable("Unsupported fcmp predicate");
        }
    }();

    // 获取当前指令所属的模块
    Module *M = FCmp->getModule();
    // 确保模块不为空
    assert(M && "the given fcmp instruction does not belong to a module");

    // 获取模块上下文
    LLVMContext &Ctx = M->getContext();
    // 定义 64 位整数类型和双精度浮点类型
    IntegerType *I64Ty = IntegerType::get(Ctx, 64);
    Type *DoubleTy = Type::getDoubleTy(Ctx);

    // 创建用于提取绝对值的掩码
    ConstantInt *SignMask = ConstantInt::get(I64Ty, ~(1L << 63));

    // 创建用于比较的非常小的浮点数值（Epsilon）
    APInt EpsilonBits(64, 0x3CB0000000000000);
    Constant *EpsilonValue = ConstantFP::get(DoubleTy, EpsilonBits.bitsToDouble());

    // 使用 IRBuilder 在当前比较指令的位置插入新的指令
    IRBuilder<> Builder(FCmp);

    // 计算两个操作数的差值
    auto *FSubInst = Builder.CreateFSub(LHS, RHS);
    // 将浮点差值转换为 64 位整数
    auto *CastToI64 = Builder.CreateBitCast(FSubInst, I64Ty);
    // 计算差值的绝对值
    auto *AbsValue = Builder.CreateAnd(CastToI64, SignMask);
    // 将绝对值转换回双精度浮点数
    auto *CastToDouble = Builder.CreateBitCast(AbsValue, DoubleTy);

    // 修改浮点比较指令的谓词
    FCmp->setPredicate(CmpPred);
    // 修改浮点比较指令的操作数
    FCmp->setOperand(0, CastToDouble);
    FCmp->setOperand(1, EpsilonValue);

    // 返回修改后的比较指令
    return FCmp;
}


static constexpr char PassArg[] = "convert-fcmp-eq";
static constexpr char PassName[] =
    "Convert floating-point equality comparisons";
static constexpr char PluginName[] = "ConvertFCmpEq";
#define DEBUG_TYPE ::PassArg
STATISTIC(FCmpEqConversionCount,
          "Number of direct floating-point equality comparisons converted");


PreservedAnalyses ConvertFCmpEq::run(Function &Func,
                                    FunctionAnalysisManager &FAM) {

    auto &Comparisons = FAM.getResult<FindFCmpEq>(Func);
    bool Modified = run(Func, Comparisons);

    return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();                                        

}

bool ConvertFCmpEq::run(llvm::Function &Func,
                        const FindFCmpEq::Result &Comparisons) {
  bool Modified = false;
  
  // 如果函数被显式标记为 'optnone'，则忽略此函数，因为不应对其进行任何更改
  if (Func.hasFnAttribute(Attribute::OptimizeNone)) {
    LLVM_DEBUG(dbgs() << "Ignoring optnone-marked function \"" << Func.getName()
                      << "\"\n");
    Modified = false;
  } else {
    // 遍历找到的浮点相等比较指令
    for (FCmpInst *FCmp : Comparisons) {
      // 尝试转换浮点相等比较指令
      if (convertFCmpEqInstruction(FCmp)) {
        // 如果转换成功，增加转换计数并标记为已修改
        ++FCmpEqConversionCount;
        Modified = true;
      }
    }
  }

  // 返回是否修改了函数
  return Modified;
}



//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
PassPluginLibraryInfo getConvertFCmpEqPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, PluginName, LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [&](StringRef Name, FunctionPassManager &FPM,
                    ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name.equals(PassArg)) {
                    FPM.addPass(ConvertFCmpEq());
                    return true;
                  }

                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getConvertFCmpEqPluginInfo();
}
