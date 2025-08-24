#include "../include/RPOPass.h"
#include <cmath>

using namespace llvm;
//using namespace TestPasses;

namespace TestPasses
{
    namespace
    {
        void
        setLoopName(DenseMap<const BasicBlock*, SmallStringName> &BBTNs, Loop *L, 
            PostDominatorTree &PDT, StringRef FuncName,
            unsigned int GlobalLoopID)
        {
            SmallVector<LoopWork, 1> StackLoop_ID; //Хранит циклы и ID 
            SmallVector<unsigned, 1> IDs;
            SmallVector<BasicBlock*, 5> NamedBBs; //Хранит блоки цикла, которые уже были ранее названы для правильной отрисовки имени
            NamedBBs.reserve(L->getNumBlocks());
            unsigned Depth = L->getLoopDepth();
            IDs.reserve(Depth);
            IDs.push_back(GlobalLoopID);
            StackLoop_ID.push_back(LoopWork{L, std::move(IDs)});

            while (!StackLoop_ID.empty()) //Проход по всем элементам цикла
            {
                LoopWork CurPairLoop_ID = std::move(StackLoop_ID.pop_back_val());
                Loop *L = CurPairLoop_ID.Loop;
                SmallVector<unsigned, 1> LoopIDs = CurPairLoop_ID.LoopIDs;
                LoopIDs.reserve(Depth);

                BasicBlock *Hdr = L->getHeader();
                if (Hdr)
                {
                    BBTNs[Hdr].appendSafeLoopName(LoopIDs, StringRef(".header"));
                    BBTNs[Hdr].setCurLoopFlag(true);
                    BBTNs[Hdr].setExternLoopFlag(true);
                    NamedBBs.push_back(Hdr);
                }

                if (BasicBlock *PH = L->getLoopPreheader())
                {
                    BBTNs[PH].appendSafeLoopName(LoopIDs, StringRef(".preheader"));
                    BBTNs[PH].setCurLoopFlag(true);
                    BBTNs[PH].setExternLoopFlag(true);
                    NamedBBs.push_back(PH);
                }
                else 
                {
                    BasicBlock *PseudoPH = nullptr;
                    for (auto *Pred : predecessors(Hdr))
                    {
                        if (!L->contains(Pred))
                        {
                            if (!PseudoPH)
                            {
                                PseudoPH = Pred;
                            }
                            else if (PseudoPH != Pred)
                            {
                                PseudoPH = nullptr;
                                break;
                            }
                        }
                    }

                    if (PseudoPH)
                    {
                        BBTNs[PseudoPH].appendSafeLoopName(LoopIDs, StringRef(".preheader_like"));
                        BBTNs[PseudoPH].setCurLoopFlag(true);
                        BBTNs[PseudoPH].setExternLoopFlag(true);
                        NamedBBs.push_back(PseudoPH);
                    }
                }

                for (BasicBlock *Succ : successors(Hdr))
                {
                    if (L->contains(Succ) && Succ != Hdr)
                    {
                        BBTNs[Succ].appendSafeLoopName(LoopIDs, StringRef(".body"));
                        BBTNs[Succ].setCurLoopFlag(true);
                        BBTNs[Succ].setExternLoopFlag(true);
                        NamedBBs.push_back(Succ);
                    }
                }

                SmallVector<BasicBlock*, 4> Latches;
                L->getLoopLatches(Latches);
                unsigned LatchID{};
                for (BasicBlock *LT : Latches)
                {
                    BBTNs[LT].appendSafeLoopName(LoopIDs, Twine(".latch") + Twine(LatchID));
                    BBTNs[LT].setCurLoopFlag(true);
                    BBTNs[LT].setExternLoopFlag(true);
                    NamedBBs.push_back(LT);
                    ++LatchID;
                }

                SmallVector<BasicBlock*, 4> ExitBlocks;
                L->getExitingBlocks(ExitBlocks);
                unsigned ExitID{};
                for (BasicBlock *EB : ExitBlocks)
                {
                    BBTNs[EB].appendSafeLoopName(LoopIDs, Twine(".exit") + Twine(ExitID));
                    BBTNs[EB].setCurLoopFlag(true);
                    BBTNs[EB].setExternLoopFlag(true);
                    NamedBBs.push_back(EB);
                    ++ExitID;
                }
                
                SmallVector<BasicBlock*, 4> PostExitBlocks;
                L->getUniqueExitBlocks(PostExitBlocks);
                unsigned PostExitID{};
                for (BasicBlock *PEB : PostExitBlocks)
                {
                    BBTNs[PEB].appendSafeLoopName(LoopIDs, Twine(".postexit") + Twine(PostExitID));
                    BBTNs[PEB].setCurLoopFlag(true);
                    BBTNs[PEB].setExternLoopFlag(true);
                    NamedBBs.push_back(PEB);
                    ++PostExitID;
                }

                if (!PostExitBlocks.empty())
                {
                    BasicBlock *End = PostExitBlocks[0];
                    for (BasicBlock *B : PostExitBlocks)
                    {
                        End = PDT.findNearestCommonDominator(End, B);
                    }
                    if (End)
                    {
                        BBTNs[End].appendSafeLoopName(LoopIDs, StringRef(".end"));
                        BBTNs[End].setCurLoopFlag(true);
                        BBTNs[End].setExternLoopFlag(true);
                        NamedBBs.push_back(End);
                    }
                }

                //Добавление наследников
                unsigned SubID{};
                for (Loop *Sub : *L)
                {
                    SmallVector<unsigned, 1> SubLoopIDs(LoopIDs);
                    SubLoopIDs.push_back(SubID);
                    StackLoop_ID.push_back(LoopWork{Sub, std::move(SubLoopIDs)});
                    ++SubID;
                }

                //Демаркирование названных блоков
                for (BasicBlock *BB : NamedBBs)
                {
                    BBTNs[BB].setCurLoopFlag(false);
                }
            }
        }

