/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author dgh
 * @date 2014.10.19
 */
#include "ndarray.h"

#include "NDArray.h"

#include <pv/pvData.h>
#include <pv/standardField.h>
#include <pv/ntndarray.h>

#include <epicsTime.h>

#include <cmath>

// set to 1 for older version of NDArray
#define AREA_DETECTOR_MAJOR_VERSION 2

using namespace epics::pvData;
using std::string;
using std::tr1::static_pointer_cast;

namespace epics { namespace adpva { 

// EPICS epoch as seconds after UNIX epoch
const int64_t EPICS_EPOCH = 631152000;

const std::string ntImageStr = "epics:nt/NTNDArray:1.0";


template<typename E>
struct ndarray_deleter
{
   ndarray_deleter(NDArray * ndarray)
   : m_ndarray(ndarray)
   {
   }

   void operator()(E a){ m_ndarray->release(); }

private:
    NDArray * m_ndarray;
};


StructureConstPtr getNDArrayStruc()
{
    static StructureConstPtr arrayStruc;

    if (arrayStruc == NULL)
    {
        nt::NTNDArrayBuilderPtr builder = nt::NTNDArray::createBuilder();
        builder->addDescriptor()->addTimeStamp()->addAlarm()->addDisplay();
        arrayStruc = builder->createStructure();
    }

    return arrayStruc;
}

StructureConstPtr getAttributeStruc()
{
    static StructureConstPtr attributeStruc;

    if (attributeStruc == NULL)
    {
        attributeStruc = std::tr1::dynamic_pointer_cast<const StructureArray>(
            getNDArrayStruc()->getField("attribute"))->getStructure();
        //attributeStruc = getNDArrayStruc()->
        //   getField<StructureArray>("attribute")->getStructure();
    }
    return attributeStruc;
}


template <typename T>
void setValueField(const PVUnionPtr & valueField, NDArray *pArray, int dataSize)
{
    typedef typename T::value_type TValueType;

    string fieldName = ScalarTypeFunc::name(T::typeCode);
    fieldName += "Value";

    std::tr1::shared_ptr<T> storedValue = valueField->select<T>(fieldName);

    if (storedValue.get())
    {
        pArray->reserve();
        int storedArrayLength = dataSize / sizeof(TValueType);
        shared_vector<TValueType> sv((TValueType*)pArray->pData,  
            ndarray_deleter<TValueType*>(pArray), 0, storedArrayLength);
        shared_vector<const TValueType> csv = freeze(sv);

        storedValue->replace(csv);
        valueField->postPut();
    }
}

void setValueField(const PVUnionPtr & valueField, NDArray *pArray, int dataSize)
{
    switch (pArray->dataType)
    {
    case NDInt8:
        setValueField<PVByteArray>(valueField, pArray, dataSize);
        break;
    
    case NDUInt8:
        setValueField<PVUByteArray>(valueField, pArray, dataSize);
        break;
    
    case NDInt16:
        setValueField<PVShortArray>(valueField, pArray, dataSize); 
        break;
    
    case NDUInt16:
        setValueField<PVUShortArray>(valueField, pArray, dataSize);      
        break;
    
    case NDInt32:
        setValueField<PVIntArray>(valueField, pArray, dataSize);
        break;
    
    case NDUInt32:
        setValueField<PVUIntArray>(valueField, pArray, dataSize);
        break;
    
    case NDFloat32:
        setValueField<PVFloatArray>(valueField, pArray, dataSize);
        break;
    
    case NDFloat64:
        setValueField<PVDoubleArray>(valueField, pArray, dataSize);
        break;
    }
}


void setDimensionField(const PVStructureArrayPtr & dimensionField, NDArray *pArray)
{
    StructureConstPtr dimensionStruc = dimensionField->getStructureArray()->getStructure();
    PVStructureArray::svector dimension(dimensionField->reuse());
    dimension.resize(pArray->ndims);

    for (int i = 0; i < pArray->ndims; ++i)
    {
        PVStructurePtr const & dimensionElement = dimension[i];
        if(dimensionElement.get() == NULL || !dimensionElement.unique())
            dimension[i] = getPVDataCreate()->createPVStructure(dimensionStruc);

        dimensionElement->getSubField<PVInt>("size")->put(pArray->dims[i].size);
        dimensionElement->getSubField<PVInt>("offset")->put(pArray->dims[i].offset);
        dimensionElement->getSubField<PVInt>("fullSize")->put(pArray->dims[i].size + pArray->dims[i].offset);
        dimensionElement->getSubField<PVInt>("binning")->put(pArray->dims[i].binning);
        dimensionElement->getSubField<PVBoolean>("reverse")->put(pArray->dims[i].reverse);
    }
    dimensionField->replace(freeze(dimension));
}


void setTimeStampField(const PVStructurePtr & timeStampField, int64_t secPastUnixEpoch, int32_t nsec)
{
    timeStampField->getSubField<PVLong>("secondsPastEpoch")->put(secPastUnixEpoch);
    timeStampField->getSubField<PVInt>("nanoseconds")->put(nsec);
    timeStampField->getSubField<PVInt>("userTag")->put(0);
}

void setTimeStampField(const PVStructurePtr & timeStampField, epicsTimeStamp ts)
{
    setTimeStampField(timeStampField, ts.secPastEpoch + EPICS_EPOCH, ts.nsec);
}

void setTimeStampField(const PVStructurePtr & timeStampField, double secsPastUnixEpoch)
{
    double mod = (secsPastUnixEpoch >= 0) ? 0.5 : -0.5;
    int64_t secs = static_cast<int64_t>(floor(secsPastUnixEpoch + mod));
    int32_t nsecs = static_cast<int32_t>((secsPastUnixEpoch-secs) * 1e9);
    setTimeStampField(timeStampField, secs, nsecs);
}


template <typename T>
void setAttributeStoredValue(PVUnionPtr const & attributeValue,
    NDAttribute * attribute, NDAttrDataType_t dataType)
{
    PVFieldPtr storedValue = attributeValue->get();

    typedef std::tr1::shared_ptr<T> T_Ptr;

    typename T::value_type value = 0;
    int status = attribute->getValue(dataType,
        &value,
        //dataSize
        0);

    if (status != ND_ERROR)
    {
        T_Ptr storedValue_T = std::tr1::dynamic_pointer_cast<T>(storedValue);

        if (storedValue_T.get() == NULL)
        {
            storedValue_T = getPVDataCreate()->createPVScalar<T>();
        }
        storedValue_T->put(value);
        attributeValue->set(storedValue_T);
    }
    else
    {
        storedValue.reset();
    }
}

void setAttributeStoredString(PVUnionPtr const & attributeValue,
    NDAttribute * attribute, size_t size)
{
    PVFieldPtr storedValue = attributeValue->get();;
    char cstr[size];

    int status = attribute->getValue(NDAttrString, cstr, size);

    if (status != ND_ERROR)
    {
        PVStringPtr storedString = std::tr1::dynamic_pointer_cast<PVString>(storedValue);

        if (storedString.get() == NULL)
        {
            storedString = getPVDataCreate()->createPVScalar<PVString>();
        }
        std::string value(cstr);
        storedString->put(value);
        attributeValue->set(storedString);
    }
    else
    {
        storedValue.reset();
    }
}

void setAttributeStoredValue(PVUnionPtr const & attributeValue,
    NDAttribute * attribute)
{
    NDAttrDataType_t dataType;
    size_t size = 0;
    int status = attribute->getValueInfo(&dataType, &size);

    if (status != ND_ERROR)
    {
        switch (dataType)
        {
        case NDAttrInt8:
            setAttributeStoredValue<PVByte>(attributeValue, attribute, dataType);
            break;

        case NDAttrInt16:
            setAttributeStoredValue<PVShort>(attributeValue,attribute, dataType);
            break;

        case NDAttrInt32:
           setAttributeStoredValue<PVInt>(attributeValue, attribute, dataType);
           break;

        case NDAttrFloat32:
            setAttributeStoredValue<PVFloat>(attributeValue, attribute, dataType);
            break;

        case NDAttrFloat64:
            setAttributeStoredValue<PVDouble>(attributeValue, attribute, dataType);
            break;

        case NDAttrString:
            setAttributeStoredString(attributeValue, attribute, size); 
            break;

        default:
            break;
        }
    }
}

template <typename T>
PVFieldPtr createAttributeStoredValue(NDAttribute * attribute, NDAttrDataType_t dataType)
{
    PVFieldPtr attStoredValue;
    typename T::value_type value = 0;
    int status = attribute->getValue(dataType,
        &value,
        //dataSize
        0);
    if (status != ND_ERROR)
    {
        std::tr1::shared_ptr<T> storedValue = getPVDataCreate()->createPVScalar<T>();
        storedValue->put(value);
        attStoredValue = storedValue;
    }
    return attStoredValue;
}


void setAttribute(PVStructurePtr const & attribute,
     NDAttribute * ndattribute)
{
#if AREA_DETECTOR_MAJOR_VERSION >= 2
    attribute->getSubField<PVString>("name")->put(ndattribute->getName());
    attribute->getSubField<PVString>("descriptor")->put(ndattribute->getDescription());
    NDAttrSource_t sourceType;
    ndattribute->getSourceInfo(&sourceType);
    attribute->getSubField<PVInt>("sourceType")->put(static_cast<int>(sourceType));
    attribute->getSubField<PVString>("source")->put(ndattribute->getSource());
#else
    attribute->getSubField<PVString>("name")->put(ndattribute->pName);
    attribute->getSubField<PVString>("descriptor")->put(ndattribute->pDescription);
    attribute->getSubField<PVInt>("sourceType")->put(static_cast<int>(ndattribute->sourceType));
    attribute->getSubField<PVString>("source")->put(ndattribute->pSource);
#endif
    setAttributeStoredValue(attribute->getSubField<PVUnion>("value"), ndattribute);


}


void setAttributeField(const PVStructureArrayPtr & attributeField, NDAttributeList * list)
{
    const int numberOfAttributes = list->count();

    StructureConstPtr attributeStruc = attributeField->getStructureArray()->getStructure();

    PVStructureArray::svector attribute(attributeField->reuse());
    attribute.resize(numberOfAttributes);

    NDAttribute * ndattribute = list->next(NULL);

    size_t i = 0;
    while (ndattribute != NULL)
    {
        PVStructurePtr const & attributeElement = attribute[i];
        if (attributeElement.get() == NULL || !attributeElement.unique())
        {
            attribute[i++] = getPVDataCreate()->
                createPVStructure(getAttributeStruc());
        }
        setAttribute(attributeElement, ndattribute);
        ndattribute = list->next(ndattribute);
    }
    attribute.slice(0,i);
    attributeField->replace(freeze(attribute));
}


void setValueAndCodec(const PVStructurePtr & pvStructure, NDArray *pArray)
{ 
    NDArrayInfo_t ndarr_info;
    pArray->getInfo(&ndarr_info);

    int dataSize = ndarr_info.totalBytes;
    int compressedSize = dataSize;
    string codec = "";

    setValueField(pvStructure->getSubField<PVUnion>("value"), pArray, dataSize);
    pvStructure->getSubField<PVStructure>("codec")->getSubField<PVString>("name")->put(codec);

    pvStructure->getSubField<PVLong>("uncompressedSize")->put(dataSize);
    pvStructure->getSubField<PVLong>("compressedSize")->put(compressedSize);
}


void setTimeStamps(const PVStructurePtr & pvStructure, NDArray *pArray)
{
    PVStructurePtr dataTimeStampField = pvStructure->getSubField<PVStructure>("dataTimeStamp");
    // Currently assuming NDArray timeStamp is seconds past EPICS epoch. Actual
    // meaning is  driver-dependent. AD documentation suggests seconds since
    // UNIX epoch. In practice EPICS epoch-based seems to be most common.
    setTimeStampField(dataTimeStampField, pArray->timeStamp + EPICS_EPOCH);

    PVStructurePtr timeStampField = pvStructure->getSubField<PVStructure>("timeStamp");

    if (timeStampField.get())
    {
#if AREA_DETECTOR_MAJOR_VERSION >= 2
        setTimeStampField(timeStampField, pArray->epicsTS);
#else
        setTimeStampField(timeStampField, epicsTime::getCurrent());
#endif
    }
}



void putNDArrayToNTNDArray(const PVStructurePtr & pvStructure, NDArray *pArray)
{
    setValueAndCodec(pvStructure, pArray);

    setDimensionField(pvStructure->getSubField<PVStructureArray>("dimension"), pArray);

    pvStructure->getSubField<PVInt>("uniqueId")->put(pArray->uniqueId);

    setTimeStamps(pvStructure, pArray);

    setAttributeField(pvStructure->getSubField<PVStructureArray>("attribute"),
        pArray->pAttributeList);
}

PVStructurePtr createNTNDArray()
{
    return getPVDataCreate()->createPVStructure(getNDArrayStruc());
}

}}

