#include "DependentVariable.h"
#include "DependentVariable_private.h"
/**
 * @file DependentVariable_accessors.c
 * @brief Property getters and setters for DependentVariable objects
 *
 * This file implements the accessors and mutators for the DependentVariable type,
 * allowing manipulation of its properties such as unit, name, description, etc.
 */
OCStringRef DependentVariableGetEncoding(DependentVariableRef dv) {
    return dv ? dv->encoding : NULL;
}
OCStringRef DependentVariableCopyEncoding(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->encoding ? OCStringCreateCopy(dv->encoding) : NULL;
}
bool DependentVariableSetEncoding(DependentVariableRef dv, OCStringRef newEnc) {
    if (!dv || !newEnc) return false;
    OCStringRef copy = OCStringCreateCopy(newEnc);
    if (!copy) return false;
    OCRelease(dv->encoding);
    dv->encoding = copy;
    return true;
}
OCStringRef DependentVariableGetType(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->type;
}
OCStringRef DependentVariableCopyType(DependentVariableRef dv) {
    if (!dv) return NULL;
    // Type should never be NULL - if it is, the object is corrupted
    if (!dv->type) {
        return NULL;
    }
    return OCStringCreateCopy(dv->type);
}
bool DependentVariableShouldSerializeExternally(DependentVariableRef dv) {
    return dv && OCStringEqual(dv->type, STR(kDependentVariableComponentTypeValueExternal));
}
bool DependentVariableSetType(DependentVariableRef dv, OCStringRef newType) {
    if (!dv || !newType) return false;
    // Validate newType against allowed values
    if (!OCStringEqual(newType, STR(kDependentVariableComponentTypeValueInternal)) &&
        !OCStringEqual(newType, STR(kDependentVariableComponentTypeValueExternal))) {
        return false;
    }
    OCStringRef copy = OCStringCreateCopy(newType);
    if (!copy) return false;
    OCRelease(dv->type);
    dv->type = copy;
    return true;
}
OCStringRef DependentVariableGetComponentsURL(DependentVariableRef dv) {
    return dv ? dv->componentsURL : NULL;
}
bool DependentVariableSetComponentsURL(DependentVariableRef dv, OCStringRef url) {
    if (!dv) return false;
    OCStringRef copy = url ? OCStringCreateCopy(url) : NULL;
    OCRelease(dv->componentsURL);
    dv->componentsURL = copy;
    return true;
}
OCStringRef DependentVariableGetName(DependentVariableRef dv) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, NULL);
    if (!dv->name) {
        return NULL;
    }
    return dv->name;
}
OCStringRef DependentVariableCopyName(DependentVariableRef dv) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, NULL);
    if (!dv->name) {
        return NULL;
    }
    return OCStringCreateCopy(dv->name);
}
bool DependentVariableSetName(DependentVariableRef dv, OCStringRef newName) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, false);
    if (dv->name == newName) {
        return true;
    }
    // Copy the incoming string (or default to "")
    OCStringRef copy = newName
                           ? OCStringCreateCopy(newName)
                           : STR("");
    if (!copy) {
        // allocation failed
        return false;
    }
    // Replace old name
    OCRelease(dv->name);
    dv->name = copy;
    return true;
}
OCStringRef DependentVariableGetDescription(DependentVariableRef dv) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, NULL);
    // Description should never be NULL - if it is, the object is corrupted
    if (!dv->description) {
        return NULL;
    }
    return dv->description;
}
OCStringRef DependentVariableCopyDescription(DependentVariableRef dv) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, NULL);
    // Description should never be NULL - if it is, the object is corrupted
    if (!dv->description) {
        return NULL;
    }
    return OCStringCreateCopy(dv->description);
}
bool DependentVariableSetDescription(DependentVariableRef dv, OCStringRef newDesc) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, false);
    // If it's literally the same object, nothing to do
    if (dv->description == newDesc) {
        return true;
    }
    // Prepare a copy (or get the interned empty string)
    OCStringRef copy = newDesc
                           ? OCStringCreateCopy(newDesc)
                           : STR("");
    if (!copy) {
        return false;
    }
    // Swap it in
    OCRelease(dv->description);
    dv->description = copy;
    return true;
}
bool DependentVariableIsScalarType(DependentVariableRef dv) {
    if (!dv || !dv->quantityType) return false;
    const char *qt = OCStringGetCString(dv->quantityType);
    return qt && strcmp(qt, "scalar") == 0;
}
bool DependentVariableIsVectorType(DependentVariableRef dv, OCIndex *outCount) {
    if (!dv || !dv->quantityType || !outCount) return false;
    const char *qt = OCStringGetCString(dv->quantityType);
    if (!qt) return false;
    // “vector_N”
    if (strncmp(qt, "vector_", 7) == 0) {
        long n = 0;
        if (sscanf(qt + 7, "%ld", &n) == 1) {
            *outCount = (OCIndex)n;
            return true;
        }
    }
    return false;
}
bool DependentVariableIsPixelType(DependentVariableRef dv, OCIndex *outCount) {
    if (!dv || !dv->quantityType || !outCount) return false;
    const char *qt = OCStringGetCString(dv->quantityType);
    if (!qt) return false;
    // “pixel_N”
    if (strncmp(qt, "pixel_", 6) == 0) {
        long n = 0;
        if (sscanf(qt + 6, "%ld", &n) == 1) {
            *outCount = (OCIndex)n;
            return true;
        }
    }
    return false;
}
bool DependentVariableIsMatrixType(DependentVariableRef dv, OCIndex *outRows, OCIndex *outCols) {
    if (!dv || !dv->quantityType || !outRows || !outCols) return false;
    const char *qt = OCStringGetCString(dv->quantityType);
    if (!qt) return false;
    // “matrix_R_C”
    if (strncmp(qt, "matrix_", 7) == 0) {
        long r = 0, c = 0;
        if (sscanf(qt + 7, "%ld_%ld", &r, &c) == 2) {
            *outRows = (OCIndex)r;
            *outCols = (OCIndex)c;
            return true;
        }
    }
    return false;
}
bool DependentVariableIsSymmetricMatrixType(DependentVariableRef dv, OCIndex *outN) {
    if (!dv || !dv->quantityType || !outN) return false;
    const char *qt = OCStringGetCString(dv->quantityType);
    if (!qt) return false;
    // “symmetric_matrix_N”
    if (strncmp(qt, "symmetric_matrix_", 17) == 0) {
        long n = 0;
        if (sscanf(qt + 17, "%ld", &n) == 1) {
            *outN = (OCIndex)n;
            return true;
        }
    }
    return false;
}
OCIndex DependentVariableComponentsCountFromQuantityType(OCStringRef quantityType) {
    if (!quantityType) return 1;  // default scalar
    const char *qt = OCStringGetCString(quantityType);
    if (!qt) return 1;
    // scalar
    if (strcmp(qt, "scalar") == 0) {
        return 1;
    }
    // pixel_N
    if (strncmp(qt, "pixel_", 6) == 0) {
        long n = 0;
        if (sscanf(qt + 6, "%ld", &n) == 1) {
            return (OCIndex)n;
        }
    }
    // vector_N
    if (strncmp(qt, "vector_", 7) == 0) {
        long n = 0;
        if (sscanf(qt + 7, "%ld", &n) == 1) {
            return (OCIndex)n;
        }
    }
    // matrix_R_C
    if (strncmp(qt, "matrix_", 7) == 0) {
        long r = 0, c = 0;
        if (sscanf(qt + 7, "%ld_%ld", &r, &c) == 2) {
            return (OCIndex)(r * c);
        }
    }
    // symmetric_matrix_N
    if (strncmp(qt, "symmetric_matrix_", 17) == 0) {
        long n = 0;
        if (sscanf(qt + 17, "%ld", &n) == 1) {
            return (OCIndex)(n * (n + 1) / 2);
        }
    }
    return (OCIndex)kOCNotFound;
}
OCMutableArrayRef DependentVariableCreateQuantityTypesArray(DependentVariableRef dv) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, NULL);
    OCIndex count = OCArrayGetCount(dv->components);
    OCMutableArrayRef types = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    if (!types) return NULL;
    // scalar
    if (count == 1) {
        OCArrayAppendValue(types, STR("scalar"));
    }
    // vector_N
    OCStringRef vstr = OCStringCreateWithFormat(
        STR("vector_%ld"),
        (long)count);
    OCArrayAppendValue(types, vstr);
    OCRelease(vstr);
    // pixel_N
    OCStringRef pstr = OCStringCreateWithFormat(
        STR("pixel_%ld"),
        (long)count);
    OCArrayAppendValue(types, pstr);
    OCRelease(pstr);
    // symmetric_matrix_N  if N*(N+1)/2 == count
    for (OCIndex n = 1; n < count; ++n) {
        if (n * (n + 1) / 2 == count) {
            OCStringRef sstr = OCStringCreateWithFormat(
                STR("symmetric_matrix_%ld"),
                (long)n);
            OCArrayAppendValue(types, sstr);
            OCRelease(sstr);
        }
    }
    // matrix_M_N  if M*N == count
    for (OCIndex m = 1; m <= count; ++m) {
        for (OCIndex n = 1; n <= count; ++n) {
            if (m * n == count) {
                OCStringRef mstr = OCStringCreateWithFormat(
                    STR("matrix_%ld_%ld"),
                    (long)m, (long)n);
                OCArrayAppendValue(types, mstr);
                OCRelease(mstr);
            }
        }
    }
    return types;
}
OCStringRef DependentVariableGetQuantityType(DependentVariableRef dv) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, NULL);
    // QuantityType should never be NULL - if it is, the object is corrupted
    if (!dv->quantityType) {
        return NULL;
    }
    return dv->quantityType;
}
OCStringRef DependentVariableCopyQuantityType(DependentVariableRef dv) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, NULL);
    // QuantityType should never be NULL - if it is, the object is corrupted
    if (!dv->quantityType) {
        return NULL;
    }
    return OCStringCreateCopy(dv->quantityType);
}
bool DependentVariableSetQuantityType(DependentVariableRef dv, OCStringRef qt) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, false);
    if (!qt) return false;
    const char *cstr = OCStringGetCString(qt);
    if (!cstr) return false;
    size_t len = strlen(cstr);
    OCIndex compCount = OCArrayGetCount(dv->components);
    // scalar
    if (strcmp(cstr, "scalar") == 0) {
        if (compCount != 1) return false;
        OCRelease(dv->quantityType);
        dv->quantityType = OCRetain(qt);
        return true;
    }
    // pixel_N
    if (len > 5 && strncmp(cstr, "pixel", 5) == 0) {
        long n;
        if (sscanf(cstr, "pixel_%ld", &n) != 1 || n != compCount)
            return false;
        OCRelease(dv->quantityType);
        dv->quantityType = OCRetain(qt);
        return true;
    }
    // vector_N
    if (len > 6 && strncmp(cstr, "vector", 6) == 0) {
        long n;
        if (sscanf(cstr, "vector_%ld", &n) != 1 || n != compCount)
            return false;
        OCRelease(dv->quantityType);
        dv->quantityType = OCRetain(qt);
        return true;
    }
    // matrix_N_M
    if (len > 6 && strncmp(cstr, "matrix", 6) == 0) {
        long n, m;
        if (sscanf(cstr, "matrix_%ld_%ld", &n, &m) != 2 || n * m != compCount)
            return false;
        OCRelease(dv->quantityType);
        dv->quantityType = OCRetain(qt);
        return true;
    }
    // symmetric_matrix_N
    if (len > 16 && strncmp(cstr, "symmetric_matrix", 16) == 0) {
        long n;
        if (sscanf(cstr, "symmetric_matrix_%ld", &n) != 1 || (n * (n + 1) / 2) != compCount)
            return false;
        OCRelease(dv->quantityType);
        dv->quantityType = OCRetain(qt);
        return true;
    }
    return false;
}
OCDictionaryRef DependentVariableGetApplicationMetaData(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->application;
}
bool DependentVariableSetApplicationMetaData(DependentVariableRef dv, OCDictionaryRef dict) {
    if (!dv) return false;
    OCRelease(dv->application);
    dv->application = dict ? OCTypeDeepCopyMutable(dict) : OCDictionaryCreateMutable(0);
    return dv->application != NULL;
}
OCTypeRef DependentVariableGetOwner(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->owner;
}
bool DependentVariableSetOwner(DependentVariableRef dv, OCTypeRef owner) {
    if (!dv) return false;
    dv->owner = owner;
    return true;
}
SparseSamplingRef DependentVariableGetSparseSampling(DependentVariableRef dv) {
    return dv ? dv->sparseSampling : NULL;
}
bool DependentVariableSetSparseSampling(DependentVariableRef dv,
                                        SparseSamplingRef ss) {
    if (!dv) return false;
    // Release any existing sparseSampling
    OCRelease(dv->sparseSampling);
    if (ss) {
        // Deep-copy the provided SparseSampling
        dv->sparseSampling = (SparseSamplingRef)OCTypeDeepCopyMutable(ss);
        return dv->sparseSampling != NULL;
    } else {
        // Clearing out sparseSampling
        dv->sparseSampling = NULL;
        return true;
    }
}
OCStringRef DependentVariableGetQuantityName(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->quantityName;
}
OCStringRef DependentVariableCopyQuantityName(DependentVariableRef dv) {
    if (!dv) return NULL;
    // Note: quantityName can be NULL (optional field), unlike name/description/quantityType
    return dv->quantityName ? OCStringCreateCopy(dv->quantityName) : NULL;
}
bool DependentVariableSetQuantityName(DependentVariableRef dv, OCStringRef quantityName) {
    if (!dv || !quantityName) return false;
    // 1) Check that the name corresponds to a known dimensionality
    OCStringRef err = NULL;
    SIDimensionalityRef qDim = SIDimensionalityForQuantity(quantityName, &err);
    if (!qDim) {
        if (err) OCRelease(err);
        return false;
    }
    OCRelease(err);
    // 2) If it’s already set to exactly that same object, we’re done
    if (dv->quantityName == quantityName) return true;
    // 3) Validate everything else still lines up (unit, type, labels, count)
    OCIndex count = OCArrayGetCount(dv->components);
    if (!validateDependentVariableParameters(
            dv->type, dv->unit, quantityName,
            dv->quantityType, dv->componentLabels, count,
            dv->sparseSampling))
        return false;
    // 4) All good—swap in the new name
    OCRelease(dv->quantityName);
    dv->quantityName = OCStringCreateCopy(quantityName);
    return dv->quantityName != NULL;
}
