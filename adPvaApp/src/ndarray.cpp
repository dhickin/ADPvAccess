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


UnionConstPtr getValueType()
{
    static UnionConstPtr valueType;

    if (valueType == NULL)
    {
        FieldBuilderPtr fb = getFieldCreate()->createFieldBuilder();
        
        for (int i = pvBoolean; i < pvString; ++i)
        {
            ScalarType st = static_cast<ScalarType>(i);
            fb->addArray(std::string(ScalarTypeFunc::name(st)) + "Value", st);
        }

        valueType = fb->createUnion();                
    }
    return valueType;
}

StructureConstPtr getAttributeStruc()
{
    static StructureConstPtr attributeStruc;

    if (attributeStruc == NULL)
    {
        FieldBuilderPtr fb = getFieldCreate()->createFieldBuilder();
        
        attributeStruc = fb->setId("epics:nt/NTAttribute:1.0")->
            add("name", pvString)->
            add("value", getFieldCreate()->createVariantUnion())->
            add("descriptor", pvString)->
            add("sourceType", pvInt)->
            add("source", pvString)->
            createStructure();           
    }
    return attributeStruc;
}

StructureConstPtr getNDArrayStruc()
{
    static StructureConstPtr arrayStruc;

    if (arrayStruc == NULL)
    {
        FieldCreatePtr fieldCreate = getFieldCreate();
        StructureConstPtr timeStruc = getStandardField()->timeStamp();
        FieldBuilderPtr fb = fieldCreate->createFieldBuilder();

        StructureConstPtr codecStruc = fb->setId("codec_t")->
            add("name", pvString)->
            add("parameters", fieldCreate->createVariantUnion())->
            createStructure();

        StructureConstPtr dimensionStruc = fb->setId("dimension_t")->
            add("size", pvInt)->
            add("offset",  pvInt)->
            add("fullSize",  pvInt)->
            add("binning",  pvInt)->
            add("reverse",  pvBoolean)->
            createStructure();

        arrayStruc = fb->setId(ntImageStr)->
            add("value", getValueType())->
            add("codec", codecStruc)->
            add("compressedSize", pvLong)->
            add("uncompressedSize", pvLong)->
            addArray("dimension", dimensionStruc)->
            add("dataTimeStamp", timeStruc)->
            add("uniqueId", pvInt)->
            addArray("attribute", getAttributeStruc())->
            add("timeStamp", timeStruc)->
            createStructure();
    }

    return arrayStruc;
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
    PVStructureArray::svector dimension;
    dimension.reserve(pArray->ndims);

    for (int i = 0; i < pArray->ndims; ++i)
    {
        PVStructurePtr dimensionElement = getPVDataCreate()->createPVStructure(dimensionStruc);
        dimensionElement->getSubField<PVInt>("size")->put(pArray->dims[i].size);
        dimensionElement->getSubField<PVInt>("offset")->put(pArray->dims[i].offset);
        dimensionElement->getSubField<PVInt>("fullSize")->put(pArray->dims[i].size + pArray->dims[i].offset);
        dimensionElement->getSubField<PVInt>("binning")->put(pArray->dims[i].binning);
        dimensionElement->getSubField<PVBoolean>("reverse")->put(pArray->dims[i].reverse);
        dimension.push_back(dimensionElement);         
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

PVFieldPtr createAttributeStoredString(NDAttribute * attribute, size_t size)
{
    PVFieldPtr attStoredValue;
    char cstr[size];

    int status = attribute->getValue(NDAttrString, cstr, size);

    if (status != ND_ERROR)
    {
        std::string value(cstr);
        PVStringPtr storedValue = getPVDataCreate()->createPVScalar<PVString>();
        storedValue->put(value);
        attStoredValue = storedValue;
    }

    return attStoredValue;
}


PVFieldPtr createAttributeValue(NDAttribute * attribute)
{
    PVFieldPtr storedValue;
    NDAttrDataType_t dataType;
    size_t size = 0;
    int status = attribute->getValueInfo(&dataType, &size);

    if (status != ND_ERROR)
    {
        switch (dataType)
        {
        case NDAttrInt8:
            storedValue = createAttributeStoredValue<PVByte>(attribute, dataType);
            break;

        case NDAttrInt16:
            storedValue = createAttributeStoredValue<PVShort>(attribute, dataType);
            break;

        case NDAttrInt32:
           storedValue = createAttributeStoredValue<PVInt>(attribute, dataType);
           break;

        case NDAttrFloat32:
            storedValue = createAttributeStoredValue<PVFloat>(attribute, dataType);
            break;

        case NDAttrFloat64:
            storedValue = createAttributeStoredValue<PVDouble>(attribute, dataType);
            break;

        case NDAttrString:
            storedValue = createAttributeStoredString(attribute, size); 
            break;

        default:
            break;
        }
    }

    return storedValue;
}


PVStructurePtr createAttribute(NDAttribute * ndattribute)
{
    PVStructurePtr attribute;

    PVFieldPtr attStoredValue = createAttributeValue(ndattribute);

    if (attStoredValue.get() != NULL)
    {
        attribute = getPVDataCreate()->
            createPVStructure(getAttributeStruc());

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
        attribute->getSubField<PVUnion>("value")->set(attStoredValue);
    }

    return attribute;
}



void setAttributeField(const PVStructureArrayPtr & attributeField, NDAttributeList * list)
{
    const int numberOfAttributes = list->count();

    PVStructureArray::svector attribute;
    attribute.reserve(numberOfAttributes);

    NDAttribute * ndattribute = list->next(NULL);

    while (ndattribute != NULL)
    {
        PVStructurePtr attributeElement = createAttribute(ndattribute);

        if (attributeElement.get() != NULL)
        {
            attribute.push_back(attributeElement);
        }
        ndattribute = list->next(ndattribute);
    }
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

