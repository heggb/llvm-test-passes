#include "../include/MyPass.h"

using namespace llvm;
//using namespace TestPasses;

namespace TestPasses 
{
    namespace  
    {
        void 
        visitFunction( Function &F) 
        {
            errs() << "Function name: " << F.getName() << "\n";
            errs() << "    number of arguments: " << F.arg_size() << "\n";
        } 
    } //namespace 
    

    PreservedAnalyses 
    MyPass::run( Function &Function, 
        FunctionAnalysisManager &AnalysisManager) 
    {
        visitFunction( Function);
        return (PreservedAnalyses::all());
    }

    bool 
    MyPass::isRequired( void) 
    { 
        return (true); 
    }
} //namespace TestPasses


