

#ifndef TCAUTLComponentMacro_H
#define TCAUTLComponentMacro_H

#include "CATICreateInstance.h"     
#include "iostream.h" 
#include  "TIE_CATICreateInstance.h"  

//---------------------------------------------------------------------------
// TCA_UTL_COMPONENT_CPP 
//---------------------------------------------------------------------------                                                                            
#define TCA_UTL_COMPONENT_CPP( TCAUTLComponent )                            \
                                                                            \
CATImplementClass( TCAUTLComponent, Implementation,                         \
                      CATBaseUnknown, CATNull );                            \
TCAUTLComponent :: TCAUTLComponent(){}                                      \
TCAUTLComponent ::~##TCAUTLComponent(){}                                    \
                                                                            \
CATImplementClass(  TCAUTLComponent##Inst, CodeExtension,                   \
                    CATBaseUnknown,  TCAUTLComponent );                     \
                                                                            \
TIE_CATICreateInstance( TCAUTLComponent##Inst);                             \
TCAUTLComponent##Inst :: TCAUTLComponent##Inst(){}                          \
TCAUTLComponent##Inst ::~##TCAUTLComponent##Inst(){}                        \
HRESULT __stdcall TCAUTLComponent##Inst ::CreateInstance (void ** oPPV)     \
{                                                                           \
  TCAUTLComponent *pt = new TCAUTLComponent ();                             \
  if( NULL == pt ) return ( E_OUTOFMEMORY );                                \
  *oPPV = ( void* ) pt;                                                     \
  return S_OK;                                                              \
}                                                                           \

//---------------------------------------------------------------------------
// TCA_UTL_COMPONENT_H
//---------------------------------------------------------------------------                                                                            
#define TCA_UTL_COMPONENT_H( TCAUTLComponent )                              \
class  TCAUTLComponent : public CATBaseUnknown                              \
{                                                                           \
  CATDeclareClass;                                                          \
                                                                            \
  public:                                                                   \
     TCAUTLComponent ();                                                    \
     virtual ~##TCAUTLComponent ();                                         \
                                                                            \
  private:                                                                  \
    TCAUTLComponent ( TCAUTLComponent &);                                   \
    TCAUTLComponent & operator=( TCAUTLComponent &);                        \
};                                                                          \
                                                                            \
class TCAUTLComponent##Inst: public CATBaseUnknown                          \
                                                                            \
{                                                                           \
  CATDeclareClass;                                                          \
  public:                                                                   \
     TCAUTLComponent##Inst ();                                              \
     virtual ~##TCAUTLComponent##Inst ();                                   \
     HRESULT __stdcall CreateInstance (void ** oPPV ) ;                     \
  private:                                                                  \
                                                                            \
    TCAUTLComponent##Inst ( TCAUTLComponent##Inst &);                       \
    TCAUTLComponent##Inst& operator=( TCAUTLComponent##Inst&);              \
                                                                            \
};                                                                          \




















//-----------------------------------------------------------------------
#endif