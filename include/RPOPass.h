#ifndef RPOPASS_H
#define RPOPASS_H

#include "llvm/Passes/PassBuilder.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/PostDominators.h"

namespace TestPasses
{    
    struct 
    BBInfo //Хранит информацию для DFS
    {
        unsigned pre = 0, post = 0;
    };

    struct
    LoopWork //Хранит цикл и ID текущего и внешних циклов
    {
        llvm::Loop *Loop;
        llvm::SmallVector<unsigned, 1> LoopIDs;
    };

    class
    SmallStringName //Реализует безопасное и правильное формирование имён BB
    {
    private:
        llvm::SmallString<128> Name{};
        bool CurLoopFlag{}; //Флаг для проверки того, что для блока текущего цикла имя изменено
        bool ExternLoopFlag{}; //Флаг для проверкитого, что для блока внешних циклов имя изменено
    public:
        SmallStringName() = default;
        SmallStringName(llvm::StringRef FuncName);

        void
        appendSafeName(llvm::StringRef AddName);
        void
        appendSafeName(const llvm::Twine &AddName);

        void
        appendSafeLoopName(llvm::SmallVectorImpl<unsigned> &LoopIDs, llvm::StringRef AddName);
        void
        appendSafeLoopName(llvm::SmallVectorImpl<unsigned> &LoopIDs, const llvm::Twine &AddName);

        void 
        setCurLoopFlag(bool CurLoopFlag);
        void 
        setExternLoopFlag(bool ExternLoopFlag);

        llvm::StringRef
        getName() const;
    };

    struct 
    RPOPass : llvm::PassInfoMixin<RPOPass>
    {
        llvm::PreservedAnalyses 
        run( llvm::Function &Function, 
        llvm::FunctionAnalysisManager &AnalysisManager);

        static bool 
        isRequired( void);
    };
}

#endif