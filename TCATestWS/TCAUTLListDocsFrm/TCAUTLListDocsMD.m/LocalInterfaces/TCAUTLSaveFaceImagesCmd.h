#ifndef TCAUTLSaveFaceImagesCmd_h
#define TCAUTLSaveFaceImagesCmd_h
#include "CATCommand.h"   
#include "CATNotification.h"
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
#include "CATISpecAttrAccess.h"
#include "CATISpecAttrKey.h"
#include "CATIVisProperties.h"
#include "CATIGeometricalElement.h"
#include "CATFace.h"
#include "TCAIUTLGo.h"
#include "CATIDrwText.h"
#include "CATIModelEvents.h"
#include "CATI3DGeoVisu.h"

class TCAUTLSaveFaceImagesCmd : public CATCommand
{
private:

  TCAIUTLGo_var m_spSingleTon;

  CATIGeometricalElement_var m_spGeomObj;
  CATLISTP(CATCell) m_Faces;


  CATFace_var m_spFaceToReset;

  CATIDrwText_var m_spAnnot;

public:

  TCAUTLSaveFaceImagesCmd(void);
  virtual ~TCAUTLSaveFaceImagesCmd();

  virtual CATStatusChangeRC Activate(
    CATCommand* iFromClient,
    CATNotification* iEvtDat);

  static void sRunScriptAndTriggerSecondPartTransition(CATCommand* iTCAMTAJobsStateCmd, int iSubscribedType,
                                                       CATString* iScriptName);

private:

  double  getFaceArea(CATFace* i_pFace);

  CATBoolean isInShow(CATBaseUnknown_var i_spUnk);

  int listFromCont(CATIContainer_var i_spClientCont, CATListValCATBaseUnknown_var& o_rList);

  void initGeometry(void);

  CATISpecObject_var getSelectedElement(void);

  void refreshVisu(CATIModelEvents_var i_spModelEvents);

  CATBoolean setFaceColor(const int& i_rFaceIdx = 1);

  void inspectVisu(CATI3DGeoVisu_var i_sp3DGeoVisu);

  void   showCAT3DBagRep(CATRep* i_p3DBagRap, const int& i_rLevel=1);
};
#endif
