#include "TCAUTLSaveFaceImagesCmd.h"
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
#include "CATIModelEvents.h"
#include "CATModify.h"
#include "CATCell.h" 
#include "CATIBRepAccess.h"
#include "CATMfBRepDecode.h"
#include "CATFace.h"
#include "CATDynMassProperties3D.h"
#include "CATSoftwareConfiguration.h"
#include "CATI3DGeoVisu.h" 
#include "CATRender.h" 
#include "CATSupport.h" 
#include "CATGraphicPrimitive.h" 
#include "CATSurfacicRep.h" 
#include "CAT3DFaceGP.h" 
#include "CATVisManager.h" 
#include "CATIBuildPath.h" 
#include "CAT3DViewpoint.h"  
#include "CATRepPath.h" 
// #include "CATGeomGPSet.h" 

CATCreateClass(TCAUTLSaveFaceImagesCmd);

//---------------------------------------------------------------------------
// sRunScriptAndTriggerSecondPartTransition 
//---------------------------------------------------------------------------
void TCAUTLSaveFaceImagesCmd::sRunScriptAndTriggerSecondPartTransition(
  CATCommand* iTCAUTLTestCmd, int iSubscribedType,
  CATString* iScriptName)
{
  TCAIUTLGo_var spSingleTon = TCAUTLSingleTon();
  CATPixelImage* pImg = TCAUTLGetImage();
  if( !pImg || spSingleTon == NULL_var )
    return;

  CATString imgPath = "C:\\EXAMPLES\\MTA\\2026\\20260109_Compare_Doors\\ALIAS\\IMG_NAME.png";

  CATUnicodeString surfAlias;
  spSingleTon->getValue("surf_alias", surfAlias);
  imgPath.ReplaceSubString("ALIAS", CATString(surfAlias.ConvertToChar()));

  CATUnicodeString etiquette;
  spSingleTon->getValue("etiquette", etiquette);
  etiquette.ReplaceAll("*", "_");

  imgPath.ReplaceSubString("IMG_NAME", CATString(etiquette.ConvertToChar()));

  pImg->WriteToFile("PNG", imgPath);
  spSingleTon->setValue("img_path", imgPath);


  CATCommand* pCmd = NULL;
  CATAfrStartCommand("fi", pCmd);

  return;
}  // sRunScriptAndTriggerSecondPartTransition

//---------------------------------------------------------------------------
// TCAUTLSaveFaceImagesCmd
//---------------------------------------------------------------------------
TCAUTLSaveFaceImagesCmd::TCAUTLSaveFaceImagesCmd(void) : CATCommand(NULL, "TCAUTLSaveFaceImagesCmd")
{
  m_spSingleTon = TCAUTLSingleTon();

  RequestStatusChange(CATCommandMsgRequestExclusiveMode);
} // TCAUTLSaveFaceImagesCmd

//---------------------------------------------------------------------------
// ~TCAUTLSaveFaceImagesCmd
//---------------------------------------------------------------------------
TCAUTLSaveFaceImagesCmd::~TCAUTLSaveFaceImagesCmd()
{} // ~TCAUTLSaveFaceImagesCmd

