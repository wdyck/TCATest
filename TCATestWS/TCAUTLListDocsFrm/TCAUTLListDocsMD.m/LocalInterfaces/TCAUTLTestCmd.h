#ifndef TCAUTLTestCmd_h
#define TCAUTLTestCmd_h
#include "CATCommand.h"   
#include "CATNotification.h"


class TCAUTLTestCmd : public CATCommand
{
public:

  TCAUTLTestCmd(void);
  virtual ~TCAUTLTestCmd();

  virtual CATStatusChangeRC Activate(
    CATCommand* iFromClient,
    CATNotification* iEvtDat);

  static void onTimeOut(CATCommand* i_pToClient, int i_Time, void* i_pUsefulData);

  static void sRunScriptAndTriggerSecondPartTransition(CATCommand* iTCAMTAJobsStateCmd, int iSubscribedType,
                                                       CATString* iScriptName);

};
#endif
