
#include "CATGetEnvValue.h"
#include "CATIDftPattern.h"
#include "CATIProduct.h"
#include "CATBody.h"
#include "CATIDrwGenDrawShape.h"
#include "CATListOfInt.h"
#include "CATIDftDrawing.h"
#include "CATISpecObject.h"
#include "CATIDftDocumentServices.h"
#include "CATSession.h"
#include "iostream.h" 
#include "CATSessionServices.h"
#include "CATDocumentServices.h" 
#include "TCAXmlUtils.h"
#include "CATInit.h"
#include "CATLISTP_CATMathPoint2D.h"
#include "CATIPrtContainer.h"
#include "CATIColInvariantId.h"
#include "CATIABase.h"
#include "CATITPSTextContent.h"
#include "TCAIUTLGo.h"
#include <Python.h>
#include <iostream>
#include <cstring>
#include "CATIIniSearchServices.h"
#include "CATIIniSearchCriterion.h"
#include "CATCreateInstance.h"
#include "CATIniSearchServicesComponent.h"
#include "TCAIUTLDoIt.h"
#include "CATPrint3DRepImage.h"
#include "CATI3DGeoVisu.h"
#include "CATPrintFileDevice.h"
#include "CATPrintParameters.h"
#include "CATPixelImage.h"
#include "CATPrintGenerator.h"
#include "CATVisManager.h"
#include "CAT3DViewpoint.h"
#include "CATIBuildPath.h"
#include "CATIGeometricalElement.h"

//Print Framework
#include "CATPrintFileImage.h"   // To create an image from the input file
#include "CATPrinterManager.h"   // To make initialization

//-----------------------------------------------------------------------------
// TCAListFromCont
//-----------------------------------------------------------------------------
int TCAListFromCont(CATIContainer_var i_spClientCont, CATListValCATBaseUnknown_var& o_rList)
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
} // TCAListFromCont

//----------------------------------------------------------------------------------
// TCAListGetDrwCont
//----------------------------------------------------------------------------------
CATIContainer_var TCAListGetDrwCont(CATDocument* i_pDrwDoc)
{
  CATIContainer_var spDrwCont;
  if( i_pDrwDoc )
  {
    CATIDftDocumentServices* piDftDocServices = NULL;
    if( SUCCEEDED(i_pDrwDoc->QueryInterface(IID_CATIDftDocumentServices, (void**)&piDftDocServices)) )
    {
      CATIDftDrawing* piDftDrawing = NULL;
      if( SUCCEEDED(piDftDocServices->GetDrawing(IID_CATIDftDrawing, (void**)&piDftDrawing)) )
      {
        CATISpecObject_var spDrwRoot = piDftDrawing;
        if( !!spDrwRoot )
        {
          spDrwCont = spDrwRoot->GetFeatContainer();
        }
        piDftDrawing->Release();
        piDftDrawing = NULL;
      }
      piDftDocServices->Release();
      piDftDocServices = NULL;
    }
  }
  return spDrwCont;
} // TCAListGetDrwCont

//----------------------------------------------------------------------------------
// TCAUDTTestSearch
//----------------------------------------------------------------------------------
void TCAUDTTestSearch(void)
{
  cerr << __FUNCTION__ << endl;

  CATIIniSearchServices* m_piSearchServices = NULL;
  CATIIniSearchCriterion* m_piSearchCriterion = NULL;

  ::CATCreateInstance(CLSID_CATIniSearchServicesComponent,
                      NULL,
                      0,
                      IID_CATIIniSearchServices,
                      (void**)&m_piSearchServices);

  CATUnicodeString selString = "NAME IN GRAPH=*_30*_999*";

  HRESULT hr = m_piSearchServices->DecodeStringToCriterion(selString, FALSE, m_piSearchCriterion);
  if( FAILED(hr) || !m_piSearchCriterion )
  {
    m_piSearchCriterion = 0L;
    return;
  }

  TCAIUTLGo_var spGo = TCAUTLInst("UTLGo3DPoint");
  if( !!spGo )
  {
    hr = m_piSearchCriterion->IsObjectMatching(spGo);
  }



  return;
} // TCAUDTTestSearch