        void
        setNameFuncBBs ( Function& F, 
            FunctionAnalysisManager &AnalysisManager,
            SmallVectorImpl<const BasicBlock*> &StackInterBBs )
        {
            
            auto &DT = AnalysisManager.getResult<DominatorTreeAnalysis>(F);
            auto &LI = AnalysisManager.getResult<LoopAnalysis>(F);
            auto &PDT = AnalysisManager.getResult<PostDominatorTreeAnalysis>(F);
            
            StringRef FuncName = F.getName();
            //Счётчики для присвоения ID в дополнительных названий блокам  
            unsigned UnreachID{}, ExitID{}, IfID{}, BrID{}, SwitchID{}, 
            InvokeID{}, CallBrID{}, IndirBrID{}, UndefID{}, ResID{},
            CatchSwitchID{}, CatchRetID{}, CleanRetID{}, LoopID{};

            bool flag_name = false; //Нужен для вставки универсального названия, если проход по всем терминаторам не дал нужного результата

            DenseMap<const BasicBlock*, SmallStringName> BBTNs;
            BBTNs.reserve(F.size());

            BasicBlock *EntryBB = nullptr;

            for (BasicBlock &BB : F)
            {
                BBTNs.try_emplace(&BB, SmallStringName(FuncName));
            }
            
            for (Loop *L : LI)
            {
                setLoopName(BBTNs, L, PDT, FuncName, LoopID);
                ++LoopID;
            }
            
            for (BasicBlock &BB : F)
            {
                Instruction* TI = BB.getTerminator();
                
                if (BB.isEntryBlock())
                {
                    BBTNs[&BB].appendSafeName(StringRef(".entry"));
                    EntryBB = &BB;
                    flag_name = true;
                }

                //Добавляем только такие недостижимые блоки, у которых нет предков, чтобы на этапе получения RPO порядка их не искать заново
                if (DT.getNode(&BB) == nullptr || isa<UnreachableInst>(TI))
                {
                    BBTNs[&BB].appendSafeName(Twine(".unreach") + Twine(UnreachID));
                    if (pred_empty(&BB))
                    {
                        StackInterBBs.push_back(&BB);
                    }
                    ++UnreachID;
                    flag_name = true;
                }
                
                if (isa<ReturnInst>(TI))
                {
                    BBTNs[&BB].appendSafeName(Twine(".exit") + Twine(ExitID));
                    ++ExitID;
                    flag_name = true;
                }

                if (isa<ResumeInst>(TI))
                {
                    BBTNs[&BB].appendSafeName(Twine(".resume") + Twine(ResID));
                    ++ResID;
                    flag_name = true;
                }
                
                if (auto *BI = dyn_cast<BranchInst>(TI))
                {
                    if (BI->isConditional())
                    {
                        BBTNs[&BB].appendSafeName(Twine(".if") + Twine(IfID) + Twine(".cond"));

                        BasicBlock *SuccT = BI->getSuccessor(0);
                        BBTNs[SuccT].appendSafeName(Twine(".if") + Twine(IfID) + Twine(".then"));

                        BasicBlock* SuccF = BI->getSuccessor(1);
                        BBTNs[SuccF].appendSafeName(Twine(".if") + Twine(IfID) + Twine(".else"));

                        if (BasicBlock *Merge = PDT.findNearestCommonDominator(SuccT, SuccF))
                        {
                            BBTNs[Merge].appendSafeName(Twine(".if") + Twine(IfID) + Twine(".end"));
                        }

                        ++IfID;
                        flag_name = true;
                    }
                    else 
                    {
                        BBTNs[&BB].appendSafeName(Twine(".br") + Twine(BrID));
                        ++BrID;
                        flag_name = true;
                    }
                }

                SmallPtrSet<const BasicBlock*, 2> SwitchBBs;
                if (auto *SI = dyn_cast<SwitchInst>(TI))
                {
                    BBTNs[&BB].appendSafeName(Twine(".switch") + Twine(SwitchID) + Twine(".cond"));
                    SwitchBBs.insert(&BB);

                    for (auto &Case : SI->cases())
                    {
                        const BasicBlock* CaseBB = Case.getCaseSuccessor();
                        const Twine SwitchName = SwitchBBs.count(CaseBB) ? Twine() : Twine(".switch") + Twine(SwitchID);
                        BBTNs[CaseBB].appendSafeName(SwitchName + Twine(".case") + 
                            Twine(Case.getCaseValue()->getSExtValue()));
                        SwitchBBs.insert(CaseBB);
                    }

                    if (const BasicBlock *Def = SI->getDefaultDest())
                    {
                        const Twine SwitchName = SwitchBBs.count(Def) ? Twine() : Twine(".switch") + Twine(SwitchID);
                        BBTNs[Def].appendSafeName(SwitchName + Twine(".default"));
                        SwitchBBs.insert(Def);
                    }

                    ++SwitchID;
                    flag_name = true;
                }

                if (auto *II = dyn_cast<InvokeInst>(TI))
                {
                    BBTNs[&BB].appendSafeName(Twine("invoke") + Twine(InvokeID) + Twine(".callsite"));
                    BBTNs[II->getNormalDest()].appendSafeName(Twine("invoke") + Twine(InvokeID) + 
                        Twine(".callsite"));
                    BBTNs[II->getUnwindDest()].appendSafeName(Twine("invoke") + Twine(InvokeID) + 
                        Twine(".unwind"));
                    ++InvokeID;
                    flag_name = true;
                }

                if (auto *CBI = dyn_cast<CallBrInst>(TI))
                {
                    BBTNs[&BB].appendSafeName(Twine(".callbr") + Twine(CallBrID) + 
                        Twine(".dispatch"));
                    
                    if (BasicBlock *Def = CBI->getDefaultDest())
                    {
                        BBTNs[Def].appendSafeName(Twine(".callbr") + Twine(CallBrID) + 
                            Twine(".default"));
                    }

                    for (unsigned i = 0, e = CBI->getNumIndirectDests(); i != e; ++i)
                    {
                        if (BasicBlock *Dst = CBI->getIndirectDest(i))
                        {
                            BBTNs[Dst].appendSafeName(Twine(".callbr") + Twine(CallBrID) + 
                                Twine(".dest") + Twine(i));
                        }
                    }

                    ++CallBrID;
                    flag_name = true;
                }

                if (auto *IBR = dyn_cast<IndirectBrInst>(TI))
                {

                    BBTNs[&BB].appendSafeName(Twine(".indirectbr") + Twine(IndirBrID) + 
                        Twine(".dispatch"));
                    
                    for (unsigned i = 0, e = IBR->getNumDestinations(); i != e; ++i)
                    {
                        if (BasicBlock *Dst = IBR->getDestination(i))
                        {
                            BBTNs[Dst].appendSafeName(Twine(".indirectbr") + 
                                Twine(IndirBrID) + Twine(".dest") + Twine(i));
                        }
                    }

                    ++IndirBrID;
                    flag_name = true;
                }

                if (auto *CSI = dyn_cast<CatchSwitchInst>(TI))
                {
                    BBTNs[&BB].appendSafeName(Twine(".catchswitch") + Twine(CatchSwitchID));

                    unsigned HandlerID{};
                    for (BasicBlock *H : CSI->handlers())
                    {
                        BBTNs[H].appendSafeName(Twine(".catchswitch") + Twine(CatchSwitchID) + 
                            Twine(".catchpad") + Twine(HandlerID));
                        ++HandlerID;
                    }

                    if (CSI->hasUnwindDest())
                    {
                        if (BasicBlock *U = CSI->getUnwindDest())
                        {
                            BBTNs[U].appendSafeName(Twine(".catchswitch") + Twine(CatchSwitchID) +
                                Twine(".unwind"));
                        }
                    }
                    
                    ++CatchSwitchID;
                    flag_name = true;
                }

                if (auto *CRI = dyn_cast<CatchReturnInst>(TI))
                {
                    BBTNs[&BB].appendSafeName(Twine(".catchret") + Twine(CatchRetID));

                    if (auto *Succ = CRI->getSuccessor())
                    {
                        BBTNs[Succ].appendSafeName(Twine(".catchret") + Twine(CatchRetID) +
                            Twine(".cont"));
                    }
                    
                    ++CatchRetID;
                    flag_name = true;
                }

                else if (auto *CLRI = dyn_cast<CleanupReturnInst>(TI))
                {
                    BBTNs[&BB].appendSafeName(Twine(".cleanupret") + Twine(CleanRetID));

                    if (CLRI->hasUnwindDest())
                    {
                        if (BasicBlock *U = CLRI->getUnwindDest())
                        {
                            BBTNs[U].appendSafeName(Twine(".cleanupret") + Twine(CleanRetID) +
                                Twine(".unwind"));
                        }
                    }
                    
                    ++CleanRetID;
                    flag_name = true;
                }

                if (!flag_name)
                {
                    BBTNs[&BB].appendSafeName(Twine(".bb") + Twine(UndefID));
                    ++UndefID;
                }
                flag_name = false;
            }

            for (BasicBlock &BB : F)
            {
                BB.setName(BBTNs[&BB].getName());
            }

            StackInterBBs.push_back(EntryBB);
        }


