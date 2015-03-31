/* NDArrayRecord.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author dgh
 * @date 2014.10.19
 */
#ifndef NDARRAYRECORD_H
#define NDARRAYRECORD_H

#include <pv/pvDatabase.h>
#include "NDArray.h"

namespace epics { namespace adpva { 


class NDArrayRecord;
typedef std::tr1::shared_ptr<NDArrayRecord> NDArrayRecordPtr;

class epicsShareClass NDArrayRecord :
    public epics::pvDatabase::PVRecord
{
public:
    POINTER_DEFINITIONS(NDArrayRecord);
    static NDArrayRecordPtr create(
        std::string const & recordName);
    virtual ~NDArrayRecord();
    virtual bool init();
    virtual void destroy();
    void put(NDArray *pArray);

private:
    NDArrayRecord(std::string const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
};


}}

#endif  // NDARRAYRECORD_H