//----------------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------------
int main1(int iArgc, char* iArgv[])
{

  CATSession* pSession = NULL;
  ::Create_Session("TestSession", pSession);

  CATDocument* pDoc = NULL;
  CATDocumentServices::OpenDocument(iArgv[1], pDoc);
  TCAIUTLGo_var spJSON = UTL_INST(UTLGoExportJSON);
  if( spJSON == NULL_var || !pDoc )
  {
    return -1;
  }
  CATBoolean writeFile = TRUE;
  spJSON->setInput(pDoc);
  spJSON->setInput(writeFile);
  if( FAILED(spJSON->run()) )
  {
    return -1;
  }
  CATISpecObject_var spSpecObjectOnMechanicalPart;
  CATIContainer_var  spMechCont;
  CATInit_var spInit = pDoc;
  if( !!spInit )
  {
    const CATIdent contID = "CATIPrtContainer";
    CATIPrtContainer* piPartContainer = (CATIPrtContainer*)spInit->GetRootContainer(contID);
    if( piPartContainer )
    {
      spMechCont = piPartContainer;
      spSpecObjectOnMechanicalPart = piPartContainer->GetPart();
      piPartContainer->Release();
      piPartContainer = NULL;
    }
  }

  if( NULL_var == spSpecObjectOnMechanicalPart )
  {
    cout << "The MechanicalPart is NULL_var" << endl;
    return 1;
  }

  CATListValCATBaseUnknown_var listObj;
  int  cntObj = TCAListFromCont(spMechCont, listObj);

  HRESULT hr = E_FAIL;

  CATISpecObject_var spSpec = NULL_var;

  for( int i = 1; i <= cntObj; i++ )
  {
    if( !listObj[i]->GetImpl()->IsAKindOf("GSMBiDim") )
    {
      continue;
    }

    CATUnicodeString alias = TCAUTLGetAlias(listObj[i]);
    if( alias.SearchSubString("Bauteil_2") > -1 )
    {
      spSpec = listObj[i];
      break;
    }
  }

  // transform CATBody
  CATIGeometricalElement_var spGeoElm = spSpec;
  if( !!spGeoElm )
  {
    CATBody_var spCATBody = spGeoElm->GetBodyResult();
    if( !!spCATBody )
    {
      // spCATBody->
    }
  }

  //------------------------------------------------------------------
  //4 - Creates a Path with MechanicalPart feature of the Part document
  //------------------------------------------------------------------  
  CATPathElement* pRootObjectPath = new CATPathElement(spSpec); // spSpecObjectOnMechanicalPart); //

  //------------------------------------------------------------------
  //5 - Retrieves the unique CATVisManager instance
  //------------------------------------------------------------------

  CATVisManager* pVisManager = CATVisManager::GetVisManager();
  if( NULL == pVisManager )
  {
    cout << " ERROR by retrieving the CATVisManager instance" << endl;
    return 1;
  }

  //------------------------------------------------------------------
  //6 - Attaches the Path to manage by the CATVisManager
  //------------------------------------------------------------------

    // 6-1 List of CATIVisu interfaces used to display the model
  list<IID> ListIVisu3d;
  IID* pIIDInf = new IID(IID_CATI3DGeoVisu);
  ListIVisu3d.fastadd(pIIDInf);

  CAT3DViewpoint viewPnt;
  CATMathPointf origin = ( 0.0f, -100.0f, 800.0f );
  CATMathDirectionf sight(CATMathDirection(0.00, -1.00, 0.00));
  CATMathDirectionf up(CATMathDirection(0.00, 1.00, 1.00));
  viewPnt.Set(origin, sight, up);


  // 6-3 Attaching the root to the CATVisManager: The graphic representations
  //     are created.
  //
  HRESULT rc = E_FAIL;
  rc = pVisManager->AttachTo(pRootObjectPath, &viewPnt, ListIVisu3d, NULL, 1, 1, 1); //  , NULL, 1, 1);
  if( FAILED(rc) )
  {
    cout << " ERROR in the AttachTo method" << endl;
    return 1;
  }
  pVisManager->Commit();

  delete pIIDInf;
  pIIDInf = NULL;
  ListIVisu3d.empty();

  pRootObjectPath->Release();
  pRootObjectPath = NULL;


  //------------------------------------------------------------------
  //7 - Retrieves the Graphic Representation of the MechanicalPart feature
  //------------------------------------------------------------------
  CATI3DGeoVisu* pIVisuOnRoot = NULL;
  rc = spSpec->QueryInterface(IID_CATI3DGeoVisu, (void**)&pIVisuOnRoot);
  if( SUCCEEDED(rc) )
  {
    // GiveRep is the only one method to retrieve the rep associated with
    // an object. This method does not compute the rep. 
    // Do not release the rep returned by this method
    //
    CATRep* pRep = pIVisuOnRoot->GiveRep();
    if( NULL != pRep )
    {
      CAT3DRep* p3DRep = (CAT3DRep*)pRep;

      // -------------------------------       
      // 8 - Retrieves the Bounding Box
      // -------------------------------

      CAT3DBoundingSphere pBe = p3DRep->GetBoundingElement();
      float radius = pBe.GetRadius();
      cout << " The radius of the bounding box = " << radius << endl;

      p3DRep->SetColorRGBA(255, 0, 0);
      p3DRep->SetColor(1);

      pVisManager->Commit();

      CATString imgPath("c:\\tmp\\left_door\\CATPart.png");


      CATPrint3DRepImage print3DImg(p3DRep, 1800, 1800);
      print3DImg.SetTitle("Image_Title");


      CATPixelImage* pPixelImg = print3DImg.Rasterize();
      if( pPixelImg )
      {
        int x = 0;
        int y = 0;
        pPixelImg->GetSize(x, y);
        cerr << x << "; " << y << endl;
        CATString imgPath("c:\\tmp\\left_door\\door1.png");
        pPixelImg->WriteToFile("PNG", imgPath);
      }

    }

    pIVisuOnRoot->Release();
    pIVisuOnRoot = NULL;

  }
  else
  {
    cout << " ERROR to retrieve the CATI3DGeoVisu interface" << endl;
    return 1;
  }

  //------------------------------------------------------------------
  //9 - Detach the VP, the root and the list of interfaces 
  //------------------------------------------------------------------

  rc = pVisManager->DetachFrom(&viewPnt, 0);
  if( FAILED(rc) )
  {
    cout << " ERROR in the DetachFrom method" << endl;
    return 1;
  }


  //------------------------------------------------------------------
  //10 - Closes the Part Document 
  //------------------------------------------------------------------

  rc = CATDocumentServices::Remove(*pDoc);
  pDoc = NULL;

  //------------------------------------------------------------------
  //11 - Deletes the session 
  //------------------------------------------------------------------

  rc = ::Delete_Session("TestSession");
  if( FAILED(rc) )
  {
    cout << "ERROR in delete session" << endl;
    return 1;
  }

  cout << "The CAAGviVisuBatch main program is ended." << endl << endl;

  return 0;
} // main