        void
        setRPO( Function &F, 
            SmallVectorImpl<const BasicBlock*> &StackInterBBs )
        {
            unsigned time = 0;
            unsigned Size = F.size();
            SmallVector<const BasicBlock*, 1> StackResultBBs;
            StackResultBBs.reserve(Size);
            
            DenseMap<const BasicBlock*, BBInfo> DataDFS;
            DataDFS.reserve(Size);

            //Цикл для обработки ранее полученных блоков
            while (!StackInterBBs.empty()) 
            {
                const BasicBlock *CurBB = std::move(StackInterBBs.pop_back_val());
                BBInfo &CurInfo = DataDFS.try_emplace(CurBB, BBInfo{0, 0}).first->second;

                if (CurInfo.pre == 0)
                {
                    CurInfo.pre = ++time;
                    StackInterBBs.push_back(CurBB);
                    const Instruction *TInst = CurBB->getTerminator();
                    
                    for (unsigned i = 0, n = TInst->getNumSuccessors(); i < n; ++i)
                    {
                        BasicBlock *Succ = TInst->getSuccessor(i);
                        BBInfo &SuccInfo = DataDFS.try_emplace(Succ, BBInfo{0, 0}).first->second;
                        
                        if (SuccInfo.pre == 0)
                        {
                            StackInterBBs.push_back(Succ);
                        }
                        else if (SuccInfo.post == 0)
                        {
                            errs() << "\tFound loop " << CurBB->getName() << "->" << Succ->getName() << "\n";
                        }
                    }
                }

                else if (CurInfo.post == 0)
                {
                    CurInfo.post = ++time;
                    StackResultBBs.push_back(CurBB);
                }
            }

            //Замена, после "очистки" входного стека
            StackInterBBs = std::move(StackResultBBs);
        }

    } //namespace
    
