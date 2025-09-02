/**
 * @file Dataset_io.c
 * @brief File I/O operations for Dataset
 *
 * This module handles file import/export operations, directory creation,
 * and file path management for Dataset objects.
 */

#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "../../RMNLibrary.h"
#include "Dataset_private.h"

#if defined(_WIN32)
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#define PATH_SEPARATOR '\\'
#else
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0755)
#define PATH_SEPARATOR '/'
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
#pragma region Utility Functions
// ============================================================================

const char *parse_components_url_path(const char *url) {
    if (!url) return NULL;
    // Check if it starts with "file:"
    if (strncmp(url, "file:", 5) == 0) {
        return url + 5;  // Skip the "file:" prefix
    }
    // Check if it's an HTTP/HTTPS URL - extract just the filename
    if (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) {
        const char *last_slash = strrchr(url, '/');
        if (last_slash && last_slash[1] != '\0') {
            return last_slash + 1;  // Return filename part
        }
    }
    // For any other scheme or plain paths, return as-is
    return url;
}

bool join_path(char *dest, size_t dest_size, const char *dir, char sep, const char *filename) {
    if (!dest || !dir || !filename) return false;
    int len = snprintf(dest, dest_size, "%s%c%s", dir, sep, filename);
    return len > 0 && (size_t)len < dest_size;
}

bool ensure_directory(const char *dir, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dir || !*dir) {
        if (outError) *outError = STR("Invalid directory path");
        return false;
    }
    size_t dlen = strnlen(dir, PATH_MAX);
    if (dlen >= PATH_MAX) {
        if (outError) *outError = STR("Directory path too long");
        return false;
    }
    // Copy into a mutable buffer and strip trailing slashes
    char path[PATH_MAX];
    memcpy(path, dir, dlen);
    path[dlen] = '\0';
    while (dlen > 0 && (path[dlen - 1] == '/' || path[dlen - 1] == '\\')) {
        path[--dlen] = '\0';
    }
    // Iteratively create each path segment
    for (size_t i = 1; i <= dlen; ++i) {
        if (path[i] == '/' || path[i] == '\\' || i == dlen) {
            char saved = path[i];
            path[i] = '\0';
            errno = 0;
            if (MKDIR(path) != 0 && errno != EEXIST) {
                if (outError) {
                    OCStringRef fp = OCStringCreateWithCString(path);
                    *outError = OCStringCreateWithFormat(
                        STR("Failed to create directory %@"), fp);
                    OCRelease(fp);
                }
                return false;
            }
            path[i] = saved;
        }
    }
    return true;
}

