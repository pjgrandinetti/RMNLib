#include "RMNLibrary.h"
#include "cJSON.h"

static OCTypeID kDatumID = kOCNotATypeID;

struct impl_Datum {
    OCBase base;
    SIScalarRef response;
    OCIndex dependentVariableIndex;
    OCIndex componentIndex;
    OCIndex memOffset;
    OCArrayRef coordinates;
};

OCTypeID DatumGetTypeID(void) {
    if (kDatumID == kOCNotATypeID) kDatumID = OCRegisterType("Datum");
    return kDatumID;
}

static bool impl_DatumEqual(const void *theType1, const void *theType2) {
    DatumRef input1 = (DatumRef)theType1;
    DatumRef input2 = (DatumRef)theType2;
    if (input1->base.typeID != input2->base.typeID) return false;
    if (NULL == input1 || NULL == input2) return false;
    if (input1 == input2) return true;
    // Compare response
    if (!SIScalarEqual(input1->response, input2->response)) return false;
    // Compare coordinate arrays
    if (OCArrayGetCount(input1->coordinates) != OCArrayGetCount(input2->coordinates)) return false;
    OCIndex coordinateCount = OCArrayGetCount(input1->coordinates);
    for (OCIndex idim = 0; idim < coordinateCount; idim++) {
        if (SIScalarCompare(
                OCArrayGetValueAtIndex(input1->coordinates, idim),
                OCArrayGetValueAtIndex(input2->coordinates, idim)) != kOCCompareEqualTo) return false;
    }
    // Compare indices
    if (input1->dependentVariableIndex != input2->dependentVariableIndex) return false;
    if (input1->componentIndex != input2->componentIndex) return false;
    if (input1->memOffset != input2->memOffset) return false;
    return true;
}

static void impl_DatumFinalize(const void *theType) {
    if (NULL == theType) return;
    DatumRef theDatum = (DatumRef)theType;
    if (theDatum->response) {
        OCRelease(theDatum->response);
        ((struct impl_Datum *)theDatum)->response = NULL;
    }
    if (theDatum->coordinates) {
        OCRelease(theDatum->coordinates);
        ((struct impl_Datum *)theDatum)->coordinates = NULL;
    }
}

static OCStringRef impl_DatumCopyFormattingDescription(OCTypeRef theType) {
    DatumRef datum = (DatumRef)theType;
    return SIScalarCopyFormattingDescription(datum->response);
}

// cJSON serialization
cJSON *impl_DatumCreateJSON(const void *obj) {
    DatumRef datum = (DatumRef)obj;
    if (!datum) return cJSON_CreateNull();

    cJSON *json = cJSON_CreateObject();
    if (!json) return NULL;

    // response (SIScalarRef)
    if (datum->response) {
        cJSON *resp_json = OCTypeCopyJSON((OCTypeRef)datum->response);
        if (resp_json)
            cJSON_AddItemToObject(json, "response", resp_json);
    }

    // dependentVariableIndex (OCIndex)
    cJSON_AddNumberToObject(json, "dependentVariableIndex", (int)datum->dependentVariableIndex);

    // componentIndex (OCIndex)
    cJSON_AddNumberToObject(json, "componentIndex", (int)datum->componentIndex);

    // memOffset (OCIndex)
    cJSON_AddNumberToObject(json, "memOffset", (int)datum->memOffset);

    // coordinates (OCArrayRef)
    if (datum->coordinates) {
        cJSON *coords_json = OCTypeCopyJSON((OCTypeRef)datum->coordinates);
        if (coords_json)
            cJSON_AddItemToObject(json, "coordinates", coords_json);
    }

    return json;
}
static void *impl_DatumDeepCopy(const void *theType) {
    if (!theType) return NULL;
    DatumRef orig = (DatumRef)theType;

    // Use DatumCopyAsDictionary to get a dictionary copy of all fields
    OCDictionaryRef dict = DatumCopyAsDictionary(orig);
    if (!dict) return NULL;

    // Use DatumCreateFromDictionary to reconstruct the DatumRef
    OCStringRef error = NULL;
    DatumRef copy = DatumCreateFromDictionary(dict, &error);

    OCRelease(dict);
    if (error) OCRelease(error);
    return copy;
}

