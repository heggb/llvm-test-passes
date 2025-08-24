#ifndef INSTSEARCHPASS_H
#define INSTSEARCHPASS_H

#include "llvm/Passes/PassBuilder.h"

//TestPasses
namespace TestPasses 
{
    // void 
    // visitFunction( llvm::Function &F);

    struct 
    InstSearchPass : llvm::PassInfoMixin<InstSearchPass>
    {
        llvm::PreservedAnalyses 
        run( llvm::Function &Function, 
            llvm::FunctionAnalysisManager &AnalysisManager);

        static bool 
        isRequired( void);
    };
} //namespace TestPasses

#endif