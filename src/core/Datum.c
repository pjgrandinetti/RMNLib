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
    if (kDatumID == kOCNotATypeID) kDatumID = OCRegisterType("Datum", (OCTypeRef (*)(cJSON *, OCStringRef *))DatumCreateFromJSON);
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
// cJSON serialization for Datum - create cJSON object directly
static cJSON *impl_DatumCopyJSON(const void *obj, bool typed) {
    return DatumCopyAsJSON((DatumRef)obj, typed);
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
                       impl_DatumCopyJSON,
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
cJSON *DatumCopyAsJSON(DatumRef datum, bool typed) {
    if (!datum)
        return cJSON_CreateNull();
    
    cJSON *json = cJSON_CreateObject();
    if (!json)
        return cJSON_CreateNull();
    
    // Add dependent_variable_index
    cJSON *dvIndex = cJSON_CreateNumber(datum->dependentVariableIndex);
    if (dvIndex) cJSON_AddItemToObject(json, kDatumDependentVariableIndexKey, dvIndex);
    
    // Add component_index
    cJSON *compIndex = cJSON_CreateNumber(datum->componentIndex);
    if (compIndex) cJSON_AddItemToObject(json, kDatumComponentIndexKey, compIndex);
    
    // Add mem_offset
    cJSON *memOffset = cJSON_CreateNumber(datum->memOffset);
    if (memOffset) cJSON_AddItemToObject(json, kDatumMemOffsetKey, memOffset);
    
    // Add response value using SIScalar's JSON serialization
    // Create a proper SIScalar copy to avoid infinite recursion
    SIScalarRef scalarCopy = SIScalarCreateCopy((SIScalarRef)datum);
    cJSON *response = OCTypeCopyJSON((OCTypeRef)scalarCopy, typed);
    if (response) cJSON_AddItemToObject(json, kDatumResponseKey, response);
    OCRelease(scalarCopy);
    
    if (typed) {
        // Wrap in typed object format
        cJSON *wrapper = cJSON_CreateObject();
        if (wrapper) {
            cJSON_AddStringToObject(wrapper, "type", "Datum");
            cJSON_AddItemToObject(wrapper, "value", json);
            return wrapper;
        } else {
            cJSON_Delete(json);
            return cJSON_CreateNull();
        }
    }
    
    return json;
}

DatumRef DatumCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json) {
        if (outError) *outError = STR("JSON is NULL");
        return NULL;
    }
    
    cJSON *actualJson = json;
    
    // Check if this is a typed JSON object (has "type" and "value" fields)
    if (cJSON_IsObject(json)) {
        cJSON *typeItem = cJSON_GetObjectItemCaseSensitive(json, "type");
        cJSON *valueItem = cJSON_GetObjectItemCaseSensitive(json, "value");
        
        if (typeItem && cJSON_IsString(typeItem) && 
            strcmp(typeItem->valuestring, "Datum") == 0 && 
            valueItem && cJSON_IsObject(valueItem)) {
            // This is a typed JSON, use the value part
            actualJson = valueItem;
        }
    }
    
    if (!cJSON_IsObject(actualJson)) {
        if (outError) *outError = STR("Expected a JSON object");
        return NULL;
    }
    
    // Parse fields directly from JSON
    cJSON *item = NULL;
    
    // Required: dependent_variable_index
    item = cJSON_GetObjectItemCaseSensitive(actualJson, kDatumDependentVariableIndexKey);
    if (!cJSON_IsNumber(item)) {
        if (outError) *outError = STR("Missing or invalid \"dependent_variable_index\"");
        return NULL;
    }
    OCIndex dependentVariableIndex = item->valueint;
    
    // Required: component_index
    item = cJSON_GetObjectItemCaseSensitive(actualJson, kDatumComponentIndexKey);
    if (!cJSON_IsNumber(item)) {
        if (outError) *outError = STR("Missing or invalid \"component_index\"");
        return NULL;
    }
    OCIndex componentIndex = item->valueint;
    
    // Required: mem_offset
    item = cJSON_GetObjectItemCaseSensitive(actualJson, kDatumMemOffsetKey);
    if (!cJSON_IsNumber(item)) {
        if (outError) *outError = STR("Missing or invalid \"mem_offset\"");
        return NULL;
    }
    OCIndex memOffset = item->valueint;
    
    // Required: response (can be string or object depending on how it was serialized)
    item = cJSON_GetObjectItemCaseSensitive(actualJson, kDatumResponseKey);
    if (!item) {
        if (outError) *outError = STR("Missing \"response\"");
        return NULL;
    }
    
    SIScalarRef response = NULL;
    if (cJSON_IsString(item)) {
        // Simple string representation - parse as expression
        OCStringRef responseStr = OCStringCreateWithCString(item->valuestring);
        response = SIScalarCreateFromExpression(responseStr, outError);
        OCRelease(responseStr);
    } else if (cJSON_IsObject(item)) {
        // Complex JSON object - parse using SIScalar's JSON parser
        response = SIScalarCreateFromJSON(item, NULL);
        if (response && OCGetTypeID(response) != SIScalarGetTypeID()) {
            if (outError) *outError = STR("Response is not a SIScalar");
            OCRelease(response);
            response = NULL;
        }
    } else {
        if (outError) *outError = STR("Invalid response format");
        return NULL;
    }
    
    if (!response) return NULL;
    
    DatumRef datum = DatumCreate(response, dependentVariableIndex, componentIndex, memOffset, NULL, outError);
    OCRelease(response);
    return datum;
}
