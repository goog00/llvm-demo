
#include "DuplicateBB.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/RandomNumberGenerator.h"

#include <random>

#define DEBUG_TYPE "duplicate-bb"

STATISTIC(DuplicateBBCountStats, "The # of duplicated blocks");

using namespace llvm;

DuplicateBB::BBToSingleRIVMap
DuplicateBB::findBBSToDuplicate(Function &F, const RIV::Result &RIVResult)
{
    BBToSingleRIVMap BlocksToDuplicate;

    for (BasicBlock &BB : F){
        if (BB.isLandingPad())
            continue;

        auto const &ReachableValues = RIVResult.lookup(&BB);
        size_t ReachableValuesCount = ReachableValues.size();

        if(0 ==  ReachableValuesCount) {
            LLVM_DEBUG(errs() << "No Context values for this BB ");
            continue;
        }

        auto Iter = ReachableValues.begin();
        std::uniform_int_distribution<> Dist(0, ReachableValuesCount -1);
        std::advance(Iter, Dist(*pRNG));

        if(dyn_cast<GlobalValue>(*Iter)) {
            LLVM_DEBUG(errs() << "Random context value is a global variable." << "Skipping this BB\n");
            continue;
        }

        LLVM_DEBUG(errs() << "Random context value: " << **Iter << "\n");

        BlocksToDuplicate.emplace_back(&BB,*Iter);
        

    }

    return BlocksToDuplicate;
}


void DuplicateBB::cloneBB(BasicBlock &BB, Value *ContextValue, ValueToPhiMap &ReMapper) {

    Instruction *BBHead = BB.getFirstNonPHI();

    IRBuilder<> Builder(BBHead);
    Value *Cond = Builder.CreateIsNull(ReMapper.count(ContextValue) ? ReMapper[ContextValue] : ContextValue);

}