    SmallStringName::SmallStringName(llvm::StringRef FuncName) 
        : Name(FuncName) {}

    void
    SmallStringName::appendSafeName(const llvm::StringRef AddName)
    {
        if ((this->Name).ends_with(AddName))
            return;

        this->Name += AddName;
    }
    
    void
    SmallStringName::appendSafeName(const llvm::Twine &AddName)
    {
        SmallString<64> add;
        AddName.toStringRef(add);

        if ((this->Name).ends_with(add))
            return;

        this->Name += add;
    }

    void
    SmallStringName::appendSafeLoopName (SmallVectorImpl<unsigned> &LoopIDs, const Twine &AddName)
    {
        if (!CurLoopFlag)
        {
            if (ExternLoopFlag) //Проверка, чтобы не добавить лишних .loop<ID>
            {
                unsigned ID = LoopIDs[LoopIDs.size() - 2];
                SmallString<64> add;
                (Twine(".loop") + Twine(ID)).toStringRef(add);
                (this->Name) += add;
            }
            else 
            {
                for (unsigned ID : LoopIDs)
                {
                    SmallString<64> add;
                    (Twine(".loop") + Twine(ID)).toStringRef(add);
                    (this->Name) += add;
                }
            }
        }
        
        this->appendSafeName(AddName);
    }

