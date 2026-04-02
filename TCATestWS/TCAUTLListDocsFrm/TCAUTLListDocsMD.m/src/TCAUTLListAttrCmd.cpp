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
#include "TCAUTLListAttrCmd.h"
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

#ifdef TCAUTLListAttrCmd_ParameterEditorInclude
#include "CATIParameterEditorFactory.h"
#include "CATIParameterEditor.h"
#include "CATICkeParm.h"
#endif

#include "CATCreateExternalObject.h"
CATCreateClass(TCAUTLListAttrCmd);

int StringCompare(CATUnicodeString* i_pParam1, CATUnicodeString* i_pParam2);
int TypeCompare(CATBaseUnknown_var* i_spObj1, CATBaseUnknown_var* i_spObj2);

// #define CHROMA_MODULE "TCAPYAIChromDB1" // for command execution
#define CHROMA_MODULE "TCAPYAIChromDB2" // for geometry analysis

//-------------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------------
TCAUTLListAttrCmd::TCAUTLListAttrCmd(CATISpecObject_var i_spSpec) :
  CATDlgDialog(( CATApplicationFrame::GetApplicationFrame() )->GetMainWindow(),
               "TCAUTLListAttrCmd", CATDlgGridLayout | CATDlgWndBtnOKApplyClose)
  , m_pCurrentDoc(NULL)
  , m_pQuestionEditor(NULL)
{
  RequestStatusChange(CATCommandMsgRequestSharedMode);


  m_IsBuilt = FALSE;

  m_OpenAI = FALSE;

  m_LastFound = 0;
  m_spSpec = i_spSpec;
  m_FirstTime = 1;
  m_ShowContSpecCallback = -1;
  this->initInterfaceDirectories();

  CATUnicodeString openAIKey;
  if( this->GetSystemVariable("OPENAI_API_KEY", openAIKey) )
  {
    m_OpenAI = TRUE;
    PyRun_SimpleString("import sys; sys.path.insert(0, 'C:\\EXAMPLES\\AI\\AskAI')");
  }
}

//-------------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------------
TCAUTLListAttrCmd::~TCAUTLListAttrCmd()
{
  m_pCurrentDoc = NULL;
  m_pQuestionEditor = NULL;
}

//-------------------------------------------------------------------------
// Activate
//-------------------------------------------------------------------------
CATStatusChangeRC TCAUTLListAttrCmd::Activate(CATCommand* iFromClient, CATNotification* iEvtDat)
{
  if( m_IsBuilt )
  {
    this->SetVisibility(CATDlgShow);
    return ( CATStatusChangeRCCompleted );
  }

  this->Build();


  // Notifications 
  this->AddAnalyseNotificationCB(this,
                                 this->GetDiaOKNotification(),
                                 (CATCommandMethod)&TCAUTLListAttrCmd::Cancel,
                                 NULL);

  this->AddAnalyseNotificationCB(this,
                                 this->GetDiaCANCELNotification(),
                                 (CATCommandMethod)&TCAUTLListAttrCmd::Cancel,
                                 NULL);

  this->AddAnalyseNotificationCB(this,
                                 this->GetDiaCLOSENotification(),
                                 (CATCommandMethod)&TCAUTLListAttrCmd::Cancel,
                                 NULL);

  this->AddAnalyseNotificationCB(this,
                                 this->GetDiaAPPLYNotification(),
                                 (CATCommandMethod)&TCAUTLListAttrCmd::AIHelp,
                                 NULL);

  this->SetVisibility(CATDlgShow);
  return ( CATStatusChangeRCCompleted );
} // Activate

//-------------------------------------------------------------------------
// Desactivate
//-------------------------------------------------------------------------
CATStatusChangeRC TCAUTLListAttrCmd::Desactivate(CATCommand* iFromClient, CATNotification* iEvtDat)
{
  SetVisibility(CATDlgHide);
  return ( CATStatusChangeRCCompleted );
} // Desactivate

//-------------------------------------------------------------------------
// Cancel
//-------------------------------------------------------------------------
CATStatusChangeRC TCAUTLListAttrCmd::Cancel(CATCommand* iFromClient, CATNotification* iEvtDat)
{
  this->SetVisibility(CATDlgHide);
  this->RequestDelayedDestruction();
  return( CATStatusChangeRCCompleted );
} // Cancel

//-------------------------------------------------------------------------
// Cancel
//-------------------------------------------------------------------------
CATStatusChangeRC TCAUTLListAttrCmd::AIHelp(CATCommand* iFromClient, CATNotification* iEvtDat)
{
  CATUnicodeString targetReportPath("https://flow.technia.com/playground/cd539af6-5a5a-46bc-b011-99c778529d8b");

  // open csv file with default application (e.g. excel.exe)
  TCHAR pszParseName[MAX_PATH];
  targetReportPath.ConvertToWChar(pszParseName);

  SHELLEXECUTEINFO shExecInfo;
  shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
  shExecInfo.fMask = NULL;
  shExecInfo.hwnd = NULL;
  shExecInfo.lpVerb = NULL;
  shExecInfo.lpFile = pszParseName;
  shExecInfo.lpParameters = NULL;
  shExecInfo.lpDirectory = NULL;
  shExecInfo.nShow = SW_MAXIMIZE;
  shExecInfo.hInstApp = NULL;
  ShellExecuteEx(&shExecInfo);

  return( CATStatusChangeRCCompleted );
} // Cancel

//-------------------------------------------------------------------------
// getSelectedElement
//-------------------------------------------------------------------------
CATISpecObject_var TCAUTLListAttrCmd::getSelectedElement(void)
{
  CATISpecObject_var spSelected = m_spSpec;
  if( !!spSelected )
    return spSelected;

  CATFrmEditor* pFrmEditor = CATFrmEditor::GetCurrentEditor();
  if( pFrmEditor )
  {
    CATCSO* pCSO = pFrmEditor->GetCSO();
    if( pCSO )
    {
      pCSO->InitElementList();
      CATPathElement* pPathElement = NULL;
      while( pPathElement = (CATPathElement*)pCSO->NextElement() )
      {
        spSelected = pPathElement->FindElement(IID_CATISpecObject);
        if( !!spSelected )
          break;
      }
    }
  }
  return spSelected;
} // getSelectedElement

//-------------------------------------------------------------------------
// Build
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::Build(void)
{
  m_IsBuilt = TRUE;

  this->SetAPPLYTitle("Ask AI");
  CATUnicodeString title;
  CATUnicodeString alias;
  CATFrmEditor* pEditor = CATFrmEditor::GetCurrentEditor();
  if( pEditor )
  {
    m_pCurrentDoc = pEditor->GetDocument();
  }

  m_spSpec = this->getSelectedElement();
  if( !!m_spSpec )
  {

    CATILinkableObject_var spLinkObj = m_spSpec;
    if( !!spLinkObj )
    {
      m_pCurrentDoc = spLinkObj->GetDocument();
    }

    CATUnicodeString type = m_spSpec->GetImpl()->IsA();
    CATIAlias_var spAlias = m_spSpec;
    if( !!spAlias )
    {
      alias = spAlias->GetAlias();
    }

    title = type + " '" + alias + "'";
    if( !m_spSpec->IsUpToDate() )
    {
      title.Append(" -> not uptodate!");
    }

    CATIContainer_var spCont = m_spSpec->GetFeatContainer();
    if( !!spCont )
    {
      title.Append(" in Container: ");
      title.Append(spCont->GetImpl()->IsA());
    }
  }

  if( !!m_pCurrentDoc )
  {
    CATUnicodeString storageName = m_pCurrentDoc->StorageName();
    storageName.ReplaceAll("\\", "\\\\");
  }

  this->SetTitle(title);

  this->SetGridColumnResizable(0, 1);
  this->SetGridRowResizable(0, 1);

  // pTabCont
  CATDlgTabContainer* pTabCont = new CATDlgTabContainer(this, "pTabCont");
  {
    pTabCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

    // AI Tab
    if( m_OpenAI )
    {
      CATDlgTabPage* pAITab = new CATDlgTabPage(pTabCont, "pInterfacesTab", CATDlgGridLayout);
      {
        pAITab->SetTitle("AI");
        pAITab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pAITab->SetGridRowResizable(0, 0);
        pAITab->SetGridRowResizable(1, 0);
        pAITab->SetGridRowResizable(2, 1);
        pAITab->SetGridColumnResizable(0, 1);

        CATDlgFrame* pAskFrame = new CATDlgFrame(pAITab, "pAskFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        {
          pAskFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
          pAskFrame->SetGridColumnResizable(1, 1);
          pAskFrame->SetGridRowResizable(0, 0);

          CATDlgPushButton* pAskAIButton = new CATDlgPushButton(pAskFrame, "pAskAIButton");
          pAskAIButton->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
          pAskAIButton->SetTitle(" ASK ");
          this->AddAnalyseNotificationCB(pAskAIButton, pAskAIButton->GetPushBActivateNotification(), (CATCommandMethod)&TCAUTLListAttrCmd::askAI, NULL);

          m_pPersonCombo = new CATDlgCombo(pAskFrame, "m_pPersonCombo", CATDlgCmbDropDown | CATDlgCmbEntry);
          m_pPersonCombo->SetGridConstraints(0, 1, 1, 1, CATGRID_4SIDES);

          m_pPersonCombo->SetLine("Albert Einstein");
          m_pPersonCombo->SetLine("Isaac Newton");
          m_pPersonCombo->SetLine("Karl Marx");
          m_pPersonCombo->SetLine("Donald Trump");
          m_pPersonCombo->SetLine("Elon Musk");

          CATDlgPushButton* pStoreDBButton = new CATDlgPushButton(pAskFrame, "pAskAIButton");
          pStoreDBButton->SetGridConstraints(0, 2, 1, 1, CATGRID_4SIDES);
          pStoreDBButton->SetTitle("Store to Database");
          this->AddAnalyseNotificationCB(pStoreDBButton, pStoreDBButton->GetPushBActivateNotification(), (CATCommandMethod)&TCAUTLListAttrCmd::storeDBBulk, NULL);
        }


        m_pQuestionEditor = new CATDlgEditor(pAITab, "m_pQuestionEditor", CATDlgEdtMultiline);
        m_pQuestionEditor->SetGridConstraints(1, 0, 1, 1, CATGRID_4SIDES);
        m_pQuestionEditor->SetVisibleTextWidth(50);
        m_pQuestionEditor->SetVisibleTextHeight(5);

        CATDlgFrame* pFrame = new CATDlgFrame(pAITab, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(2, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0, 1);
        pFrame->SetGridRowResizable(0, 1);

        this->buildAIContainer(pFrame);
      }
    } // AI Tab


    if( m_pCurrentDoc )
    {
      // pContainerTab
      CATDlgTabPage* pContainerTab = new CATDlgTabPage(pTabCont, "pComponentTab", CATDlgGridLayout);
      {
        pContainerTab->SetTitle("Containers");
        pContainerTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pContainerTab->SetGridRowResizable(0, 1);
        pContainerTab->SetGridColumnResizable(0, 0);
        pContainerTab->SetGridColumnResizable(1, 1);

        CATDlgFrame* pContFrame = new CATDlgFrame(pContainerTab, "pContFrame", CATDlgFraNoFrame | CATDlgGridLayout);
        {
          pContFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
          pContFrame->SetGridColumnResizable(0);
          this->showContainers(pContFrame, m_pCurrentDoc);
        }

        m_pContListFrame = new CATDlgFrame(pContainerTab, "pFrame", CATDlgGridLayout);
        m_pContListFrame->SetGridConstraints(0, 1, 1, 1, CATGRID_4SIDES);
        m_pContListFrame->SetGridColumnResizable(0);
        m_pContListFrame->SetGridRowResizable(0);

        // create new table
        m_pMultiListContainer = new CATDlgMultiList(m_pContListFrame, "multiListContainer");
        m_pMultiListContainer->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        m_pMultiListContainer->SetVisibleTextWidth(52);
        m_pMultiListContainer->SetColumnTextWidth(0, 20);
        m_pMultiListContainer->SetColumnTextWidth(1, 30);
        m_pMultiListContainer->SetColumnTextWidth(2, 20);

        // set names for 
        CATUnicodeString titleList[3];
        titleList[0] = "TYPE";
        titleList[1] = "ALIAS";
        titleList[2] = "NOT UPTODATE";

        m_pMultiListContainer->SetColumnTitles(3, titleList);

      } // pContainerTab

      // Document properties
      CATDlgTabPage* pDocPropertiesTab = new CATDlgTabPage(pTabCont, "pInterfacesTab", CATDlgGridLayout);
      {
        pDocPropertiesTab->SetTitle("Document");
        pDocPropertiesTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pDocPropertiesTab->SetGridRowResizable(0, 1);
        pDocPropertiesTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pDocPropertiesTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0, 1);
        pFrame->SetGridColumnResizable(1, 1);

        int yGridUuid = this->getDocUuid(m_pCurrentDoc, pFrame);
      } // Document properties

      // OMB-Properties
      CATDlgTabPage* pOmbPropertiesTab = new CATDlgTabPage(pTabCont, "pInterfacesTab", CATDlgGridLayout);
      {
        pOmbPropertiesTab->SetTitle("OMB Properties");
        pOmbPropertiesTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pOmbPropertiesTab->SetGridRowResizable(0, 1);
        pOmbPropertiesTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pOmbPropertiesTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0, 1);
        pFrame->SetGridColumnResizable(1, 1);

        CATIDocId* piDocId = NULL;
        HRESULT hr = m_pCurrentDoc->GetDocId(&piDocId);
        if( piDocId )
        {
          CATLISTV(CATUnicodeString) listProperties;
          hr = CATOmbDocPropertyServices::GetAllNames(piDocId, listProperties);
          for( int p = 1; p <= listProperties.Size(); p++ )
          {
            CATUnicodeString propValue;
            CATOmbDocPropertyServices::GetValue(piDocId, listProperties[p], propValue);
            this->buildFrameEditor(listProperties[p], propValue, pFrame, p - 1);

          }
        }
      } // OMB-Properties

      // Added Properties
      CATDlgTabPage* pAddedPropertiesTab = new CATDlgTabPage(pTabCont, "pInterfacesTab", CATDlgGridLayout);
      {
        pAddedPropertiesTab->SetTitle("Added Properties");
        pAddedPropertiesTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pAddedPropertiesTab->SetGridRowResizable(0, 1);
        pAddedPropertiesTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pAddedPropertiesTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showAddedProperties(pFrame, m_pCurrentDoc);
      } // Added Properties
    }

    if( !!m_spSpec )
    {
      // Attributes as XML
      CATDlgTabPage* pXMLTab = new CATDlgTabPage(pTabCont, "pInterfacesTab", CATDlgGridLayout);
      {
        pXMLTab->SetTitle("XML");
        pXMLTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pXMLTab->SetGridRowResizable(0, 1);
        pXMLTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pXMLTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0, 1);
        pFrame->SetGridColumnResizable(0, 1);

        this->showAttributesXML(pFrame);

      } // Attributes as XML 

      // pInterfacesTab
      CATDlgTabPage* pInterfacesTab = new CATDlgTabPage(pTabCont, "pInterfacesTab", CATDlgGridLayout);
      {
        pInterfacesTab->SetTitle("interfaces");
        pInterfacesTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pInterfacesTab->SetGridRowResizable(0, 1);
        pInterfacesTab->SetGridColumnResizable(0, 1);

        CATDlgFrame* pFrame = new CATDlgFrame(pInterfacesTab, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0, 0);
        pFrame->SetGridColumnResizable(1, 1);
        pFrame->SetGridColumnResizable(2, 0);
        pFrame->SetGridColumnResizable(3, 1);
        pFrame->SetGridRowResizable(1, 1);

        this->showInterfaces(pFrame, m_spSpec);
      } // pInterfacesTab 


      // pClassesTab
      CATDlgTabPage* pClassesTab = new CATDlgTabPage(pTabCont, "pClassesTab", CATDlgGridLayout);
      {
        pClassesTab->SetTitle("Classes");
        pClassesTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pClassesTab->SetGridRowResizable(0, 1);
        pClassesTab->SetGridColumnResizable(0, 1);

        CATDlgFrame* pFrame = new CATDlgFrame(pClassesTab, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0, 0);
        pFrame->SetGridColumnResizable(1, 1);
        pFrame->SetGridColumnResizable(2, 0);
        pFrame->SetGridColumnResizable(3, 1);
        pFrame->SetGridRowResizable(1, 1);

        this->showClasses(pFrame);
      } // pClassesTab 

      // pExternalTab
      CATDlgTabPage* pExternalTab = new CATDlgTabPage(pTabCont, "pExternalTab", CATDlgGridLayout);
      {
        pExternalTab->SetTitle("tk_external");
        pExternalTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pExternalTab->SetGridRowResizable(0, 1);
        pExternalTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pExternalTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showExternalAttr(pFrame, m_spSpec);
      } // pExternalTab 

      // pListExternalTab
      CATDlgTabPage* pListExternalTab = new CATDlgTabPage(pTabCont, "pListExternalTab", CATDlgGridLayout);
      {
        pListExternalTab->SetTitle("list of tk_external");
        pListExternalTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pListExternalTab->SetGridRowResizable(0, 1);
        pListExternalTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pListExternalTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showListExternalAttr(pFrame, m_spSpec);
      } // pListExternalTab 

      // pComponentTab
      CATDlgTabPage* pComponentTab = new CATDlgTabPage(pTabCont, "pComponentTab", CATDlgGridLayout);
      {
        pComponentTab->SetTitle("tk_component");
        pComponentTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pComponentTab->SetGridRowResizable(0, 1);
        pComponentTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pComponentTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showComponentAttr(pFrame, m_spSpec);
      } // pComponentTab 

      // pListComponentTab
      CATDlgTabPage* pListComponentTab = new CATDlgTabPage(pTabCont, "pComponentTab", CATDlgGridLayout);
      {
        pListComponentTab->SetTitle("tk_list of tk_component");
        pListComponentTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pListComponentTab->SetGridRowResizable(0, 1);
        pListComponentTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pListComponentTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showListComponentAttr(pFrame, m_spSpec);
      } // pListComponentTab

      // pSpecObjectTab
      CATDlgTabPage* pSpecObjectTab = new CATDlgTabPage(pTabCont, "pSpecObjectTab", CATDlgGridLayout);
      {
        pSpecObjectTab->SetTitle("tk_specobject");
        pSpecObjectTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pSpecObjectTab->SetGridRowResizable(0, 1);
        pSpecObjectTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pSpecObjectTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showSpecObjectAttr(pFrame, m_spSpec);
      } // pSpecObjectTab

      // pListSpecObjectTab
      CATDlgTabPage* pListSpecObjectTab = new CATDlgTabPage(pTabCont, "pComponentTab", CATDlgGridLayout);
      {
        pListSpecObjectTab->SetTitle("tk_list of tk_specobject");
        pListSpecObjectTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pListSpecObjectTab->SetGridRowResizable(0, 1);
        pListSpecObjectTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pListSpecObjectTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showListSpecObjectAttr(pFrame, m_spSpec);
      } // pListSpecObjectTab

      // pPointingTab
      CATDlgTabPage* pPointingTab = new CATDlgTabPage(pTabCont, "pComponentTab", CATDlgGridLayout);
      {
        pPointingTab->SetTitle("pointing");
        pPointingTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pPointingTab->SetGridRowResizable(0, 1);
        pPointingTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pPointingTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showPointingAttr(pFrame, m_spSpec);
      } // pPointingTab

      // pStringTab
      CATDlgTabPage* pStringTab = new CATDlgTabPage(pTabCont, "pStringTab", CATDlgGridLayout);
      {
        pStringTab->SetTitle("tk_string");
        pStringTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pStringTab->SetGridRowResizable(0, 1);
        pStringTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pStringTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showStringAttr(pFrame, m_spSpec);
      } // pStringTab

      // pIntegerTab
      CATDlgTabPage* pIntegerTab = new CATDlgTabPage(pTabCont, "pIntegerTab", CATDlgGridLayout);
      {
        pIntegerTab->SetTitle("tk_integer");
        pIntegerTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pIntegerTab->SetGridRowResizable(0, 1);
        pIntegerTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pIntegerTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showIntegerAttr(pFrame, m_spSpec);
      } // pIntegerTab

      // pDoubleTab
      CATDlgTabPage* pDoubleTab = new CATDlgTabPage(pTabCont, "pDoubleTab", CATDlgGridLayout);
      {
        pDoubleTab->SetTitle("tk_double");
        pDoubleTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pDoubleTab->SetGridRowResizable(0, 1);
        pDoubleTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pDoubleTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showDoubleAttr(pFrame, m_spSpec);
      } // pDoubleTab

      // pBooleanTab
      CATDlgTabPage* pBooleanTab = new CATDlgTabPage(pTabCont, "pBooleanTab", CATDlgGridLayout);
      {
        pBooleanTab->SetTitle("tk_boolean");
        pBooleanTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pBooleanTab->SetGridRowResizable(0, 1);
        pBooleanTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pBooleanTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showBooleanAttr(pFrame, m_spSpec);
      } // pBooleanTab

      // pListStringTab
      CATDlgTabPage* pListStringTab = new CATDlgTabPage(pTabCont, "pListStringTab", CATDlgGridLayout);
      {
        pListStringTab->SetTitle("tk_list of tk_string");
        pListStringTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pListStringTab->SetGridRowResizable(0, 1);
        pListStringTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pListStringTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showListStringAttr(pFrame, m_spSpec);
      } // pListStringTab

      // pListIntegerTab
      CATDlgTabPage* pListIntegerTab = new CATDlgTabPage(pTabCont, "pListIntegerTab", CATDlgGridLayout);
      {
        pListIntegerTab->SetTitle("tk_list of tk_integer");
        pListIntegerTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pListIntegerTab->SetGridRowResizable(0, 1);
        pListIntegerTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pListIntegerTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showListIntegerAttr(pFrame, m_spSpec);
      } // pListIntegerTab

      // pListDoubleTab
      CATDlgTabPage* pListDoubleTab = new CATDlgTabPage(pTabCont, "pListDoubleTab", CATDlgGridLayout);
      {
        pListDoubleTab->SetTitle("tk_list of tk_double");
        pListDoubleTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pListDoubleTab->SetGridRowResizable(0, 1);
        pListDoubleTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pListDoubleTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showListDoubleAttr(pFrame, m_spSpec);
      } // pListDoubleTab

      // pListBooleanTab
      CATDlgTabPage* pListBooleanTab = new CATDlgTabPage(pTabCont, "pListBooleanTab", CATDlgGridLayout);
      {
        pListBooleanTab->SetTitle("tk_list of tk_boolean");
        pListBooleanTab->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);

        pListBooleanTab->SetGridRowResizable(0, 1);
        pListBooleanTab->SetGridColumnResizable(0, 1);

        CATDlgContainer* pCont = new CATDlgContainer(pListBooleanTab, "Cont", CATDlgGridLayout);
        pCont->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pCont->SetRectDimensions(1, 1, 600, 600);

        CATDlgFrame* pFrame = new CATDlgFrame(pCont, "pFrame", CATDlgGridLayout | CATDlgFraNoFrame);
        pFrame->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetGridColumnResizable(0);

        this->showListBooleanAttr(pFrame, m_spSpec);
      } // pListBooleanTab


    }
  } // pTabCont

  return;
} // Build

