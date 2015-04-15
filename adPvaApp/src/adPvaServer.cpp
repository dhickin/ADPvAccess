/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author dgh
 * @date 2014.10.19
 */
#include "NDArrayRecord.h"

#include "NDPluginDriver.h"

#include <pv/pvDatabase.h>

#include <epicsExport.h>
#include <iocsh.h>

#include <string>

using namespace epics::pvData;
using namespace epics::pvDatabase;
using std::string;

namespace epics { namespace adpva {

class adPvaServer : public NDPluginDriver {
public:
    adPvaServer(const string & portName, const string & imageName,
        int queueSize, int blockingCallbacks,
        const string & NDArrayPort, int NDArrayAddr,
        int maxbuffers, int maxmemory);

    /** This is called when the plugin gets a new array callback */
    virtual void processCallbacks(NDArray *pArray);

    ~adPvaServer();
protected:
    int arrayPVName;
    #define FIRST_ADPVASERVER_PARAM arrayPVName
    #define LAST_ADPVASERVER_PARAM arrayPVName

    // Number of parameters to support in this plugin
    #define NUM_AD_IMAGE_SERVER_PARAMS ((int)(&LAST_ADPVASERVER_PARAM - &FIRST_ADPVASERVER_PARAM + 1))

private:
    NDArrayRecordPtr record;
    std::string imageName;
};

adPvaServer::adPvaServer(
    const string & portName,
    const string & imageName,
    int queueSize,
    int blockingCallbacks,
    const string & NDArrayPort,
    int NDArrayAddr,
    int maxbuffers,
    int maxmemory)
: NDPluginDriver(portName.c_str(), queueSize, blockingCallbacks, 
      NDArrayPort.c_str(), NDArrayAddr, 1, NUM_AD_IMAGE_SERVER_PARAMS,
      maxbuffers, maxmemory,
      0,  // interfaceMask
      0,  // interruptMask,
      ASYN_CANBLOCK,
      1,  // autoconnect
      0,  // priority
      0), // stack-size
  imageName(imageName)
{
    createParam("arrayPVName", asynParamOctet, &arrayPVName);

    setStringParam(NDPluginDriverPluginType, "EPICS V4 AD Image Server");
    setStringParam(arrayPVName, imageName.c_str());

    callParamCallbacks();
 
    PVDatabasePtr master = PVDatabase::getMaster();
    record = NDArrayRecord::create(imageName);
    bool result = master->addRecord(record);
    if(!result) std::cerr << "recordname" << " not added" << std::endl;

    connectToArrayPort();
}

adPvaServer::~adPvaServer()
{
}


void adPvaServer::processCallbacks(NDArray *pArray)
{
    // Call the base class method first 
    NDPluginDriver::processCallbacks(pArray);

    record->lock();
    try
    {
        record->beginGroupPut();
        record->put(pArray);
        record->endGroupPut();
    }
    catch(...)
    {
       record->unlock();
       throw;
    }
    record->unlock();

    return;
}

}}


extern "C" int adPvaServerConfigure(const char *portName, const char *imageName, int queueSize, int blockingCallbacks,
        const char *NDArrayPort, int NDArrayAddr,
                 int maxbuffers, int maxmemory)
{
    using namespace epics::adpva;
    adPvaServer * aps = new adPvaServer(portName, imageName, queueSize,
        blockingCallbacks, NDArrayPort, NDArrayAddr, maxbuffers, maxmemory);
    aps = NULL;

    return(0);
}


/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "Image PV", iocshArgString};
static const iocshArg initArg2 = { "frame queue size",iocshArgInt};
static const iocshArg initArg3 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg4 = { "NDArray Port",iocshArgString};
static const iocshArg initArg5 = { "NDArray Addr",iocshArgInt};
static const iocshArg initArg6 = { "Max Buffers",iocshArgInt};
static const iocshArg initArg7 = { "Max Memory",iocshArgInt};

static const iocshArg * const initArgs[] = { &initArg0, &initArg1, &initArg2,
    &initArg3, &initArg4, &initArg5, &initArg6, &initArg7 };

static const iocshFuncDef initFuncDef = {"adPvaServerConfigure",7,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    adPvaServerConfigure(args[0].sval, args[1].sval, args[2].ival, args[3].ival, 
                     args[4].sval, args[5].ival,
                     args[6].ival, args[7].ival);
}


extern "C" void adPvaServerRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar( adPvaServerRegister );
}






