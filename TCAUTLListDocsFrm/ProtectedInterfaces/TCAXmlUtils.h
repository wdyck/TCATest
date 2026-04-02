
#ifndef TCAXmlUtils_H
#define TCAXmlUtils_H

#include "CATIDOMText.h"
#include "CATListOfCATUnicodeString.h"
#include "CATIXMLDOMDocumentBuilder.h"
#include "CATIDOMElement.h"
#include "CATIDOMDocument.h"
#include "CATIDOMImplementation.h"
#include "CATIDOMDocumentType.h"
#include "CATIDOMNode.h"
#include "CATIDOMAttr.h"
#include "CATIDOMNamedNodeMap.h"
#include "CATIDOMNodeList.h"
#include "CATError.h"
#include "CATISAXXMLReader.h"
#include "CATISAXParser.h"
#include "CATISAXInputSource.h"
#include "CATIXMLSAXFactory.h"
#include "CATIAlias.h"
#include "CATIDOMDocument.h"
#include "TCADRTXmlMD.h"
#include "CATCommandHeader.h" 
#include "CATCmdAccess.h"
#include "CATDialog.h"
#include "CATSession.h"
#include "CATSessionServices.h"
#include "CATICreateInstance.h"     
#include "CATBaseUnknown.h" 
#include "CATISpecObject.h" 
#include "CATUnicodeString.h" 
#include "iostream.h" 
#include "CATISpecAttrAccess.h" 
#include "CATISpecAttrKey.h" 
#include "CATToken.h"
#include "CATIView.h"
#include "CATTimeSpan.h"
#include "CATISpecAttrKey.h"
#include "CATLISTV_CATISpecAttrKey.h"
#include "CATISpecAttrAccess.h"
#include "CATISpecAttribute.h"
#include "iostream.h"
#include "CATStdLib.h"
#include "CATLib.h"
#include "CATListOfInt.h"
#include "fstream.h"
#include "CATIRedrawEvent.h"
#include "CATPixelImage.h"
#include "CATIContainer.h"  
#include "CATMathPoint.h"
#include "CATIGeometricalElement.h"

#define ENDL '\n'

//---------------------------------------------------------------------------
#define UTL_INST( obj )	TCAUTLInst( #obj )
//---------------------------------------------------------------------------

#ifndef UTL_QUERY
//---------------------------------------------------------------------------
// UTL_QUERY  
//---------------------------------------------------------------------------                                                                            
#define UTL_QUERY( toQuery , Interface, piResult )                           \
                                                                             \