uint8_t *read_file_bytes(const char *path, size_t *out_len) {
    if (out_len) *out_len = 0;
    if (!path) return NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    struct stat st;
    if (fstat(fileno(f), &st) != 0) {
        fclose(f);
        return NULL;
    }
    size_t len = (size_t)st.st_size;
    // Guard against malloc(0)
    uint8_t *buf = malloc(len ? len : 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t got = fread(buf, 1, len, f);
    fclose(f);
    if (got != len) {
        free(buf);
        return NULL;
    }
    if (out_len) *out_len = len;
    return buf;
}

bool ensure_parent_dirs(const char *fullpath, OCStringRef *outError) {
    if (!fullpath) {
        if (outError) *outError = STR("Invalid path");
        return false;
    }
    // Copy since dirname() may modify its argument
    char tmp[PATH_MAX];
    size_t plen = strnlen(fullpath, PATH_MAX);
    if (plen >= PATH_MAX) {
        if (outError) *outError = STR("Path too long");
        return false;
    }
    memcpy(tmp, fullpath, plen);
    tmp[plen] = '\0';
    char *dir = dirname(tmp);
    // If dirname returns "." or empty, nothing to do
    if (!dir || !*dir || (dir[0] == '.' && dir[1] == '\0'))
        return true;
    return ensure_directory(dir, outError);
}

bool derive_directory_from_path(const char *filepath, char *output, size_t output_size) {
    if (!filepath || !output || output_size == 0) return false;
    size_t len = strnlen(filepath, PATH_MAX);
    if (len == 0 || len >= PATH_MAX) return false;
    // Copy the path and find the last directory separator
    char temp[PATH_MAX];
    memcpy(temp, filepath, len);
    temp[len] = '\0';
    char *last_sep = strrchr(temp, '/');
    if (!last_sep) {
        last_sep = strrchr(temp, '\\');  // Windows support
    }
    if (last_sep) {
        // Terminate at the separator to get directory
        *last_sep = '\0';
        size_t dir_len = strlen(temp);
        if (dir_len >= output_size) return false;
        strcpy(output, temp);
    } else {
        // No separator found, use current directory
        if (output_size < 2) return false;
        strcpy(output, ".");
    }
    return true;
}

// ============================================================================
#pragma region Export Functions
// ============================================================================

bool DatasetExport(DatasetRef ds,
                   const char *json_path,
                   const char *binary_dir,
                   OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!ds || !json_path) {
        if (outError) *outError = STR("Invalid arguments");
        return false;
    }
    // If binary_dir is NULL, derive it from json_path
    char derived_binary_dir[PATH_MAX];
    if (!binary_dir) {
        if (!derive_directory_from_path(json_path, derived_binary_dir, sizeof(derived_binary_dir))) {
            if (outError) *outError = STR("Cannot determine binary directory from JSON path");
            return false;
        }
        binary_dir = derived_binary_dir;
    }
    // 0) decide extension
    bool hasExternal = false;
    OCArrayRef dvsArray = DatasetGetDependentVariables(ds);
    OCIndex dvCount = dvsArray ? OCArrayGetCount(dvsArray) : 0;
    for (OCIndex i = 0; i < dvCount; ++i) {
        DependentVariableRef dv =
            (DependentVariableRef)OCArrayGetValueAtIndex(dvsArray, i);
        if (dv && DependentVariableShouldSerializeExternally(dv)) {
            hasExternal = true;
            break;
        }
    }
    const char *wantExt = hasExternal ? "csdfe" : "csdf";
    const char *dot = strrchr(json_path, '.');
    const char *gotExt = dot ? dot + 1 : "";
    if (strcasecmp(gotExt, wantExt) != 0) {
        if (outError) {
            OCStringRef p = OCStringCreateWithCString(json_path);
            OCStringRef e = OCStringCreateWithCString(wantExt);
            OCStringRef s = OCStringCreateWithCString(
                hasExternal ? "contains" : "does not contain");
            *outError = OCStringCreateWithFormat(
                STR("CSDM requires extension '%@' when file %@ external data; got '%@'"),
                e, s, p);
            OCRelease(p);
            OCRelease(e);
            OCRelease(s);
        }
        return false;
    }
    // 1) build full JSON directly
    cJSON *core = OCTypeCopyJSON((OCTypeRef)ds, false);
    if (!core) {
        if (outError) *outError = STR("Failed to create JSON from Dataset");
        return false;
    }
    // 1a) strip inline components/encoding on externals
    {
        cJSON *dvList = cJSON_GetObjectItemCaseSensitive(core, kDatasetDependentVariablesKey);
        if (dvList && cJSON_IsArray(dvList)) {
            cJSON *dv_item;
            cJSON_ArrayForEach(dv_item, dvList) {
                if (!cJSON_IsObject(dv_item)) continue;
                cJSON *type_item = cJSON_GetObjectItemCaseSensitive(dv_item, kDependentVariableTypeKey);
                if (type_item && cJSON_IsString(type_item) && 
                    strcmp(type_item->valuestring, kDependentVariableComponentTypeValueExternal) == 0) {
                    cJSON_DeleteItemFromObject(dv_item, kDependentVariableComponentsKey);
                    cJSON_DeleteItemFromObject(dv_item, kDependentVariableEncodingKey);
                }
            }
        }
    }
    // 2) add envelope fields
    {
        cJSON_AddStringToObject(core, kDatasetVersionKey, "1.0");
        {
            OCStringRef ts = OCCreateISO8601Timestamp();
            cJSON_AddStringToObject(core, kDatasetTimestampKey, OCStringGetCString(ts));
            OCRelease(ts);
        }
        if (DatasetGetReadOnly(ds)) {
            cJSON_AddBoolToObject(core, kDatasetReadOnlyKey, true);
        }
        if (DatasetGetGeographicCoordinate(ds)) {
            GeographicCoordinateRef gc = DatasetGetGeographicCoordinate(ds);
            cJSON *gcJson = OCTypeCopyJSON((OCTypeRef)gc, false);
            if (gcJson) {
                cJSON_AddItemToObject(core, kDatasetGeoCoordinateKey, gcJson);
            }
        }
    }
    // 3) wrap under "csdm"
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        cJSON_Delete(core);
        if (outError) *outError = STR("Failed to create root JSON object");
        return false;
    }
    cJSON_AddItemToObject(json, kDatasetCsdmEnvelopeKey, core);
    // 4) serialize JSON → file
    // (json object is already created above)
    if (!json) {
        if (outError) *outError = STR("Failed to convert dataset to JSON");
        return false;
    }
    char *json_text = cJSON_Print(json);
    cJSON_Delete(json);
    if (!json_text) {
        if (outError) *outError = STR("Failed to generate JSON string");
        return false;
    }
    size_t json_len = strlen(json_text);
    if (!ensure_parent_dirs(json_path, outError)) {
        free(json_text);
        return false;
    }
    FILE *jf = fopen(json_path, "wb");
    if (!jf) {
        free(json_text);
        if (outError) *outError = STR("Failed to open JSON output file");
        return false;
    }
    size_t wrote = fwrite(json_text, 1, json_len, jf);
    fclose(jf);
    free(json_text);
    if (wrote != json_len) {
        if (outError) *outError = STR("Error writing JSON file");
        return false;
    }
    // 5) dump binary blobs
    if (!ensure_directory(binary_dir, outError))
        return false;
    for (OCIndex i = 0; i < dvCount; ++i) {
        DependentVariableRef dv =
            (DependentVariableRef)OCArrayGetValueAtIndex(dvsArray, i);
        if (!dv || !DependentVariableShouldSerializeExternally(dv))
            continue;
        OCStringRef url = DependentVariableGetComponentsURL(dv);
        if (!url) {
            if (outError) *outError = STR("External DV missing components_url");
            return false;
        }
        const char *rel = parse_components_url_path(OCStringGetCString(url));
        if (!rel) {
            if (outError) *outError = STR("Invalid components_url");
            return false;
        }
        char fullpath[PATH_MAX];
        if (!join_path(fullpath, sizeof(fullpath),
                       binary_dir, PATH_SEPARATOR, rel)) {
            if (outError) *outError = STR("Binary path too long");
            return false;
        }
        if (!ensure_parent_dirs(fullpath, outError))
            return false;
        OCDataRef blob = DependentVariableCreateCSDMComponentsData(
            dv, DatasetGetDimensions(ds));
        if (!blob) {
            if (outError) *outError = STR("Failed to create binary blob");
            return false;
        }
        FILE *bf = fopen(fullpath, "wb");
        if (!bf) {
            OCRelease(blob);
            if (outError) *outError = STR("Failed to open binary output file");
            return false;
        }
        const void *bytes = OCDataGetBytesPtr(blob);
        size_t len = (size_t)OCDataGetLength(blob);
        bool ok = (fwrite(bytes, 1, len, bf) == len);
        fclose(bf);
        OCRelease(blob);
        if (!ok) {
            if (outError) *outError = STR("Error writing binary blob");
            return false;
        }
    }
    return true;
}

