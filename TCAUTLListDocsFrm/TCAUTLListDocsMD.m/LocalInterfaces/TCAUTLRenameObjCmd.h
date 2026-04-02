#ifndef TCAUTLRenameObjCmd_h
#define TCAUTLRenameObjCmd_h
#include "CATCommand.h"   
#include "CATNotification.h"


class TCAUTLRenameObjCmd : public CATCommand
{
public:

  TCAUTLRenameObjCmd(void);
  virtual ~TCAUTLRenameObjCmd();

  virtual CATStatusChangeRC Activate(
    CATCommand* iFromClient,
    CATNotification* iEvtDat);

  static void onTimeOut(CATCommand* i_pToClient, int i_Time, void* i_pUsefulData);

  static void sRunScriptAndTriggerSecondPartTransition(CATCommand* iTCAMTAJobsStateCmd, int iSubscribedType,
                                                       CATString* iScriptName); 

};
#endif
