
#include "RMNLibrary.h"

static OCTypeID kRMNDatumID = _kOCNotATypeID;

// SIScalar Opaque Type
struct __RMNDatum {
    OCBase _base;

    // __SIQuantity Type attributes
    SIUnitRef       unit;
    SINumberType    type;
    __SINumber      value;

    int             dependentVariableIndex;
    int             componentIndex;
    int             memOffset;
    OCArrayRef      coordinates;

};


static bool __RMNDatumEqual(const void * theType1, const void * theType2)
{
    RMNDatumRef input1 = (RMNDatumRef) theType1;
    RMNDatumRef input2 = (RMNDatumRef) theType2;
    if(input1->_base.typeID != input2->_base.typeID) return false;

    if(NULL == input1 || NULL == input2) return false;
    if(input1 == input2) return true;

    
	if(!SIScalarEqual((SIScalarRef) input1, (SIScalarRef) input2)) return false;
    if(OCArrayGetCount(input1->coordinates) != OCArrayGetCount(input2->coordinates)) return false;
    int coordinateCount = OCArrayGetCount(input1->coordinates);
    for(int idim = 0; idim<coordinateCount; idim++) {
        if(SIScalarCompare((SIScalarRef) OCArrayGetValueAtIndex(input1->coordinates, idim),
                          (SIScalarRef) OCArrayGetValueAtIndex(input2->coordinates, idim))!=kSICompareEqualTo) return false;
    }
	return true;
}

static void __RMNDatumFinalize(const void * theType)
{
    if(NULL == theType) return;
    RMNDatumRef theDatum = (RMNDatumRef) theType;
    if(theDatum->unit) OCRelease(theDatum->unit);
    free((void *)theDatum);
}

static OCStringRef __RMNDatumCopyFormattingDescription(OCTypeRef theType)
{
    return __SIScalarCopyFormattingDescription(theType);
}

OCTypeID RMNDatumGetTypeID(void)
{
    if(kRMNDatumID == _kOCNotATypeID) kRMNDatumID = OCRegisterType("RMNDatum");
    return kRMNDatumID;
}

static struct __RMNDatum *RMNDatumAllocate()
{
    struct __RMNDatum *theDatum = malloc(sizeof(struct __RMNDatum));
    if(NULL == theDatum) return NULL;
    theDatum->_base.typeID = SIScalarGetTypeID();
    theDatum->_base.static_instance = false; 
    theDatum->_base.finalize = __RMNDatumFinalize;
    theDatum->_base.equal = __RMNDatumEqual;
    theDatum->_base.copyFormattingDesc = __RMNDatumCopyFormattingDescription;
    theDatum->_base.retainCount = 0;
    OCRetain(theDatum);

    theDatum->unit = NULL;
    theDatum->type = kSINumberFloat32Type;
    return theDatum;
}


RMNDatumRef RMNDatumCreate(SIScalarRef theScalar,
                         OCArrayRef coordinates,
                         int dependentVariableIndex,
                         int componentIndex,
                         int memOffset)
{
    if(NULL==theScalar) return NULL;
    
    // Initialize object
    struct __RMNDatum *newDatum = RMNDatumAllocate();

    // *** Setup attributes ***
    newDatum->elementType = SIQuantityGetElementType(theScalar);
    newDatum->unit = SIQuantityGetUnit(theScalar);
    newDatum->value = SIScalarGetValue(theScalar);

    // Optional Attributes
    if(coordinates) newDatum->coordinates = OCArrayCreateCopy(kCFAllocatorDefault, coordinates);
    newDatum->dependentVariableIndex = dependentVariableIndex;
    newDatum->componentIndex = componentIndex;
    newDatum->memOffset = memOffset;

    return (RMNDatumRef) newDatum;
}



RMNDatumRef RMNDatumCopy(RMNDatumRef theDatum)
{
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    
    SIScalarRef response = RMNDatumCreateResponse(theDatum);

    RMNDatumRef copy = RMNDatumCreate(response,
                                    theDatum->coordinates,
                                    theDatum->dependentVariableIndex,
                                    theDatum->componentIndex,
                                    theDatum->memOffset);
    OCRelease(response);
    return copy;
}

