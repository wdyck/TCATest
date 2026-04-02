
#include "CATListOfDouble.h"
#include "CATDlgNotify.h"
#include "CATDialog.h"
#include "CATDlgWindow.h"
#include "CATApplicationFrame.h"
#include "CATGetEnvValue.h"
#include "CATIAlias.h"
#include "CATInstantiateComponent.h"
#include "CATLib.h"
#include "CATDLName.h"
#include "CATDocumentIntegrityServices.h"
#include "CATIReporter.h"
#include "TCAXmlUtils.h"
#include "CATFrmLayout.h"
#include "CATViewer.h"
#include "CATFrmWindow.h" 
#include "CATNavigation3DViewer.h"
#include "CATFrmEditor.h"
#include "CATInit.h"
#include "CATIPrtContainer.h"
#include "CATCSO.h"
#include "CAT3DRep.h"
#include "CATI3DGeoVisu.h"
#include "CATIReframeCamera.h"
#include "CAT3DViewpoint.h"
#include "CATCafCenterGraph.h"
#include "CATFrmNavigGraphicWindow.h"
#include "CAT3DLineGP.h"
#include "CAT3DAnnotationTextGP.h"
#include "CAT3DMarkerGP.h"
#include "CAT3DCustomRep.h"
#include "CATGraphicAttributeSet.h"
#include "CATBody.h" 

//---------------------------------------------------------------------------
// TCADRTGetAlias
//--------------------------------------------------------------------------- 
CATUnicodeString TCADRTGetAlias(CATBaseUnknown_var i_spUnk)
{
  CATUnicodeString alias;
  CATIAlias_var spAlias = i_spUnk;
  if( !!spAlias )
  {
    alias = spAlias->GetAlias();
  }
  return alias;
} // TCADRTGetAlias

//---------------------------------------------------------------------------
// TCADRTRedraw
//--------------------------------------------------------------------------- 
void TCADRTRedraw(CATIRedrawEvent_var i_spRedrawEvent)
{
  if( !!i_spRedrawEvent )
  {
    i_spRedrawEvent->Redraw();
  }
  return;
} // TCADRTRedraw

//---------------------------------------------------------------------------
// TCADRTDOMDocument 
//---------------------------------------------------------------------------
CATIDOMDocument_var TCADRTDOMDocument(CATIXMLDOMDocumentBuilder_var& o_rspXMLBuilder)
{
  CATIDOMDocument_var spDOMDocument;
  ::CreateCATIXMLDOMDocumentBuilder(o_rspXMLBuilder);
  if( !!o_rspXMLBuilder )
  {
    CATIDOMImplementation_var spDOMImplementation;
    o_rspXMLBuilder->GetDOMImplementation(spDOMImplementation);
    if( !!spDOMImplementation )
    {
      CATIDOMDocumentType_var pDocumentType;
      spDOMImplementation->CreateDocument("", "TCADRTXMLNode", pDocumentType, spDOMDocument);
    }
  }
  return spDOMDocument;
} // TCADRTDOMDocument 

//-----------------------------------------------------------------------------
// TCADRTDOMMNode
//-----------------------------------------------------------------------------
CATIDOMNode_var TCADRTDOMMNode(CATIDOMDocument_var i_spDoc, const CATUnicodeString& i_rNodeName, CATIDOMNode_var i_spParentNode, const int& i_rLevel)
{
  CATIDOMElement_var spNewNode;
  if( !!i_spDoc )
  {
    i_spDoc->CreateElement(i_rNodeName, spNewNode);
  }
  if( !!i_spParentNode && !!spNewNode )
  {
    CATIDOMText_var spText1, spText2;
    i_spDoc->CreateTextNode("\n", spText1);
    i_spDoc->CreateTextNode("\n", spText2);
    i_spParentNode->AppendChild(spText1);
    if( i_rLevel > 0 )
    {
      CATUnicodeString levelStr;
      for( int i = 0; i < i_rLevel; i++ )
      {
        levelStr += "  ";
      }
      CATIDOMText_var spLevel;
      i_spDoc->CreateTextNode(levelStr, spLevel);
      i_spParentNode->AppendChild(spLevel);
    }
    i_spParentNode->AppendChild(spNewNode);
    i_spParentNode->AppendChild(spText2);
  }
  return spNewNode;
} // TCADRTDOMMNode