// ============================================================================
#pragma region Import Functions
// ============================================================================

DatasetRef DatasetCreateWithImport(const char *json_path,
                                   const char *binary_dir,
                                   OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json_path || !binary_dir) {
        if (outError)
            *outError = STR("Dataset import failed: invalid arguments");
        return NULL;
    }
    // 1) read + parse JSON
    FILE *jf = fopen(json_path, "rb");
    if (!jf) {
        if (outError) {
            OCStringRef p = OCStringCreateWithCString(json_path);
            *outError = OCStringCreateWithFormat(
                STR("Dataset import failed: cannot open JSON file '%@'"), p);
            OCRelease(p);
        }
        return NULL;
    }
    if (fseek(jf, 0, SEEK_END) != 0) {
        fclose(jf);
        if (outError) *outError = STR("Dataset import failed: cannot seek JSON file");
        return NULL;
    }
    long fsize = ftell(jf);
    if (fsize < 0) {
        fclose(jf);
        if (outError) *outError = STR("Dataset import failed: cannot determine JSON file size");
        return NULL;
    }
    rewind(jf);
    char *buffer = malloc((size_t)fsize + 1);
    if (!buffer) {
        fclose(jf);
        if (outError) *outError = STR("Dataset import failed: memory allocation error");
        return NULL;
    }
    size_t got = fread(buffer, 1, (size_t)fsize, jf);
    fclose(jf);
    if (got != (size_t)fsize) {
        free(buffer);
        if (outError) *outError = STR("Dataset import failed: incomplete read of JSON file");
        return NULL;
    }
    buffer[fsize] = '\0';
    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        const char *e = cJSON_GetErrorPtr();
        if (outError) {
            if (e) {
                OCStringRef estr = OCStringCreateWithCString(e);
                *outError = OCStringCreateWithFormat(
                    STR("Dataset import failed: JSON parse error at '%@'"), estr);
                OCRelease(estr);
            } else {
                *outError = STR("Dataset import failed: invalid JSON format");
            }
        }
        return NULL;
    }
    
    // 2) Extract dataset JSON from CSDM envelope
    cJSON *csdm_envelope = cJSON_GetObjectItemCaseSensitive(root, kDatasetCsdmEnvelopeKey);
    if (!csdm_envelope) {
        cJSON_Delete(root);
        if (outError) *outError = STR("Dataset import failed: missing CSDM envelope");
        return NULL;
    }
    
    // Create a copy of the dataset content and remove envelope-specific fields
    cJSON *dataset_json = cJSON_Duplicate(csdm_envelope, true);
    if (!dataset_json) {
        cJSON_Delete(root);
        if (outError) *outError = STR("Dataset import failed: cannot extract dataset from envelope");
        return NULL;
    }
    
    // Remove only envelope-specific fields, keep dataset core fields
    cJSON_DeleteItemFromObject(dataset_json, kDatasetTimestampKey);
    cJSON_DeleteItemFromObject(dataset_json, kDatasetReadOnlyKey);
    cJSON_DeleteItemFromObject(dataset_json, kDatasetGeoCoordinateKey);
    
    // 3) Create Dataset from cleaned JSON (without envelope)
    DatasetRef ds = DatasetCreateFromJSON(dataset_json, outError);
    cJSON_Delete(root);
    cJSON_Delete(dataset_json);
    if (!ds) return NULL;
    // 4) compute expected number of points from the dataset's dimensions
    OCArrayRef dims = DatasetGetDimensions(ds);
    OCIndex expectedSize = RMNCalculateSizeFromDimensions(dims);
    // 5) process dependent variables
    OCArrayRef dvsArray = DatasetGetDependentVariables(ds);
    OCIndex dvCount = dvsArray ? OCArrayGetCount(dvsArray) : 0;
    OCStringRef keyInternal = STR(kDependentVariableComponentTypeValueInternal);
    OCStringRef keyBase64 = STR(kDependentVariableEncodingValueBase64);
    for (OCIndex i = 0; i < dvCount; ++i) {
        DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(dvsArray, i);
        if (!dv || !DependentVariableShouldSerializeExternally(dv))
            continue;
        // ensure DV.size is set
        OCIndex npts = DependentVariableGetSize(dv);
        if (npts == 0) {
            npts = expectedSize;
            DependentVariableSetSize(dv, npts);
        }
        // path resolution
        OCStringRef url = DependentVariableGetComponentsURL(dv);
        if (!url) {
            if (outError)
                *outError = STR("Dataset import failed: missing components_url for external variable");
            OCRelease(ds);
            return NULL;
        }
        const char *rel = parse_components_url_path(OCStringGetCString(url));
        if (!rel) {
            if (outError)
                *outError = STR("Dataset import failed: invalid components_url");
            OCRelease(ds);
            return NULL;
        }
        char fullpath[PATH_MAX];
        if (!join_path(fullpath, sizeof(fullpath), binary_dir, PATH_SEPARATOR, rel)) {
            if (outError)
                *outError = STR("Dataset import failed: binary path too long for component");
            OCRelease(ds);
            return NULL;
        }
        size_t total_bytes = 0;
        uint8_t *bytes = read_file_bytes(fullpath, &total_bytes);
        if (!bytes) {
            if (outError) {
                OCStringRef p = OCStringCreateWithCString(fullpath);
                *outError = OCStringCreateWithFormat(
                    STR("Dataset import failed: cannot read binary component '%@'"), p);
                OCRelease(p);
            }
            OCRelease(ds);
            return NULL;
        }
        OCIndex ncomps = DependentVariableGetComponentCount(dv);
        if (ncomps == 0) {
            OCStringRef qt = DependentVariableGetQuantityType(dv);
            ncomps = DependentVariableComponentsCountFromQuantityType(qt);
        }
        size_t elemSize = SIQuantityElementSize((SIQuantityRef)dv);
        size_t chunk = (size_t)npts * elemSize;
        if (chunk * (size_t)ncomps != total_bytes) {
            free(bytes);
            if (outError)
                *outError = STR("Dataset import failed: binary size mismatch for component");
            OCRelease(ds);
            return NULL;
        }
        OCMutableArrayRef comps = OCArrayCreateMutable(ncomps, &kOCTypeArrayCallBacks);
        if (!comps) {
            free(bytes);
            if (outError)
                *outError = STR("Dataset import failed: cannot allocate components array");
            OCRelease(ds);
            return NULL;
        }
        for (OCIndex ci = 0; ci < ncomps; ++ci) {
            OCMutableDataRef buf = OCDataCreateMutable(chunk);
            if (!buf || !OCDataAppendBytes(buf, bytes + (ci * chunk), chunk)) {
                OCRelease(buf);
                OCRelease(comps);
                free(bytes);
                if (outError)
                    *outError = STR("Dataset import failed: cannot create component buffer");
                OCRelease(ds);
                return NULL;
            }
            OCArrayAppendValue(comps, buf);
            OCRelease(buf);
        }
        free(bytes);
        if (!DependentVariableSetComponents(dv, comps)) {
            OCRelease(comps);
            if (outError)
                *outError = STR("Dataset import failed: cannot install DV components");
            OCRelease(ds);
            return NULL;
        }
        OCRelease(comps);
        // flip to internal, base64, and NULL components URL
        DependentVariableSetType(dv, keyInternal);
        DependentVariableSetEncoding(dv, keyBase64);
        DependentVariableSetComponentsURL(dv, NULL);
    }
    return ds;
}

#ifdef __cplusplus
}
#endif