    void
    SmallStringName::appendSafeLoopName (SmallVectorImpl<unsigned> &LoopIDs, const StringRef AddName)
    {
        if (!CurLoopFlag)
        {
            if (ExternLoopFlag) //Проверка, чтобы не добавить лишних .loop<ID>
            {
                unsigned ID = LoopIDs[LoopIDs.size() - 2];
                SmallString<64> add;
                (Twine(".loop") + Twine(ID)).toStringRef(add);
                (this->Name) += add;
            }
            else 
            {
                for (unsigned ID : LoopIDs)
                {
                    SmallString<64> add;
                    (Twine(".loop") + Twine(ID)).toStringRef(add);
                    (this->Name) += add;
                }
            }
        }
        
        this->appendSafeName(AddName);
    }

    void
    SmallStringName::setCurLoopFlag(bool CurLoopFlag)
    {
        this->CurLoopFlag = CurLoopFlag;
    }

    void
    SmallStringName::setExternLoopFlag(bool ExternLoopFlag)
    {
        this->ExternLoopFlag = ExternLoopFlag;
    }

    StringRef
    SmallStringName::getName() const
    {
        return (this->Name);
    }

    PreservedAnalyses 
    RPOPass::run( Function &Function, 
    FunctionAnalysisManager &AnalysisManager) 
    {
        errs() << "Function name: " << Function.getName() << ", size: " << Function.size() << "\n";

        SmallVector<const BasicBlock*, 1> StackInterBBs;
        if (!Function.empty() && !Function.isDeclaration())
        {
            errs() << "\n";
            const unsigned Size = Function.size();      
            StackInterBBs.reserve(Size);
            setNameFuncBBs(Function, AnalysisManager, StackInterBBs);
            setRPO(Function, StackInterBBs);
            errs() << "\n"; 
        }

        while (!StackInterBBs.empty())
        {
            const BasicBlock *BB = StackInterBBs.pop_back_val();
            errs() << "\t" << BB->getName() << "\n"; 
        }
        errs() << "-------------------------------------------------------------------------------\n";
        return (PreservedAnalyses::none());
    }

    bool 
    RPOPass::isRequired( void) 
    { 
        return (true); 
    }
} //namespace TestPasses

