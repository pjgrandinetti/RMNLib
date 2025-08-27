#include "../RMNLibrary.h"
#include "cJSON.h"
#define kDatumResponseKey "response"
#define kDatumDependentVariableIndexKey "dependent_variable_index"
#define kDatumComponentIndexKey "component_index"
#define kDatumMemOffsetKey "mem_offset"
static OCTypeID kDatumID = kOCNotATypeID;
// Inherits base structure from SIScalar and SIQuantity
// Can be cast to SIScalarRef or SIQuantityRef
struct impl_Datum {
    OCBase base;
    // impl_SIQuantity Type attributes
    SIUnitRef unit;
    SINumberType type;
    // impl_SIScalar Type attributes
    impl_SINumber responseValue;

    // Indexes tells us where this datum came from in the parent DependentVariable
    OCIndex dependentVariableIndex;
    OCIndex componentIndex;
    OCIndex memOffset;

    OCTypeRef owner;          // weak reference to owning Dataset object, if any
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
    if (!SIScalarEqual((SIScalarRef) input1, (SIScalarRef)  input2)) return false;
    // Compare indices
    if (input1->dependentVariableIndex != input2->dependentVariableIndex) return false;
    if (input1->componentIndex != input2->componentIndex) return false;
    if (input1->memOffset != input2->memOffset) return false;
    return true;
}
static void impl_DatumFinalize(const void *theType) {
    if (NULL == theType) return;
    DatumRef theDatum = (DatumRef)theType;
    if (theDatum->unit) {
        OCRelease(theDatum->unit);
        // Cast away const to allow nulling the field
        ((struct impl_Datum *)theDatum)->unit = NULL;
    }
    // Note: owner is a weak reference, so we don't release it
}
static OCStringRef impl_DatumCopyFormattingDescription(OCTypeRef theType) {
    DatumRef datum = (DatumRef)theType;
    return SIScalarCopyFormattingDescription((SIScalarRef) datum);
}
// cJSON serialization for Datum now entirely via OCDictionary → JSON
static cJSON *impl_DatumCreateJSON(const void *obj) {
    DatumRef datum = (DatumRef)obj;
    if (!datum)
        return cJSON_CreateNull();
    // 1) Get a plain OC‐dictionary of all fields
    OCDictionaryRef dict = DatumCopyAsDictionary(datum);
    if (!dict)
        return cJSON_CreateNull();
    // 2) Convert that dictionary to cJSON in one shot
    cJSON *json = OCDictionaryCreateJSON(dict);
    // 3) Clean up
    OCRelease(dict);
    // 4) In case your OCDictionaryCreateJSON can return NULL on failure:
    return json ? json : cJSON_CreateNull();
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
                     OCIndex dependentVariableIndex,
                     OCIndex componentIndex,
                     OCIndex memOffset,
                     OCTypeRef owner,
                     OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (NULL == response) {
        if (outError) *outError = STR("DatumCreate: response cannot be NULL");
        return NULL;
    }
    struct impl_Datum *newDatum = DatumAllocate();
    if (!newDatum) {
        if (outError) *outError = STR("DatumCreate: failed to allocate Datum");
        return NULL;
    }
    newDatum->unit = SIQuantityGetUnit((SIQuantityRef)response);
    newDatum->type = SIQuantityGetNumericType((SIQuantityRef)response);
    newDatum->responseValue = SIScalarGetValue(response);
    newDatum->dependentVariableIndex = dependentVariableIndex;
    newDatum->componentIndex = componentIndex;
    newDatum->memOffset = memOffset;
    newDatum->owner = owner;  // Store the weak reference to the owner
    return (DatumRef)newDatum;
    
    // Note: Error handling removed as it's currently unused
    // This could be added back if validation logic is needed in the future
}
DatumRef DatumCopy(DatumRef theDatum) {
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    return impl_DatumDeepCopy(theDatum);
}
bool DatumHasSameReducedDimensionalities(DatumRef input1, DatumRef input2) {
    IF_NO_OBJECT_EXISTS_RETURN(input1, false);
    IF_NO_OBJECT_EXISTS_RETURN(input2, false);
    // Note: coordinate counting logic removed as it was unused
    // This function currently only checks dimensional compatibility
    if (!SIDimensionalityHasSameReducedDimensionality(SIUnitGetDimensionality(input1->unit), SIUnitGetDimensionality(input2->unit))) return false;
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
OCTypeRef DatumGetCoordinateAtIndex(DatumRef theDatum, OCIndex index) {
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    IF_NO_OBJECT_EXISTS_RETURN(theDatum->owner, NULL);
    OCArrayRef dimensions = DatasetGetDimensions((DatasetRef)theDatum->owner);
    IF_NO_OBJECT_EXISTS_RETURN(dimensions, NULL);
    if (index < 0 || index >= OCArrayGetCount(dimensions)) return NULL;
    DimensionRef dimension = (DimensionRef)OCArrayGetValueAtIndex(dimensions, index);
    if (!dimension) return NULL;
    return DimensionCopyCoordinateAtIndex(dimension, theDatum->memOffset);
}

SIScalarRef DatumCreateResponse(DatumRef theDatum) {
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    return SIScalarCreateCopy((SIScalarRef)theDatum);
}
OCIndex DatumCoordinatesCount(DatumRef theDatum) {
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, 0);
    if(theDatum->owner) {
        OCArrayRef dimensions = DatasetGetDimensions((DatasetRef)theDatum->owner);
        if(dimensions) return OCArrayGetCount(dimensions);
    }
    return 0;
}
OCDictionaryRef DatumCopyAsDictionary(DatumRef theDatum) {
    IF_NO_OBJECT_EXISTS_RETURN(theDatum, NULL);
    OCMutableDictionaryRef dictionary = OCDictionaryCreateMutable(0);
    OCNumberRef number = OCNumberCreateWithOCIndex(theDatum->dependentVariableIndex);
    OCDictionarySetValue(dictionary, STR(kDatumDependentVariableIndexKey), number);
    OCRelease(number);
    number = OCNumberCreateWithOCIndex(theDatum->componentIndex);
    OCDictionarySetValue(dictionary, STR(kDatumComponentIndexKey), number);
    OCRelease(number);
    number = OCNumberCreateWithOCIndex(theDatum->memOffset);
    OCDictionarySetValue(dictionary, STR(kDatumMemOffsetKey), number);
    OCRelease(number);
    OCStringRef stringValue = SIScalarCreateStringValue((SIScalarRef)theDatum);
    OCDictionarySetValue(dictionary, STR(kDatumResponseKey), stringValue);
    OCRelease(stringValue);

    return dictionary;
}
DatumRef DatumCreateFromDictionary(OCDictionaryRef dictionary, OCStringRef *error) {
    if (error && *error) {
        return NULL;
    }
    IF_NO_OBJECT_EXISTS_RETURN(dictionary, NULL);
    OCIndex dependentVariableIndex = 0;
    if (OCDictionaryContainsKey(dictionary, STR(kDatumDependentVariableIndexKey)))
        OCNumberTryGetOCIndex(OCDictionaryGetValue(dictionary, STR(kDatumDependentVariableIndexKey)), &dependentVariableIndex);
    else
        return NULL;
    OCIndex componentIndex = 0;
    if (OCDictionaryContainsKey(dictionary, STR(kDatumComponentIndexKey)))
        OCNumberTryGetOCIndex(OCDictionaryGetValue(dictionary, STR(kDatumComponentIndexKey)), &componentIndex);
    else
        return NULL;
    OCIndex memOffset = 0;
    if (OCDictionaryContainsKey(dictionary, STR(kDatumMemOffsetKey)))
        OCNumberTryGetOCIndex(OCDictionaryGetValue(dictionary, STR(kDatumMemOffsetKey)), &memOffset);
    else
        return NULL;
    SIScalarRef response = NULL;
    if (OCDictionaryContainsKey(dictionary, STR(kDatumResponseKey))) {
        response = SIScalarCreateFromExpression(OCDictionaryGetValue(dictionary, STR(kDatumResponseKey)), error);
    }
    DatumRef datum = DatumCreate(response, dependentVariableIndex, componentIndex, memOffset, NULL, error);
    if (response) OCRelease(response);
    return datum;
}
static OCDictionaryRef DatumDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for Datum");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    cJSON *item = NULL;
    // Required: dependent_variable_index
    item = cJSON_GetObjectItemCaseSensitive(json, kDatumDependentVariableIndexKey);
    if (!cJSON_IsNumber(item)) {
        if (outError) *outError = STR("Missing or invalid \"dependent_variable_index\"");
        OCRelease(dict);
        return NULL;
    }
    OCNumberRef dvIdx = OCNumberCreateWithOCIndex(item->valueint);
    OCDictionarySetValue(dict, STR(kDatumDependentVariableIndexKey), dvIdx);
    OCRelease(dvIdx);
    // Required: component_index
    item = cJSON_GetObjectItemCaseSensitive(json, kDatumComponentIndexKey);
    if (!cJSON_IsNumber(item)) {
        if (outError) *outError = STR("Missing or invalid \"component_index\"");
        OCRelease(dict);
        return NULL;
    }
    OCNumberRef compIdx = OCNumberCreateWithOCIndex(item->valueint);
    OCDictionarySetValue(dict, STR(kDatumComponentIndexKey), compIdx);
    OCRelease(compIdx);
    // Required: mem_offset
    item = cJSON_GetObjectItemCaseSensitive(json, kDatumMemOffsetKey);
    if (!cJSON_IsNumber(item)) {
        if (outError) *outError = STR("Missing or invalid \"mem_offset\"");
        OCRelease(dict);
        return NULL;
    }
    OCNumberRef memOffset = OCNumberCreateWithOCIndex(item->valueint);
    OCDictionarySetValue(dict, STR(kDatumMemOffsetKey), memOffset);
    OCRelease(memOffset);
    // Optional: response
    item = cJSON_GetObjectItemCaseSensitive(json, kDatumResponseKey);
    if (cJSON_IsString(item)) {
        OCStringRef response = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDatumResponseKey), response);
        OCRelease(response);
    }
    return dict;
}
DatumRef DatumCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected a JSON object");
        return NULL;
    }
    // Step 1: Convert JSON → OCDictionary
    OCDictionaryRef dict = DatumDictionaryCreateFromJSON(json, outError);
    if (!dict) return NULL;
    // Step 2: Convert dictionary → Datum
    DatumRef datum = DatumCreateFromDictionary(dict, outError);
    OCRelease(dict);
    return datum;
}