if( !toQuery || toQuery == NULL_var )                                        \
{                                                                            \
  cerr  <<  "\n--FAILED--" << endl                                           \
        <<  #toQuery  " is NULL or NULL_var\n"                              \
        << "file: " << __FILE__ << endl                                       \
        << "line: " << __LINE__ << endl << endl;                                     \
}                                                                            \
else if( FAILED( toQuery->QueryInterface( IID_##Interface ,(void**)&piResult )))  \
{                                                                            \
  cerr  <<  "\n--FAILED--" << endl                                             \
        <<  #toQuery << "->QueryInterface( IID_" << #Interface << ", (void**)&" << #piResult << " );" \
        << endl << "Object: " << toQuery->GetImpl()->IsA() << endl      \
        << "file: " << __FILE__ << endl                                 \
        << "line: " << __LINE__ << endl << endl;                        \
}       
#endif


ExportedByTCADRTXmlMD void TCADRTRedraw(CATIRedrawEvent_var i_spRedrawEvent);

ExportedByTCADRTXmlMD CATUnicodeString TCADRTGetAlias(CATBaseUnknown_var i_spUnk);

ExportedByTCADRTXmlMD CATIDOMDocument_var TCADRTDOMDocument(CATIXMLDOMDocumentBuilder_var& o_rspXMLBuilder);
ExportedByTCADRTXmlMD CATIDOMNode_var TCADRTDOMMNode(CATIDOMDocument_var i_spDoc, const CATUnicodeString& i_rNodeName, CATIDOMNode_var i_spParentNode = NULL_var, const int& i_rLevel = 0);
ExportedByTCADRTXmlMD void TCADRTDOMSpecAttrChildNodes(CATISpecAttrAccess_var i_spAttrAcc, CATIDOMDocument_var i_spDoc, CATIDOMNode_var i_spDOMNode);
ExportedByTCADRTXmlMD void TCADRTDOMSetAttr(CATIDOMElement_var i_spDOMElm, const CATUnicodeString& i_rAttrName, const CATUnicodeString& i_rAttrValue);
ExportedByTCADRTXmlMD void TCADRTGetXMLString(CATBaseUnknown_var i_spUnk, CATUnicodeString& o_rXMLStr);
ExportedByTCADRTXmlMD HRESULT TCADRTReadFile(CATUnicodeString i_FileName,
                                             CATListOfCATUnicodeString& o_rLines,
                                             CATBoolean i_PrepareForSemikolons = FALSE,
                                             CATBoolean i_IgnoreEmptyLines = TRUE);

// build CATUnicodeString from double with precision read from CATSettings
//------------------------------------------------------------------------
ExportedByTCADRTXmlMD CATUnicodeString TCAUTLBuildFromNum(const double i_Value, CATBoolean i_Round, int i_DecimalPlacesCount);
ExportedByTCADRTXmlMD CATUnicodeString TCAUTLBuildFromNum(const double i_Value, CATBoolean i_Round);
ExportedByTCADRTXmlMD CATUnicodeString TCAUTLBuildFromNum(const double i_Value, const char* i_CFormat);
ExportedByTCADRTXmlMD CATUnicodeString TCAUTLBuildFromDbl(const double i_Value, int i_DecimalPlacesCount = 3, CATBoolean i_CutZeros = TRUE);


ExportedByTCADRTXmlMD CATUnicodeString TCAUTLBuildFromNum(const int i_Value);
ExportedByTCADRTXmlMD CATUnicodeString TCAUTLBuildFromNum(const unsigned int i_Value);

ExportedByTCADRTXmlMD CATUnicodeString TCAUTLGetAlias(CATBaseUnknown_var i_spSpecOnFeature);
ExportedByTCADRTXmlMD void TCAUTLSetAlias(CATIAlias_var i_spAlias, const CATUnicodeString& i_rAlias);
ExportedByTCADRTXmlMD CATBaseUnknown_var TCAUTLInst(const CATUnicodeString& i_rType);
ExportedByTCADRTXmlMD CATBaseUnknown_var TCAUTLSingleTon(void);


ExportedByTCADRTXmlMD CATPixelImage* TCAUTLGetImage(void);

ExportedByTCADRTXmlMD CATISpecObject_var TCAUTLGetSelectedElement(void);
ExportedByTCADRTXmlMD void TCAUTLGetAllGSMBiDims(CATListValCATBaseUnknown_var& o_rBiDims);
ExportedByTCADRTXmlMD void TCAUTLGetAllGSMLines(CATListValCATBaseUnknown_var & o_rGSMLines);
ExportedByTCADRTXmlMD int TCAUTLListFromCont(CATIContainer_var i_spClientCont, CATListValCATBaseUnknown_var& o_rList);
ExportedByTCADRTXmlMD CATISpecObject_var TCAUTLReframeOnSpec(const CATUnicodeString& i_rSubAlias);
ExportedByTCADRTXmlMD void TCAUTLAddLineRep(const CATMathPoint& i_rPnt1, const CATMathPoint& i_rPnt2, const CATUnicodeString& i_rLabel = "");
ExportedByTCADRTXmlMD CATMathPoint TCAUTLGetCenterPoint(CATIGeometricalElement_var i_spGeomElm);

//-----------------------------------------------------------------------
#endif

