#ifndef MYPASS_H
#define MYPASS_H

#include "llvm/Passes/PassBuilder.h"

namespace TestPasses 
{
    struct 
    MyPass : llvm::PassInfoMixin<MyPass>
    {
        llvm::PreservedAnalyses 
        run( llvm::Function &Function, 
            llvm::FunctionAnalysisManager &AnalysisManager);

        static bool 
        isRequired( void);
    };
} //namespace TestPasses
#endif