//---------------------------------------------------------------------------
// TCADRTDOMSetAttr
//--------------------------------------------------------------------------- 
void TCADRTDOMSetAttr(CATIDOMElement_var i_spDOMElm, const CATUnicodeString& i_rAttrName, const CATUnicodeString& i_rAttrValue)
{
  if( !!i_spDOMElm && i_rAttrName != "" )
  {
    i_spDOMElm->SetAttribute(i_rAttrName, i_rAttrValue);
  }
  return;
} // TCADRTDOMSetAttr

//---------------------------------------------------------------------------
// TCADRTDOMSpecAttrChildNodes
//--------------------------------------------------------------------------- 
void TCADRTDOMSpecAttrChildNodes(CATISpecAttrAccess_var i_spAttrAcc, CATIDOMDocument_var i_spDoc, CATIDOMNode_var i_spDOMNode)
{
  if( !!i_spAttrAcc && !!i_spDoc && !!i_spDOMNode )
  {
    CATListValCATISpecAttrKey_var attrKeys;
    HRESULT hr = i_spAttrAcc->ListAttrKeys(attrKeys);
    for( int i = 1; i <= attrKeys.Size(); i++ )
    {
      CATISpecAttrKey* piAttrKey = NULL;
      attrKeys[i]->QueryInterface(IID_CATISpecAttrKey, (void**)&piAttrKey);
      if( piAttrKey )
      {
        int countValues = 0;
        CATUnicodeString attrType;
        CATUnicodeString attrValue;
        CATUnicodeString attrName = piAttrKey->GetName();
        CATAttrKind attrKind = piAttrKey->GetType();
        CATAttrKind attrKindList = piAttrKey->GetListType();
        if( attrKind == tk_list )
        {
          attrType = "tk_list_";
          if( attrKindList == tk_double )
          {
            attrType.Append("double");
            countValues = i_spAttrAcc->GetListSize(piAttrKey);
            for( int a = 0; a < countValues; a++ )
            {
              CATUnicodeString str;
              str.BuildFromNum(i_spAttrAcc->GetDouble(piAttrKey, a + 1), "%.2f");
              if( a > 0 )
              {
                attrValue.Append(";");
              }
              attrValue.Append(str);
            }
          }
          else if( attrKindList == tk_integer )
          {
            attrType.Append("integer");
            countValues = i_spAttrAcc->GetListSize(piAttrKey);
            for( int a = 0; a < countValues; a++ )
            {
              CATUnicodeString str;
              str.BuildFromNum(i_spAttrAcc->GetInteger(piAttrKey, a + 1));
              if( a > 0 )
              {
                attrValue.Append(";");
              }
              attrValue.Append(str);
            }
          }
        }
        else if( attrKind == tk_integer )
        {
          attrType = "tk_integer";
          countValues = 1;
          attrValue.BuildFromNum(i_spAttrAcc->GetInteger(piAttrKey));
        }
        else if( attrKind == tk_double )
        {
          attrType = "tk_double";
          countValues = 1;
          attrValue.BuildFromNum(i_spAttrAcc->GetDouble(piAttrKey), "%.2f");
        }
        else if( attrKind == tk_string )
        {
          attrType = "tk_string";
          countValues = 1;
          attrValue = i_spAttrAcc->GetString(piAttrKey);
        }
        CATIDOMNode_var spChildNode = TCADRTDOMMNode(i_spDoc, attrType, i_spDOMNode, 1);
        if( !!spChildNode )
        {
          TCADRTDOMSetAttr(spChildNode, "name", attrName);
          CATUnicodeString countStr;
          countStr.BuildFromNum(countValues);
          TCADRTDOMSetAttr(spChildNode, "count", countStr);
          CATIDOMText_var spDataNode;
          i_spDoc->CreateTextNode(attrValue, spDataNode);
          spChildNode->AppendChild(spDataNode);
        }
        piAttrKey->Release();
        piAttrKey = NULL;
      }
    }
  }
  return;
} // TCADRTDOMSpecAttrChildNodes