//-------------------------------------------------------------------------
// getSelectedElement
//-------------------------------------------------------------------------
CATISpecObject_var TCAUTLSaveFaceImagesCmd::getSelectedElement(void)
{
  CATISpecObject_var spSelected;

  CATFrmEditor* pFrmEditor = CATFrmEditor::GetCurrentEditor();
  if( pFrmEditor )
  {
    /*
    CATBaseUnknown* pUnk = pFrmEditor->GetCompass();
    if( pUnk )
    {
      cerr << pUnk->GetImpl()->IsA() << endl;
    }*/

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

//-----------------------------------------------------------------------------
// getFaceArea
//-----------------------------------------------------------------------------
double  TCAUTLSaveFaceImagesCmd::getFaceArea(CATFace* i_pFace)
{
  double area = 0.0;
  CATSoftwareConfiguration* pSwConfig = new CATSoftwareConfiguration();
  if( NULL == pSwConfig || !i_pFace )
  {
    return area;
  }
  CATTopData topData(pSwConfig, NULL);
  CATDynMassProperties3D* pDynMassProperties3D = NULL;
  CATTry
  {
    pDynMassProperties3D = CATDynCreateMassProperties3D(&topData,i_pFace);
  }
    CATCatch(CATError, pError)
  {
    pDynMassProperties3D = NULL;
  }
  CATEndTry

    if( pDynMassProperties3D )
    {
      area = pDynMassProperties3D->GetWetArea();
      delete pDynMassProperties3D;
    }

  return area;
}

//---------------------------------------------------------------------------
// setFaceColor
//---------------------------------------------------------------------------
CATBoolean TCAUTLSaveFaceImagesCmd::setFaceColor(const int& i_rFaceIdx)
{
  if( m_spGeomObj == NULL_var || m_spSingleTon == NULL_var )
  {
    return FALSE;
  }

  CATVisPropertiesValues colorRed;
  CATVisPropertiesValues colorWhite;
  colorRed.SetColor(255, 0, 0, 1);
  colorWhite.SetColor(255, 255, 255, 1);

  if( !!m_spFaceToReset )
  {
    CATIBRepAccess_var spBRepAccess(CATBRepDecode(m_spFaceToReset, m_spGeomObj));
    CATIVisProperties_var spBRepAccessAsGraphics = spBRepAccess;
    if( !!spBRepAccessAsGraphics )
    {
      spBRepAccessAsGraphics->SetPropertiesAtt(colorWhite, CATVPColor, CATVPMesh);
      this->refreshVisu(m_spGeomObj);
    }
    m_spFaceToReset = NULL_var;
  }

  int faceIdx = i_rFaceIdx + 1;

  int cnt = m_Faces.Size();
  if( i_rFaceIdx < 1 || i_rFaceIdx >cnt )
  {
    m_spSingleTon->setValue("face_idx", faceIdx); // END
    m_spGeomObj = NULL_var;
    m_spSingleTon->setValue("geom_obj", m_spGeomObj);
    return FALSE;
  }

  CATFace_var spFace = m_Faces[i_rFaceIdx];
  if( spFace == NULL_var )
  {
    return FALSE;
  }

  // test
  this->inspectVisu(spFace);

  CATBoolean repExist = FALSE;

  CATBoolean colorSet = FALSE;
  double area = this->getFaceArea((CATFace*)m_Faces[i_rFaceIdx]);
  if( area > 1000 )
  {
    CATIBRepAccess_var spBRepAccess(CATBRepDecode(spFace, m_spGeomObj));
    CATIVisProperties_var spBRepAccessAsGraphics = spBRepAccess;
    if( !!spBRepAccessAsGraphics )
    {
      CATUnicodeString etiquette;
      CATCell_var spCell = spBRepAccess->GetSelectingCell();
      if( !!spCell )
      {
        int tagCell = spCell->GetPersistentTag();
        etiquette.BuildFromNum(tagCell);
      }
      m_spSingleTon->setValue("etiquette", etiquette);

      spBRepAccessAsGraphics->SetPropertiesAtt(colorRed, CATVPColor, CATVPMesh);


      // test
      // this->inspectVisu(spBRepAccess);

      // test
      // this->inspectVisu(spCell);

      if( !!m_spAnnot )
      {
        CATUnicodeString strIdx;
        strIdx.BuildFromNum(i_rFaceIdx);
        int strLen = strIdx.GetLengthInChar();
        m_spAnnot->SetStringAt(strIdx, 0, strLen);
        this->refreshVisu(m_spAnnot);
      }
      this->refreshVisu(m_spGeomObj);
      colorSet = TRUE;
    }
    m_spFaceToReset = spFace;
  }

  m_spSingleTon->setValue("face_to_reset", m_spFaceToReset);

  m_spSingleTon->setValue("face_idx", faceIdx);


  return colorSet;
}// setFaceColor

 //---------------------------------------------------------------------------
 // refreshVisu
 //---------------------------------------------------------------------------
void TCAUTLSaveFaceImagesCmd::refreshVisu(CATIModelEvents_var i_spModelEvents)
{
  if( !!i_spModelEvents )
  {
    CATBaseUnknown* piUnknown = NULL;
    HRESULT hr = i_spModelEvents->QueryInterface(IID_CATBaseUnknown, (void**)&piUnknown);
    if( SUCCEEDED(hr) && piUnknown != NULL )
    {
      CATModify Notif(piUnknown);
      i_spModelEvents->Dispatch(Notif);

      if( piUnknown != NULL )
      {
        piUnknown->Release();
        piUnknown = NULL;
      }
    }
  } // spModelEvents != NULL_var
  return;
} // refreshVisu

//-----------------------------------------------------------------------------
// TCAListFromCont
//-----------------------------------------------------------------------------
int TCAUTLSaveFaceImagesCmd::listFromCont(CATIContainer_var i_spClientCont, CATListValCATBaseUnknown_var& o_rList)
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

//----------------------------------------------------------------
// isInShow
//----------------------------------------------------------------
CATBoolean TCAUTLSaveFaceImagesCmd::isInShow(CATBaseUnknown_var i_spUnk)
{
  CATShowAttribut showAttr = CATNoShowAttr;
  CATIVisProperties_var spShowAt = i_spUnk;
  if( spShowAt != NULL_var )
  {
    CATVisPropertiesValues myProp;
    spShowAt->GetPropertiesAtt(myProp, CATVPShow, CATVPGlobalType);
    myProp.GetShowAttr(showAttr);
  }
  CATBoolean isShown = showAttr == CATShowAttr;
  return isShown;
} // isInShow

//----------------------------------------------------------------------------------
// inspectVisu
//----------------------------------------------------------------------------------
void TCAUTLSaveFaceImagesCmd::inspectVisu(CATI3DGeoVisu_var i_sp3DGeoVisu)
{
  CATIBuildPath_var spBuildPath = m_spGeomObj;
  CATPathElement* pPathElement = NULL;
  if( !!spBuildPath )
  {
    CATPathElement pathElmContext = CATFrmEditor::GetCurrentEditor()->GetUIActiveObject();
    cerr << spBuildPath->GetImpl()->IsA() << endl;
    spBuildPath->ExtractPathElement(&pathElmContext, &pPathElement);
  }
  if( pPathElement )
  {
    // pPathElement->AddChildElement(i_sp3DGeoVisu);
    int cnt = pPathElement->GetSize();
    cerr << cnt << endl;
    cerr << pPathElement->GetCurrentObject()->GetImpl()->IsA() << endl;

    for( int e = 0; e < cnt; e++ )
    {
      cerr << ( *pPathElement )[e]->GetImpl()->IsA() << endl;
    }

  }

  if( i_sp3DGeoVisu == NULL_var )
  {
    return;
  }
  CATVisManager* pViMng = CATVisManager::GetVisManager();
  if( !pViMng )
  {
    return;
  }
  CAT3DBagRep* p3DBagRep = (CAT3DBagRep*)i_sp3DGeoVisu->GetRep(); // GetRep(); // ;
  if( !p3DBagRep )
  {
    return;
  }



  // Instanciation of our render:
  CATFrmLayout* pCurrentLayout = CATFrmLayout::GetCurrentLayout();
  CATFrmWindow* pCurrentWindow = pCurrentLayout->GetCurrentWindow();
  CATViewer* pViewer = pCurrentWindow->GetViewer();
  const CATSupport& support = pViewer->GetSupport();
  CAT3DViewpoint& viewPoint3D = pViewer->GetMain3DViewpoint();
  CATRender render(support);


  // we create the output structure that the VisManager will fill in.
  CATRepPath rep_path;
  pViMng->GenerateRepPathFromPathElement(( *pPathElement ), &viewPoint3D, rep_path);
  cerr << "rep_path->IsValid(): " << rep_path.IsValid() << endl;

  render.DrawPath(rep_path, viewPoint3D);

  int inside = 0;
  /*
  cerr << p3DBagRep->GetImpl()->IsA() << endl;
  cerr << "p3DBagRep->IsHidden(): " << p3DBagRep->IsHidden() << endl;
  cerr << "p3DBagRep->IsShowFree(): " << p3DBagRep->IsShowFree() << endl;
  cerr << "p3DBagRep->IsInvalid(): " << p3DBagRep->IsInvalid() << endl;
  int okToDraw = p3DBagRep->IsOkToDraw(render, inside);
  cerr << "IsOkToDraw: " << okToDraw << "\t" << inside << endl;
  */

  cerr << viewPoint3D.GetImpl()->IsA() << endl;
  list<CATRep>& repList = viewPoint3D.GetRepList();
  int len1 = repList.length();
  cerr << "repList.length(): " << len1 << endl;
  for( int r = 0; r < len1; r++ )
  {
    this->showCAT3DBagRep(repList[r]);
    /*

    cerr << repList[r]->GetImpl()->IsA() << endl;
    CAT3DBagRep* p3DRep = (CAT3DBagRep*)repList[r];
    if( p3DRep )
    {

      list<CATRep>* pRepChildren = ( (CAT3DBagRep*)p3DRep )->GetChildren();
      if( pRepChildren )
      {
        int len = pRepChildren->length();
        cerr << "pRepChildren->length(): " << len << endl;
        for( int s = 0; s < len; s++ )
        {
          cerr << "\t" << ( *pRepChildren )[s]->GetImpl()->IsA() << endl;
          CAT3DBagRep* p3DRep1 = (CAT3DBagRep*)( *pRepChildren )[s];
          if( p3DRep1 )
          {
            list<CATRep>* pRepChildren1 = p3DRep1->GetChildren();
            int cnt1 = pRepChildren1->length();
            cerr << "pRepChildren1->length(): " << cnt1 << endl;
            for( int s1 = 0; s1 < cnt1; s1++ )
            {
              this->showCAT3DBagRep(( *pRepChildren1 )[s1]);
              // CATSurfacicRep* pRep = (CATSurfacicRep*)( *pRepChildren1 )[s1];
              // if( pRep )
              {
              }
            }
          }
        }
      }
    }*/
  }
  if( true )
  {
    return;
  }

  // !!!!!
  // const CAT3DBagRep* p3DBagRep = viewPoint3D.GetBag();
  if( p3DBagRep )
  {
    cerr << p3DBagRep->GetImpl()->IsA() << endl;
    list<CATRep>* pRepChildren = ( (CAT3DBagRep*)p3DBagRep )->GetChildren();
    if( pRepChildren )
    {
      int len = pRepChildren->length();
      cerr << "pRepChildren->length(): " << len << endl;
      for( int s = 0; s < len; s++ )
      {
        CATSurfacicRep* pRep = (CATSurfacicRep*)( *pRepChildren )[s];
        if( pRep )
        {

          cerr << pRep->GetImpl()->IsA() << endl;

          cerr << "pRep->IsHidden(): " << pRep->IsHidden() << endl;
          cerr << "pRep->IsShowFree(): " << pRep->IsShowFree() << endl;
          cerr << "pRep->IsInvalid(): " << pRep->IsInvalid() << endl;

          int okToDraw = pRep->IsOkToDraw(render, inside);
          cerr << "IsOkToDraw: " << okToDraw << "\t" << inside << endl;

          list<CATRep>* pSurfChildren = pRep->GetChildren();
          if( pSurfChildren )
          {
            int len = pSurfChildren->length();
            cerr << "pSurfChildren->length(): " << len << endl;
            for( int s = 0; s < len; s++ )
            {
              CATRep* pRep = ( *pSurfChildren )[s];
              if( pRep )
              {
                cerr << pRep->GetImpl()->IsA() << endl;
                cerr << "pRep->IsHidden(): " << pRep->IsHidden() << endl;
                cerr << "pRep->IsShowFree(): " << pRep->IsShowFree() << endl;
                cerr << "pRep->IsInvalid(): " << pRep->IsInvalid() << endl;

                okToDraw = pRep->IsOkToDraw(render, inside);
                cerr << "IsOkToDraw: " << okToDraw << "\t" << inside << endl;
              }
            }
          }

        }
      }
    }
  }



  // we get the positionning matrix of the last graphical repreesntation
  int repPathCnt = rep_path.Size();
  for( int p = 0; p < repPathCnt; p++ )
  {
    CATRep* pRepGen = rep_path[p];
    // we test whether or not it is a graphical representation bag
    if( pRepGen )
    {
      cerr << pRepGen->GetImpl()->IsA() << endl;
      cerr << "pRepGen->IsHidden(): " << pRepGen->IsHidden() << endl;
      cerr << "pRepGen->IsShowFree(): " << pRepGen->IsShowFree() << endl;
      cerr << "pRepGen->IsInvalid(): " << pRepGen->IsInvalid() << endl;

      int okToDraw = pRepGen->IsOkToDraw(render, inside);
      cerr << "IsOkToDraw: " << okToDraw << "\t" << inside << endl;

      list<CATRep>* pSurfChildren = pRepGen->GetChildren();
      if( pSurfChildren )
      {
        int len = pSurfChildren->length();
        cerr << "pSurfChildren->length(): " << len << endl;
        for( int s = 1; s <= len; s++ )
        {
          CATRep* pRep = ( *pSurfChildren )[s];
          if( pRep )
          {
            cerr << pRep->GetImpl()->IsA() << endl;
            cerr << "pRep->IsHidden(): " << pRep->IsHidden() << endl;
            cerr << "pRep->IsShowFree(): " << pRep->IsShowFree() << endl;
            cerr << "pRep->IsInvalid(): " << pRep->IsInvalid() << endl;

            okToDraw = pRep->IsOkToDraw(render, inside);
            cerr << "IsOkToDraw: " << okToDraw << "\t" << inside << endl;
          }
        }
      }
    }
  }
  return;
} // inspectVisu

//----------------------------------------------------------------------------------
// showCAT3DBagRep
//----------------------------------------------------------------------------------
void   TCAUTLSaveFaceImagesCmd::showCAT3DBagRep(CATRep* i_pCATRap, const int& i_rLevel)
{
  if( !i_pCATRap )
  {
    return;
  }
  for( int i = 0; i < i_rLevel; i++ ) cerr << "\t";
  cerr << i_pCATRap->GetImpl()->IsA() << endl;
  if( i_pCATRap->GetImpl()->IsAKindOf("CAT3DBagRep") )
  {
    CAT3DBagRep* p3DBagRep = (CAT3DBagRep*)i_pCATRap;
    if( p3DBagRep )
    {
      list<CATRep>* pRepChildren = p3DBagRep->GetChildren();
      if( pRepChildren )
      {
        int len = pRepChildren->length();
        for( int s = 0; s < len; s++ )
        {
          this->showCAT3DBagRep(( *pRepChildren )[s], i_rLevel + 1);
        }
      }
    }
  }
  else if( i_pCATRap->GetImpl()->IsAKindOf("CATSurfacicRep") )
  {
    // Instanciation of our render:
    CATFrmLayout* pCurrentLayout = CATFrmLayout::GetCurrentLayout();
    CATFrmWindow* pCurrentWindow = pCurrentLayout->GetCurrentWindow();
    CATViewer* pViewer = pCurrentWindow->GetViewer();
    CAT3DViewpoint& viewPoint3D = pViewer->GetMain3DViewpoint();
    const CATSupport& support = pViewer->GetSupport();
    CATRender render(support);

    CATSurfacicRep* pSurfRep = (CATSurfacicRep*)i_pCATRap;
    if( !!pSurfRep )
    {

      const CAT3DBoundingSphere& bndSphere = pSurfRep->GetBoundingElement();

      const CATRepPath* pRepPath = pSurfRep->GetBoundingBoxPath();
      if( pRepPath )
      {
        // render.DrawPath((*pRepPath), viewPoint3D);

        CATRep* pCurrentRep = ( (CATRepPath*)pRepPath )->GetCurrentRep();
        if( pCurrentRep )
        {
          this->showCAT3DBagRep(pCurrentRep, i_rLevel + 1);
        }

      }
      int nbFaces = pSurfRep->GeomNumberOfFaces();
      for( int i = 0; i < i_rLevel; i++ ) cerr << "\t";
      cerr << "Numnber faces: " << nbFaces << endl;
      for( int f = 0; f < nbFaces; f++ )
      {
        CATGraphicAttributeSet grAttrSet;
        if( SUCCEEDED(pSurfRep->GetGeomFaceAttribut(f, grAttrSet)) )
        {
          for( int i = 0; i < i_rLevel; i++ ) cerr << "\t";
          cerr << "IsDrawable: " << render.IsDrawable(grAttrSet, *i_pCATRap) << endl;


          CAT3DFaceGP* pFaceGP = pSurfRep->GeomFace(f);
          if( pFaceGP )
          {
            for( int i = 0; i < i_rLevel; i++ ) cerr << "\t";
            pFaceGP->Draw(render);
            cerr << pFaceGP->GetImpl()->IsA() << ": " << pFaceGP->GetIsEmpty() << endl;
            cerr << "Get3DModelCulling:\t" << render.Get3DModelCulling(bndSphere) << endl;



            // cerr << "IsDrawable: " << render.IsDrawable(grAttrSet) << endl;

            /*

            // Assume pPrimitive is a valid pointer to a CATGraphicPrimitive
            int drawable = pFaceGP->IsDrawable(attrSet);

            if( drawable == 1 )
            {
              float culling = pFaceGP->Get3DModelCulling(sphere);

              if( culling > 0.0f )
              {
                // ✅ The primitive is drawable and not culled → visible in 3D
                cout << "Primitive is shown in 3D space." << endl;
              }
              else
              {
                // ⚙️ Drawable but culled (inside no active frustum, too small, etc.)
                cout << "Primitive currently culled, not shown in 3D." << endl;
              }
            }
            else
            {
              // ❌ Not drawable at all (hidden or with no geometry)
              cout << "Primitive not drawable, not shown in 3D." << endl;
            }*/
          }
          for( int i = 0; i < i_rLevel; i++ ) cerr << "\t";
          cerr << "\tIsHidden:\t" << grAttrSet.IsHidden() << endl;
          for( int i = 0; i < i_rLevel; i++ ) cerr << "\t";
          cerr << "\tIsShowFree:\t" << grAttrSet.IsShowFree() << endl;
        }

      }
    }
  }
  return;
} // showCAT3DBagRep

//----------------------------------------------------------------------------------
// initGeometry
//----------------------------------------------------------------------------------
void TCAUTLSaveFaceImagesCmd::initGeometry(void)
{
  this->getSelectedElement(); // to hide the compass
  if( m_spSingleTon == NULL_var )
  {
    return;
  }

  m_spGeomObj = m_spSingleTon->getValue("geom_obj");
  if( !!m_spGeomObj )
  {
    m_spAnnot = m_spSingleTon->getValue("annotation");
    m_spFaceToReset = m_spSingleTon->getValue("face_to_reset");
    m_spSingleTon->getValue("all_faces", m_Faces);

    CATUnicodeString surfAlias = TCAUTLGetAlias(m_spGeomObj);
    m_spSingleTon->setValue("surf_alias", surfAlias);

    return;
  }

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
  int  cntObj = this->listFromCont(spMechCont, listObj);

  for( int i = 1; i <= cntObj; i++ )
  {
    if( listObj[i]->GetImpl()->IsAKindOf("CATTPSText") )
    {
      m_spAnnot = listObj[i];
    }

    if( listObj[i]->GetImpl()->IsAKindOf("GSMBiDim") != 1 || listObj[i]->GetImpl()->IsAKindOf("GSMPlane") || !this->isInShow(listObj[i]) )
    {
      continue;
    }


    m_spGeomObj = listObj[i];
    if( !!m_spGeomObj && !!m_spAnnot )
    {
      break;
    }
  }
  if( !!m_spGeomObj )
  {
    CATBody_var spCATBody = m_spGeomObj->GetBodyResult();
    if( !!spCATBody )
    {
      spCATBody->GetAllCells(m_Faces, 2);
      m_spSingleTon->setValue("geom_obj", m_spGeomObj);
      m_spSingleTon->setValue("all_faces", m_Faces);
      int faceIdx = 1;
      int faceCnt = m_Faces.Size();
      m_spSingleTon->setValue("face_idx", faceIdx);
      m_spSingleTon->setValue("face_cnt", faceCnt);
      m_spSingleTon->setValue("annotation", m_spAnnot);
    }
  }
  return;
} // initGeometry

//---------------------------------------------------------------------------
// Activate
//---------------------------------------------------------------------------
CATStatusChangeRC TCAUTLSaveFaceImagesCmd::Activate(CATCommand* FromClient, CATNotification* EvtDat)
{
  this->initGeometry();
  if( !!m_spGeomObj )
  {
    // test
    this->inspectVisu(m_spGeomObj);

    int faceIdx = 0;
    int faceCnt = 0;
    m_spSingleTon->getValue("face_idx", faceIdx);
    m_spSingleTon->getValue("face_cnt", faceCnt);
    if( faceIdx > 0 && faceIdx <= faceCnt )
    {
      CATBoolean colorSet = this->setFaceColor(faceIdx);
      if( colorSet )
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
      }
      else
      {
        CATCommand* pCmd = NULL;
        CATAfrStartCommand("fi", pCmd);
      }
    }
  }
  this->RequestDelayedDestruction();
  return ( CATStatusChangeRCCompleted );
}
