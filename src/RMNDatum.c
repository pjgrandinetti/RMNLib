
#include "RMNLibrary.h"

static OCTypeID kRMNDatumID = _kOCNotATypeID;

// SIScalar Opaque Type
struct __RMNDatum {
    OCBase _base;

    // __SIQuantity Type attributes
    SIUnitRef       unit;
    SINumberType    type;

    // __SIScalar Type attributes
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
                          (SIScalarRef) OCArrayGetValueAtIndex(input2->coordinates, idim))!=kOCCompareEqualTo) return false;
    }
	return true;
}

static void __RMNDatumFinalize(const void * theType)
{
    if(NULL == theType) return;
    RMNDatumRef theDatum = (RMNDatumRef) theType;
    if (theDatum->unit) {
        OCRelease(theDatum->unit);
        // Cast away const to allow nulling the field
        ((struct __RMNDatum *)theDatum)->unit = NULL;
    }
    if (theDatum->coordinates) {
        OCRelease(theDatum->coordinates);
        // Cast away const to allow nulling the field
        ((struct __RMNDatum *)theDatum)->coordinates = NULL;
    }
}

static OCStringRef __RMNDatumCopyFormattingDescription(OCTypeRef theType)
{
    return SIScalarCopyFormattingDescription((SIScalarRef) theType);
}

OCTypeID RMNDatumGetTypeID(void)
{
    if(kRMNDatumID == _kOCNotATypeID) kRMNDatumID = OCRegisterType("RMNDatum");
    return kRMNDatumID;
}

static struct __RMNDatum *RMNDatumAllocate(void)
{
    struct __RMNDatum *obj = OCTypeAlloc(struct __RMNDatum,
                                         RMNDatumGetTypeID(),
                                         __RMNDatumFinalize,
                                         __RMNDatumEqual,
                                         __RMNDatumCopyFormattingDescription);
    obj->unit = NULL;
    obj->type = kSINumberFloat32Type;
    memset(&obj->value, 0, sizeof(obj->value));
    
    return obj;
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
    newDatum->type = SIQuantityGetElementType((SIQuantityRef) theScalar);
    newDatum->unit = SIQuantityGetUnit((SIQuantityRef)  theScalar);
    newDatum->value = SIScalarGetValue(theScalar);

    // Optional Attributes
    if(coordinates) newDatum->coordinates = OCArrayCreateCopy(coordinates);
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
    if((input1->dependentVariableIndex==input2->dependentVariableIndex) && !SIQuantityHasSameReducedDimensionality((SIQuantityRef) input1,(SIQuantityRef) input2)) return false;
    
    for(int idim = 0; idim<coordinateCount1; idim++) {
        if(!SIQuantityHasSameReducedDimensionality((SIQuantityRef) OCArrayGetValueAtIndex(input1->coordinates, idim), 
                          (SIQuantityRef) OCArrayGetValueAtIndex(input2->coordinates, idim))) return false;
    }
	return true;
}

int RMNDatumGetComponentIndex(RMNDatumRef theDatum)
{
    if(NULL==theDatum) return kOCNotFound;
    return theDatum->componentIndex;
}

void RMNDatumSetComponentIndex(RMNDatumRef theDatum, int componentIndex)
{
    if(theDatum) theDatum->componentIndex = componentIndex;
}

int RMNDatumGetDependentVariableIndex(RMNDatumRef theDatum)
{
    if(NULL==theDatum) return kOCNotFound;
    return theDatum->dependentVariableIndex;
}

void RMNDatumSetDependentVariableIndex(RMNDatumRef theDatum, int dependentVariableIndex)
{
    if(theDatum) theDatum->dependentVariableIndex = dependentVariableIndex;
}

int RMNDatumGetMemOffset(RMNDatumRef theDatum)
{
    if(NULL==theDatum) return kOCNotFound;
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



OCDictionaryRef RMNDatumCreateDictionary(RMNDatumRef theDatum)
{
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    
    OCMutableDictionaryRef dictionary = OCDictionaryCreateMutable(0);
    
    OCNumberRef number = OCNumberCreateWithInt(theDatum->dependentVariableIndex);
    OCDictionarySetValue(dictionary, STR("dependent_variable_index"), number);
    OCRelease(number);
    
    number = OCNumberCreateWithInt(theDatum->componentIndex);
    OCDictionarySetValue(dictionary, STR("component_index"), number);
    OCRelease(number);
    
    number = OCNumberCreateWithInt(theDatum->memOffset);
    OCDictionarySetValue(dictionary, STR("mem_offset"), number);
    OCRelease(number);
    
    OCStringRef stringValue = SIScalarCreateStringValue((SIScalarRef) theDatum);
    if(stringValue) {
        OCDictionarySetValue( dictionary, STR("response"), stringValue);
        OCRelease(stringValue);
    }
    
    if(theDatum->coordinates) {
        int coordinatesCount = OCArrayGetCount(theDatum->coordinates);
        OCMutableArrayRef coordinates = OCArrayCreateMutable(coordinatesCount, &kOCTypeArrayCallBacks);
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



RMNDatumRef RMNDatumCreateWithDictionary(OCDictionaryRef dictionary, OCStringRef *error)
{
    if(error) if(*error) return NULL;
    IF_NO_OBJECT_EXISTS_RETURN(dictionary, NULL);
    
    int dependentVariableIndex = 0;
    if(OCDictionaryContainsKey(dictionary, STR("dependent_variable_index")))
        OCNumberGetValue(OCDictionaryGetValue(dictionary, STR("dependent_variable_index")),kOCNumberIntType,&dependentVariableIndex);
    else return NULL;
    
    int componentIndex = 0;
    if(OCDictionaryContainsKey(dictionary, STR("component_index")))
        OCNumberGetValue(OCDictionaryGetValue(dictionary, STR("component_index")),kOCNumberIntType,&componentIndex);
    else return NULL;
    
    int memOffset = 0;
    if(OCDictionaryContainsKey(dictionary, STR("mem_offset")))
        OCNumberGetValue(OCDictionaryGetValue(dictionary, STR("mem_offset")),kOCNumberIntType,&memOffset);
    else return NULL;
    
    
    OCMutableArrayRef coordinates = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
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

