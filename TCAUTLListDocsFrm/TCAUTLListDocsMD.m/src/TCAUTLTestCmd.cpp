#include "TCAUTLTestCmd.h"
#include "CATApplication.h"
#include "CATScriptUtilities.h"
#include "CATAutoConversions.h"
#include "CATDialogAgent.h"
#include "CATGetEnvValue.h"
#include "CATCreateExternalObject.h" 
#include "CATInteractiveApplication.h" 
#include "CATAfrCommandHeaderServices.h"
#include "TCAIUTLGo.h"
#include "TCAXmlUtils.h"

CATCreateClass(TCAUTLTestCmd);

//---------------------------------------------------------------------------
// sRunScriptAndTriggerSecondPartTransition 
//---------------------------------------------------------------------------
void TCAUTLTestCmd::sRunScriptAndTriggerSecondPartTransition(
  CATCommand* iTCAUTLTestCmd, int iSubscribedType,
  CATString* iScriptName)
{
  CATVariant outResult;
  long ReturnValue = 0;
  HRESULT hr = BuildVariant((const long)ReturnValue, outResult);


  CATUnicodeString scriptDir("C:\\EXAMPLES\\AI\\AskAI\\Macros");

  CATVariant VariantReturnValue;
  long returnValue = 0;

  // CATString scriptName("UTLGoCATPart.catvbs");
  CATString scriptName;

  TCAIUTLGo_var spSingleTon = TCAUTLSingleTon();
  if( !!spSingleTon )
  {
    spSingleTon->getValue("macro_name", scriptName);
    returnValue = 0;
    hr = BuildVariant((const long)returnValue, VariantReturnValue);
    hr = CATScriptUtilities::ExecuteScript(scriptDir, catScriptLibraryTypeDirectory, scriptName.CastToCharPtr(), VariantReturnValue, "CATMain");
  }

  CATCommand* pCmd = NULL;
  CATAfrStartCommand("CATGPT", pCmd);
  if( pCmd )
  {
    pCmd = NULL;
  } 

  return;
}  // sRunScriptAndTriggerSecondPartTransition

//---------------------------------------------------------------------------
// TCAUTLTestCmd 
//---------------------------------------------------------------------------
TCAUTLTestCmd::TCAUTLTestCmd(void) : CATCommand(NULL, "TCAUTLTestCmd")
{
  RequestStatusChange(CATCommandMsgRequestExclusiveMode);
} // TCAUTLTestCmd 

//---------------------------------------------------------------------------
// ~TCAUTLTestCmd 
//---------------------------------------------------------------------------
TCAUTLTestCmd::~TCAUTLTestCmd()
{} // ~TCAUTLTestCmd 

//---------------------------------------------------------------------------
// Activate 
//---------------------------------------------------------------------------
CATStatusChangeRC TCAUTLTestCmd::Activate(CATCommand* FromClient, CATNotification* EvtDat)
{
  CATInteractiveApplication* pInterApplication = NULL;
  pInterApplication = (CATInteractiveApplication*)CATApplication::MainApplication();
  if( pInterApplication )
  {
    int timeOut = 1000;
    void* pUsefulData = (void*)pInterApplication;

    CATString   _ScriptName = "MyScript.catvbs";
    pInterApplication->AddTimeOut(timeOut, this, &_ScriptName, ( void( * )( ) )sRunScriptAndTriggerSecondPartTransition);
  }
  this->RequestDelayedDestruction();
  return ( CATStatusChangeRCCompleted );
}