bool RMNDatumHasSameReducedDimensionalities(RMNDatumRef input1, RMNDatumRef input2)
{
    IF_NO_OBJECT_EXISTS_RETURN(input1, false);
    IF_NO_OBJECT_EXISTS_RETURN(input2, false);

    int coordinateCount1 = 0;
    int coordinateCount2 = 0;
    if(input1->coordinates) coordinateCount1 = OCArrayGetCount(input1->coordinates);
    if(input2->coordinates) coordinateCount2 = OCArrayGetCount(input2->coordinates);
    if(coordinateCount1 != coordinateCount2) return false;
    if((input1->dependentVariableIndex==input2->dependentVariableIndex) && !PSQuantityHasSameReducedDimensionality((PSQuantityRef) input1,(PSQuantityRef) input2)) return false;
    
    for(int idim = 0; idim<coordinateCount1; idim++) {
        if(!PSQuantityHasSameReducedDimensionality((PSQuantityRef) OCArrayGetValueAtIndex(input1->coordinates, idim), 
                          (PSQuantityRef) OCArrayGetValueAtIndex(input2->coordinates, idim))) return false;
    }
	return true;
}

int RMNDatumGetComponentIndex(RMNDatumRef theDatum)
{
    if(NULL==theDatum) return kCFNotFound;
    return theDatum->componentIndex;
}

void RMNDatumSetComponentIndex(RMNDatumRef theDatum, int componentIndex)
{
    if(theDatum) theDatum->componentIndex = componentIndex;
}

int RMNDatumGetDependentVariableIndex(RMNDatumRef theDatum)
{
    if(NULL==theDatum) return kCFNotFound;
    return theDatum->dependentVariableIndex;
}

void RMNDatumSetDependentVariableIndex(RMNDatumRef theDatum, int dependentVariableIndex)
{
    if(theDatum) theDatum->dependentVariableIndex = dependentVariableIndex;
}

int RMNDatumGetMemOffset(RMNDatumRef theDatum)
{
    if(NULL==theDatum) return kCFNotFound;
    return theDatum->memOffset;
}

void RMNDatumSetMemOffset(RMNDatumRef theDatum, int memOffset)
{
    if(theDatum) theDatum->memOffset = memOffset;
}


SIScalarRef RMNDatumGetCoordinateAtIndex(RMNDatumRef theDatum, int index)
{
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    if(index==-1) return (SIScalarRef) theDatum;
    IF_NO_OBJECT_EXISTS_RETURN(theDatum->coordinates, NULL);
    return OCArrayGetValueAtIndex(theDatum->coordinates, index);
}

SIScalarRef RMNDatumCreateResponse(RMNDatumRef theDatum)
{
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    return SIScalarCreateCopy((SIScalarRef) theDatum);
}

int RMNDatumCoordinatesCount(RMNDatumRef theDatum)
{
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, 0);
    if(theDatum->coordinates) return OCArrayGetCount(theDatum->coordinates);
    return 0;
}



OCDictionaryRef RMNDatumCreatePList(RMNDatumRef theDatum)
{
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    
    OCMutableDictionaryRef dictionary = OCDictionaryCreateMutable(0);
    
    OCNumberRef number = PSOCNumberCreateWithint(theDatum->dependentVariableIndex);
    OCDictionarySetValue(dictionary, STR("dependent_variable_index"), number);
    OCRelease(number);
    
    number = PSOCNumberCreateWithint(theDatum->componentIndex);
    OCDictionarySetValue(dictionary, STR("component_index"), number);
    OCRelease(number);
    
    number = PSOCNumberCreateWithint(theDatum->memOffset);
    OCDictionarySetValue(dictionary, STR("mem_offset"), number);
    OCRelease(number);
    
    OCStringRef stringValue = SIScalarCreateStringValue((SIScalarRef) theDatum);
    if(stringValue) {
        OCDictionarySetValue( dictionary, STR("response"), stringValue);
        OCRelease(stringValue);
    }
    
    if(theDatum->coordinates) {
        int coordinatesCount = OCArrayGetCount(theDatum->coordinates);
        OCMutableArrayRef coordinates = OCArrayCreateMutable(coordinatesCount, &kCFTypeArrayCallBacks);
        for(int index =0; index<coordinatesCount; index++) {
            OCStringRef stringValue = SIScalarCreateStringValue(OCArrayGetValueAtIndex(theDatum->coordinates, index));
            OCArrayAppendValue(coordinates, stringValue);
            OCRelease(stringValue);
        }
        OCDictionarySetValue(dictionary, STR("coordinates"),coordinates);
        OCRelease(coordinates);
    }
	return dictionary;
}



