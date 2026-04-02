#include "TCAUTLRenameObjCmd.h"
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
#include "CATViewer.h"
#include "CATPixelImage.h"
#include "CATUuid.h"
#include <windows.h>
#include "CATInteractiveApplication.h"
#include "CATFrmLayout.h"
#include "CATFrmWindow.h"
#include <Python.h>
#include <iostream>
#include <cstring>

CATCreateClass(TCAUTLRenameObjCmd);

//---------------------------------------------------------------------------
// sRunScriptAndTriggerSecondPartTransition 
//---------------------------------------------------------------------------
void TCAUTLRenameObjCmd::sRunScriptAndTriggerSecondPartTransition(
  CATCommand* iTCAUTLTestCmd, int iSubscribedType,
  CATString* iScriptName)
{

  CATFrmLayout* pCurrentLayout = CATFrmLayout::GetCurrentLayout();
  if( !pCurrentLayout )
    return;

  CATFrmWindow* pCurrentWindow = pCurrentLayout->GetCurrentWindow();
  if( !pCurrentWindow )
    return;

  CATViewer* pViewer = pCurrentWindow->GetViewer();
  if( !pViewer )
    return;

  CATString imgPath = "c:\\tmp\\IMG_NAME.png";



  CATUuid uuid;
  char* c = new char[255];
  uuid.UUID_to_chaine(c);
  imgPath.ReplaceSubString("IMG_NAME", CATString(c));
  delete[] c;

  CATPixelImage* pImg = pViewer->GrabPixelImage();

  pImg->WriteToFile("PNG", imgPath);


  TCAIUTLGo_var spSingleTon = TCAUTLSingleTon();
  if( !!spSingleTon )
  {
    spSingleTon->setValue("img_path", imgPath);
  }

  return;
}  // sRunScriptAndTriggerSecondPartTransition

//---------------------------------------------------------------------------
// onTimeOut 
//---------------------------------------------------------------------------
void TCAUTLRenameObjCmd::onTimeOut(CATCommand* i_pToClient, int i_Time, void* i_pUsefulData)
{
  CATUnicodeString answerAI; 

  CATString imgPath;
  TCAIUTLGo_var spSingleTon = TCAUTLSingleTon();
  if( !!spSingleTon )
  {
    spSingleTon->getValue("img_path", imgPath);
  }

  // Check Python version
  const char* version = Py_GetVersion();
  cerr << "Python version in use: " << version << endl;

  static PyObject* s_pAIMmodule = NULL;
  if( !s_pAIMmodule )
  {
    s_pAIMmodule = PyImport_ImportModule("TCACATPythonCAA");
  }

  if( !s_pAIMmodule )
  {
    PyErr_Print();
    std::cerr << "Failed to load TCACATPythonCAA.h\n";
    return;
  }

  PyObject* s_pFunc = NULL;
  if( !s_pFunc )
  {
    s_pFunc = PyObject_GetAttrString(s_pAIMmodule, "describe_screnshot");  // 3. Get aksAI()
  }
  if( !s_pFunc || !PyCallable_Check(s_pFunc) )
  {
    PyErr_Print();
    std::cerr << "Cannot find function TCACATPythonCAA::describe_screnshot()\n";
    return;
  } 

  const char* cstr = imgPath.ConvertToChar();
  PyObject* arg1 = PyUnicode_FromString(cstr);
  PyObject* args = PyTuple_Pack(1, arg1 ); 

  // PyObject* args = PyTuple_Pack(1, arg1); 
  PyObject* result = PyObject_CallObject(s_pFunc, args);

  if( result )
  {
    // answerAI = PyUnicode_AsUTF8(result);
    const char* result_utf8 = PyUnicode_AsUTF8(result);
    if( result_utf8 )
    {
      // answerAI = CATUnicodeString(result_utf8);
      size_t utf8Length = strlen(result_utf8);
      answerAI.BuildFromUTF8(result_utf8, utf8Length);
    }
    std::cout << "Python function returned: " << answerAI << "\n";
    Py_DECREF(result);

  }
  else
  {
    PyErr_Print();
  }
   
  cerr << "Object name: " << answerAI << endl;

  TCAIUTLGo_var spRenameObj = UTL_INST(UTLGoRenameObjects);
  if( !!spRenameObj )
  {
    spRenameObj->setValue("obj_name", answerAI);
    spRenameObj->run();
  }

  return;
} // onTimeOut 

//---------------------------------------------------------------------------
// TCAUTLRenameObjCmd 
//---------------------------------------------------------------------------
TCAUTLRenameObjCmd::TCAUTLRenameObjCmd(void) : CATCommand(NULL, "TCAUTLRenameObjCmd")
{
  RequestStatusChange(CATCommandMsgRequestExclusiveMode);
} // TCAUTLRenameObjCmd 

//---------------------------------------------------------------------------
// ~TCAUTLRenameObjCmd 
//---------------------------------------------------------------------------
TCAUTLRenameObjCmd::~TCAUTLRenameObjCmd()
{} // ~TCAUTLRenameObjCmd 

//---------------------------------------------------------------------------
// Activate 
//---------------------------------------------------------------------------
CATStatusChangeRC TCAUTLRenameObjCmd::Activate(CATCommand* FromClient, CATNotification* EvtDat)
{
  CATInteractiveApplication* pInterApplication = NULL;
  pInterApplication = (CATInteractiveApplication*)CATApplication::MainApplication();
  if( pInterApplication )
  {
    int timeOut = 1000;
    void* pUsefulData = (void*)pInterApplication;

    CATString   _ScriptName = "MyScript.catvbs";
    pInterApplication->AddTimeOut(timeOut, this, &_ScriptName, ( void( * )( ) )sRunScriptAndTriggerSecondPartTransition);

    int timeOut1 = 3000;

    // Time out sets on the application
    pInterApplication->AddTimeOut(timeOut1, this, pUsefulData, ( void( * )( ) )TCAUTLRenameObjCmd::onTimeOut);
  }
  this->RequestDelayedDestruction();
  return ( CATStatusChangeRCCompleted );
}
