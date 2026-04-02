#include "CATIDftGenGeom.h"
#include "CATIBRepAccess.h"
#include "CATCell.h" 
#include "CATGeometry.h"
#include "CATBody.h"
#include "CATIGeometricalElement.h"
#include "CATICGMObject.h"
#include "CATUuid.h"
#include "CATISpecAttribute.h"
#include "CATToken.h"
#include "fstream.h"
#include "CATIParmPublisher.h"
#include "TCAUTLGPTCmd.h"
#include "CATApplicationFrame.h"
#include "CATDlgGridConstraints.h"
#include "CATMsgCatalog.h"
#include "iostream.h"
#include "CATDocument.h"
#include "CATDocumentsInSession.h"
#include "CATSessionServices.h"
#include "CATIDocId.h"
#include "CATListOfCATUnicodeString.h"
#include "CATFrmEditor.h"
#include "CATCSO.h"
#include "CATIAlias.h"
#include "TCAIUTLDoIt.h"
#include "CATISpecAttrKey.h"
#include "CATLISTV_CATISpecAttrKey.h"
#include "CATISpecAttrAccess.h"
#include "CATMfBRepDecode.h"
#include "CATDlgContainer.h"
#include "CATDlgEditor.h"
#include "CATDlgTabContainer.h"
#include "CATDlgTabPage.h"
#include "CATDlgPushButton.h"
#include "CATDlgLabel.h"
#include "CATSpecPointing.h"
#include "CATIContainer.h"
#include "CATILinkableObject.h"
#include "CATMetaClass.h"
#include "CATOmbDocPropertyServices.h"
#include "CATIPrdProperties.h"
#include "CATIPrtContainer.h"
#include "CATIPrtPart.h"
#include "CATInit.h"
#include "CATICkeParm.h"
#include  "CATGetEnvValue.h"
#include  "CATIProduct.h"
#include "TCAXmlUtils.h"
#include "CATDLName.h"
#include "CATScriptUtilities.h"
#include <Python.h>
#include <iostream>
#include <cstring>

#include "string"
#include "iostream"
#include "cstdlib"

#include "TCAIUTLGo.h"
#include "CATAfrCommandHeaderServices.h"

#ifdef TCAUTLGPTCmd_ParameterEditorInclude
#include "CATIParameterEditorFactory.h"
#include "CATIParameterEditor.h"
#include "CATICkeParm.h"
#endif

#include "CATCreateExternalObject.h"
CATCreateClass(TCAUTLGPTCmd);

int StringCompare(CATUnicodeString* i_pParam1, CATUnicodeString* i_pParam2);
int TypeCompare(CATBaseUnknown_var* i_spObj1, CATBaseUnknown_var* i_spObj2);

#define CHROMA_MODULE "TCAPYAIChromDB1"

//-------------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------------
TCAUTLGPTCmd::TCAUTLGPTCmd(CATISpecObject_var i_spSpec) :
  CATDlgDialog(( CATApplicationFrame::GetApplicationFrame() )->GetMainWindow(),
               "TCAUTLGPTCmd", CATDlgGridLayout | CATDlgWndBtnOKCancel)
  , m_pCurrentDoc(NULL)
  , m_pQuestionEditor(NULL)
{
  m_OpenAI = FALSE;

  CATUnicodeString openAIKey;
  if( this->GetSystemVariable("OPENAI_API_KEY", openAIKey) )
  {
    m_OpenAI = TRUE;
    PyRun_SimpleString("import sys; sys.path.insert(0, 'C:\\EXAMPLES\\AI\\AskAI')");
  }
  RequestStatusChange(CATCommandMsgRequestExclusiveMode);
}

//-------------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------------
TCAUTLGPTCmd::~TCAUTLGPTCmd()
{
  m_pCurrentDoc = NULL;
  m_pQuestionEditor = NULL;
}