static void *impl_DatumDeepCopyMutable(const void *theType) {
    // No mutable variant; fallback to immutable copy
    return impl_DatumDeepCopy(theType);
}

static struct impl_Datum *DatumAllocate(void) {
    return OCTypeAlloc(struct impl_Datum,
                                      DatumGetTypeID(),
                                      impl_DatumFinalize,
                                      impl_DatumEqual,
                                      impl_DatumCopyFormattingDescription,
                                      impl_DatumCreateJSON,
                                      impl_DatumDeepCopy,
                                      impl_DatumDeepCopyMutable);
}

DatumRef DatumCreate(SIScalarRef response,
                     OCArrayRef coordinates,
                     OCIndex dependentVariableIndex,
                     OCIndex componentIndex,
                     OCIndex memOffset) {
    if (NULL == response) return NULL;
    struct impl_Datum *newDatum = DatumAllocate();
    newDatum->response = SIScalarCreateCopy(response);
    if (coordinates) newDatum->coordinates = OCArrayCreateCopy(coordinates);
    newDatum->dependentVariableIndex = dependentVariableIndex;
    newDatum->componentIndex = componentIndex;
    newDatum->memOffset = memOffset;
    return (DatumRef)newDatum;
}

DatumRef DatumCopy(DatumRef theDatum) {
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    return impl_DatumDeepCopy(theDatum);
}

bool DatumHasSameReducedDimensionalities(DatumRef input1, DatumRef input2) {
    IF_NO_OBJECT_EXISTS_RETURN(input1, false);
    IF_NO_OBJECT_EXISTS_RETURN(input2, false);
    OCIndex coordinateCount1 = 0, coordinateCount2 = 0;
    if(!SIQuantityHasSameReducedDimensionality((SIQuantityRef) input1->response,(SIQuantityRef) input2->response)) return false;
    if (input1->coordinates) coordinateCount1 = OCArrayGetCount(input1->coordinates);
    if (input2->coordinates) coordinateCount2 = OCArrayGetCount(input2->coordinates);
    if (coordinateCount1 != coordinateCount2) return false;
    for (OCIndex idim = 0; idim < coordinateCount1; idim++) {
        if (!SIQuantityHasSameReducedDimensionality(
                OCArrayGetValueAtIndex(input1->coordinates, idim),
                OCArrayGetValueAtIndex(input2->coordinates, idim)))
            return false;
    }
    return true;
}

OCIndex DatumGetComponentIndex(DatumRef theDatum) {
    if (NULL == theDatum) return kOCNotFound;
    return theDatum->componentIndex;
}
void DatumSetComponentIndex(DatumRef theDatum, OCIndex componentIndex) {
    if (theDatum) theDatum->componentIndex = componentIndex;
}
OCIndex DatumGetDependentVariableIndex(DatumRef theDatum) {
    if (NULL == theDatum) return kOCNotFound;
    return theDatum->dependentVariableIndex;
}
void DatumSetDependentVariableIndex(DatumRef theDatum, OCIndex dependentVariableIndex) {
    if (theDatum) theDatum->dependentVariableIndex = dependentVariableIndex;
}
OCIndex DatumGetMemOffset(DatumRef theDatum) {
    if (NULL == theDatum) return kOCNotFound;
    return theDatum->memOffset;
}
void DatumSetMemOffset(DatumRef theDatum, OCIndex memOffset) {
    if (theDatum) theDatum->memOffset = memOffset;
}
SIScalarRef DatumGetCoordinateAtIndex(DatumRef theDatum, OCIndex index) {
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    IF_NO_OBJECT_EXISTS_RETURN(theDatum->coordinates, NULL);
    return OCArrayGetValueAtIndex(theDatum->coordinates, index);
}
SIScalarRef DatumCreateResponse(DatumRef theDatum) {
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    if (!theDatum->response) return NULL;
    return SIScalarCreateCopy(theDatum->response);
}
OCIndex DatumCoordinatesCount(DatumRef theDatum) {
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, 0);
    if (theDatum->coordinates) return OCArrayGetCount(theDatum->coordinates);
    return 0;
}