//-------------------------------------------------------------------------
// showContainers
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showContainers(CATDlgFrame* i_pFrame, CATDocument* i_pDoc)
{
  if( !i_pDoc )
    return;

  CATIContainer_var spClientCont = i_pDoc->GetCompoundContainer();
  if( !!spClientCont )
  {
    int y = 0;
    SEQUENCE(CATBaseUnknown_ptr) ListObj;
    CATLONG32 cnt = spClientCont->ListMembersHere("CATIContainer", ListObj);
    for( int i = 0; i < cnt; i++ )
    {
      CATBaseUnknown* piObj = NULL;
      if( ListObj[i] && SUCCEEDED(ListObj[i]->QueryInterface(IID_CATBaseUnknown, (void**)&piObj)) )
      {
        CATUnicodeString contName = piObj->GetImpl()->IsA();
        CATDlgPushButton* pButton = new CATDlgPushButton(i_pFrame, "pButton");
        pButton->SetGridConstraints(y++, 0, 1, 1, CATGRID_4SIDES);
        pButton->SetTitle(contName);

        this->AddAnalyseNotificationCB(pButton, pButton->GetPushBActivateNotification(), (CATCommandMethod)&TCAUTLListAttrCmd::readContainer, piObj);

        piObj->Release();
        piObj = NULL;
      }
    }

    m_pContTypeCombo = new CATDlgCombo(i_pFrame, "m_pContTypeCombo", CATDlgCmbDropDown);
    m_pContTypeCombo->SetGridConstraints(y++, 0, 1, 1, CATGRID_4SIDES);
    m_pContTypeCombo->SetVisibleTextWidth(20);
    this->AddAnalyseNotificationCB(m_pContTypeCombo, m_pContTypeCombo->GetComboSelectNotification(), (CATCommandMethod)&TCAUTLListAttrCmd::typeSelected, NULL);

  }
  return;
} // showContainers

//-------------------------------------------------------------------------
// typeSelected
//------------------------------------------------------------------------- 
void TCAUTLListAttrCmd::typeSelected(CATCommand*, CATNotification*, CATCommandClientData data)
{
  int idx = m_pContTypeCombo->GetSelect();
  if( idx > -1 )
  {
    m_pContTypeCombo->GetLine(m_TypeSelected, idx);
    CATGeoFactory_var spGeoFact = m_spContainer;
    if( !!spGeoFact )
    {
      this->showGeoFactContainer(m_spContainer);
    }
    else if( !!m_spContainer )
    {
      this->showContainer(m_spContainer);
    }
  }
  return;
} // typeSelected

//-------------------------------------------------------------------------
// showStringAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showStringAttr(CATDlgFrame* i_pFrame, CATISpecObject_var i_spSpec)
{
  CATISpecAttrAccess_var spSpecAttrAcc = i_spSpec;
  if( !i_pFrame || spSpecAttrAcc == NULL_var )
    return;

  CATUnicodeString attrName;
  CATUnicodeString value;

  int yGrid = 0;

  CATListValCATISpecAttrKey_var attrKeys;
  HRESULT hr = spSpecAttrAcc->ListAttrKeys(attrKeys);
  for( int i = 1; i <= attrKeys.Size(); i++ )
  {
    CATISpecAttrKey_var spAttrKey = attrKeys[i];
    if( spAttrKey == NULL_var )
      continue;

    CATISpecAttrKey* piAttrKey = NULL;
    spAttrKey->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
    if( piAttrKey )
    {
      attrName = piAttrKey->GetName();
      if( !spSpecAttrAcc->IsUpToDate(piAttrKey) )
      {
        attrName.Append(" @");
      }
      CATAttrKind attrKind = piAttrKey->GetType();
      CATAttrKind attrKindList = piAttrKey->GetListType();

      if( attrKind == tk_string )
      {
        i_pFrame->SetGridRowResizable(yGrid, 1);

        CATDlgFrame* pFrame = new CATDlgFrame(i_pFrame, "pFrame", CATDlgGridLayout);
        pFrame->SetGridConstraints(yGrid++, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetTitle(attrName);

        pFrame->SetGridColumnResizable(0);
        pFrame->SetGridRowResizable(0);

        value = spSpecAttrAcc->GetString(piAttrKey);

        CATDlgEditor* pEditor = new CATDlgEditor(pFrame, "pEditor", CATDlgEdtMultiline);
        pEditor->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pEditor->SetVisibleTextWidth(50);
        pEditor->SetText(value);

        // Save Button
        CATDlgPushButton* pSaveButton = new CATDlgPushButton(pFrame, "pButton");
        pSaveButton->SetGridConstraints(0, 1, 1, 1, CATGRID_CENTER);
        pSaveButton->SetTitle("Save");

        this->AddAnalyseNotificationCB(pSaveButton, pSaveButton->GetPushBActivateNotification(),
                                       (CATCommandMethod)&TCAUTLListAttrCmd::saveStrAttr, piAttrKey);
      }
      piAttrKey->Release();
      piAttrKey = NULL;
    }
  }
  return;
} // showStringAttr

//-------------------------------------------------------------------------
// saveStrAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::saveStrAttr(CATCommand* i_pSaveBtn, CATNotification*, CATCommandClientData data)
{
  CATISpecAttrKey* piAttrKey = (CATISpecAttrKey*)data;
  CATDlgPushButton* pSaveButton = (CATDlgPushButton*)i_pSaveBtn;
  CATISpecAttrAccess_var spSpecAttrAcc = m_spSpec;
  if( piAttrKey && !!spSpecAttrAcc && pSaveButton )
  {
    CATUnicodeString attrName = piAttrKey->GetName();
    CATDlgFrame* pFrame = (CATDlgFrame*)pSaveButton->GetFather();
    if( pFrame )
    {
      CATDlgEditor* pEditor = (CATDlgEditor*)pFrame->GetChildFromChildNumber(0);
      if( pEditor )
      {
        CATUnicodeString strToSave = pEditor->GetText();
        spSpecAttrAcc->SetString(piAttrKey, strToSave);
      }
    }
  }
  return;
} // saveStrAttr

//-------------------------------------------------------------------------
// showIntegerAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showIntegerAttr(CATDlgFrame* i_pFrame,
                                        CATISpecObject_var i_spSpec)
{
  CATISpecAttrAccess_var spSpecAttrAcc = i_spSpec;
  if( !i_pFrame || spSpecAttrAcc == NULL_var )
    return;

  int yGrid = 0;

  CATListValCATISpecAttrKey_var attrKeys;
  HRESULT hr = spSpecAttrAcc->ListAttrKeys(attrKeys);
  for( int i = 1; i <= attrKeys.Size(); i++ )
  {
    CATISpecAttrKey_var spAttrKey = attrKeys[i];
    if( spAttrKey == NULL_var )
      continue;

    CATISpecAttrKey* piAttrKey = NULL;
    spAttrKey->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
    if( piAttrKey )
    {
      CATUnicodeString attrName = piAttrKey->GetName();
      if( !spSpecAttrAcc->IsUpToDate(piAttrKey) )
      {
        attrName.Append(" @");
      }

      CATAttrKind attrKind = piAttrKey->GetType();
      CATAttrKind attrKindList = piAttrKey->GetListType();

      if( attrKind == tk_integer )
      {
        i_pFrame->SetGridRowResizable(yGrid, 1);

        CATDlgFrame* pFrame = new CATDlgFrame(i_pFrame, "pFrame", CATDlgGridLayout);
        pFrame->SetGridConstraints(yGrid++, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetTitle(attrName);

        pFrame->SetGridColumnResizable(0);
        pFrame->SetGridRowResizable(0);

        int value = spSpecAttrAcc->GetInteger(piAttrKey);

        CATDlgEditor* pEditor = new CATDlgEditor(pFrame, "pEditor", CATDlgEdtInteger);
        pEditor->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pEditor->SetVisibleTextWidth(50);
        pEditor->SetIntegerValue(value);

      }

      piAttrKey->Release();
      piAttrKey = NULL;
    }
  }

  return;
} // showIntegerAttr

//-------------------------------------------------------------------------
// showDoubleAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showDoubleAttr(CATDlgFrame* i_pFrame,
                                       CATISpecObject_var i_spSpec)
{
  CATISpecAttrAccess_var spSpecAttrAcc = i_spSpec;
  if( !i_pFrame || spSpecAttrAcc == NULL_var )
    return;

  int yGrid = 0;

  CATListValCATISpecAttrKey_var attrKeys;
  HRESULT hr = spSpecAttrAcc->ListAttrKeys(attrKeys);
  for( int i = 1; i <= attrKeys.Size(); i++ )
  {
    CATISpecAttrKey_var spAttrKey = attrKeys[i];
    if( spAttrKey == NULL_var )
      continue;

    CATISpecAttrKey* piAttrKey = NULL;
    spAttrKey->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
    if( piAttrKey )
    {
      CATUnicodeString attrName = piAttrKey->GetName();
      if( !spSpecAttrAcc->IsUpToDate(piAttrKey) )
      {
        attrName.Append(" @");
      }
      CATAttrKind attrKind = piAttrKey->GetType();
      CATAttrKind attrKindList = piAttrKey->GetListType();

      if( attrKind == tk_double )
      {
        i_pFrame->SetGridRowResizable(yGrid, 1);

        CATDlgFrame* pFrame = new CATDlgFrame(i_pFrame, "pFrame", CATDlgGridLayout);
        pFrame->SetGridConstraints(yGrid++, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetTitle(attrName);

        pFrame->SetGridColumnResizable(0);
        pFrame->SetGridRowResizable(0);

        double value = spSpecAttrAcc->GetDouble(piAttrKey);

        CATDlgEditor* pEditor = new CATDlgEditor(pFrame, "pEditor", CATDlgEdtDouble);
        pEditor->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pEditor->SetVisibleTextWidth(50);
        pEditor->SetValue(value);


        CATUnicodeString attrValue;
        attrValue.BuildFromNum(value);
        attrName.ReplaceAll(" @", "");
      }

      piAttrKey->Release();
      piAttrKey = NULL;
    }
  }

  return;
} // showDoubleAttr

//-------------------------------------------------------------------------
// showBooleanAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showBooleanAttr(CATDlgFrame* i_pFrame,
                                        CATISpecObject_var i_spSpec)
{
  CATISpecAttrAccess_var spSpecAttrAcc = i_spSpec;
  if( !i_pFrame || spSpecAttrAcc == NULL_var )
    return;

  int yGrid = 0;

  CATListValCATISpecAttrKey_var attrKeys;
  HRESULT hr = spSpecAttrAcc->ListAttrKeys(attrKeys);
  for( int i = 1; i <= attrKeys.Size(); i++ )
  {
    CATISpecAttrKey_var spAttrKey = attrKeys[i];
    if( spAttrKey == NULL_var )
      continue;

    CATISpecAttrKey* piAttrKey = NULL;
    spAttrKey->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
    if( piAttrKey )
    {
      CATUnicodeString attrName = piAttrKey->GetName();
      if( !spSpecAttrAcc->IsUpToDate(piAttrKey) )
      {
        attrName.Append(" @");
      }
      CATAttrKind attrKind = piAttrKey->GetType();
      CATAttrKind attrKindList = piAttrKey->GetListType();

      if( attrKind == tk_boolean )
      {
        i_pFrame->SetGridRowResizable(yGrid, 1);

        CATDlgFrame* pFrame = new CATDlgFrame(i_pFrame, "pFrame", CATDlgGridLayout);
        pFrame->SetGridConstraints(yGrid++, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetTitle(attrName);

        pFrame->SetGridColumnResizable(0);
        pFrame->SetGridRowResizable(0);

        CATBoolean value = spSpecAttrAcc->GetBoolean(piAttrKey);

        CATDlgEditor* pEditor = new CATDlgEditor(pFrame, "pEditor");
        pEditor->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pEditor->SetVisibleTextWidth(50);
        pEditor->SetText(value ? "TRUE" : "FALSE");

      }

      piAttrKey->Release();
      piAttrKey = NULL;
    }
  }

  return;
} // showBooleanAttr

//-------------------------------------------------------------------------
// showListStringAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showListStringAttr(CATDlgFrame* i_pFrame,
                                           CATISpecObject_var i_spSpec)
{
  CATISpecAttrAccess_var spSpecAttrAcc = i_spSpec;
  if( !i_pFrame || spSpecAttrAcc == NULL_var )
    return;

  int xGrid = 0;

  CATListValCATISpecAttrKey_var attrKeys;
  HRESULT hr = spSpecAttrAcc->ListAttrKeys(attrKeys);
  for( int i = 1; i <= attrKeys.Size(); i++ )
  {
    CATISpecAttrKey_var spAttrKey = attrKeys[i];
    if( spAttrKey == NULL_var )
      continue;

    CATISpecAttrKey* piAttrKey = NULL;
    spAttrKey->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
    if( piAttrKey )
    {
      CATUnicodeString attrName = piAttrKey->GetName();
      if( !spSpecAttrAcc->IsUpToDate(piAttrKey) )
      {
        attrName.Append(" @");
      }
      CATAttrKind attrKind = piAttrKey->GetType();
      CATAttrKind attrKindList = piAttrKey->GetListType();

      if( attrKindList == tk_string )
      {
        i_pFrame->SetGridColumnResizable(xGrid, 1);


        int size = spSpecAttrAcc->GetListSize(piAttrKey);
        CATUnicodeString title;
        title.BuildFromNum(size);
        title.Append(" x ");
        title.Append(attrName);

        CATDlgFrame* pFrame = new CATDlgFrame(i_pFrame, "pFrame", CATDlgGridLayout);
        pFrame->SetGridConstraints(0, xGrid++, 1, 1, CATGRID_4SIDES);

        pFrame->SetTitle(title);

        pFrame->SetGridColumnResizable(0);
        pFrame->SetGridRowResizable(0);


        CATDlgMultiList* pMultiListStr = new CATDlgMultiList(pFrame, "multiListString");
        pMultiListStr->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pMultiListStr->SetVisibleTextWidth(30);
        pMultiListStr->SetColumnTextWidth(0, 28);
        pMultiListStr->SetVisibleLineCount(30);

        // list header
        CATUnicodeString titleList[1];
        titleList[0] = "Values";
        pMultiListStr->SetColumnTitles(1, titleList);

        for( int a = 0; a < size; a++ )
        {
          CATUnicodeString value = spSpecAttrAcc->GetString(piAttrKey, a + 1);
          pMultiListStr->SetColumnItem(0, value, -1, CATDlgDataAdd);
        }
      }

      piAttrKey->Release();
      piAttrKey = NULL;
    }
  }

  return;
} // showListStringAttr