//-------------------------------------------------------------------------
// GetSystemVariable
//-------------------------------------------------------------------------
CATBoolean TCAUTLGPTCmd::GetSystemVariable(const CATUnicodeString& iSystemVariable, CATUnicodeString& oVarValue)
{
  CATBoolean retValue(FALSE);

  if( iSystemVariable != "" )
  {
    char* pTempChar = NULL;
    const char* varName = iSystemVariable.ConvertToChar();

    CATLibStatus status = ::CATGetEnvValue(varName, &pTempChar);

    if( status == CATLibSuccess )
    {
      retValue = TRUE;

      CATUnicodeString tempString(pTempChar);

      oVarValue = tempString;

    }
    if( NULL != pTempChar )
      free(pTempChar);
  }

  return retValue;
} // GetSystemVariable

//-------------------------------------------------------------------------
// Activate
//-------------------------------------------------------------------------
CATStatusChangeRC TCAUTLGPTCmd::Activate(CATCommand* iFromClient, CATNotification* iEvtDat)
{
  this->Build();


  // Notifications 
  this->AddAnalyseNotificationCB(this,
                                 this->GetDiaOKNotification(),
                                 (CATCommandMethod)&TCAUTLGPTCmd::askAI,
                                 NULL);

  this->AddAnalyseNotificationCB(this,
                                 this->GetDiaCANCELNotification(),
                                 (CATCommandMethod)&TCAUTLGPTCmd::Cancel,
                                 NULL);

  this->AddAnalyseNotificationCB(this,
                                 this->GetDiaCLOSENotification(),
                                 (CATCommandMethod)&TCAUTLGPTCmd::Cancel,
                                 NULL);

  this->SetVisibility(CATDlgShow);
  return ( CATStatusChangeRCCompleted );
} // Activate

//-------------------------------------------------------------------------
// Desactivate
//-------------------------------------------------------------------------
CATStatusChangeRC TCAUTLGPTCmd::Desactivate(CATCommand* iFromClient, CATNotification* iEvtDat)
{
  SetVisibility(CATDlgHide);
  return ( CATStatusChangeRCCompleted );
} // Desactivate

//-------------------------------------------------------------------------
// Cancel
//-------------------------------------------------------------------------
CATStatusChangeRC TCAUTLGPTCmd::Cancel(CATCommand* iFromClient, CATNotification* iEvtDat)
{
  this->SetVisibility(CATDlgHide);
  this->RequestDelayedDestruction();
  return( CATStatusChangeRCCompleted );
} // Cancel

//-------------------------------------------------------------------------
// Build
//-------------------------------------------------------------------------
void TCAUTLGPTCmd::Build(void)
{
  CATFrmEditor* pEditor = CATFrmEditor::GetCurrentEditor();
  if( pEditor )
  {
    m_pCurrentDoc = pEditor->GetDocument();
  }
  this->SetTitle("CATGPT");

  this->SetGridColumnResizable(0, 1);
  this->SetGridRowResizable(0, 1);

  // AI Tab
  if( m_OpenAI )
  {
    m_pQuestionEditor = new CATDlgEditor(this, "m_pQuestionEditor", CATDlgEdtMultiline);
    m_pQuestionEditor->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
    m_pQuestionEditor->SetVisibleTextWidth(30);
    m_pQuestionEditor->SetVisibleTextHeight(20);
  }
  this->SetDefaultButton(CATDlgWndOK);
  this->SetIconName("CATGPT");
  return;
} // Build