RMNDatumRef RMNDatumCreateWithPList(OCDictionaryRef dictionary, CFErrorRef *error)
{
    if(error) if(*error) return NULL;
    IF_NO_OBJECT_EXISTS_RETURN(dictionary, NULL);
    
    int dependentVariableIndex = 0;
    if(OCDictionaryContainsKey(dictionary, STR("dependent_variable_index")))
        OCNumberGetValue(OCDictionaryGetValue(dictionary, STR("dependent_variable_index")),kOCNumberintType,&dependentVariableIndex);
    else return NULL;
    
    int componentIndex = 0;
    if(OCDictionaryContainsKey(dictionary, STR("component_index")))
        OCNumberGetValue(OCDictionaryGetValue(dictionary, STR("component_index")),kOCNumberintType,&componentIndex);
    else return NULL;
    
    int memOffset = 0;
    if(OCDictionaryContainsKey(dictionary, STR("mem_offset")))
        OCNumberGetValue(OCDictionaryGetValue(dictionary, STR("mem_offset")),kOCNumberintType,&memOffset);
    else return NULL;
    
    
    OCMutableArrayRef coordinates = OCArrayCreateMutable(0, &kCFTypeArrayCallBacks);
    if(OCDictionaryContainsKey (dictionary,STR("coordinates"))) {
        OCMutableArrayRef stringValues = (OCMutableArrayRef) OCDictionaryGetValue(dictionary, STR("coordinates"));
        int coordinatesCount = OCArrayGetCount(stringValues);
        for(int index=0;index<coordinatesCount;index++) {
            OCStringRef stringValue = OCArrayGetValueAtIndex(stringValues, index);
            SIScalarRef coordinate = SIScalarCreateWithOCString(stringValue,error);
            OCArrayAppendValue(coordinates, coordinate);
            OCRelease(coordinate);
        }
    }

    SIScalarRef response = NULL;
    if(OCDictionaryContainsKey (dictionary,STR("response"))) {
        response = SIScalarCreateWithOCString(OCDictionaryGetValue(dictionary, STR("response")),error);
    }
    
	RMNDatumRef datum = RMNDatumCreate(response, coordinates, dependentVariableIndex, componentIndex, memOffset);
    if(response) OCRelease(response);
    if(coordinates) OCRelease(coordinates);
    return datum;
}

RMNDatumRef RMNDatumCreateWithOldDataFormat(CFDataRef data, CFErrorRef *error)
{
    if(error) if(*error) return NULL;
    IF_NO_OBJECT_EXISTS_RETURN(data, NULL);
    CFPropertyListFormat format;
    OCDictionaryRef dictionary  = CFPropertyListCreateWithData(data,kCFPropertyListImmutable,&format,error);
    IF_NO_OBJECT_EXISTS_RETURN(dictionary, NULL);
    
    int componentIndex = 0;
    if(OCDictionaryContainsKey(dictionary, STR("signalIndex")))
        OCNumberGetValue(OCDictionaryGetValue(dictionary, STR("signalIndex")),kOCNumberintType,&componentIndex);
    
    int memOffset = 0;
    if(OCDictionaryContainsKey(dictionary, STR("memOffset")))
        OCNumberGetValue(OCDictionaryGetValue(dictionary, STR("memOffset")),kOCNumberintType,&memOffset);
    
    OCMutableArrayRef coordinates = NULL;
    if(OCDictionaryContainsKey (dictionary,STR("coordinates"))) {
        coordinates = OCArrayCreateMutable(0, &kCFTypeArrayCallBacks);
        OCMutableArrayRef array = (OCMutableArrayRef) OCDictionaryGetValue(dictionary, STR("coordinates"));
        int count = OCArrayGetCount(array);
        for(int index=0; index<count; index++) {
            CFDataRef coordinateData = OCArrayGetValueAtIndex(array, index);
            SIScalarRef coordinate = SIScalarCreateWithData(coordinateData, error);
            OCArrayAppendValue(coordinates, coordinate);
            OCRelease(coordinate);
        }
    }
    
    SIScalarRef response = NULL;
    if(OCDictionaryContainsKey (dictionary,STR("response")))
        response = SIScalarCreateWithData(OCDictionaryGetValue(dictionary, STR("response")), error);
    
    RMNDatumRef datum = RMNDatumCreate(response,
                                     coordinates,
                                     0,
                                     componentIndex,
                                     memOffset);

    
    if(response) OCRelease(response);
    if(coordinates) OCRelease(coordinates);
    OCRelease(dictionary);
    return datum;
}