//---------------------------------------------------------------------------
// TCADRTGetXMLString
//--------------------------------------------------------------------------- 
void TCADRTGetXMLString(CATBaseUnknown_var i_spUnk, CATUnicodeString& o_rXMLStr)
{
  static CATBoolean sInited = FALSE;
  static CATListOfCATUnicodeString sOptions;
  static CATListOfCATUnicodeString sValues;
  if( !sInited )
  {
    sOptions.Append("CATExpandEntityReferences");
    sValues.Append("true");
    sInited = TRUE;
  }
  o_rXMLStr = "";
  CATIXMLDOMDocumentBuilder_var spXMLBuilder;
  CATIDOMDocument_var spDOMDoc = TCADRTDOMDocument(spXMLBuilder);
  if( !!spDOMDoc && !!spXMLBuilder && !!i_spUnk )
  {
    CATIDOMNode_var spDOMNode = TCADRTDOMMNode(spDOMDoc, i_spUnk->GetImpl()->IsA());
    TCADRTDOMSetAttr(spDOMNode, "alias", TCADRTGetAlias(i_spUnk));

    // internal name
    CATUnicodeString internalName;
    CATISpecObject_var spSpec = i_spUnk;
    if( !!spSpec )
    {
      internalName = spSpec->GetName();
    }
    TCADRTDOMSetAttr(spDOMNode, "internal_name", internalName);
    // internal name

    // Child node for each spec attr
    TCADRTDOMSpecAttrChildNodes(i_spUnk, spDOMDoc, spDOMNode);
    if( !!spDOMNode )
    {
      spXMLBuilder->Write(spDOMNode, o_rXMLStr, sOptions, sValues);
      o_rXMLStr.ReplaceAll("\n\n", "\n");
    }
  }
  return;
} // TCADRTGetXMLString


//---------------------------------------------------------------------------
// TCADRTReadFile 
//---------------------------------------------------------------------------
HRESULT TCADRTReadFile(CATUnicodeString i_FileName,
                       CATListOfCATUnicodeString& o_rLines,
                       CATBoolean i_PrepareForSemikolons,
                       CATBoolean i_IgnoreEmptyLines)
{
  o_rLines.RemoveAll();
  char str[200000]; // VTA-286 VTA Variantenmanagement Anpassung
  fstream file(i_FileName.ConvertToChar(), ios::in);
  if( file.is_open() )
  {
    while( !file.eof() && file.getline(str, 200000) )
    {
      CATUnicodeString line(str);
      if( !i_IgnoreEmptyLines || line != "" )
      {
        if( i_PrepareForSemikolons )
        {
          CATUnicodeChar quotes('"');
          CATUnicodeChar semicol(';');
          CATUnicodeString pipe('|');
          int length = line.GetLengthInChar();
          CATBoolean opened = FALSE;
          for( size_t i = 0; i < length; i++ )
          {
            CATUnicodeChar letter = line[i];
            if( letter == quotes )
            {
              opened = !opened;
            }
            else if( letter == semicol && opened )
            {
              line.ReplaceSubString(i, 1, pipe);
            }
          }
        }
        o_rLines.Append(line);
      }
    }
  }
  return S_OK;
} // TCADRTReadFile 


//---------------------------------------------------------------------------
// TCAUTLBuildFromNum 
//---------------------------------------------------------------------------
CATUnicodeString TCAUTLBuildFromNum(const double i_Value, const char* i_CFormat)
{
  CATUnicodeString toReturn;
  toReturn.BuildFromNum(i_Value, i_CFormat);
  return toReturn;
} // TCAUTLBuildFromNum

//---------------------------------------------------------------------------
// TCAUTLBuildFromNum
//---------------------------------------------------------------------------
CATUnicodeString TCAUTLBuildFromNum(const double i_Value, CATBoolean i_Round, int i_DecimalPlacesCount)
{
  CATUnicodeString valToReturn;
  valToReturn.BuildFromNum(i_Value);
  if( !i_Round )
    return valToReturn;

  // retrieve precision 
  CATUnicodeString precStr("%.");
  CATUnicodeString prStr;
  prStr.BuildFromNum(i_DecimalPlacesCount);
  precStr.Append(prStr);
  precStr.Append("f");

  // convert with precision 
  valToReturn.BuildFromNum(i_Value, precStr.ConvertToChar());
  return valToReturn;
}

//---------------------------------------------------------------------------
// TCAUTLBuildFromNum 
//---------------------------------------------------------------------------
CATUnicodeString TCAUTLBuildFromNum(const double i_Value, CATBoolean i_Round)
{
  static int decimalPlacesCount = 3;
  return TCAUTLBuildFromNum(i_Value, i_Round, decimalPlacesCount);
} // TCAUTLBuildFromNum