//-------------------------------------------------------------------------
// getUTF8Str
//-------------------------------------------------------------------------
CATUnicodeString TCAUTLGPTCmd::getUTF8Str(const CATUnicodeString& i_rStr)
{
  CATUnicodeString utf8Str;
  if( i_rStr.SearchSubString("Dassault") > -1 )
  {
    cerr << i_rStr << endl;
    return utf8Str;
  }
  size_t size4 = i_rStr.GetLengthInChar();
  for( int c = 0; c < size4; c++ )
  {
    CATUnicodeChar uc = i_rStr[c];

    if( ( uc.IsCntrl() || !uc.IsPrint() || !uc.IsAlnum() )
       && uc != CATUnicodeChar('\n') && uc != CATUnicodeChar(' ')
       && uc != CATUnicodeChar('(') && uc != CATUnicodeChar(')')
       && uc != CATUnicodeChar('[') && uc != CATUnicodeChar('[')
       && uc != CATUnicodeChar(',') && uc != CATUnicodeChar(';') && uc != CATUnicodeChar('.') )
    {
      // cerr << "Ctrl: '" << uc << "'" << endl;
    }
    else
    {
      CATUnicodeChar ucLow = uc;
      ucLow.ToLower();
      if( ucLow == CATUnicodeChar('ö') )
      {
        utf8Str.Append("oe");
      }
      else if( ucLow == CATUnicodeChar('ä') )
      {
        utf8Str.Append("ae");
      }
      else if( ucLow == CATUnicodeChar('ü') )
      {
        utf8Str.Append("ue");
      }
      else if( ucLow == CATUnicodeChar('ß') )
      {
        utf8Str.Append("ss");
      }
      else
      {
        utf8Str.Append(uc);
      }
    }
  }

  CATUnicodeString utf8StrToReturn;
  size4 = utf8Str.GetLengthInChar() * 4;
  if( size4 > 0 ) // [VTA-1617] Base64 from UTF-8 
  {
    /**
     * Convert the current string into a UTF-8 character string.
     * @param oUTF8String
     *   The resulting UTF-8 string
     *   It should be allocated as a table of
     *   4*(this->GetLengthInChar()) elements
     * @param oByteCount
     *   String length in byte count
     */
    char* pUTF8String = new char[size4];
    utf8Str.ConvertToUTF8(pUTF8String, &size4);
    if( pUTF8String )
    {
      utf8StrToReturn.BuildFromUTF8(pUTF8String, size4);
      delete[] pUTF8String;
    }
  }
  return utf8StrToReturn;
} // getUTF8Str