//-------------------------------------------------------------------------
// showListIntegerAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showListIntegerAttr(CATDlgFrame* i_pFrame,
                                            CATISpecObject_var i_spSpec)
{
  CATISpecAttrAccess_var spSpecAttrAcc = i_spSpec;
  if( !i_pFrame || spSpecAttrAcc == NULL_var )
    return;

  int xGrid = 0;

  CATListValCATISpecAttrKey_var attrKeys;
  HRESULT hr = spSpecAttrAcc->ListAttrKeys(attrKeys);
  for( int i = 1; i <= attrKeys.Size(); i++ )
  {
    CATISpecAttrKey_var spAttrKey = attrKeys[i];
    if( spAttrKey == NULL_var )
      continue;

    CATISpecAttrKey* piAttrKey = NULL;
    spAttrKey->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
    if( piAttrKey )
    {
      CATUnicodeString attrName = piAttrKey->GetName();
      if( !spSpecAttrAcc->IsUpToDate(piAttrKey) )
      {
        attrName.Append(" @");
      }
      CATAttrKind attrKind = piAttrKey->GetType();
      CATAttrKind attrKindList = piAttrKey->GetListType();

      if( attrKindList == tk_integer )
      {
        i_pFrame->SetGridColumnResizable(xGrid, 1);

        int size = spSpecAttrAcc->GetListSize(piAttrKey);
        CATUnicodeString title;
        title.BuildFromNum(size);
        title.Append(" x ");
        title.Append(attrName);

        CATDlgFrame* pFrame = new CATDlgFrame(i_pFrame, "pFrame", CATDlgGridLayout);
        pFrame->SetGridConstraints(0, xGrid++, 1, 1, CATGRID_4SIDES);
        pFrame->SetTitle(title);

        pFrame->SetGridColumnResizable(0);
        pFrame->SetGridRowResizable(0);

        CATDlgMultiList* pMultiListInteger = new CATDlgMultiList(pFrame, "multiListContainer");
        pMultiListInteger->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pMultiListInteger->SetVisibleTextWidth(10);
        pMultiListInteger->SetColumnTextWidth(0, 9);
        pMultiListInteger->SetVisibleLineCount(30);

        // list header
        CATUnicodeString titleList[1];
        titleList[0] = "Values";
        pMultiListInteger->SetColumnTitles(1, titleList);

        for( int a = 0; a < size; a++ )
        {
          int value = spSpecAttrAcc->GetInteger(piAttrKey, a + 1);
          CATUnicodeString vlaueStr;
          vlaueStr.BuildFromNum(value);
          pMultiListInteger->SetColumnItem(0, vlaueStr, -1, CATDlgDataAdd);
        }
      }

      piAttrKey->Release();
      piAttrKey = NULL;
    }
  }

  return;
} // showListIntegerAttr

//-------------------------------------------------------------------------
// showListDoubleAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showListDoubleAttr(CATDlgFrame* i_pFrame,
                                           CATISpecObject_var i_spSpec)
{
  CATISpecAttrAccess_var spSpecAttrAcc = i_spSpec;
  if( !i_pFrame || spSpecAttrAcc == NULL_var )
    return;

  int xGrid = 0;

  CATListValCATISpecAttrKey_var attrKeys;
  HRESULT hr = spSpecAttrAcc->ListAttrKeys(attrKeys);
  for( int i = 1; i <= attrKeys.Size(); i++ )
  {
    CATISpecAttrKey_var spAttrKey = attrKeys[i];
    if( spAttrKey == NULL_var )
      continue;

    CATISpecAttrKey* piAttrKey = NULL;
    spAttrKey->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
    if( piAttrKey )
    {
      CATUnicodeString attrName = piAttrKey->GetName();
      if( !spSpecAttrAcc->IsUpToDate(piAttrKey) )
      {
        attrName.Append(" @");
      }
      CATAttrKind attrKind = piAttrKey->GetType();
      CATAttrKind attrKindList = piAttrKey->GetListType();

      if( attrKindList == tk_double )
      {
        int size = spSpecAttrAcc->GetListSize(piAttrKey);

        i_pFrame->SetGridRowResizable(0, 1);
        i_pFrame->SetGridColumnResizable(xGrid, 1);

        CATDlgFrame* pFrame = new CATDlgFrame(i_pFrame, "pFrame", CATDlgGridLayout);
        pFrame->SetGridConstraints(0, xGrid++, 1, 1, CATGRID_4SIDES);

        CATUnicodeString sizeStr;
        sizeStr.BuildFromNum(size);
        attrName.Append(": ");
        attrName.Append(sizeStr);

        pFrame->SetTitle(attrName);

        pFrame->SetGridColumnResizable(0, 1);
        pFrame->SetGridRowResizable(0, 1);

        CATDlgMultiList* pMultiListDouble = new CATDlgMultiList(pFrame, "multiListContainer");
        pMultiListDouble->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pMultiListDouble->SetVisibleTextWidth(10);
        pMultiListDouble->SetColumnTextWidth(0, 9);
        pMultiListDouble->SetVisibleLineCount(30);

        // list header
        CATUnicodeString titleList[1];
        titleList[0] = "Values";
        pMultiListDouble->SetColumnTitles(1, titleList);

        for( int a = 0; a < size; a++ )
        {
          double value = spSpecAttrAcc->GetDouble(piAttrKey, a + 1);
          CATUnicodeString vlaueStr;
          vlaueStr.BuildFromNum(value, "%.2f");
          pMultiListDouble->SetColumnItem(0, vlaueStr, -1, CATDlgDataAdd);
        }
      }
      piAttrKey->Release();
      piAttrKey = NULL;
    }
  }

  return;
} // showListDoubleAttr

//-------------------------------------------------------------------------
// showListBooleanAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showListBooleanAttr(CATDlgFrame* i_pFrame,
                                            CATISpecObject_var i_spSpec)
{
  CATISpecAttrAccess_var spSpecAttrAcc = i_spSpec;
  if( !i_pFrame || spSpecAttrAcc == NULL_var )
    return;

  int xGrid = 0;

  CATListValCATISpecAttrKey_var attrKeys;
  HRESULT hr = spSpecAttrAcc->ListAttrKeys(attrKeys);
  for( int i = 1; i <= attrKeys.Size(); i++ )
  {
    CATISpecAttrKey_var spAttrKey = attrKeys[i];
    if( spAttrKey == NULL_var )
      continue;

    CATISpecAttrKey* piAttrKey = NULL;
    spAttrKey->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
    if( piAttrKey )
    {
      CATUnicodeString attrName = piAttrKey->GetName();
      if( !spSpecAttrAcc->IsUpToDate(piAttrKey) )
      {
        attrName.Append(" @");
      }
      CATAttrKind attrKind = piAttrKey->GetType();
      CATAttrKind attrKindList = piAttrKey->GetListType();

      if( attrKindList == tk_boolean )
      {
        i_pFrame->SetGridColumnResizable(xGrid, 1);

        int size = spSpecAttrAcc->GetListSize(piAttrKey);
        CATUnicodeString title;
        title.BuildFromNum(size);
        title.Append(" x ");
        title.Append(attrName);


        CATDlgFrame* pFrame = new CATDlgFrame(i_pFrame, "pFrame", CATDlgGridLayout);
        pFrame->SetGridConstraints(0, xGrid++, 1, 1, CATGRID_4SIDES);
        pFrame->SetTitle(title);

        pFrame->SetGridColumnResizable(0);
        pFrame->SetGridRowResizable(0);


        CATDlgMultiList* pMultiListBoolean = new CATDlgMultiList(pFrame, "multiListContainer");
        pMultiListBoolean->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
        pMultiListBoolean->SetVisibleTextWidth(7);
        pMultiListBoolean->SetColumnTextWidth(0, 5);
        pMultiListBoolean->SetVisibleLineCount(30);

        // list header
        CATUnicodeString titleList[1];
        titleList[0] = "Values";
        pMultiListBoolean->SetColumnTitles(1, titleList);

        for( int a = 0; a < size; a++ )
        {
          CATBoolean value = spSpecAttrAcc->GetBoolean(piAttrKey, a + 1);
          CATUnicodeString vlaueStr = "FALSE";
          if( value )
          {
            vlaueStr = "TRUE";
          }
          pMultiListBoolean->SetColumnItem(0, vlaueStr, -1, CATDlgDataAdd);
        }
      }

      piAttrKey->Release();
      piAttrKey = NULL;
    }
  }

  return;
} // showListBooleanAttr

//-------------------------------------------------------------------------
// showSpecObjectAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showSpecObjectAttr(CATDlgFrame* i_pFrame,
                                           CATISpecObject_var i_spSpec)
{
  CATISpecAttrAccess_var spSpecAttrAcc = i_spSpec;
  if( !i_pFrame || spSpecAttrAcc == NULL_var )
    return;

  int yGrid = 0;

  CATListValCATISpecAttrKey_var attrKeys;
  HRESULT hr = spSpecAttrAcc->ListAttrKeys(attrKeys);
  for( int i = 1; i <= attrKeys.Size(); i++ )
  {
    CATISpecAttrKey_var spAttrKey = attrKeys[i];
    if( spAttrKey == NULL_var )
      continue;

    CATISpecAttrKey* piAttrKey = NULL;
    spAttrKey->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
    if( piAttrKey )
    {
      CATUnicodeString attrName = piAttrKey->GetName();
      if( !spSpecAttrAcc->IsUpToDate(piAttrKey) )
      {
        attrName.Append(" @");
      }
      CATAttrKind attrKind = piAttrKey->GetType();
      CATAttrKind attrKindList = piAttrKey->GetListType();

      if( attrKind == tk_specobject )
      {
        i_pFrame->SetGridRowResizable(yGrid, 1);

        CATDlgLabel* pLabel = new CATDlgLabel(i_pFrame, "pFrame");
        pLabel->SetGridConstraints(yGrid, 0, 1, 1, CATGRID_4SIDES);
        pLabel->SetTitle(attrName);

        CATISpecObject* piSpec = spSpecAttrAcc->GetSpecObject(piAttrKey);
        if( piSpec )
        {
          CATDlgEditor* pEditor = new CATDlgEditor(i_pFrame, "pEditor");
          pEditor->SetGridConstraints(yGrid, 1, 1, 1, CATGRID_4SIDES);
          pEditor->SetVisibleTextWidth(20);

          CATUnicodeString type = piSpec->GetImpl()->IsA();
          pEditor->SetText(type);

          CATDlgPushButton* pButton = new CATDlgPushButton(i_pFrame, "pButton");
          pButton->SetGridConstraints(yGrid, 2, 1, 1, CATGRID_4SIDES);

          // sp_IN/OUT/NETRAL & not uptodate
          this->setUpdateFacetLabel(i_pFrame, yGrid, 3, piAttrKey, piSpec);

          CATUnicodeString alias("undefined");
          CATIAlias_var spAlias = piSpec;
          if( !!spAlias )
            alias = spAlias->GetAlias();

          CATUnicodeString title = type + " " + alias;
          pButton->SetTitle(alias);

          attrName.ReplaceAll(" @", "");


          m_ListSpec.Append(piSpec);
          int idx = m_ListSpec.Size();

          this->AddAnalyseNotificationCB(pButton,
                                         pButton->GetPushBActivateNotification(),
                                         (CATCommandMethod)&TCAUTLListAttrCmd::showSpec,
                                         CATINT32ToPtr(idx));

          piSpec->Release();
          piSpec = NULL;
        }
        yGrid++;
      }

      piAttrKey->Release();
      piAttrKey = NULL;
    }
  }

  return;
} // showSpecObjectAttr

//-------------------------------------------------------------------------
// showComponentAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showComponentAttr(CATDlgFrame* i_pFrame,
                                          CATISpecObject_var i_spSpec)
{
  CATISpecAttrAccess_var spSpecAttrAcc = i_spSpec;
  if( !i_pFrame || spSpecAttrAcc == NULL_var )
    return;

  int yGrid = 0;

  CATListValCATISpecAttrKey_var attrKeys;
  HRESULT hr = spSpecAttrAcc->ListAttrKeys(attrKeys);
  for( int i = 1; i <= attrKeys.Size(); i++ )
  {
    CATISpecAttrKey_var spAttrKey = attrKeys[i];
    if( spAttrKey == NULL_var )
      continue;

    CATISpecAttrKey* piAttrKey = NULL;
    spAttrKey->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
    if( piAttrKey )
    {
      CATUnicodeString attrName = piAttrKey->GetName();
      if( !spSpecAttrAcc->IsUpToDate(piAttrKey) )
      {
        attrName.Append(" @");
      }
      CATAttrKind attrKind = piAttrKey->GetType();
      CATAttrKind attrKindList = piAttrKey->GetListType();

      if( attrKind == tk_component )
      {
        i_pFrame->SetGridRowResizable(yGrid, 1);

        CATDlgLabel* pLabel = new CATDlgLabel(i_pFrame, "pFrame");
        pLabel->SetGridConstraints(yGrid, 0, 1, 1, CATGRID_4SIDES);
        pLabel->SetTitle(attrName);

        CATISpecObject* piSpec = spSpecAttrAcc->GetSpecObject(piAttrKey);
        if( piSpec )
        {
          CATDlgEditor* pEditor = new CATDlgEditor(i_pFrame, "pEditor");
          pEditor->SetGridConstraints(yGrid, 1, 1, 1, CATGRID_4SIDES);
          pEditor->SetVisibleTextWidth(20);

          CATUnicodeString type = piSpec->GetImpl()->IsA();
          pEditor->SetText(type);

          CATDlgPushButton* pButton = new CATDlgPushButton(i_pFrame, "pButton");
          pButton->SetGridConstraints(yGrid, 2, 1, 1, CATGRID_4SIDES);

          // sp_IN/OUT/NETRAL & not uptodate
          this->setUpdateFacetLabel(i_pFrame, yGrid, 3, piAttrKey, piSpec);

          CATUnicodeString alias("undefined");
          CATIAlias_var spAlias = piSpec;
          if( !!spAlias )
            alias = spAlias->GetAlias();

          CATUnicodeString title = type + " " + alias;
          pButton->SetTitle(alias);

          attrName.ReplaceAll(" @", "");

          m_ListSpec.Append(piSpec);
          int idx = m_ListSpec.Size();

          this->AddAnalyseNotificationCB(pButton,
                                         pButton->GetPushBActivateNotification(),
                                         (CATCommandMethod)&TCAUTLListAttrCmd::showSpec,
                                         CATINT32ToPtr(idx));

          piSpec->Release();
          piSpec = NULL;
        }
        yGrid++;
      }

      piAttrKey->Release();
      piAttrKey = NULL;
    }
  }

  return;
} // showComponentAttr

