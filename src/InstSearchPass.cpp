#include "../include/InstSearchPass.h"

using namespace llvm;

namespace TestPasses 
{
    namespace
    {
        void
        printInstFunc (Function &F)
        {
            errs() << "Function name: " << F.getName();
            DenseMap<StringRef, unsigned> InstCount;
            unsigned Size{};
            for (BasicBlock &BB : F)
            {
                for (Instruction &I : BB)
                {
                    ++InstCount[I.getOpcodeName()];
                    ++Size;
                }
            }

            errs() << ", num Instructions: " << Size << "\n";

            //Вывод в алфавитном порядке для удобства тестирования
            SmallVector<StringRef, 1> Keys;
            Keys.reserve(Size);
            for (auto &KV : InstCount) 
            {
                Keys.push_back(KV.first);
            }
            llvm::sort(Keys);
            
            for (auto K : Keys)
            {
                errs() << "\t" << K << ": " << InstCount.lookup(K) << "\n";
            }
        }
    } //namespace

    PreservedAnalyses 
    InstSearchPass::run( Function &Function, 
        FunctionAnalysisManager &AnalysisManager) 
    {
        printInstFunc(Function);
        return (PreservedAnalyses::all());
    }

    bool 
    InstSearchPass::isRequired( void) 
    { 
        return (true); 
    }

} //namespace TestPasses