//-------------------------------------------------------------------------
// askAI
//-------------------------------------------------------------------------
void TCAUTLGPTCmd::askAI(CATCommand*, CATNotification*, CATCommandClientData i_Data)
{
  if( !m_pQuestionEditor )
  {
    return;
  }

  CATUnicodeString answerAI;
  cerr << __FUNCTION__ << endl;

  // Check Python version
  const char* version = Py_GetVersion();
  cerr << "Python version in use: " << version << endl;

  static PyObject* s_pMmodule = NULL;
  if( !s_pMmodule )
  {
    s_pMmodule = PyImport_ImportModule(CHROMA_MODULE);  // 2.AnalizeCATDoc
  }

  if( !s_pMmodule )
  {
    PyErr_Print();
    m_pQuestionEditor->SetText("Failed to load hello modul 'TCAPYAIChromDB'\n");
    std::cerr << "Failed to load hello s_pMmodule\n";
    return;
  }

  PyObject* s_pFunc = NULL;
  if( !s_pFunc )
  {
    s_pFunc = PyObject_GetAttrString(s_pMmodule, "askAI");  // 3. Get aksAI()
  }
  if( !s_pFunc || !PyCallable_Check(s_pFunc) )
  {
    PyErr_Print();
    m_pQuestionEditor->SetText("Cannot find function 'askAI'\n");
    std::cerr << "Cannot find function askAI\n";
    return;
  }

  CATUnicodeString  question = "Who are you?";
  if( m_pQuestionEditor )
  {
    CATUnicodeString str = this->getUTF8Str(m_pQuestionEditor->GetText());
    if( str != "" )
    {
      question = str;
    }
  }

  CATUnicodeString person("Albert Einstein");

  const char* cstrName = person.ConvertToChar();
  if( cstrName == nullptr )
  {
    std::cerr << "Error: Person string is null or invalid." << std::endl;
    return; // Or handle the error appropriately
  }

  // 4. Call greet("World")
  const char* cstr = question.ConvertToChar();
  if( cstr == nullptr )
  {
    std::cerr << "Error: The question string is null or invalid." << std::endl;
    return; // Or handle the error appropriately
  }


  const char* cstr3 = "not needed";
  if( cstr3 == nullptr )
  {
    std::cerr << "Error: m_DocUUID string is null or invalid." << std::endl;
    return; // Or handle the error appropriately
  }


  // Now safely call PyUnicode_FromString with a valid C string
  PyObject* arg1 = PyUnicode_Decode(cstr, strlen(cstr), "utf-8", "strict");
  PyObject* arg2 = PyUnicode_Decode(cstrName, strlen(cstrName), "utf-8", "strict");
  PyObject* arg3 = PyUnicode_Decode(cstr3, strlen(cstr3), "utf-8", "strict");
  // Check if PyUnicode_FromString succeeded
  if( arg1 == nullptr || arg2 == nullptr || arg3 == nullptr )
  {
    std::cerr << "Error: Failed to create Python Unicode object." << std::endl;
    return; // Or handle the error appropriately
  }

  // PyObject* args = PyTuple_Pack(1, arg1);
  PyObject* args = PyTuple_Pack(3, arg1, arg2, arg3);
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

  if( m_pQuestionEditor )
  {
    m_pQuestionEditor->SetText(answerAI);
  }

  answerAI.ReplaceAll(".", ",");
  CATListOfCATUnicodeString tokens;
  CATToken token(answerAI);
  CATUnicodeString sep(";");
  CATUnicodeString tok = token.GetNextToken(sep);
  while( tok != "" )
  {
    tokens.Append(tok);
    tok = token.GetNextToken(sep);
  }
  CATUnicodeString compName = answerAI;
  if( tokens.Size() > 0 )
  {
    compName = tokens[1];
    tokens.RemovePosition(1);
  }

  // get partnumber from AI
  CATUnicodeString partNumber;
  CATUnicodeString prompt("Welches dieser Bauteile wird benötigt, um die folgende Anfrage zu erfüllen? Gib nur die Nummer des Bauteils zurück. Anfrage: ");
  prompt.Append(question);
  // ask AI for vehicle part
  PyObject* pFunc1 = PyObject_GetAttrString(s_pMmodule, "askAIAboutPart");  // 3. Get aksAI() 
  if( !pFunc1 || !PyCallable_Check(pFunc1) )
  {
    PyErr_Print();
    m_pQuestionEditor->SetText("Cannot find function 'askAIAboutPart'\n");
    std::cerr << "Cannot find function askAIAboutPart\n";
    return;
  }
  else
  {
    CATUnicodeString str = this->getUTF8Str(prompt);
    const char* cstrPrompt = str.ConvertToChar();
    if( cstrPrompt == nullptr )
    {
      std::cerr << "Error: The prompt string is null or invalid: " << prompt << endl << std::endl;
      return; // Or handle the error appropriately
    }

    PyObject* argPrompt = PyUnicode_Decode(cstrPrompt, strlen(cstrPrompt), "utf-8", "strict");
    if( argPrompt == nullptr )
    {
      cerr << "FAILED PyUnicode_Decode: " << prompt << endl;
    }
    else if( compName == "UTLGoMeasureFenderChargingFlap" )
    {
      PyObject* argsPrompt = PyTuple_Pack(1, argPrompt);
      PyObject* result = PyObject_CallObject(pFunc1, argsPrompt);
      if( result )
      {
        CATUnicodeString partNumber;
        const char* result_utf8 = PyUnicode_AsUTF8(result);
        if( result_utf8 )
        {
          size_t utf8Length = strlen(result_utf8);
          partNumber.BuildFromUTF8(result_utf8, utf8Length);
        }
        std::cout << "Python function returned part number: " << partNumber << "\n";
        Py_DECREF(result);
        CATISpecObject_var spSpec = TCAUTLReframeOnSpec(partNumber);
        if( !!spSpec )
        {
          CATUnicodeString text("Requested element:\n\n");
          text.Append(TCAUTLGetAlias(spSpec));
          m_pQuestionEditor->SetText(text);
        }
      }
      else
      {
        PyErr_Print();
      }
    }
  } // get partnumber from AI

  TCAIUTLGo_var spGo = TCAUTLInst(compName);
  if( !!spGo )
  {
    spGo->setInput(partNumber);

    spGo->setInput(tokens);
    if( SUCCEEDED(spGo->run()) )
    {
      this->SetVisibility(CATDlgHide);
      this->RequestDelayedDestruction();
    }
  }
  else
  {
    CATCommand* pCmd = NULL;
    CATString cmdName(compName.ConvertToChar());
    CATAfrStartCommand(cmdName, pCmd);
    if( pCmd )
    {
      pCmd = NULL;
      this->RequestDelayedDestruction();
    }
  }
  return;
} // askAI