//-------------------------------------------------------------------------
// showListComponentAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showListComponentAttr(CATDlgFrame* i_pFrame,
                                              CATISpecObject_var i_spSpec)
{
  CATISpecAttrAccess_var spSpecAttrAcc = i_spSpec;
  if( !i_pFrame || spSpecAttrAcc == NULL_var )
    return;

  int yGrid = 0;

  CATListValCATISpecAttrKey_var attrKeys;
  HRESULT hr = spSpecAttrAcc->ListAttrKeys(attrKeys);
  for( int i = 1; i <= attrKeys.Size(); i++ )
  {
    CATISpecAttrKey_var spAttrKey = attrKeys[i];
    if( spAttrKey == NULL_var )
      continue;

    CATISpecAttrKey* piAttrKey = NULL;
    spAttrKey->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
    if( piAttrKey )
    {
      CATUnicodeString attrName = piAttrKey->GetName();
      if( !spSpecAttrAcc->IsUpToDate(piAttrKey) )
      {
        attrName.Append(" @");
      }
      CATAttrKind attrKind = piAttrKey->GetType();
      CATAttrKind attrKindList = piAttrKey->GetListType();

      if( attrKindList == tk_component )
      {
        i_pFrame->SetGridRowResizable(yGrid, 1);

        CATDlgFrame* pFrame = new CATDlgFrame(i_pFrame, "pFrame", CATDlgGridLayout);
        pFrame->SetGridConstraints(yGrid++, 0, 1, 1, CATGRID_4SIDES);
        {
          pFrame->SetGridRowResizable(0, 1);

          int size = spSpecAttrAcc->GetListSize(piAttrKey);
          CATUnicodeString sizeStr;
          sizeStr.BuildFromNum(size);
          attrName.Append(": ");
          attrName.Append(sizeStr);
          attrName.Append(" elements");
          pFrame->SetTitle(attrName);

          if( size == 0 )
          {
            CATDlgLabel* pLabel = new CATDlgLabel(pFrame, "pFrame");
            pLabel->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
            pLabel->SetTitle("empty");
          }
          else
          {
            // create new table
            CATDlgMultiList* pMultiListComponents = new CATDlgMultiList(pFrame, "multiListContainer");
            pMultiListComponents->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
            pMultiListComponents->SetVisibleTextWidth(80);
            pMultiListComponents->SetColumnTextWidth(0, 20);
            pMultiListComponents->SetColumnTextWidth(1, 30);
            pMultiListComponents->SetColumnTextWidth(2, 20);

            // set notification
            int idx = m_ListSpec.Size() + 1;
            this->AddAnalyseNotificationCB(pMultiListComponents, pMultiListComponents->GetListActivateNotification(), (CATCommandMethod)&TCAUTLListAttrCmd::onMultiListComponents, CATINT32ToPtr(idx));

            // set names for 
            CATUnicodeString titleList[3];
            titleList[0] = "TYPE";
            titleList[1] = "ALIAS";
            titleList[2] = "UPDATE STATUS";

            pMultiListComponents->SetColumnTitles(3, titleList);

            int visibleCnt = size + 1;
            if( visibleCnt > 20 )
            {
              visibleCnt = 20;
            }
            pMultiListComponents->SetVisibleLineCount(visibleCnt);

            for( int a = 0; a < size; a++ )
            {
              CATISpecObject* piSpec = spSpecAttrAcc->GetSpecObject(piAttrKey, a + 1);
              if( NULL != piSpec )
              {
                CATUnicodeString type = piSpec->GetImpl()->IsA();


                CATUnicodeString alias("undefined");
                CATIAlias_var spAlias = piSpec;
                if( !!spAlias )
                  alias = spAlias->GetAlias();

                m_ListSpec.Append(piSpec);
                int idx = m_ListSpec.Size();


                pMultiListComponents->SetColumnItem(0, type, -1, CATDlgDataAdd);
                pMultiListComponents->SetColumnItem(1, alias, -1, CATDlgDataAdd);

                if( !piSpec->IsUpToDate() )
                {
                  pMultiListComponents->SetColumnItem(2, "NOT UPTODATE", -1, CATDlgDataAdd);
                }
                else
                {
                  pMultiListComponents->SetColumnItem(2, "ok", -1, CATDlgDataAdd);
                }

                piSpec->Release();
                piSpec = NULL;
              }
            }
          }
        }

        piAttrKey->Release();
        piAttrKey = NULL;
      }
    }
  }
  return;
} // showListComponentAttr

//-------------------------------------------------------------------------
// onMultiListComponents
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::onMultiListComponents(CATCommand* i_Cmd, CATNotification*, CATCommandClientData i_Data)
{
  int idx1st = CATPtrToINT32(i_Data);
  CATDlgMultiList* pMultiListComponents = (CATDlgMultiList*)i_Cmd;
  if( idx1st > 0 && pMultiListComponents )
  {
    int idxSelected = -1;
    pMultiListComponents->GetSelect(&idxSelected, 1);
    if( idxSelected >= 0 )
    {
      idxSelected += idx1st;
      if( idxSelected <= m_ListSpec.Size() )
      {
        CATISpecObject_var spSpec = m_ListSpec[idxSelected];
        if( !!spSpec )
        {
          TCAUTLListAttrCmd* pCom = new TCAUTLListAttrCmd(spSpec);
          // pCom->Activate(NULL, NULL);
        }
      }
    }
  }
  return;
} // onMultiListComponents

//-------------------------------------------------------------------------
// showListSpecObjectAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showListSpecObjectAttr(CATDlgFrame* i_pFrame, CATISpecObject_var i_spSpec)
{
  CATISpecAttrAccess_var spSpecAttrAcc = i_spSpec;
  if( !i_pFrame || spSpecAttrAcc == NULL_var )
    return;

  int yGrid = 0;

  CATListValCATISpecAttrKey_var attrKeys;
  HRESULT hr = spSpecAttrAcc->ListAttrKeys(attrKeys);
  for( int i = 1; i <= attrKeys.Size(); i++ )
  {
    CATISpecAttrKey_var spAttrKey = attrKeys[i];
    if( spAttrKey == NULL_var )
      continue;

    CATISpecAttrKey* piAttrKey = NULL;
    spAttrKey->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
    if( piAttrKey )
    {
      CATUnicodeString attrName = piAttrKey->GetName();
      if( !spSpecAttrAcc->IsUpToDate(piAttrKey) )
      {
        attrName.Append(" @");
      }
      CATAttrKind attrKind = piAttrKey->GetType();
      CATAttrKind attrKindList = piAttrKey->GetListType();

      if( attrKindList == tk_specobject )
      {
        i_pFrame->SetGridRowResizable(yGrid, 1);

        CATDlgFrame* pFrame = new CATDlgFrame(i_pFrame, "pFrame", CATDlgGridLayout);
        pFrame->SetGridConstraints(yGrid++, 0, 1, 1, CATGRID_4SIDES);
        {
          pFrame->SetGridRowResizable(0, 1);

          int size = spSpecAttrAcc->GetListSize(piAttrKey);
          CATUnicodeString sizeStr;
          sizeStr.BuildFromNum(size);
          attrName.Append(": ");
          attrName.Append(sizeStr);
          attrName.Append(" elements");
          pFrame->SetTitle(attrName);

          if( size == 0 )
          {
            CATDlgLabel* pLabel = new CATDlgLabel(pFrame, "pFrame");
            pLabel->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
            pLabel->SetTitle("empty");
          }
          else
          {
            // create new table
            CATDlgMultiList* pMultiListComponents = new CATDlgMultiList(pFrame, "multiListContainer");
            pMultiListComponents->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
            pMultiListComponents->SetVisibleTextWidth(80);
            pMultiListComponents->SetColumnTextWidth(0, 20);
            pMultiListComponents->SetColumnTextWidth(1, 30);
            pMultiListComponents->SetColumnTextWidth(2, 20);

            // set notification
            int idx = m_ListSpec.Size() + 1;
            this->AddAnalyseNotificationCB(pMultiListComponents, pMultiListComponents->GetListActivateNotification(), (CATCommandMethod)&TCAUTLListAttrCmd::onMultiListComponents, CATINT32ToPtr(idx));

            // set names for 
            CATUnicodeString titleList[3];
            titleList[0] = "TYPE";
            titleList[1] = "ALIAS";
            titleList[2] = "UPDATE STATUS";

            pMultiListComponents->SetColumnTitles(3, titleList);

            int visibleCnt = size + 1;
            if( visibleCnt > 20 )
            {
              visibleCnt = 20;
            }
            pMultiListComponents->SetVisibleLineCount(visibleCnt);

            for( int a = 0; a < size; a++ )
            {
              CATISpecObject* piSpec = spSpecAttrAcc->GetSpecObject(piAttrKey, a + 1);
              if( NULL != piSpec )
              {
                CATUnicodeString type = piSpec->GetImpl()->IsA();

                // sp_IN/OUT/NETRAL & not uptodate
                // this->setUpdateFacetLabel( pFrame, a, 3, piAttrKey, piSpec );

                CATUnicodeString alias("undefined");
                CATIAlias_var spAlias = piSpec;
                if( !!spAlias )
                  alias = spAlias->GetAlias();

                m_ListSpec.Append(piSpec);
                int idx = m_ListSpec.Size();

                /*
                this->AddAnalyseNotificationCB( pButton,
                pButton->GetPushBActivateNotification(),
                (CATCommandMethod)&TCAUTLListAttrCmd::showSpec,
                CATINT32ToPtr( idx ) );*/


                pMultiListComponents->SetColumnItem(0, type, -1, CATDlgDataAdd);
                pMultiListComponents->SetColumnItem(1, alias, -1, CATDlgDataAdd);

                if( !piSpec->IsUpToDate() )
                {
                  pMultiListComponents->SetColumnItem(2, "NOT UPTODATE", -1, CATDlgDataAdd);
                }
                else
                {
                  pMultiListComponents->SetColumnItem(2, "ok", -1, CATDlgDataAdd);
                }

                piSpec->Release();
                piSpec = NULL;
              }
            }
          }
        }
      }

      piAttrKey->Release();
      piAttrKey = NULL;
    }
  }

  return;
} // showListSpecObjectAttr

//-------------------------------------------------------------------------
// showPointingAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showPointingAttr(CATDlgFrame* i_pFrame,
                                         CATISpecObject_var i_spSpec)
{
  if( !i_pFrame || i_spSpec == NULL_var )
    return;

  int yGrid = 0;

  CATListPtrCATSpecPointing listOfAttr;
  i_spSpec->ListPointing(listOfAttr);

  for( int i = 1; i <= listOfAttr.Size(); i++ )
  {
    CATSpecPointing* pSpecPointing = listOfAttr[i];
    if( pSpecPointing == NULL )
      continue;

    CATISpecAttrKey_var spAttrKey = pSpecPointing->GetKey();
    if( spAttrKey == NULL_var )
      continue;

    CATISpecObject_var spSpec = pSpecPointing->GetAccess();
    if( spSpec == NULL_var )
      continue;

    CATUnicodeString attrName = spAttrKey->GetName();
    CATAttrKind attrKind = spAttrKey->GetType();
    CATAttrKind attrKindList = spAttrKey->GetListType();

    i_pFrame->SetGridRowResizable(yGrid, 1);

    CATDlgLabel* pLabel = new CATDlgLabel(i_pFrame, "pFrame");
    pLabel->SetGridConstraints(yGrid, 0, 1, 1, CATGRID_4SIDES);
    pLabel->SetTitle(attrName);

    CATDlgEditor* pEditor = new CATDlgEditor(i_pFrame, "pEditor");
    pEditor->SetGridConstraints(yGrid, 1, 1, 1, CATGRID_4SIDES);
    pEditor->SetVisibleTextWidth(20);

    CATUnicodeString type = spSpec->GetImpl()->IsA();
    pEditor->SetText(type);

    CATDlgPushButton* pButton = new CATDlgPushButton(i_pFrame, "pButton");
    pButton->SetGridConstraints(yGrid, 2, 1, 1, CATGRID_4SIDES);

    CATUnicodeString alias("undefined");
    CATIAlias_var spAlias = spSpec;
    if( !!spAlias )
      alias = spAlias->GetAlias();

    CATUnicodeString title = type + " " + alias;
    pButton->SetTitle(alias);

    m_ListSpec.Append(spSpec);
    int idx = m_ListSpec.Size();

    this->AddAnalyseNotificationCB(pButton,
                                   pButton->GetPushBActivateNotification(),
                                   (CATCommandMethod)&TCAUTLListAttrCmd::showSpec,
                                   CATINT32ToPtr(idx));

    yGrid++;
  }

  return;
} // showPointingAttr


//-------------------------------------------------------------------------
// showContainer
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showContainer(CATBaseUnknown_var i_spUnk)
{
  if( m_pMultiListContainer )
  {
    m_pMultiListContainer->ClearLine();
  }

  if( i_spUnk == NULL_var )
    return;

  int yGrid = 0;

  CATIContainer_var spClientCont = i_spUnk;
  if( spClientCont == NULL_var )
    return;

  CATUnicodeString contType = spClientCont->GetImpl()->IsA();

  SEQUENCE(CATBaseUnknown_ptr) ListObj;
  CATLONG32 NbObj = spClientCont->ListMembersHere("IUnknown", ListObj);


  m_ListOfContainerObjects.RemoveAll();
  for( int i = 0; i < NbObj; i++ )
  {
    CATBaseUnknown* piSpec = NULL;
    if( ListObj[i] && SUCCEEDED(ListObj[i]->QueryInterface(IID_CATBaseUnknown, (void**)&piSpec)) )
    {
      if( m_TypeSelected == "" || piSpec->GetImpl()->IsAKindOf(m_TypeSelected) )
      {
        m_ListOfContainerObjects.Append(piSpec);
      }
      piSpec->Release();
      piSpec = NULL;
    }
  }

  m_ListOfContainerObjects.QuickSort(&TypeCompare);

  int cnt1 = m_ListOfContainerObjects.Size();
  CATUnicodeString objCount;
  objCount.BuildFromNum(cnt1);
  objCount.Append(" elements in ");
  objCount.Append(i_spUnk->GetImpl()->IsA());
  m_pContListFrame->SetTitle(objCount);

  for( int j = 1; j <= m_ListOfContainerObjects.Size(); j++ )
  {
    CATBaseUnknown_var spUnk = m_ListOfContainerObjects[j];
    if( !!spUnk )
    {
      CATUnicodeString type = spUnk->GetImpl()->IsA();
      CATUnicodeString alias("undefined");
      CATIAlias_var spAlias = spUnk;
      if( !!spAlias )
      {
        alias = spAlias->GetAlias();
      }
      if( m_pMultiListContainer )
      {
        m_pMultiListContainer->SetColumnItem(0, type, -1, CATDlgDataAdd);
        m_pMultiListContainer->SetColumnItem(1, alias, -1, CATDlgDataAdd);
        CATISpecObject_var spSpec = spUnk;
        if( !!spSpec && !spSpec->IsUpToDate() )
        {
          m_pMultiListContainer->SetColumnItem(2, "NOT UPTODATE", -1, CATDlgDataAdd);
        }
      }
    }
  }
  // set notification
  if( m_ShowContSpecCallback == -1 )
  {
    m_ShowContSpecCallback = this->AddAnalyseNotificationCB(m_pMultiListContainer, m_pMultiListContainer->GetListActivateNotification(), (CATCommandMethod)&TCAUTLListAttrCmd::ShowContainerSpec, CATINT32ToPtr(0));
  }
  return;
} // showContainer

//---------------------------------------------------------------------------
// TypeCompare
//---------------------------------------------------------------------------
int TypeCompare(CATBaseUnknown_var* i_spObj1, CATBaseUnknown_var* i_spObj2)
{
  int returnValue = 0;
  if( i_spObj1 && i_spObj2 )
  {
    CATUnicodeString type1 = ( *i_spObj1 )->GetImpl()->IsA();
    CATUnicodeString type2 = ( *i_spObj2 )->GetImpl()->IsA();
    if( type1 < type2 )
    {
      returnValue = -1;
    }
    else if( type1 > type2 )
    {
      returnValue = 1;
    }
    else
    {
      CATUnicodeString  alias1;
      CATUnicodeString  alias2;
      CATIAlias_var spAlias1 = ( *i_spObj1 );
      CATIAlias_var spAlias2 = ( *i_spObj2 );
      if( !!spAlias1 && !!spAlias2 )
      {
        alias1 = spAlias1->GetAlias();
        alias2 = spAlias2->GetAlias();
      }
      if( alias1 < alias2 )
      {
        returnValue = -1;
      }
      else if( alias1 > alias2 )
      {
        returnValue = 1;
      }
    }
  }
  return returnValue;
} // TypeCompare