OCDictionaryRef DatumCopyAsDictionary(DatumRef theDatum) {
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    OCMutableDictionaryRef dictionary = OCDictionaryCreateMutable(0);
    OCNumberRef number = OCNumberCreateWithOCIndex(theDatum->dependentVariableIndex);
    OCDictionarySetValue(dictionary, STR("dependent_variable_index"), number);
    OCRelease(number);
    number = OCNumberCreateWithOCIndex(theDatum->componentIndex);
    OCDictionarySetValue(dictionary, STR("component_index"), number);
    OCRelease(number);
    number = OCNumberCreateWithOCIndex(theDatum->memOffset);
    OCDictionarySetValue(dictionary, STR("mem_offset"), number);
    OCRelease(number);

    if (theDatum->response) {
        OCStringRef stringValue = SIScalarCreateStringValue(theDatum->response);
        OCDictionarySetValue(dictionary, STR("response"), stringValue);
        OCRelease(stringValue);
    }

    if (theDatum->coordinates) {
        OCIndex coordinatesCount = OCArrayGetCount(theDatum->coordinates);
        OCMutableArrayRef coordinates = OCArrayCreateMutable(coordinatesCount, &kOCTypeArrayCallBacks);
        for (OCIndex index = 0; index < coordinatesCount; index++) {
            OCStringRef stringValue = SIScalarCreateStringValue(OCArrayGetValueAtIndex(theDatum->coordinates, index));
            OCArrayAppendValue(coordinates, stringValue);
            OCRelease(stringValue);
        }
        OCDictionarySetValue(dictionary, STR("coordinates"), coordinates);
        OCRelease(coordinates);
    }
    return dictionary;
}

DatumRef DatumCreateFromDictionary(OCDictionaryRef dictionary, OCStringRef *error) {
    if (error)
        if (*error) return NULL;
    IF_NO_OBJECT_EXISTS_RETURN(dictionary, NULL);

    OCIndex dependentVariableIndex = 0;
    if (OCDictionaryContainsKey(dictionary, STR("dependent_variable_index")))
        OCNumberGetValue(OCDictionaryGetValue(dictionary, STR("dependent_variable_index")), kOCNumberIntType, &dependentVariableIndex);
    else
        return NULL;

    OCIndex componentIndex = 0;
    if (OCDictionaryContainsKey(dictionary, STR("component_index")))
        OCNumberGetValue(OCDictionaryGetValue(dictionary, STR("component_index")), kOCNumberIntType, &componentIndex);
    else
        return NULL;

    OCIndex memOffset = 0;
    if (OCDictionaryContainsKey(dictionary, STR("mem_offset")))
        OCNumberGetValue(OCDictionaryGetValue(dictionary, STR("mem_offset")), kOCNumberIntType, &memOffset);
    else
        return NULL;

    OCMutableArrayRef coordinates = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    if (OCDictionaryContainsKey(dictionary, STR("coordinates"))) {
        OCMutableArrayRef stringValues = (OCMutableArrayRef)OCDictionaryGetValue(dictionary, STR("coordinates"));
        OCIndex coordinatesCount = OCArrayGetCount(stringValues);
        for (OCIndex index = 0; index < coordinatesCount; index++) {
            OCStringRef stringValue = OCArrayGetValueAtIndex(stringValues, index);
            SIScalarRef coordinate = SIScalarCreateFromExpression(stringValue, error);
            OCArrayAppendValue(coordinates, coordinate);
            OCRelease(coordinate);
        }
    }

    SIScalarRef response = NULL;
    if (OCDictionaryContainsKey(dictionary, STR("response"))) {
        response = SIScalarCreateFromExpression(OCDictionaryGetValue(dictionary, STR("response")), error);
    }

    DatumRef datum = DatumCreate(response, coordinates, dependentVariableIndex, componentIndex, memOffset);
    if (response) OCRelease(response);
    if (coordinates) OCRelease(coordinates);
    return datum;
}
