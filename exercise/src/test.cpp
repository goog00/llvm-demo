
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"


using namespace  llvm;

int main() {


    LLVMContext Context;

    std::unique_ptr<Module> M = std::make_unique<Module>("my_module",Context);

    //获取datalayout 对象
    const DataLayout &DL = M->getDataLayout();

    //定义一个类型 
    Type *Int32Ty = Type::getInt32Ty(Context);

    uint64_t Size = DL.getTypeStoreSize(Int32Ty);

    //输出大小
    printf("the storage size of int32 is %llu bytes\n",Size);

    return 0;


}


