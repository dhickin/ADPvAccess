/* NDArrayRecord.cpp */
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
#include "ndarray.h"

#include <pv/standardPVField.h>
#include <pv/ntndarray.h>

using namespace epics::pvData;
using namespace epics::pvDatabase;
using std::string;

namespace epics { namespace adpva { 

NDArrayRecordPtr NDArrayRecord::create(
    string const & recordName)
{
    nt::NTNDArrayBuilderPtr builder = nt::NTNDArray::createBuilder();
    builder->addDescriptor()->addTimeStamp()->addAlarm()->addDisplay();

    NDArrayRecordPtr pvRecord(
        new NDArrayRecord(recordName,builder->createPVStructure()));    

    if(!pvRecord->init()) pvRecord.reset();

    return pvRecord;
}

NDArrayRecord::NDArrayRecord(
    string const & recordName,
    PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure)
{
}

NDArrayRecord::~NDArrayRecord()
{
}

void NDArrayRecord::destroy()
{
    PVRecord::destroy();
}

bool NDArrayRecord::init()
{
    initPVRecord();
    setTraceLevel(0);
    return true;
}

void NDArrayRecord::put(NDArray *pArray)
{
    putNDArrayToNTNDArray(getPVStructure(), pArray);
}

}}