//-------------------------------------------------------------------------
// showExternalAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showExternalAttr(CATDlgFrame* i_pFrame,
                                         CATISpecObject_var i_spSpec,
                                         int i_YGrid)
{
  CATISpecAttrAccess_var spSpecAttrAcc = i_spSpec;
  if( !i_pFrame || spSpecAttrAcc == NULL_var )
    return;

  int yGrid = i_YGrid;

  CATListValCATISpecAttrKey_var attrKeys;
  HRESULT hr = spSpecAttrAcc->ListAttrKeys(attrKeys);
  for( int i = 1; i <= attrKeys.Size(); i++ )
  {
    CATISpecAttrKey_var spAttrKey = attrKeys[i];
    if( spAttrKey == NULL_var )
      continue;

    CATISpecAttrKey* piAttrKey = NULL;
    spAttrKey->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
    if( piAttrKey )
    {
      CATUnicodeString attrName = piAttrKey->GetName();
      CATUnicodeString attrNameMod = attrName;
      if( !spSpecAttrAcc->IsUpToDate(piAttrKey) )
      {
        attrNameMod.Append(" @");
      }

      CATAttrKind attrKind = piAttrKey->GetType();
      CATAttrKind attrKindList = piAttrKey->GetListType();

      if( attrKind == tk_external )
      {
        i_pFrame->SetGridRowResizable(yGrid, 1);

        CATDlgFrame* pFrame = new CATDlgFrame(i_pFrame, "pFrame", CATDlgGridLayout);
        pFrame->SetGridConstraints(yGrid++, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetTitle(attrNameMod);

        int yGrid1 = 0;
        CATDlgEditor* pEditor = new CATDlgEditor(pFrame, "pEditor");
        pEditor->SetGridConstraints(yGrid1++, 0, 1, 1, CATGRID_4SIDES);
        pEditor->SetVisibleTextWidth(50);

        CATUnicodeString value;
        SEQUENCE(octet) octet = spSpecAttrAcc->GetExternalObjectName(piAttrKey);
        int length = octet.length();
        for( int i = 0; i < length; i++ )
          value.Append(CATUnicodeString(octet[i]));

        if( value != "" )
        {
          //           cerr <<  i_spSpec->GetImpl()->IsA() << " : " << value << endl; 
          pEditor->SetText(value);
        }

        CATUnicodeString unsignedCharValue = this->octetToStringInt(octet, pFrame, yGrid1++);
        this->buildFrameEditor("GetExternalObjectName", unsignedCharValue, pFrame, yGrid1++);

        CATISpecAttribute* piSpecAttr = i_spSpec->GetAttribute(attrName);
        if( piSpecAttr )
        {
          CATUnicodeString  value = piSpecAttr->DumpValue();
          this->buildFrameEditor("Attribute Value", value, pFrame, yGrid1++);
          piSpecAttr->Release();
          piSpecAttr = NULL;
        }

        CATILinkableObject* piLinkObj = NULL;
        HRESULT hr = spSpecAttrAcc->GetExternalObject(piAttrKey, /*DONT_BIND  ANYWHERE IN_SESSION*/ ANYWHERE, &piLinkObj);
        if( piLinkObj )
        {
          CATDlgLabel* pLabel = new CATDlgLabel(pFrame, "pFrame");
          pLabel->SetGridConstraints(yGrid1++, 0, 1, 1, CATGRID_4SIDES);
          pLabel->SetTitle(piLinkObj->GetImpl()->IsA());

          CATIAlias_var spAlias = piLinkObj;
          if( !!spAlias )
          {
            CATUnicodeString  alias = spAlias->GetAlias();
            CATDlgLabel* pLabel1 = new CATDlgLabel(pFrame, "pFrame");
            pLabel1->SetGridConstraints(yGrid1++, 0, 1, 1, CATGRID_4SIDES);
            pLabel1->SetTitle(alias);

            CATDlgLabel* pLabelPrd = new CATDlgLabel(pFrame, "pFrame");
            pLabelPrd->SetGridConstraints(yGrid1++, 0, 1, 1, CATGRID_4SIDES);

            CATUnicodeString prdPath;
            CATIProduct_var spFatherPrd = spAlias;
            while( !!spFatherPrd )
            {
              CATUnicodeString instName;
              spFatherPrd->GetPrdInstanceName(instName);
              prdPath.Append(instName);
              prdPath.Append("!");
              spFatherPrd = spFatherPrd->GetFatherProduct();
            }

            pLabelPrd->SetTitle(prdPath);

          }

          CATDocument* pDoc = piLinkObj->GetDocument();
          if( pDoc )
          {
            CATUnicodeString storageName = pDoc->StorageName();
            this->buildFrameEditor("CATDocument", storageName, pFrame, yGrid1++);
          }

          // CATIBRepAccess_var
          CATIBRepAccess_var spBrepAccess = piLinkObj;
          if( !!spBrepAccess )
          {
            CATUnicodeString etiquette = spBrepAccess->GetEtiquette();
            this->buildFrameEditor("Etiquette", etiquette, pFrame, yGrid1++);

            CATCell_var spCell = spBrepAccess->GetSelectingCell();
            CATBody_var spBody = spBrepAccess->GetSelectingBody();
            if( !!spCell && !!spBody )
            {
              int tagBody = spBody->GetPersistentTag();
              int tagCell = spCell->GetPersistentTag();
              CATUnicodeString tagBodyStr;
              CATUnicodeString tagCellStr;
              tagBodyStr.BuildFromNum(tagBody);
              tagCellStr.BuildFromNum(tagCell);
              this->buildFrameEditor("CATBody tag", tagBodyStr, pFrame, yGrid1++);
              this->buildFrameEditor("CATCell tag", tagCellStr, pFrame, yGrid1++);
            }

            CATISpecObject_var spLastFeature = spBrepAccess->GetLastFeature();
            if( !!spLastFeature )
            {
              CATUnicodeString lastFeat = spLastFeature->GetImpl()->IsA();
              lastFeat.Append(" ");
              lastFeat.Append(spLastFeature->GetName());
              this->buildFrameEditor("LastFeature", lastFeat, pFrame, yGrid1++);
            }
          }

          /*
          SEQUENCE(octet) identOctet;
          boolean isUuid = 0;
          piLinkObj->GetIdentifier( identOctet, isUuid );
          CATUnicodeString identOctetCharValue = this->octetToStringInt( identOctet,        pFrame, yGrid1++ );
          this->buildFrameEditor( "CATILinkableObject::GetIdentifier", identOctetCharValue, pFrame, yGrid1++ );
          */

          piLinkObj->Release();
          piLinkObj = NULL;
        }
      }

      piAttrKey->Release();
      piAttrKey = NULL;
    }
  }

  // CATIDftGenGeom 
  CATIDftGenGeom* piDftGenGeom = NULL;
  spSpecAttrAcc->QueryInterface(IID_CATIDftGenGeom, (void**)&piDftGenGeom);
  if( piDftGenGeom )
  {
    CATIProduct* piPrd = NULL;
    piDftGenGeom->GetProduct(IID_CATIProduct, (IUnknown**)&piPrd);
    if( piPrd )
    {
      CATDlgFrame* pFrame = new CATDlgFrame(i_pFrame, "pFrame", CATDlgGridLayout);
      pFrame->SetGridConstraints(yGrid++, 0, 1, 1, CATGRID_4SIDES);
      pFrame->SetTitle("CATIDftGenGeom::GetProduct");

      int yGrid1 = 0;
      CATIAlias_var spAlias = piPrd;
      if( !!spAlias )
      {
        CATUnicodeString  alias = spAlias->GetAlias();
        CATDlgLabel* pLabel1 = new CATDlgLabel(pFrame, "pFrame");
        pLabel1->SetGridConstraints(yGrid1++, 0, 1, 1, CATGRID_4SIDES);
        pLabel1->SetTitle(alias);

        CATDlgLabel* pLabelPrd = new CATDlgLabel(pFrame, "pFrame");
        pLabelPrd->SetGridConstraints(yGrid1++, 0, 1, 1, CATGRID_4SIDES);

        CATUnicodeString prdPath;
        CATIProduct_var spFatherPrd = spAlias;
        while( !!spFatherPrd )
        {
          CATUnicodeString instName;
          spFatherPrd->GetPrdInstanceName(instName);
          prdPath.Append(instName);
          prdPath.Append("!");
          spFatherPrd = spFatherPrd->GetFatherProduct();
        }
        pLabelPrd->SetTitle(prdPath);
      }
      piPrd->Release();
      piPrd = NULL;
    }
    piDftGenGeom->Release();
    piDftGenGeom = NULL;
  }  // CATIDftGenGeom 

  return;
} // showExternalAttr

//-------------------------------------------------------------------------
// showListExternalAttr
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showListExternalAttr(CATDlgFrame* i_pFrame,
                                             CATISpecObject_var i_spSpec,
                                             int i_YGrid)
{
  CATISpecAttrAccess_var spSpecAttrAcc = i_spSpec;
  if( !i_pFrame || spSpecAttrAcc == NULL_var )
    return;

  int yGrid = i_YGrid;

  CATListValCATISpecAttrKey_var attrKeys;
  HRESULT hr = spSpecAttrAcc->ListAttrKeys(attrKeys);
  for( int i = 1; i <= attrKeys.Size(); i++ )
  {
    CATISpecAttrKey_var spAttrKey = attrKeys[i];
    if( spAttrKey == NULL_var )
      continue;

    CATISpecAttrKey* piAttrKey = NULL;
    spAttrKey->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
    if( piAttrKey )
    {
      CATUnicodeString attrName = piAttrKey->GetName();
      if( !spSpecAttrAcc->IsUpToDate(piAttrKey) )
      {
        attrName.Append(" @");
      }
      CATAttrKind attrKind = piAttrKey->GetType();
      CATAttrKind attrKindList = piAttrKey->GetListType();

      if( attrKindList == tk_external )
      {
        i_pFrame->SetGridRowResizable(yGrid, 1);

        CATDlgFrame* pFrame = new CATDlgFrame(i_pFrame, "pFrame", CATDlgGridLayout);
        pFrame->SetGridConstraints(yGrid++, 0, 1, 1, CATGRID_4SIDES);
        pFrame->SetTitle(attrName);

        int size = spSpecAttrAcc->GetListSize(piAttrKey);
        for( int a = 0; a < size; a++ )
        {
          CATDlgFrame* pFrame1 = new CATDlgFrame(pFrame, "pFrame", CATDlgGridLayout | CATDlgFraNoTitle);
          pFrame1->SetGridConstraints(a, 0, 1, 1, CATGRID_4SIDES);

          CATDlgEditor* pEditor = new CATDlgEditor(pFrame1, "pEditor");
          pEditor->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
          pEditor->SetVisibleTextWidth(50);

          CATUnicodeString value;
          SEQUENCE(octet) octet = spSpecAttrAcc->GetExternalObjectName(piAttrKey, a + 1);
          int length = octet.length();
          for( int i = 0; i < length; i++ )
            value.Append(CATUnicodeString(octet[i]));

          if( value != "" )
          {
            //            cerr <<  i_spSpec->GetImpl()->IsA() << " : " << value << endl; 
            pEditor->SetText(value);
          }

          CATILinkableObject* piLinkObj = NULL;
          HRESULT hr = spSpecAttrAcc->GetExternalObject(piAttrKey, /*DONT_BIND  ANYWHERE IN_SESSION*/ ANYWHERE, &piLinkObj);
          if( piLinkObj )
          {
            CATDlgLabel* pLabel = new CATDlgLabel(pFrame1, "pFrame");
            pLabel->SetGridConstraints(1, 0, 1, 1, CATGRID_4SIDES);
            pLabel->SetTitle(piLinkObj->GetImpl()->IsA());

            CATIAlias_var spAlias = piLinkObj;
            if( !!spAlias )
            {
              CATUnicodeString  alias = spAlias->GetAlias();

              CATDlgLabel* pLabel1 = new CATDlgLabel(pFrame1, "pFrame");
              pLabel1->SetGridConstraints(2, 0, 1, 1, CATGRID_4SIDES);
              pLabel1->SetTitle(alias);
            }

            piLinkObj->Release();
            piLinkObj = NULL;
          }
        }
      }
      piAttrKey->Release();
      piAttrKey = NULL;
    }
  }

  return;
} // showListExternalAttr

//-------------------------------------------------------------------------
// showClasses
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showClasses(CATDlgFrame* i_pFrame)
{
  if( i_pFrame )
  {
    int yGrid = 0;

    // Edit text -> such field
    m_pInterfaceEditor = new CATDlgEditor(i_pFrame, "psEditor");
    {
      m_pInterfaceEditor->SetGridConstraints(0, 1, 1, 1, CATGRID_LEFT);
      m_pInterfaceEditor->SetVisibleTextWidth(25);
    }

    // button for such field
    CATDlgPushButton* pShowButton = new CATDlgPushButton(i_pFrame, "pShowButton");
    {
      pShowButton->SetGridConstraints(0, 0, 1, 1, CATGRID_LEFT);
      pShowButton->SetTitle("SHOW");
      this->AddAnalyseNotificationCB(pShowButton, pShowButton->GetPushBActivateNotification(), (CATCommandMethod)&TCAUTLListAttrCmd::onShowClasses, CATINT32ToPtr(0));
    }

    m_pClassesMultiList = new CATDlgMultiList(i_pFrame, "multiList");
    m_pClassesMultiList->SetGridConstraints(1, 0, 2, 1, CATGRID_4SIDES);
    m_pClassesMultiList->SetVisibleTextWidth(70);

    CATUnicodeString title("Classes");
    m_pClassesMultiList->SetColumnTitles(1, &title);
    m_pClassesMultiList->SetColumnTextWidth(0, 70);
  }
  return;
} // showClasses

//-------------------------------------------------------------------------
// onShowClasses
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::onShowClasses(CATCommand*, CATNotification*, CATCommandClientData i_Data)
{
  if( m_pInterfaceEditor && m_pClassesMultiList )
  {
    m_pClassesMultiList->ClearLine();
    CATUnicodeString interfaceName = m_pInterfaceEditor->GetText();
    const SupportedClass* pList = CATMetaObject::ListOfSupportedClass(interfaceName.ConvertToChar());
    CATListOfCATUnicodeString listToSave;
    while( pList )
    {
      listToSave.Append(pList->Class);
      pList = pList->suiv;
    }
    listToSave.QuickSort(&StringCompare);
    for( int j = 1; j <= listToSave.Size(); j++ )
    {
      m_pClassesMultiList->SetColumnItem(0, listToSave[j], j, CATDlgDataAdd);
    }
  }
  return;
} // onShowClasses

//-------------------------------------------------------------------------
// showInterfaces
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showInterfaces(CATDlgFrame* i_pFrame,
                                       CATISpecObject_var i_spSpec)
{
  if( !i_pFrame || i_spSpec == NULL_var )
    return;

  CATMetaClass* pMetaClass = i_spSpec->GetImpl()->GetMetaObject();
  if( !pMetaClass )
    return;

  int yGrid = 0;

  // Edit text -> such field
  m_pShowEditor = new CATDlgEditor(i_pFrame, "psEditor");
  {
    m_pShowEditor->SetGridConstraints(0, 1, 1, 1, CATGRID_LEFT);
    m_pShowEditor->SetVisibleTextWidth(25);
  }
  // button for such field
  CATDlgPushButton* pShowButton = new CATDlgPushButton(i_pFrame, "pShowButton");
  {
    pShowButton->SetGridConstraints(0, 0, 1, 1, CATGRID_LEFT);
    pShowButton->SetTitle("SHOW");
    this->AddAnalyseNotificationCB(pShowButton, pShowButton->GetPushBActivateNotification(), (CATCommandMethod)&TCAUTLListAttrCmd::ShowButtonSpec, CATINT32ToPtr(0));//idx
  }

  m_pInterfMultiList = new CATDlgMultiList(i_pFrame, "multiList");
  m_pInterfMultiList->SetGridConstraints(1, 0, 2, 1, CATGRID_4SIDES);
  m_pInterfMultiList->SetVisibleTextWidth(30);

  CATUnicodeString title("Interface");
  m_pInterfMultiList->SetColumnTitles(1, &title);
  m_pInterfMultiList->SetColumnTextWidth(0, 30);

  // Edit text -> such field
  m_pSearchEditor = new CATDlgEditor(i_pFrame, "psEditor");
  {
    m_pSearchEditor->SetGridConstraints(0, 3, 1, 1, CATGRID_LEFT);
    m_pSearchEditor->SetVisibleTextWidth(25);
  }
  // button for such field
  CATDlgPushButton* pSearchButton = new CATDlgPushButton(i_pFrame, "pSearchButton");
  {
    pSearchButton->SetGridConstraints(0, 2, 1, 1, CATGRID_RIGHT);
    pSearchButton->SetTitle("Search");
    yGrid++;
    this->AddAnalyseNotificationCB(pSearchButton, pSearchButton->GetPushBActivateNotification(), (CATCommandMethod)&TCAUTLListAttrCmd::searchInMethod, CATINT32ToPtr(0));//idx
  }


  m_pMethodsMultiList = new CATDlgMultiList(i_pFrame, "multiList");
  m_pMethodsMultiList->SetGridConstraints(1, 2, 2, 1, CATGRID_4SIDES);
  m_pMethodsMultiList->SetVisibleTextWidth(60);

  CATUnicodeString titles[2];
  titles[0] = "Method";
  titles[1] = "Header";
  m_pMethodsMultiList->SetColumnTitles(2, titles);
  m_pMethodsMultiList->SetColumnTextWidth(0, 40);
  m_pMethodsMultiList->SetColumnTextWidth(1, 15);


  SupportedInterface* pSupportedInterface = (SupportedInterface*)pMetaClass->ListOfSupportedInterface();
  while( pSupportedInterface )
  {
    CATUnicodeString interfName = pSupportedInterface->Interface;
    m_ListOfNames.Append(interfName);

    pSupportedInterface = pSupportedInterface->suiv;
  }

  m_ListOfNames.QuickSort(&StringCompare);

  this->fillMultiList();

  return;
} // showInterfaces

//---------------------------------------------------------------------------
// fillMultiList
//---------------------------------------------------------------------------
void TCAUTLListAttrCmd::fillMultiList()
{
  if( !m_pInterfMultiList )
    return;

  m_pInterfMultiList->ClearLine();
  m_pMethodsMultiList->ClearLine();

  for( int i = 1; i <= m_ListOfNames.Size(); i++ )
  {
    this->checkFileExists(m_ListOfNames[i]);
    m_pInterfMultiList->SetColumnItem(0, m_ListOfNames[i], i, CATDlgDataAdd);
  }

  this->AddAnalyseNotificationCB(m_pInterfMultiList, m_pInterfMultiList->GetListActivateNotification(), (CATCommandMethod)&TCAUTLListAttrCmd::ShowMultiListSpec, NULL);
  this->AddAnalyseNotificationCB(m_pInterfMultiList, m_pInterfMultiList->GetListSelectNotification(), (CATCommandMethod)&TCAUTLListAttrCmd::interfaceSelected, NULL);

  this->AddAnalyseNotificationCB(m_pMethodsMultiList, m_pInterfMultiList->GetListActivateNotification(), (CATCommandMethod)&TCAUTLListAttrCmd::methodSelected, NULL);


  return;
} // fillMultiList

//---------------------------------------------------------------------------
// stringCompare
//---------------------------------------------------------------------------
int StringCompare(CATUnicodeString* i_pParam1, CATUnicodeString* i_pParam2)
{
  int returnValue = 0;
  if( i_pParam1 != NULL && i_pParam2 != NULL )
  {
    if( ( *i_pParam1 ) < ( *i_pParam2 ) )
    {
      returnValue = -1;
    }
    else if( ( *i_pParam1 ) > ( *i_pParam2 ) )
    {
      returnValue = 1;
    }
  }
  return returnValue;
} // stringCompare


//-------------------------------------------------------------------------
// ShowButtonSpec
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::ShowButtonSpec(CATCommand*, CATNotification*, CATCommandClientData i_Data)
{
  int idx = CATPtrToINT32(i_Data);

  // idx = 0 is for button "SHOW" 
  if( idx == 0 )
  {
    CATUnicodeString editedText;
    editedText = m_pShowEditor->GetText();
    editedText.ConvertToChar();

    // show list completly
    if( editedText.operator==("") )
    {
      int lineCount = m_pInterfMultiList->GetLineCount();
      // fill list again
      if( lineCount < m_ListOfNames.Size() )
      {
        m_ListOfNames.QuickSort(&StringCompare);
        this->fillMultiList();
      }
    }
    else
    {
      // new CATListValCATUnicodeString for showing results
      CATListValCATUnicodeString showListOfNames;
      // convert edittext to lower case
      editedText.ToLower();
      // TODO look for substrings, make names to lowercase 
      // copy m_listOfNames to another temp_ListOfNames
      CATListValCATUnicodeString tempListOfNames;
      tempListOfNames.operator=(m_ListOfNames);

      //look for match in the temp list
      for( int i = 1; i <= tempListOfNames.Size(); i++ )
      {
        tempListOfNames[i].ToLower();

        if( tempListOfNames[i].SearchSubString(editedText, 0, CATUnicodeString::CATSearchModeForward) != -1 )
        {
          //std::cout << "gefunden substring " << m_ListOfNames[i]  << std::endl;
          showListOfNames.Append(m_ListOfNames[i]);
        }
      }

      m_pInterfMultiList->ClearLine(); // remove all lines

      // clear m_pInterfMultiList and append all item from showListOfNames if showListOfNames isn't empty
      if( showListOfNames.Size() >= 1 )
      {
        // if found -> change list        
        for( int i = 1; i <= showListOfNames.Size(); i++ )
        {
          m_pInterfMultiList->SetColumnItem(0, showListOfNames[i], -1, CATDlgDataAdd);
        }
        // set notification
        this->AddAnalyseNotificationCB(m_pInterfMultiList,
                                       m_pInterfMultiList->GetListActivateNotification(),
                                       (CATCommandMethod)&TCAUTLListAttrCmd::ShowMultiListSpec,
                                       NULL);
      }
      showListOfNames = NULL;
    }
  }
  return;
} // ShowButtonSpec

//-------------------------------------------------------------------------
// ShowMultiListSpec
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::ShowMultiListSpec(CATCommand*, CATNotification*, CATCommandClientData i_Data)
{
  if( !m_pInterfMultiList )
    return;

  CATUnicodeString _nameOfSelectedRow;
  m_pInterfMultiList->GetSelect(0, &_nameOfSelectedRow, 1);

  // Path to your directory with .h-files
  CATUnicodeString pathName;
  for( int p = 1; p <= m_FullPathes.Size(); p++ )
  {
    if( m_FullPathes[p].SearchSubString(_nameOfSelectedRow) > -1 )
    {
      pathName = m_FullPathes[p];
      break;
    }
  }

  // if (!GetSystemVariable("CAA_V5_ENCYCLOPEDIA", pathName)) 
  //   return;
  // pathName.Append("\\" + _nameOfSelectedRow );

  ShellExecuteA(NULL, ( "open" ), LPCSTR(pathName.ConvertToChar()), NULL, NULL, SW_SHOW);

  m_pInterfMultiList->ClearSelect();

  return;
} // ShowMultiListSpec

//-------------------------------------------------------------------------
// methodSelected
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::methodSelected(CATCommand*, CATNotification*, CATCommandClientData i_Data)
{
  if( !m_pInterfMultiList )
    return;

  CATUnicodeString _nameOfSelectedRow;
  m_pMethodsMultiList->GetSelect(1, &_nameOfSelectedRow, 1);

  // Path to your directory with .h-files
  CATUnicodeString pathName;
  for( int p = 1; p <= m_FullPathes.Size(); p++ )
  {
    if( m_FullPathes[p].SearchSubString(_nameOfSelectedRow) > -1 )
    {
      pathName = m_FullPathes[p];
      break;
    }
  }

  // if ( !GetSystemVariable("CAA_V5_ENCYCLOPEDIA", pathName )) 
  //  return; 
  // pathName.Append("\\" + _nameOfSelectedRow );

  ShellExecuteA(NULL, ( "open" ), LPCSTR(pathName.ConvertToChar()), NULL, NULL, SW_SHOW);


  return;
}

//-------------------------------------------------------------------------
// GetSystemVariable
//-------------------------------------------------------------------------
CATBoolean TCAUTLListAttrCmd::GetSystemVariable(const CATUnicodeString& iSystemVariable, CATUnicodeString& oVarValue)
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
// showSpec
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showSpec(CATCommand*, CATNotification*, CATCommandClientData i_Data)
{

  int idx = CATPtrToINT32(i_Data);

  if( m_ListSpec.Size() < idx )
    return;

  CATISpecObject_var spSpec = m_ListSpec[idx];
  if( spSpec == NULL_var )
    return;

  TCAUTLListAttrCmd* pCom = new TCAUTLListAttrCmd(spSpec);
  // pCom->Activate(NULL, NULL);

  return;
} // showSpec


//-------------------------------------------------------------------------
// ShowContainerSpec
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::ShowContainerSpec(CATCommand*, CATNotification*, CATCommandClientData i_Data)
{

  int idx = CATPtrToINT32(i_Data);//0

  m_pMultiListContainer->GetSelect(&idx, 1);
  if( idx < 0 )
    return;

  CATISpecObject_var spSpec = m_ListOfContainerObjects[idx + 1];
  if( spSpec == NULL_var )
    return;

  int col = m_pMultiListContainer->GetSelectedColumn();
  if( col == 2 )
  {
    spSpec->Update();
  }

  TCAUTLListAttrCmd* pCom = new TCAUTLListAttrCmd(spSpec);
  // pCom->Activate(NULL, NULL);

  return;
} // ShowContainerSpec

//-------------------------------------------------------------------------
// readContainer
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::readContainer(CATCommand* i_pButton,
                                      CATNotification*,
                                      CATCommandClientData data)
{

  m_TypeSelected = "";
  CATBaseUnknown* piUnk = (CATBaseUnknown*)data;

  m_spContainer = piUnk;
  CATGeoFactory_var spGeoFact = m_spContainer;
  if( !!spGeoFact )
  {
    this->showGeoFactContainer(m_spContainer);
  }
  else if( !!m_spContainer )
  {
    this->showContainer(m_spContainer);
  }

  // fill m_pContTypeCombo
  if( m_pContTypeCombo )
  {
    CATListOfCATUnicodeString types;
    m_pContTypeCombo->ClearLine();
    for( int j = 1; j <= m_ListOfContainerObjects.Size(); j++ )
    {
      CATBaseUnknown_var spUnk = m_ListOfContainerObjects[j];
      if( !!spUnk )
      {
        CATUnicodeString type = spUnk->GetImpl()->IsA();
        if( types.Locate(type) == 0 )
        {
          types.Append(type);
          m_pContTypeCombo->SetLine(type);
        }
      }
    }
  } // fill m_pContTypeCombo

  return;
} // readContainer

//-------------------------------------------------------------------------
// showAddedProperties
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showAddedProperties(CATDlgFrame* i_pFrame, CATDocument* i_pDoc)
{
  if( !i_pFrame || !i_pDoc )
    return;

  CATIPrdProperties_var spPrdProperties = NULL_var;
  CATInit_var spInit = i_pDoc;
  if( !!spInit )
  {
    const CATIdent contID = "CATIPrtContainer";
    CATIPrtContainer* piPartContainer = (CATIPrtContainer*)spInit->GetRootContainer(contID);
    if( piPartContainer )
    {
      CATIPrtPart_var spPrtPart = piPartContainer->GetPart();
      if( !!spPrtPart )
      {
        spPrdProperties = spPrtPart->GetProduct();
      }
    }
  }

  if( spPrdProperties == NULL_var )
    return;

  int yGrid = 0;

  CATIParmPublisher* piParmPublisher = NULL;
  spPrdProperties->GetUserProperties(piParmPublisher, false);
  if( piParmPublisher )
  {
    CATLISTV(CATISpecObject_var) listParameters;
    piParmPublisher->GetDirectChildren(CATICkeParm::ClassName(), listParameters);
    for( int p = 1; p <= listParameters.Size(); p++ )
    {
      CATICkeParm_var spParm = listParameters[p];
      if( spParm == NULL_var )
        continue;

      CATICkeInst_var spValue = spParm->Value();
      if( spValue == NULL_var )
        continue;

      CATUnicodeString attrName = spParm->Name();
      CATUnicodeString value = spValue->AsString();

      CATIParmPublisher_var spRoot = piParmPublisher;
      CATUnicodeString relName = spParm->RelativeName(spRoot);

      i_pFrame->SetGridRowResizable(yGrid, 1);

      CATDlgLabel* pLabel = new CATDlgLabel(i_pFrame, "pFrame");
      pLabel->SetGridConstraints(yGrid, 0, 1, 1, CATGRID_4SIDES);
      pLabel->SetTitle(attrName);

      CATISpecObject_var spSpec = spParm;
      if( !!spSpec )
      {
        CATDlgEditor* pEditor = new CATDlgEditor(i_pFrame, "pEditor");
        pEditor->SetGridConstraints(yGrid, 1, 1, 1, CATGRID_4SIDES);
        pEditor->SetVisibleTextWidth(20);

        pEditor->SetText(value);

        CATDlgPushButton* pButton = new CATDlgPushButton(i_pFrame, "pButton");
        pButton->SetGridConstraints(yGrid, 2, 1, 1, CATGRID_4SIDES);

        CATUnicodeString alias("undefined");
        CATIAlias_var spAlias = spSpec;
        if( !!spAlias )
          alias = spAlias->GetAlias();

        pButton->SetTitle(alias);


        m_ListSpec.Append(spSpec);
        int idx = m_ListSpec.Size();

        this->AddAnalyseNotificationCB(pButton,
                                       pButton->GetPushBActivateNotification(),
                                       (CATCommandMethod)&TCAUTLListAttrCmd::showSpec,
                                       CATINT32ToPtr(idx));
      }
      yGrid++;
    }
    piParmPublisher->Release();
    piParmPublisher = NULL;
  }

  return;
} // showAddedProperties

//-------------------------------------------------------------------------
// checkFileExists
//-------------------------------------------------------------------------
CATBoolean TCAUTLListAttrCmd::checkFileExists(CATUnicodeString& o_rIntName)
{
  /*  CATUnicodeString pathName;
  if( !GetSystemVariable( "CAA_V5_ENCYCLOPEDIA", pathName ))
  return exists; */

  CATBoolean exists = FALSE;
  for( int d = 1; d <= m_InterfaceDirectories.Size(); d++ )
  {
    CATUnicodeString pathName = m_InterfaceDirectories[d];
    pathName.Append("\\" + o_rIntName + ".h");

    fstream file;
    file.open(pathName.ConvertToChar(), ifstream::in);
    if( !file.fail() )
    {
      if( m_FullPathes.Locate(pathName) == 0 )
        m_FullPathes.Append(pathName);

      o_rIntName.Append(".h");
      exists = TRUE;
      char str[2000];

      CATUnicodeString method;
      int lnCnt = m_pMethodsMultiList->GetLineCount();
      while( !file.eof() )
      {
        file.getline(str, 2000);
        CATUnicodeString line(str);
        line = line.Strip();
        line.ReplaceAll("\t", "");
        if( line.SearchSubString("virtual ") < 0 || line.SearchSubString("/") > -1 || line.SearchSubString("~") > -1 )
          continue;

        line.ReplaceAll("virtual ", "");
        method = line;
        while( line.SearchSubString(")") < 0 && !file.eof() )
        {
          file.getline(str, 2000);
          line = CATUnicodeString(str);
          method.Append(line);
        }

        m_pMethodsMultiList->SetColumnItem(0, method);
        m_pMethodsMultiList->SetColumnItem(1, o_rIntName);
      }

      if( method != "" )
      {
        m_ListHeaders.Append(o_rIntName);
        m_ListLinesNb.Append(lnCnt);
      }
      file.close();
      break;
    }
  }
  return exists;
} // checkFileExists

//-------------------------------------------------------------------------
// interfaceSelected
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::interfaceSelected(CATCommand*, CATNotification*, CATCommandClientData i_Data)
{
  if( !m_pInterfMultiList || !m_pMethodsMultiList )
    return;

  CATUnicodeString _nameOfSelectedRow;
  m_pInterfMultiList->GetSelect(0, &_nameOfSelectedRow, 1);

  int idx = m_ListHeaders.Locate(_nameOfSelectedRow);
  if( idx > 0 && idx <= m_ListLinesNb.Size() )
  {
    idx = m_ListLinesNb[idx];
    if( idx > -1 )
    {
      m_pMethodsMultiList->SetFirstLine(idx);
      m_pMethodsMultiList->SetSelect(&idx, 1);
    }
  }
  return;
} // interfaceSelected

//-------------------------------------------------------------------------
// searchInMethod
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::searchInMethod(CATCommand*, CATNotification*, CATCommandClientData i_DatalistOfNames)
{
  CATUnicodeString txt = m_pSearchEditor->GetText().Strip();
  if( txt == "" )
    return;

  if( m_LastTxt != txt )
  {
    m_LastTxt = txt;
    m_LastFound = 0;
  }

  txt.ToLower();
  int i = 0;
  int lnCnt = m_pMethodsMultiList->GetLineCount();
  for( i = m_LastFound; i < lnCnt; i++ )
  {
    CATUnicodeString str;
    m_pMethodsMultiList->GetColumnItem(0, str, i);
    str.ToLower();
    if( str.SearchSubString(txt) > -1 )
    {
      m_pMethodsMultiList->SetFirstLine(i);
      m_pMethodsMultiList->SetSelect(&i, 1);
      m_LastFound = i + 1;
      break;
    }
  }

  if( i == lnCnt )
    m_LastFound = 0;

  return;
} // searchInMethod

//---------------------------------------------------------------------------
// TCAMTAInstallPathes 
//---------------------------------------------------------------------------
void TCAUTLListAttrCmd::getInstallPathes(CATUnicodeString& o_rPathes)
{
  static CATUnicodeString s_GraphPathes;
  if( s_GraphPathes.GetLengthInChar() < 1 && s_GraphPathes.Compare("error") != 2 )
  {
    s_GraphPathes == "error";
    char* grPth = new char[1024];
    if( CATLibSuccess == CATGetEnvValue("CATInstallPath", &grPth) )
    {
      s_GraphPathes = CATUnicodeString(grPth);
    }
    delete[] grPth;
  }
  o_rPathes = s_GraphPathes;
  return;
} // TCAMTAInstallPathes 

//---------------------------------------------------------------------------
// isEntryOk 
//---------------------------------------------------------------------------
CATBoolean TCAUTLListAttrCmd::isEntryOk(const CATUnicodeString& i_rEntryStr)
{
  if( i_rEntryStr.GetLengthInChar() < 3 )
    return FALSE;

  if( i_rEntryStr.SearchSubString(".m") > -1 )
    return FALSE;

  if( i_rEntryStr.SearchSubString("various") > -1 )
    return FALSE;

  if( i_rEntryStr.SearchSubString("IdentityCard") > -1 )
    return FALSE;

  if( i_rEntryStr.SearchSubString("Generated") > -1 )
    return FALSE;

  if( i_rEntryStr.SearchSubString("win_b64") > -1 )
    return FALSE;

  if( i_rEntryStr.SearchSubString(".git") > -1 )
    return FALSE;

  return TRUE;
} // isEntryOk 

//---------------------------------------------------------------------------
// TCAMTAReadDirectory 
//---------------------------------------------------------------------------
HRESULT TCAUTLListAttrCmd::readDirectory(const CATUnicodeString& i_rDir, CATListOfCATUnicodeString& o_rHeadersDir)
{
  HRESULT hr = E_FAIL;
  static CATUnicodeString s_GraphPathes;
  CATDirectory dir;
  CATListOfCATUnicodeString entries;
  CATLibStatus status = ::CATOpenDirectory(i_rDir.ConvertToChar(), &dir);
  if( CATLibError != status )
  {
    hr = S_OK;
    int endOfDir = 0;
    CATDirectoryEntry entry;
    status = ::CATReadDirectory(&dir, &entry, &endOfDir);
    while( ( endOfDir != 1 ) && ( CATLibSuccess == status ) )
    {
      if( this->isEntryOk(entry.name) )
      {
        entries.Append(entry.name);
      }
      status = ::CATReadDirectory(&dir, &entry, &endOfDir);
    }
  }

  CATListOfCATUnicodeString subDirs;
  CATBoolean interfaceDirExists = FALSE;
  int  i = 1;
  for( i = 1; i <= entries.Size(); i++ )
  {
    CATUnicodeString entry = entries[i];
    CATUnicodeString subDir = i_rDir;
    subDir.Append("\\");
    subDir.ReplaceAll("\\\\", "\\");
    subDir.Append(entry);
    if( entry == "PublicInterfaces" || entry == "ProtectedInterfaces" )
    {
      interfaceDirExists = TRUE;
      o_rHeadersDir.Append(subDir);
    }
    else
    {
      subDirs.Append(subDir);
    }
  }

  if( interfaceDirExists )
    return hr;

  // search recursively 
  for( i = 1; i <= subDirs.Size(); i++ )
  {
    this->readDirectory(subDirs[i], o_rHeadersDir);
  }

  return hr;
} // TCAMTAReadDirectory

//-------------------------------------------------------------------------
// initInterfaceDirectories
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::initInterfaceDirectories(void)
{
  static CATListOfCATUnicodeString sIntDirs;
  if( sIntDirs.Size() == 0 )
  {
    CATUnicodeString installPathes;
    this->getInstallPathes(installPathes);

    installPathes.ReplaceAll("win_b64", "");

    CATToken tokens(installPathes);
    CATUnicodeString tok = tokens.GetNextToken(";");
    while( tok != "" )
    {
      CATListOfCATUnicodeString headersDir;
      if SUCCEEDED(this->readDirectory(tok, headersDir))
      {
        sIntDirs.Append(headersDir);
      }
      tok = tokens.GetNextToken(";");
    }
  }
  m_InterfaceDirectories = sIntDirs;
  return;
} // initInterfaceDirectories

//-------------------------------------------------------------------------
// setUpdateFacetLabel
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::setUpdateFacetLabel(CATDlgFrame* i_pFrame, int i_YGrid, int i_XGrid, CATBaseUnknown_var i_spAttrKey, CATISpecObject_var i_spSpec)
{
  CATUnicodeString alias;
  CATUnicodeString labelTitle;
  CATDlgLabel* pFacetLabel = NULL;
  CATTry
  {
    CATIAlias_var spAlias = i_spSpec;
    if( !!spAlias )
    {
      alias = spAlias->GetAlias();
    }

    CATISpecAttribute_var spSpecAttr = i_spAttrKey;
    if( i_pFrame && !!i_spAttrKey && !!spSpecAttr && !!i_spSpec && i_YGrid > -1 && i_XGrid > -1 )
    {
      pFacetLabel = new CATDlgLabel(i_pFrame, "FacetLabel");
      if( pFacetLabel )
      {
        pFacetLabel->SetGridConstraints(i_YGrid, i_XGrid, 1, 1, CATGRID_4SIDES);

        CATAttrInOut inOut = spSpecAttr->GetQuality();
        switch( inOut )
        {
        case sp_IN:
          {
            labelTitle = " sp_IN ";
            break;
          }
        case sp_OUT:
          {
            labelTitle = " sp_OUT ";
            break;
          }
        case sp_NEUTRAL:
          {
            labelTitle = " sp_NEUTRAL ";
            break;
          }
        }

        if( !i_spSpec->IsUpToDate() )
        {
          labelTitle.Append(" -> not uptodate!");
        }

        pFacetLabel->SetTitle(labelTitle);
      }
    }
  }
    CATCatch(CATError, pError)
  {
    if( pFacetLabel )
    {
      pFacetLabel->SetTitle(labelTitle);
    }

    CATUnicodeString errMsg = pError->GetNLSMessage();
    cerr << __FUNCTION__ << " -> " << alias << " " << errMsg.ConvertToChar() << endl;
    delete pError;
  }
  CATEndTry;
  return;
} // setUpdateFacetLabel

//-----------------------------------------------------------------------------
// getDocUuid
//-----------------------------------------------------------------------------
int TCAUTLListAttrCmd::getDocUuid(CATDocument* i_pDoc, CATDlgFrame* i_pFrame)
{
  int yGrid = 0;
  if( i_pDoc && i_pFrame )
  {
    CATIStorageProperty_var spStProp = i_pDoc->GetDocumentDescriptor();
    if( !!spStProp )
    {
      CATUuid uuid;
      spStProp->GetRootContainerUuid(uuid);
      char* c = new char[255];
      uuid.UUID_to_chaine(c);
      CATUnicodeString uuidToChain = CATUnicodeString(c);
      delete[] c;

      m_DocUUID = uuidToChain;


      CATIDocId* piDocId = NULL;
      i_pDoc->GetDocId(&piDocId);

      CATUnicodeString storageName;
      piDocId->GetIdentifier(storageName);

      fstream file;
      file.open(storageName.ConvertToChar(), ifstream::in);
      if( !file.fail() )
      {
        file.close();
      }
      else
      {
        CATUnicodeString* pRealPath = NULL;
        HRESULT hr = ::CATGetRealPath(&storageName, &pRealPath);
        storageName = *pRealPath;
      }

      this->buildFrameEditor(i_pDoc->DisplayName(), storageName, i_pFrame, yGrid++);
      this->buildFrameEditor("UUID_to_chaine", uuidToChain, i_pFrame, yGrid++);

      char* cStruct = new char[255];
      uuid.UUID_to_struct(cStruct);
      CATUnicodeString uuidToStruct = CATUnicodeString(cStruct);
      this->buildFrameEditor("UUID_to_struct", uuidToStruct, i_pFrame, yGrid++);
      delete[] cStruct;
    }
  }
  return yGrid;
} // getDocUuid

//-----------------------------------------------------------------------------
// buildFrameEditor
//-----------------------------------------------------------------------------
void TCAUTLListAttrCmd::buildFrameEditor(CATUnicodeString i_Title, const CATUnicodeString& i_rText, CATDlgFrame* i_pFrame, int i_GridY)
{
  if( i_pFrame )
  {
    CATDlgFrame* pFrame = new CATDlgFrame(i_pFrame, "Frame", CATDlgGridLayout);
    {
      pFrame->SetGridConstraints(i_GridY, 0, 1, 1, CATGRID_4SIDES);
      pFrame->SetGridColumnResizable(0);
      pFrame->SetTitle(i_Title);

      CATDlgEditor* pEditor = new CATDlgEditor(pFrame, "pEditor");
      pEditor->SetGridConstraints(0, 1, 1, 1, CATGRID_4SIDES);
      pEditor->SetVisibleTextWidth(100);
      pEditor->SetText(i_rText);
    }
  }
  return;
} // buildFrameEditor

//-----------------------------------------------------------------------------
// octetToStringInt
//-----------------------------------------------------------------------------
CATUnicodeString TCAUTLListAttrCmd::octetToStringInt(const SEQUENCE(octet)& i_rOctet, CATDlgFrame* i_pFrame, int i_GridY)
{
  CATUnicodeString toReturn;
  int length = i_rOctet.length();

  CATDlgFrame* pFrame = NULL;
  if( length > 0 && i_pFrame )
  {
    i_pFrame->SetGridColumnResizable(0);
    pFrame = new CATDlgFrame(i_pFrame, "Frame", CATDlgFraNoFrame | CATDlgGridLayout);
    pFrame->SetGridConstraints(i_GridY, 0, 1, 1, CATGRID_4SIDES);
  }

  for( int i = 0; i < length; i++ )
  {
    CATUnicodeString uint;
    uint.BuildFromNum((int)i_rOctet[i]);
    toReturn.Append(uint);
    toReturn.Append(" ");

    if( pFrame )
    {
      CATDlgFrame* pFrame1 = new CATDlgFrame(pFrame, "Frame", CATDlgGridLayout);
      {
        pFrame1->SetGridConstraints(i_GridY, i, 1, 1, CATGRID_4SIDES);
        pFrame1->SetTitle(uint);
        CATDlgEditor* pEditor = new CATDlgEditor(pFrame1, "pEditor");
        pEditor->SetGridConstraints(0, 1, 1, 1, CATGRID_4SIDES);
        pEditor->SetVisibleTextWidth(1);
        pEditor->SetText(CATUnicodeString(i_rOctet[i]));
      }
    }
  }
  return toReturn;
} // octetToStringInt 

//-----------------------------------------------------------------------------
// compareByPersistentTag
//-----------------------------------------------------------------------------
int TCAUTLListAttrCmd::compareByPersistentTag(CATBaseUnknown_var* i_pspCGMObject1, CATBaseUnknown_var* i_pspCGMObject2)
{
  if( !i_pspCGMObject1 || !i_pspCGMObject2 )
    return 0;

  CATICGMObject_var spCGMObject1 = (CATBaseUnknown_var)*i_pspCGMObject1;
  CATICGMObject_var spCGMObject2 = (CATBaseUnknown_var)*i_pspCGMObject2;
  if( spCGMObject1 == NULL_var || spCGMObject2 == NULL_var )
    return 0;

  CATULONG32 tag1 = spCGMObject1->GetPersistentTag();
  CATULONG32 tag2 = spCGMObject2->GetPersistentTag();
  int toReturn = 1;
  if( tag2 < tag1 )
  {
    toReturn = -1;
  }
  return toReturn;
} // compareByPersistentTag

//-----------------------------------------------------------------------------
// compareByPersistentTagOfBody
//-----------------------------------------------------------------------------
int TCAUTLListAttrCmd::compareByPersistentTagOfBody(CATBaseUnknown_var* i_pspGeomElm1, CATBaseUnknown_var* i_pspGeomElm2)
{
  if( !i_pspGeomElm1 || !i_pspGeomElm2 )
    return 0;

  CATIGeometricalElement_var spGeomElm1 = (CATBaseUnknown_var)*i_pspGeomElm1;
  CATIGeometricalElement_var spGeomElm2 = (CATBaseUnknown_var)*i_pspGeomElm2;
  if( spGeomElm1 == NULL_var || spGeomElm2 == NULL_var )
    return 0;

  CATBody_var spBody1 = spGeomElm1->GetBodyResult();
  CATBody_var spBody2 = spGeomElm2->GetBodyResult();
  if( spBody1 == NULL_var && spBody2 == NULL_var )
  {
    return 0;
  }
  if( spBody1 == NULL_var )
  {
    return 1;
  }
  if( spBody2 == NULL_var )
  {
    return -1;
  }

  CATULONG32 tag1 = spBody1->GetPersistentTag();
  CATULONG32 tag2 = spBody2->GetPersistentTag();
  int toReturn = 1;
  if( tag2 < tag1 )
  {
    toReturn = -1;
  }
  return toReturn;
} // compareByPersistentTagOfBody

//-------------------------------------------------------------------------
// showGeoFactContainer
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showGeoFactContainer(CATGeoFactory_var i_spGeoFact)
{
  if( m_pMultiListContainer )
  {
    m_pMultiListContainer->ClearLine();

    m_ListOfContainerObjects.RemoveAll();
    CATListOfCATUnicodeString column1, column2, column3;
    this->getGeoFactContent(i_spGeoFact, column1, column2, column3);

    int cnt1 = m_ListOfContainerObjects.Size();
    int cnt2 = column1.Size();
    int cnt3 = column2.Size();
    int cnt4 = column3.Size();
    if( cnt1 == cnt2 && cnt2 == cnt3 && cnt3 == cnt4 )
    {
      CATUnicodeString objCount;
      objCount.BuildFromNum(cnt1);
      objCount.Append(" elements in ");
      objCount.Append(i_spGeoFact->GetImpl()->IsA());
      m_pContListFrame->SetTitle(objCount);
      for( int j = 1; j <= m_ListOfContainerObjects.Size(); j++ )
      {
        m_pMultiListContainer->SetColumnItem(0, column1[j], -1, CATDlgDataAdd);
        m_pMultiListContainer->SetColumnItem(1, column2[j], -1, CATDlgDataAdd);
        m_pMultiListContainer->SetColumnItem(2, column3[j], -1, CATDlgDataAdd);
      }
    }
  }
  return;
} // showGeoFactContainer 

//-------------------------------------------------------------------------
// getGeoFactContent
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::getGeoFactContent(CATGeoFactory_var i_spGeoFact, CATListOfCATUnicodeString& o_rColumn1, CATListOfCATUnicodeString& o_rColumn2, CATListOfCATUnicodeString& o_rColumn3)
{
  m_ListOfContainerObjects.RemoveAll();
  o_rColumn1.RemoveAll();
  o_rColumn2.RemoveAll();
  o_rColumn3.RemoveAll();
  if( !!i_spGeoFact )
  {
    CATListValCATBaseUnknown_var allBodies;
    CATGeometry* piGeom = NULL;
    for( piGeom = i_spGeoFact->Next(NULL); piGeom != NULL; piGeom = i_spGeoFact->Next(piGeom) )
    {
      CATBody_var spBody = piGeom;
      if( !!spBody )
      {
        allBodies.Append(spBody);
      }
    }
    allBodies.QuickSort(&TCAUTLListAttrCmd::compareByPersistentTag);
    int cntBodies = allBodies.Size();
    for( int b = 1; b <= cntBodies; b++ )
    {
      CATBody_var spBody = allBodies[b];
      if( !!spBody )
      {
        CATULONG32 tag = spBody->GetPersistentTag();
        CATUnicodeString bodyTag;
        bodyTag.BuildFromNum(tag);

        CATLISTP(CATCell) cells;
        spBody->GetAllCells(cells);

        CATListValCATBaseUnknown_var allCells;
        for( int c = 1; c <= cells.Size(); c++ )
        {
          allCells.Append(cells[c]);
        }

        allCells.QuickSort(&TCAUTLListAttrCmd::compareByPersistentTag);
        for( int c1 = 1; c1 <= allCells.Size(); c1++ )
        {
          CATCell_var spCell = allCells[c1];
          if( !!spCell )
          {
            CATULONG32 tagCell = spCell->GetPersistentTag();
            CATUnicodeString cellTag;
            cellTag.BuildFromNum(tagCell);
            cellTag.Append(" : ");
            cellTag.Append(bodyTag);
            CATIBRepAccess_var spBrepAccess = CATBRepDecodeCellInBody(spCell, spBody);
            if( !!spBrepAccess )
            {
              CATUnicodeString etiquette;
              int strLength = 0;
              CATTry
              {
                if( m_TypeSelected == "" || spBrepAccess->GetImpl()->IsAKindOf(m_TypeSelected) )
                {
                  m_ListOfContainerObjects.Append(spBrepAccess);
                  o_rColumn1.Append(spBrepAccess->GetEtiquette());
                  o_rColumn2.Append(spBrepAccess->GetImpl()->IsA());
                  o_rColumn3.Append(cellTag);
                }
              }
                CATCatch(CATError, pError)
              {
                CATUnicodeString errorMsg = pError->GetNLSMessage();
                cerr << "errorMsg: " << errorMsg << endl;
                delete pError;
                pError = NULL;
              }
              CATEndTry;
            }
          }
        }
      }
    }
  }
  return;
} // getGeoFactContent

//-------------------------------------------------------------------------
// showAttributesXML
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::showAttributesXML(CATDlgFrame* i_pFrame)
{
  CATDlgEditor* pEditor = new CATDlgEditor(i_pFrame, "pEditor", CATDlgEdtMultiline);
  pEditor->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
  pEditor->SetVisibleTextWidth(80);
  pEditor->SetVisibleTextHeight(200);

  CATUnicodeString xmlStr;
  TCADRTGetXMLString(m_spSpec, xmlStr);
  pEditor->SetText(xmlStr);

  return;
} // showAttributesXML

//-------------------------------------------------------------------------
// buildAIContainer
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::buildAIContainer(CATDlgFrame* i_pFrame)
{
  m_pAnswerEditor = new CATDlgEditor(i_pFrame, "pAEditor", CATDlgEdtMultiline | CATDlgEdtWrap);
  m_pAnswerEditor->SetGridConstraints(0, 0, 1, 1, CATGRID_4SIDES);
  m_pAnswerEditor->SetVisibleTextWidth(50);
  m_pAnswerEditor->SetVisibleTextHeight(50);
  return;
} // buildAIContainer

//-------------------------------------------------------------------------
// askAI
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::askAI(CATCommand*, CATNotification*, CATCommandClientData i_Data)
{
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
    m_pAnswerEditor->SetText("Failed to load hello modul 'TCAPYAIChromDB'\n");
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
    m_pAnswerEditor->SetText("Cannot find function 'askAI'\n");
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
  if( m_pPersonCombo )
  {
    int sel = m_pPersonCombo->GetSelect();
    m_pPersonCombo->GetField(person);
    if( person == "" )
    {
      m_pPersonCombo->GetLine(person, sel);
    }
  }

  cerr << person << endl;

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


  const char* cstr3 = m_DocUUID.ConvertToChar();
  if( cstr3 == nullptr )
  {
    std::cerr << "Error: m_DocUUID string is null or invalid." << std::endl;
    return; // Or handle the error appropriately
  }


  // Now safely call PyUnicode_Decode with a valid C string
  PyObject* arg1 = PyUnicode_Decode(cstr, strlen(cstr), "utf-8", "strict");
  PyObject* arg2 = PyUnicode_Decode(cstrName, strlen(cstrName), "utf-8", "strict");
  PyObject* arg3 = PyUnicode_Decode(cstr3, strlen(cstr3), "utf-8", "strict"); 
  if( arg1 == nullptr || arg2 == nullptr || cstr3 == nullptr  )
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

  if( m_pAnswerEditor )
  {
    m_pAnswerEditor->SetText(answerAI);
  }

  return;
} // askAI

//-------------------------------------------------------------------------
// storeDBBulk
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::storeDBBulk(CATCommand*, CATNotification*, CATCommandClientData i_Data)
{
  CATDlgNotify* pOrderWindow = new CATDlgNotify(this, "MyOrderId", CATDlgNfyYesNo);
  int answer = pOrderWindow->DisplayBlocked("Refill Database?", "Store to CromaDB");
  // pOrderWindow->SetVisibility(CATDlgShow);
  if( answer != 3 )
  {
    return;
  }
  // clearDB 
  this->clearDB();


  CATUnicodeString answerAI;
  cerr << __FUNCTION__ << endl;

  // Add directory to sys.path

  // Check Python version
  const char* version = Py_GetVersion();
  cerr << "Python version in use: " << version << endl;

  static PyObject* s_pMmoduleDB = NULL;
  if( !s_pMmoduleDB )
  {
    s_pMmoduleDB = PyImport_ImportModule(CHROMA_MODULE);  // 2.AnalizeCATDoc
  }

  if( !s_pMmoduleDB )
  {
    PyErr_Print();
    m_pAnswerEditor->SetText("Failed to load hello modul 'TCAPYAIChromDB1'\n");
    std::cerr << "Failed to load hello s_pMmoduleDB\n";
    return;
  }

  PyObject* s_pFuncDBBulk = NULL;
  if( !s_pFuncDBBulk )
  {
    s_pFuncDBBulk = PyObject_GetAttrString(s_pMmoduleDB, "storeDB_bulk");  // 3. Get aksAI()
  }
  if( !s_pFuncDBBulk || !PyCallable_Check(s_pFuncDBBulk) )
  {
    PyErr_Print();
    m_pAnswerEditor->SetText("Cannot find function 's_pFuncDBBulk'\n");
    std::cerr << "Cannot find function storeDB\n";
    return;
  }

  this->initDataToStoreInDB();

  int cnt1 = m_DataToStoreInDB.Size();
  int cnt2 = m_IdsToStoreInDB.Size();
  int cnt3 = m_ParentIdsToStore.Size();
  if( cnt1 != cnt2 || cnt2 != cnt3 || cnt1 < 1 )
  {
    return;
  }

  PyObject* pyListData = PyList_New(cnt1); // Create Python list
  PyObject* pyListIDs = PyList_New(cnt1); // Create Python list
  PyObject* pyListMeta = PyList_New(cnt1); // Create Python list 

  for( size_t i = 1; i <= cnt1; i++ )
  {
    const char* cstr1 = m_DataToStoreInDB[i].ConvertToChar();
    const char* cstr2 = m_IdsToStoreInDB[i].ConvertToChar();
    if( cstr1 == nullptr || cstr2 == nullptr )
    {
      std::cerr << "Error: Null string at index " << i << std::endl;
      Py_DECREF(pyListData); // clean up
      Py_DECREF(pyListIDs); // clean up
      Py_DECREF(pyListMeta); // clean up
      return;
    }

    // PyObject* pyStr1 = PyUnicode_FromString(cstr1); // Convert C string to Python str
    // PyObject* pyStr2 = PyUnicode_FromString(cstr2); // Convert C string to Python str 
    PyObject* pyStr1 = PyUnicode_Decode(cstr1, strlen(cstr1), "utf-8", "strict");
    PyObject* pyStr2 = PyUnicode_Decode(cstr2, strlen(cstr2), "utf-8", "strict"); 
    PyObject* pyDict = PyDict_New();
    if( !pyStr1 )
    {
      std::cerr << "Error: Failed to convert to Python string:\n" << cstr1 << std::endl;
    }
    if( !pyStr1 || !pyStr2 || !pyDict )
    {
      std::cerr << "Error: Failed to convert to Python string." << std::endl;
      Py_DECREF(pyListData); // clean up
      Py_DECREF(pyListIDs); // clean up
      Py_DECREF(pyListMeta); // clean up
      return;
    }

    // Assuming metadata_vector[i] is a map or struct with `uuid` and `parent_uuid`
    PyDict_SetItemString(pyDict, "uuid", PyUnicode_FromString(m_IdsToStoreInDB[i].ConvertToChar()));
    PyDict_SetItemString(pyDict, "uuid_of_parent_element", PyUnicode_FromString(m_ParentIdsToStore[i].ConvertToChar()));

    // Steals reference to pyStr
    PyList_SetItem(pyListData, i - 1, pyStr1);
    PyList_SetItem(pyListIDs, i - 1, pyStr2);
    PyList_SetItem(pyListMeta, i - 1, pyDict);
  }

  // Call the Python function with pyList as an argument
  // For example, assuming Python function takes a single list parameter:
  PyObject* result = PyObject_CallFunctionObjArgs(s_pFuncDBBulk,
                                                  pyListData,     // texts
                                                  pyListIDs,      // doc_ids
                                                  pyListMeta,     // metadatas
                                                  NULL);          // must terminate with NULL

  // Clean up
  Py_DECREF(pyListData); // clean up
  Py_DECREF(pyListIDs); // clean up
  Py_DECREF(pyListMeta); // clean up

  if( result )
  {
    // handle result
    Py_DECREF(result);
  }
  else
  {
    PyErr_Print(); // Show Python error
  }

  return;
} //  storeDBBulk

//-------------------------------------------------------------------------
// clearDB
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::clearDB(void)
{
  CATUnicodeString answerAI;
  cerr << __FUNCTION__ << endl;

  // Add directory to sys.path

  // Check Python version
  const char* version = Py_GetVersion();
  cerr << "Python version in use: " << version << endl;

  static PyObject* s_pMmoduleDB = NULL;
  if( !s_pMmoduleDB )
  {
    s_pMmoduleDB = PyImport_ImportModule(CHROMA_MODULE);  // 2.AnalizeCATDoc
  }

  if( !s_pMmoduleDB )
  {
    PyErr_Print();
    m_pAnswerEditor->SetText("Failed to load hello modul 'TCAPYAIChromDB'\n");
    std::cerr << "Failed to load hello s_pMmoduleDB\n";
    return;
  }

  PyObject* s_pFuncClearDB = NULL;
  if( !s_pFuncClearDB )
  {
    s_pFuncClearDB = PyObject_GetAttrString(s_pMmoduleDB, "clearDB");  // 3. Get aksAI()
  }
  if( !s_pFuncClearDB || !PyCallable_Check(s_pFuncClearDB) )
  {
    PyErr_Print();
    m_pAnswerEditor->SetText("Cannot find function 'clearDB'\n");
    std::cerr << "Cannot find function clearDB\n";
    return;
  }

  const char* cstr1 = m_DocUUID.ConvertToChar();
  if( cstr1 == nullptr )
  {
    std::cerr << "Error: m_DocUUID string is null or invalid." << std::endl;
    return; // Or handle the error appropriately
  }


  // Now safely call PyUnicode_FromString with a valid C string
  PyObject* arg1 = PyUnicode_Decode(cstr1, strlen(cstr1), "utf-8", "strict");   
  if( arg1 == nullptr )
  {
    std::cerr << "Error: Failed to create Python Unicode object." << std::endl;
    return; // Or handle the error appropriately
  }

  PyObject* args = PyTuple_Pack(1, arg1);
  PyObject* result = PyObject_CallObject(s_pFuncClearDB, args);
  if( result )
  {
    Py_DECREF(result);
  }
  else
  {
    PyErr_Print();
  }
  return;
} // clearDB

//-------------------------------------------------------------------------
// initDataToStoreInDB
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::initDataToStoreInDB(void)
{
  m_DataToStoreInDB.RemoveAll();
  m_IdsToStoreInDB.RemoveAll();
  m_ParentIdsToStore.RemoveAll();

  /*
  // add specs
  CATListValCATBaseUnknown_var doItSpecs;
  this->getGoSpecs(doItSpecs);
  for( int i = 1; i <= doItSpecs.Size(); i++ )
  {
    CATUnicodeString alias;
    CATIAlias_var spAlias = doItSpecs[i];
    if( !!spAlias )
    {
      alias = spAlias->GetAlias();
    }

    CATUnicodeString contentComponent;
    this->getJSONContentOfSpec(doItSpecs[i], contentComponent);
    if( contentComponent != "" && alias != "" )
    {
      m_DataToStoreInDB.Append("JSON Context:" + contentComponent);
      m_IdsToStoreInDB.Append(alias);
      m_ParentIdsToStore.Append("");
    }
  }

  if( true )
  {
    return;
  }*/

  // add CAA_Components
  CATListValCATBaseUnknown_var doItComponents;
  this->getGoComponents(doItComponents);
  for( int i = 1; i <= doItComponents.Size(); i++ )
  {
    CATUnicodeString contentComponent;
    this->getJSONContentOfComponent(doItComponents[i], contentComponent);
    if( contentComponent != "" )
    {
      m_DataToStoreInDB.Append("JSON Context:" + contentComponent);
      m_IdsToStoreInDB.Append(doItComponents[i]->GetImpl()->IsA());
      m_ParentIdsToStore.Append("");
    }
  }

  // add commands
  m_CmdNames.RemoveAll();
  m_CmdDescriptions.RemoveAll();
  /*
  this->getCommands(m_CmdNames, m_CmdDescriptions);
  int cnt = m_CmdNames.Size();
  for( int c = 1; c <= cnt; c++ )
  {
    CATUnicodeString contentCmdHdr;
    this->getJSONContentOfCommandHeader(c, contentCmdHdr);
    if( contentCmdHdr != "" )
    {
      m_DataToStoreInDB.Append("JSON Context:" + contentCmdHdr);
      m_IdsToStoreInDB.Append(m_CmdNames[c]);
      m_ParentIdsToStore.Append("");
    }
  }*/

  return;
} // initDataToStoreInDB

//-----------------------------------------------------------------------------
// getListFromCont
//-----------------------------------------------------------------------------
int TCAUTLListAttrCmd::getListFromCont(CATIContainer_var i_spClientCont,
                                       CATListValCATBaseUnknown_var& o_rList)
{
  int countObjects = 0;
  o_rList.RemoveAll();
  if( !!i_spClientCont )
  {
    SEQUENCE(CATBaseUnknown_ptr) ListObj;
    CATLONG32 cnt = i_spClientCont->ListMembersHere("CATBaseUnknown", ListObj);
    for( int i = 0; i < cnt; i++ )
    {
      CATBaseUnknown* piObj = NULL;
      if( ListObj[i] && SUCCEEDED(ListObj[i]->QueryInterface(IID_CATBaseUnknown, (void**)&piObj)) )
      {
        o_rList.Append(piObj);
        piObj->Release();
        piObj = NULL;
      }
    }
    countObjects = o_rList.Size();
  }
  return countObjects;
} // getListFromCont

//-------------------------------------------------------------------------
// o_rSpecs
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::getGoSpecs(CATListValCATBaseUnknown_var& o_rSpecs)
{
  CATIContainer_var  spMechCont;
  CATInit_var spInit = m_pCurrentDoc;
  if( !!spInit )
  {
    const CATIdent contID = "CATIPrtContainer";
    CATIPrtContainer* piPartContainer = (CATIPrtContainer*)spInit->GetRootContainer(contID);
    if( piPartContainer )
    {
      spMechCont = piPartContainer->GetMechanicalContainer();
    }
  }

  CATListValCATBaseUnknown_var listObj;
  int  cntObj = this->getListFromCont(spMechCont, listObj);

  for( int i = 1; i <= cntObj; i++ )
  {
    TCAIUTLDoIt_var spSpec = listObj[i];
    if( !!spSpec )
    {
      o_rSpecs.Append(spSpec);
    }
  }
  return;
} // o_rSpecs

//-------------------------------------------------------------------------
// getGoComponents
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::getGoComponents(CATListValCATBaseUnknown_var& o_rComponents)
{
  o_rComponents.RemoveAll();
  CATUnicodeString interfaceName("TCAIUTLGo");
  const SupportedClass* pList = CATMetaObject::ListOfSupportedClass(interfaceName.ConvertToChar());
  CATListOfCATUnicodeString listToSave;
  while( pList )
  {
    CATUnicodeString compName(pList->Class);
    CATBaseUnknown_var spGo = TCAUTLInst(compName);
    if( !!spGo )
    {
      o_rComponents.Append(spGo);
    }
    pList = pList->suiv;
  }
  return;
} // getGoComponents


//-------------------------------------------------------------------------
// getJSONContentOfSpec
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::getJSONContentOfSpec(TCAIUTLDoIt_var i_spDoItSpec,
                                             CATUnicodeString& o_rJSONContent)
{
  o_rJSONContent = "";
  CATISpecObject_var spSpec = i_spDoItSpec;
  if( i_spDoItSpec == NULL_var || spSpec == NULL_var )
  {
    return;
  }
  i_spDoItSpec->getInfo(o_rJSONContent);
  cerr << endl << o_rJSONContent << endl << endl;
  return;
} // getJSONContentOfSpec

//-------------------------------------------------------------------------
// getJSONContentOfComponent
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::getJSONContentOfComponent(TCAIUTLDoIt_var i_spDoItComp,
                                                  CATUnicodeString& o_rJSONContent)
{
  o_rJSONContent = "";
  if( i_spDoItComp == NULL_var )
  {
    return;
  }

  CATUnicodeString name(i_spDoItComp->GetImpl()->IsA());
  CATUnicodeString info;
  i_spDoItComp->getInfo(info);

  CATUnicodeString catFunction("CAA-Component");
  o_rJSONContent = "{\"content\": \"" + name + ": " + info + " [Function: " + catFunction + "]\", \"metadata\" : {\"name\": \"" + name + "\", \"function\" : \"" + catFunction + "\"  }}";

  cerr << endl << o_rJSONContent << endl << endl;

  return;
} // getJSONContentOfComponent

//-------------------------------------------------------------------------
// getJSONContentOfCommandHeader
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::getJSONContentOfCommandHeader(const int& i_rIdx,
                                                      CATUnicodeString& o_rJSONContent)
{
  if( i_rIdx > m_CmdNames.Size() || i_rIdx > m_CmdDescriptions.Size() )
  {
    return;
  }

  CATUnicodeString name = m_CmdNames[i_rIdx];
  CATUnicodeString info = m_CmdDescriptions[i_rIdx];

  CATUnicodeString catFunction("CATIA-Command");
  o_rJSONContent = "{\"content\": \"" + name + ": " + info + " [Function: " + catFunction + "]\", \"metadata\" : {\"name\": \"" + name + "\", \"function\" : \"" + catFunction + "\"  }}";

  cerr << endl << o_rJSONContent << endl << endl;

  return;
} // getJSONContentOfCommandHeader

//-------------------------------------------------------------------------
// listCommands
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::getCommands(CATListOfCATUnicodeString& o_rCmdNames,
                                    CATListOfCATUnicodeString& o_rCmdDescriptions)
{
  o_rCmdNames.RemoveAll();
  o_rCmdDescriptions.RemoveAll();
  int hdrCnt = CATCommandHeader::GetHeaderCount();
  for( int h = 1; h <= hdrCnt; h++ )
  {
    CATCommandHeader* pHdr = CATCommandHeader::GetHeaderFromList(h);
    if( pHdr )
    {
      CATUnicodeString alias = this->getUTF8Str(pHdr->GetAlias());

      if( o_rCmdNames.Locate(alias) )
      {
        continue;
      }
      o_rCmdNames.Append(alias);

      CATUnicodeString cmdDesc;
      this->addToDescription(cmdDesc, this->getUTF8Str(pHdr->GetDefaultTitle()));
      this->addToDescription(cmdDesc, this->getUTF8Str(pHdr->GetTitle()));
      this->addToDescription(cmdDesc, this->getUTF8Str(pHdr->GetShortHelp()));
      this->addToDescription(cmdDesc, this->getUTF8Str(pHdr->GetDialogLongHelp()));
      this->addToDescription(cmdDesc, this->getUTF8Str(pHdr->GetHelp()));
      this->addToDescription(cmdDesc, this->getUTF8Str(pHdr->GetContextualHelp()));
      this->addToDescription(cmdDesc, this->getUTF8Str(pHdr->GetCategory2()));

      o_rCmdDescriptions.Append(cmdDesc);
    }
  }
  return;
} // listCommands

//-------------------------------------------------------------------------
// getUTF8Str
//-------------------------------------------------------------------------
CATUnicodeString TCAUTLListAttrCmd::getUTF8Str(const CATUnicodeString& i_rStr)
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
       && uc != CATUnicodeChar('[') && uc != CATUnicodeChar(']')
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
// addToDescription
//-------------------------------------------------------------------------
void TCAUTLListAttrCmd::addToDescription(CATUnicodeString& io_rDesc,
                                         CATUnicodeString& io_rStr)
{
  io_rStr.ReplaceAll("\n", " ");
  io_rStr.ReplaceAll(".....", ".");
  io_rStr.ReplaceAll("....", ".");
  io_rStr.ReplaceAll("...", ".");
  io_rStr.ReplaceAll("..", ".");
  if( io_rStr != "" && io_rDesc.SearchSubString(io_rStr) < 0 )
  {
    io_rDesc.Append(io_rStr);
    io_rDesc.Append(". ");
  }
  return;
} // addToDescription