//----------------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------------
int main(int iArgc, char** iArgv)
{
  Py_Initialize();
  TCAIUTLGo_var spGoAskAI = UTL_INST(UTLGoAskAI);
  if( !!spGoAskAI )
  {
    CATUnicodeString pyModule("TCAImgCompare");
    CATUnicodeString pyFunction("get_best_matches");
    CATUnicodeString pyImgRefDir("C:\\tmp\\doors\\old");
    CATUnicodeString pyImgTrgDir("C:\\tmp\\doors\\new");

    int argIdx = 1;
    spGoAskAI->setInput(pyModule, argIdx++);
    spGoAskAI->setInput(pyFunction, argIdx++);

    spGoAskAI->setInput(2, 1);
    spGoAskAI->setInput(pyImgRefDir, argIdx++);
    spGoAskAI->setInput(pyImgTrgDir, argIdx++);

    if( SUCCEEDED(spGoAskAI->run()) )
    {
      CATUnicodeString outputStr;
      spGoAskAI->getOutput(outputStr);
      cerr << outputStr << endl;

      CATToken token(outputStr);
      CATUnicodeString sep(";|");
      CATUnicodeString tok = token.GetNextToken(sep);
      while( tok != "" )
      {
        cerr << tok << endl;
        tok = token.GetNextToken(sep);
      }
    }
  }
  if( true )
  {
    Py_Finalize();
    return 0;
  }

  CATSession* pSession = NULL;
  ::Create_Session("TestSession", pSession);

  CATDocument* pDoc = NULL;
  CATDocumentServices::OpenDocument(iArgv[1], pDoc);
  TCAIUTLGo_var spJSON = UTL_INST(UTLGoExportJSON);
  if( spJSON == NULL_var || !pDoc )
  {
    return -1;
  }
  CATBoolean writeFile = TRUE;
  spJSON->setInput(pDoc);
  spJSON->setInput(writeFile);
  if( FAILED(spJSON->run()) )
  {
    return -1;
  }

  CATIContainer_var  spMechCont;
  CATInit_var spInit = pDoc;
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
  int  cntObj = TCAListFromCont(spMechCont, listObj);

  HRESULT hr = E_FAIL;

  for( int i = 1; i <= cntObj; i++ )
  {
    cerr << listObj[i]->GetImpl()->IsA() << endl;
    if( !listObj[i]->GetImpl()->IsAKindOf("GSMBiDim") )
    {
      continue;
    }

    CATUnicodeString alias = TCAUTLGetAlias(listObj[i]);
    if( alias.SearchSubString("Bauteil_2") < 0 )
    {
      continue;
    }

    CATI3DGeoVisu_var spVisu = listObj[i];
    if( spVisu == NULL_var )
    {
      continue;
    }

    CAT3DRep* p3DRep = (CAT3DRep*)spVisu->GetRep(); //  (CAT3DRep*)spVisu->BuildRep();
    if( p3DRep )
    {
      //Necessary 
      CATPrinterManager::Begin();


      CATString imgPath("C:\\tmp\\left_door\\CATPart.png");

      CATPrintFileImage* pImage = NULL;
      pImage = new CATPrintFileImage(imgPath, "CGM");

      //-----------------------------------------------------------------
      // 4- Creates a raster file device
      //-----------------------------------------------------------------
      CATPrintFileDevice* pDevice;
      pDevice = new CATPrintFileDevice("c:\\tmp\\left_door\\CATPart.tif", "RASTER");


      // 2. Define standard printing parameters.
      CATPrintParameters parameters;
      // Example: adjust margins or resolution if needed.
      parameters.SetMargins(0.f, 0.f, 0.f, 0.f);

      // Specifies that the white pixels are not printed in black
     // parameters.SetWhitePixel(1);
      // Specifies that the image must be resized to match the paper
      parameters.SetMapToPaper(1);
      // Adds a banner at the top of the image
      parameters.SetBanner("CAAPrtChangeFormat");
      parameters.SetBannerPosition(CATPRINT_TOP);
      // Specifies that the line width changes with the scale
      parameters.SetLineWidthSpecificationMode(CATPRINT_SCALED);
      // Specifies that the segment length of not solid lines type changes with the
      // scale
      parameters.SetLineTypeSpecificationMode(CATPRINT_SCALED);

      CATPrint3DRepImage print3DImg(p3DRep);
      print3DImg.SetTitle("Image_Title");

      //-----------------------------------------------------------------
      // 6- Writes the output file
      //-----------------------------------------------------------------
      if( 0 == print3DImg.Print(pDevice, parameters) )
      {
        cout << " Error during printing " << endl << endl;
      }
      else
      {
        cout << "The rastered file is created" << endl;
      }  // Deallocates created objects


      CAT3DViewpoint* pViewPnt = print3DImg.GetViewpoint();

      CATPrintGenerator* pPrintGen = pDevice->GetGenerator();
      if( pPrintGen )
      {
        int decoded = print3DImg.Decode(pPrintGen, parameters);
        cerr << decoded << endl;
      }

      //-----------------------------------------------------------------
      // 6- Writes the output file
      //-----------------------------------------------------------------
      if( 0 == print3DImg.Print(pDevice, parameters) )
      {
        cout << " Error during printing " << endl << endl;
      }
      else
      {
        cout << "The rastered file is created" << endl;
      }  // Deallocates created objects


      pDevice->Release();
      pImage->Release();
      CATPixelImage* pPixelImg = print3DImg.Rasterize();
      if( pPixelImg )
      {
        int x = 0;
        int y = 0;
        pPixelImg->GetSize(x, y);
        cerr << x << "; " << y << endl;
        CATString imgPath1("c:\\tmp\\left_door\\CATPart.png");
        pPixelImg->WriteToFile("PNG", imgPath1);

      }
    }
  }

  // Done with the printer manager
  CATPrinterManager::End();


  if( true )
  {
    // TCAUDTTestSearch();
    return 0;
  }

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

  /*
  Py_Initialize();

  // Add current directory to sys.path
  PyRun_SimpleString("import sys; sys.path.insert(0, 'C:\\EXAMPLES\\AI\\AskAI')");
  //  PyRun_SimpleString(importSysDir.ConvertToChar());
  PyRun_SimpleString("import sys; print('sys.path =', sys.path)");

  // Check Python version
  const char* version = Py_GetVersion();
  cerr << "Python version in use: " << version << endl;

  PyObject* module = PyImport_ImportModule("test1");  // 2. Import hello.py
  if( !module )
  {
    PyErr_Print();
    std::cerr << "Failed to load hello module\n";
    return -1;
  }

  PyObject* func = PyObject_GetAttrString(module, "greet");  // 3. Get greet()
  if( !func || !PyCallable_Check(func) )
  {
    PyErr_Print();
    std::cerr << "Cannot find function greet\n";
    return -1;
  }

  // 4. Call greet("World")
  PyObject* arg = PyUnicode_FromString("World");
  PyObject* args = PyTuple_Pack(1, arg);
  PyObject* result = PyObject_CallObject(func, args);

  if( result )
  {
    std::cout << "Python function returned: " << PyUnicode_AsUTF8(result) << "\n";
    Py_DECREF(result);
  }
  else
  {
    PyErr_Print();
  }
  Py_Finalize();
  */

  ;

  CATListOfCATUnicodeString jsonLines;
  spJSON->setInput(pDoc);

  spJSON->setInput(writeFile);
  if( SUCCEEDED(spJSON->run()) )
  {
    // spJSON->getOutput(jsonLines);
    CATListValCATBaseUnknown_var jsonItems;
    spJSON->getOutput(jsonItems);
    int cnt = jsonItems.Size();
    cerr << "jsonItems.Size(): " << cnt << endl;
    for( int j = 1; j <= cnt; j++ )
    {
      TCAIUTLGo_var spJItem = jsonItems[j];
      if( !!spJItem )
      {
        CATUnicodeString jline;
        spJItem->getJLine(jline);
        cerr << jline << endl;
      }
    }
  }


  return 0;
} // main

