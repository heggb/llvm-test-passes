#include "../include/MyPass.h"
#include "../include/RPOPass.h"
#include "../include/InstSearchPass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

bool
CallBackForPipelineParser( 
    StringRef Name, 
    FunctionPassManager &FPM,  
    ArrayRef<PassBuilder::PipelineElement>)
{
    if ( Name == "MyPass" )
    {
        FPM.addPass( TestPasses::MyPass());
	    return (true);
    } 
    else if ( Name == "RPOPass" )
    {
        FPM.addPass( TestPasses::RPOPass());
        return (true);
    }
    else if ( Name == "InstSearchPass" )
    {
        FPM.addPass( TestPasses::InstSearchPass());
        return (true);
    }
    else
    {
        return (false);
    }
}

void
CallBackForPassBuilder( PassBuilder &PB)
{
    PB.registerPipelineParsingCallback( &CallBackForPipelineParser); 
} 

PassPluginLibraryInfo 
getMyPassPluginInfo( void)
{
    uint32_t     APIversion =  LLVM_PLUGIN_API_VERSION;
    const char * PluginName =  "MyPass";
    const char * PluginVersion =  LLVM_VERSION_STRING;
    
    PassPluginLibraryInfo info = 
    { 
        APIversion, 
	    PluginName, 
	    PluginVersion, 
	    CallBackForPassBuilder
    };

  return (info);
} 


extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() 
{
  return (getMyPassPluginInfo());
} 