//---------------------------------------------------------------------------
// TCAUTLBuildFromDbl 
//---------------------------------------------------------------------------
CATUnicodeString TCAUTLBuildFromDbl(const double i_Value, int i_DecimalPlacesCount, CATBoolean i_CutZeros)
{
  CATUnicodeString str = TCAUTLBuildFromNum(i_Value, TRUE, i_DecimalPlacesCount);
  if( !i_CutZeros )
    return str;

  int idx = str.SearchSubString(".");
  if( idx < 0 )
    idx = str.SearchSubString(",");

  if( idx > 0 )
  {
    CATBoolean toBreak = FALSE;
    int length = str.GetLengthInChar();
    idx = str.SearchSubString("0", length - 1);
    while( idx > 0 && idx == length - 1 )
    {
      str.Remove(idx);
      if( toBreak )
        break;

      length = str.GetLengthInChar();
      idx = str.SearchSubString("0", length - 1);
      if( idx < 0 )
      {
        idx = str.SearchSubString(".");
        toBreak = idx > -1;
      }
      if( idx < 0 )
      {
        idx = str.SearchSubString(",");
        toBreak = idx > -1;
      }
    }
  }
  return str;
} // TCAUTLBuildFromDbl 

//---------------------------------------------------------------------------
// TCAUTLBuildFromNum 
//---------------------------------------------------------------------------
CATUnicodeString TCAUTLBuildFromNum(const int i_Value)
{
  CATUnicodeString valToReturn;
  valToReturn.BuildFromNum(i_Value);
  return valToReturn;
} // TCAUTLBuildFromNum

//---------------------------------------------------------------------------
// TCAUTLBuildFromNum 
//---------------------------------------------------------------------------
CATUnicodeString TCAUTLBuildFromNum(const unsigned int i_Value)
{
  CATUnicodeString valToReturn;
  valToReturn.BuildFromNum(i_Value);
  return valToReturn;
} // TCAUTLBuildFromNum

//-----------------------------------------------------------------------------
// TCAUTLGetAlias
//-----------------------------------------------------------------------------
CATUnicodeString TCAUTLGetAlias(CATBaseUnknown_var i_spSpecOnFeature)
{
  CATUnicodeString aliasStr;
  CATIAlias_var spAlias = i_spSpecOnFeature;
  if( !!spAlias )
  {
    aliasStr = spAlias->GetAlias();
  }
  return aliasStr;
} // TCAUTLGetAlias

//-----------------------------------------------------------------------------
// TCAUTLSetAlias
//-----------------------------------------------------------------------------
void TCAUTLSetAlias(CATIAlias_var i_spAlias, const CATUnicodeString& i_rAlias)
{
  if( !!i_spAlias )
  {
    i_spAlias->SetAlias(i_rAlias);
    TCADRTRedraw(i_spAlias);
  }
  return;
} // TCAUTLSetAlias

//---------------------------------------------------------------------------
// TCAUTLInst 
//---------------------------------------------------------------------------
CATBaseUnknown_var TCAUTLInst(const CATUnicodeString& i_rType)
{
  CATBaseUnknown_var spUnk = NULL_var;
  CATBaseUnknown* pUnk = NULL;
  HRESULT hr = ::CATInstantiateComponent(i_rType.ConvertToChar(),
                                         IID_CATBaseUnknown, (void**)&pUnk);
  if( SUCCEEDED(hr) && pUnk != NULL )
  {
    spUnk = pUnk;
    pUnk->Release();
    pUnk = NULL;
  }
  else
  {
    cerr << "FAILED to instantiate: '" << i_rType.ConvertToChar() << "'" << endl;
  }
  return spUnk;
} // TCAPAKInst 

//---------------------------------------------------------------------------
// TCAUTLSingleTon
//---------------------------------------------------------------------------
CATBaseUnknown_var TCAUTLSingleTon(void)
{
  static CATBaseUnknown_var s_spSingleTon = NULL_var;
  if( s_spSingleTon == NULL_var )
  {
    s_spSingleTon = TCAUTLInst(CATUnicodeString("UTLSingleTon"));
  }
  return s_spSingleTon;
} // TCAUTLSingleTon

