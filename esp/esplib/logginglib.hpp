#pragma warning (disable : 4786)
// logginglib.hpp: interface for the logging listener service.
//
//////////////////////////////////////////////////////////////////////
#ifndef _LOGGINGLIB_HPP__
#define _LOGGINGLIB_HPP__

#include "jiface.hpp"
//#include "logbuilder.h"
//#include "dbconnection.hpp"
//#include "respool.hpp"
//#include "textlogwriter.hpp"
#include <map>
#include <set>
#include "esp.hpp"
//#include "esploggingservice_esp.ipp"

//#include "EspErrors.hpp"
//#include "RetryFailedLogs.hpp"

//extern const char* const POOL_TIMER_NAME;

enum
{
   GENERATE_TRANSACTION_SEED=0,
   WRITE_LOG,
   READ_LOG
};

interface IEspLogListener : extends IEspService
{
    //virtual void Init(IPropertyTree& cfg, const char* theServiceName) = 0;
    //virtual bool Open() = 0;
    //virtual bool Close() = 0;
    //virtual void Write(IEspContext & context,ILogBuilder& pBuilder,const InsertType=SINGLE_INSERT) = 0;
    //virtual StringBuffer& generateTransactionSeed(StringBuffer& SeedId)=0;
    virtual bool doWork(IEspContext& context, IInterface& req, IInterface& resp) = 0;
    //virtual void setLogDir(const char* logdir) = 0;
};

#endif  //_LOGGINGLIB_HPP__
