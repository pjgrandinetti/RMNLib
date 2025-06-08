
#include "RMNLibrary.h"

static OCTypeID kDatumID = _kOCNotATypeID;

// Datum Opaque Type
struct __Datum {
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


static bool __DatumEqual(const void * theType1, const void * theType2)
{
    DatumRef input1 = (DatumRef) theType1;
    DatumRef input2 = (DatumRef) theType2;
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

static void __DatumFinalize(const void * theType)
{
    if(NULL == theType) return;
    DatumRef theDatum = (DatumRef) theType;
    if (theDatum->unit) {
        OCRelease(theDatum->unit);
        // Cast away const to allow nulling the field
        ((struct __Datum *)theDatum)->unit = NULL;
    }
    if (theDatum->coordinates) {
        OCRelease(theDatum->coordinates);
        // Cast away const to allow nulling the field
        ((struct __Datum *)theDatum)->coordinates = NULL;
    }
}

static OCStringRef __DatumCopyFormattingDescription(OCTypeRef theType)
{
    return SIScalarCopyFormattingDescription((SIScalarRef) theType);
}

static void *__DatumDeepCopy(const void *theType) {
    if (!theType) return NULL;
    DatumRef orig = (DatumRef)theType;

    SIScalarRef response = DatumCreateResponse(orig);  // creates a new SIScalar (copies value/unit)
    if (!response) return NULL;

    OCArrayRef coordCopy = NULL;
    if (orig->coordinates) {
        coordCopy = OCArrayCreateCopy(orig->coordinates);
        if (!coordCopy) {
            OCRelease(response);
            return NULL;
        }
    }

    DatumRef copy = DatumCreate(response,
                                      coordCopy,
                                      orig->dependentVariableIndex,
                                      orig->componentIndex,
                                      orig->memOffset);

    OCRelease(response);
    if (coordCopy) OCRelease(coordCopy);

    return copy;
}

static void *__DatumDeepCopyMutable(const void *theType) {
    // Currently no mutable variant exists; fallback to immutable copy
    return __DatumDeepCopy(theType);
}
OCTypeID DatumGetTypeID(void)
{
    if(kDatumID == _kOCNotATypeID) kDatumID = OCRegisterType("Datum");
    return kDatumID;
}

static struct __Datum *DatumAllocate(void)
{
    struct __Datum *obj = OCTypeAlloc(struct __Datum,
                                        DatumGetTypeID(),
                                        __DatumFinalize,
                                        __DatumEqual,
                                        __DatumCopyFormattingDescription,
                                        __DatumDeepCopy,
                                        __DatumDeepCopyMutable);

    obj->unit = NULL;
    obj->type = kSINumberFloat32Type;
    memset(&obj->value, 0, sizeof(obj->value));
    
    return obj;
}


DatumRef DatumCreate(SIScalarRef theScalar,
                         OCArrayRef coordinates,
                         int dependentVariableIndex,
                         int componentIndex,
                         int memOffset)
{
    if(NULL==theScalar) return NULL;
    
    // Initialize object
    struct __Datum *newDatum = DatumAllocate();

    // *** Setup attributes ***
    newDatum->type = SIQuantityGetElementType((SIQuantityRef) theScalar);
    newDatum->unit = SIQuantityGetUnit((SIQuantityRef)  theScalar);
    newDatum->value = SIScalarGetValue(theScalar);

    // Optional Attributes
    if(coordinates) newDatum->coordinates = OCArrayCreateCopy(coordinates);
    newDatum->dependentVariableIndex = dependentVariableIndex;
    newDatum->componentIndex = componentIndex;
    newDatum->memOffset = memOffset;

    return (DatumRef) newDatum;
}



DatumRef DatumCopy(DatumRef theDatum)
{
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    
    SIScalarRef response = DatumCreateResponse(theDatum);

    DatumRef copy = DatumCreate(response,
                                    theDatum->coordinates,
                                    theDatum->dependentVariableIndex,
                                    theDatum->componentIndex,
                                    theDatum->memOffset);
    OCRelease(response);
    return copy;
}

bool DatumHasSameReducedDimensionalities(DatumRef input1, DatumRef input2)
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

int DatumGetComponentIndex(DatumRef theDatum)
{
    if(NULL==theDatum) return kOCNotFound;
    return theDatum->componentIndex;
}

void DatumSetComponentIndex(DatumRef theDatum, int componentIndex)
{
    if(theDatum) theDatum->componentIndex = componentIndex;
}

int DatumGetDependentVariableIndex(DatumRef theDatum)
{
    if(NULL==theDatum) return kOCNotFound;
    return theDatum->dependentVariableIndex;
}

void DatumSetDependentVariableIndex(DatumRef theDatum, int dependentVariableIndex)
{
    if(theDatum) theDatum->dependentVariableIndex = dependentVariableIndex;
}

int DatumGetMemOffset(DatumRef theDatum)
{
    if(NULL==theDatum) return kOCNotFound;
    return theDatum->memOffset;
}

void DatumSetMemOffset(DatumRef theDatum, int memOffset)
{
    if(theDatum) theDatum->memOffset = memOffset;
}


SIScalarRef DatumGetCoordinateAtIndex(DatumRef theDatum, int index)
{
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    if(index==-1) return (SIScalarRef) theDatum;
    IF_NO_OBJECT_EXISTS_RETURN(theDatum->coordinates, NULL);
    return OCArrayGetValueAtIndex(theDatum->coordinates, index);
}

SIScalarRef DatumCreateResponse(DatumRef theDatum)
{
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    return SIScalarCreateCopy((SIScalarRef) theDatum);
}

int DatumCoordinatesCount(DatumRef theDatum)
{
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, 0);
    if(theDatum->coordinates) return OCArrayGetCount(theDatum->coordinates);
    return 0;
}



OCDictionaryRef DatumCreateDictionary(DatumRef theDatum)
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



DatumRef DatumCreateWithDictionary(OCDictionaryRef dictionary, OCStringRef *error)
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
    
	DatumRef datum = DatumCreate(response, coordinates, dependentVariableIndex, componentIndex, memOffset);
    if(response) OCRelease(response);
    if(coordinates) OCRelease(coordinates);
    return datum;
}