//---------------------------------------------------------------------------
// TCAUTLGetImage
//---------------------------------------------------------------------------
CATPixelImage* TCAUTLGetImage(void)
{
  CATPixelImage* pImg = NULL;
  CATFrmLayout* pCurrentLayout = CATFrmLayout::GetCurrentLayout();
  if( !pCurrentLayout )
  {
    return pImg;
  }

  // test
  CATFrmWindow* pCATWindow = pCurrentLayout->GetCurrentWindow();
  if( pCATWindow )
  {
    cerr << pCATWindow->GetViewer()->GetImpl()->IsA() << endl;
    CATNavigation3DViewer* pCATViewer = (CATNavigation3DViewer*)pCATWindow->GetViewer();
    if( pCATViewer )
    {
      cerr << pCATViewer->GetImpl()->IsA() << endl;
    }
  }
  // test

  if( pCurrentLayout )
  {
    CATFrmWindow* pCurrentWindow = pCurrentLayout->GetCurrentWindow();
    if( pCurrentWindow )
    {
      CATViewer* pViewer = pCurrentWindow->GetViewer();
      if( pViewer )
      {
        pImg = pViewer->GrabPixelImage();
      }
    }
  }
  return pImg;
} // TCAUTLGetImage

//---------------------------------------------------------------------------
// TCAUTLGetAllGSMLines
//---------------------------------------------------------------------------
void TCAUTLGetAllGSMLines(CATListValCATBaseUnknown_var& o_rGSMLines)
{
  o_rGSMLines.RemoveAll();
  CATDocument* pDoc = NULL;
  CATFrmEditor* pEditor = CATFrmEditor::GetCurrentEditor();
  if( pEditor )
  {
    pDoc = pEditor->GetDocument();
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
  int  cntObj = TCAUTLListFromCont(spMechCont, listObj);
  for( int i = 1; i <= cntObj; i++ )
  {
    if( listObj[i]->GetImpl()->IsAKindOf("GSMLine") )
    {
      o_rGSMLines.Append(listObj[i]);
    }
  }
  return;
} // TCAUTLGetAllGSMLines

//---------------------------------------------------------------------------
// TCAUTLGetAllGSMBiDims
//---------------------------------------------------------------------------
void TCAUTLGetAllGSMBiDims(CATListValCATBaseUnknown_var& o_rBiDims)
{
  o_rBiDims.RemoveAll();
  CATDocument* pDoc = NULL;
  CATFrmEditor* pEditor = CATFrmEditor::GetCurrentEditor();
  if( pEditor )
  {
    pDoc = pEditor->GetDocument();
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
  int  cntObj = TCAUTLListFromCont(spMechCont, listObj);
  for( int i = 1; i <= cntObj; i++ )
  {
    if( listObj[i]->GetImpl()->IsAKindOf("GSMBiDim") != 1 || listObj[i]->GetImpl()->IsAKindOf("GSMPlane") )
    {
      continue;
    }
    o_rBiDims.Append(listObj[i]);
  }
  return;
} // TCAUTLGetAllGSMBiDims

//-----------------------------------------------------------------------------
// TCAUTLListFromCont
//-----------------------------------------------------------------------------
int TCAUTLListFromCont(CATIContainer_var i_spClientCont, CATListValCATBaseUnknown_var& o_rList)
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

//-------------------------------------------------------------------------
// TCAUTLGetSelectedElement
//-------------------------------------------------------------------------
CATISpecObject_var TCAUTLGetSelectedElement(void)
{
  CATISpecObject_var spSelected = NULL_var;
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
} // TCAUTLGetSelectedElement

//-----------------------------------------------------------------------------
// TCAUTLReframeOnSpec
//-----------------------------------------------------------------------------
CATISpecObject_var TCAUTLReframeOnSpec(const CATUnicodeString& i_rSubAlias)
{
  CATISpecObject_var spSpecToSelect;
  CATListValCATBaseUnknown_var allSpecs;
  TCAUTLGetAllGSMBiDims(allSpecs);
  for( int i = 1; i <= allSpecs.Size(); i++ )
  {
    CATUnicodeString alias = TCAUTLGetAlias(allSpecs[i]);
    if( alias.SearchSubString(i_rSubAlias) > -1 )
    {
      spSpecToSelect = allSpecs[i];
      break;
    }
  }
  CATI3DGeoVisu_var spVisu = spSpecToSelect;
  if( spVisu == NULL_var )
  {
    return spSpecToSelect;
  }
  CATCSO* pCSO = NULL;
  CATFrmEditor* pFrmEditor = CATFrmEditor::GetCurrentEditor();
  if( pFrmEditor )
  {
    pCSO = pFrmEditor->GetCSO();
  }
  CAT3DViewer* pViewer = NULL;
  CATFrmWindow* pCurrentWindow = NULL;
  CATFrmLayout* pCurrentLayout = CATFrmLayout::GetCurrentLayout();
  if( pCurrentLayout )
  {
    pCurrentWindow = pCurrentLayout->GetCurrentWindow();
    if( pCurrentWindow )
    {
      pViewer = (CAT3DViewer*)pCurrentWindow->GetViewer();
    }
  }
  if( pCSO && pViewer )
  {
    pCSO->Empty();
    CATPathElement* pPathElm = new CATPathElement(spSpecToSelect);
    pCSO->AddElement(pPathElm);
    CAT3DRep* p3DRep = (CAT3DRep*)spVisu->GiveRep();
    if( p3DRep )
    {
      const CAT3DBoundingSphere& bndSphere = p3DRep->GetBoundingElement();
      pViewer->ReframeOn(bndSphere);
      pViewer->Draw();
      if( pCurrentWindow )
      {
        CATFrmNavigGraphicWindow* pFrmNavigGraphicWindow = (CATFrmNavigGraphicWindow*)pCurrentWindow;
        if( pFrmNavigGraphicWindow )
        {
          CATNavigBox* pNavigBox = pFrmNavigGraphicWindow->GetNavigBox();
          if( pNavigBox )
          {
            CATCafCenterGraph myCenterGraph;
            myCenterGraph.CenterGraph("OnCSO", pNavigBox);
          }
        }
      }
    }
    // pPathElm->Release();
  }
  return spSpecToSelect;
} // TCAUTLReframeOnSpec

//---------------------------------------------------------------------------
// addLineRep
//--------------------------------------------------------------------------
void TCAUTLAddLineRep(const CATMathPoint& i_rPnt1, const CATMathPoint& i_rPnt2,
                      const CATUnicodeString& i_rLabel)
{
  CAT3DViewer* p3DViewer = NULL;
  CATFrmLayout* pCurrentLayout = CATFrmLayout::GetCurrentLayout();
  if( pCurrentLayout )
  {
    CATFrmWindow* pCurrentWindow = pCurrentLayout->GetCurrentWindow();
    if( pCurrentWindow )
    {
      p3DViewer = (CAT3DViewer*)pCurrentWindow->GetViewer();
    }
  }
  if( !p3DViewer )
  {
    return;
  }

  float coord[6];
  coord[0] = i_rPnt1.GetX();
  coord[1] = i_rPnt1.GetY();
  coord[2] = i_rPnt1.GetZ();
  coord[3] = i_rPnt2.GetX();
  coord[4] = i_rPnt2.GetY();
  coord[5] = i_rPnt2.GetZ();

  CAT3DLineGP* pLineGP = new CAT3DLineGP(coord, 2, ALLOCATE, LINES);

  CAT3DCustomRep* pLineRep = new CAT3DCustomRep();
  CATGraphicAttributeSet ga;
  ga.SetColorRGBA(0, 255, 0);
  ga.SetColor(TRUECOLOR);
  pLineRep->AddGP(pLineGP, ga);
  p3DViewer->AddRep(pLineRep);
  if( i_rLabel == "" )
  {
    return;
  }

  // ---- Text Representation ----
  float txtHeigh = 200.0f;
  float coordOrig[3];
  coordOrig[0] = ( coord[0] + coord[3] ) / 2.0f;
  coordOrig[1] = ( coord[1] + coord[4] ) / 2.0f;
  coordOrig[2] = ( coord[2] + coord[5] ) / 2.0f;
  CATMathPointf textOrigin(coordOrig);
  CAT3DAnnotationTextGP* pTextGp = new CAT3DAnnotationTextGP(textOrigin, i_rLabel, BASE_LEFT, txtHeigh);
  CATGraphicAttributeSet textGa;
  textGa.SetColorRGBA(0, 255, 0);
  textGa.SetColor(TRUECOLOR);

  CAT3DCustomRep* pTextRep = new CAT3DCustomRep();
  pTextRep->AddGP(pTextGp, textGa);
  p3DViewer->AddRep(pTextRep);

  return;
} // addLineRep

//---------------------------------------------------------------------------
// TCAUTLGetCenterPoint 
//---------------------------------------------------------------------------
CATMathPoint TCAUTLGetCenterPoint(CATIGeometricalElement_var i_spGeomElm)
{
  CATMathPoint mathPoint;
  CATBody_var spCATBody;
  if( !!i_spGeomElm )
  {
    spCATBody = i_spGeomElm->GetBodyResult();
  }
  if( !!spCATBody )
  {
    CATMathBox mathBox;
    spCATBody->GetBoundingBox(mathBox);
    mathBox.GetBoxCenter(mathPoint);
  }
  return mathPoint;
} // TCAUTLGetCenterPoint