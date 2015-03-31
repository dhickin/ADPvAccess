/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author dgh
 * @date 2014.10.19
 */
#ifndef ADPVA_NDARRAY_H
#define ADPVA_NDARRAY_H

#include <pv/pvData.h>

class NDArray;

namespace epics { namespace adpva { 

epics::pvData::PVStructurePtr createNTNDArray();
void putNDArrayToNTNDArray(const epics::pvData::PVStructurePtr & pvStructure, NDArray *pArray);

}}

#endif

