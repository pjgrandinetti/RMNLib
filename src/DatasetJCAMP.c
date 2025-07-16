//
//  DatasetImportJCAMP.c
//  RMN 2.0
//
//  Created by Philip J. Grandinetti on 2/21/14.
//  Copyright (c) 2014 PhySy. All rights reserved.
//
#include "RMNLibrary.h"
#include <string.h>

// Return a new dictionary or NULL + error string on failure.
OCDictionaryRef
DatasetImportJCAMPCreateDictionaryWithLines(OCArrayRef lines,
                                            OCIndex      *indexOut,
                                            OCStringRef  *error)
{
    if (OCArrayGetCount(lines) < 1) {
        if (error) *error = STR("JCAMP import: input `lines` array is empty");
        return NULL;
    }

    // 1) Verify TITLE or DTYPx
    OCIndex idx = 0;
    OCStringRef firstLine = (OCStringRef)OCArrayGetValueAtIndex(lines, 0);
    bool hasDTYPx = false;
    if (OCStringCompare(firstLine, STR("DTYPx"), 0) == kOCCompareEqualTo) {
        if (OCArrayGetCount(lines) < 2) {
            if (error) *error = STR("JCAMP import: 'DTYPx' found on line 0, but no line 1 to read TITLE from");
            return NULL;
        }
        hasDTYPx = true;
        firstLine = (OCStringRef)OCArrayGetValueAtIndex(lines, 1);
        idx = 1;
    }

    // split KEY=VALUE
    OCArrayRef kv = OCStringCreateArrayBySeparatingStrings(firstLine, STR("="));
    if (OCArrayGetCount(kv) != 2) {
        if (error) {
            *error = OCStringCreateWithFormat(
                STR("JCAMP import: malformed header on line %u: '%@'"),
                idx,
                firstLine);
        }
        OCRelease(kv);
        return NULL;
    }

    OCStringRef key0 = OCArrayGetValueAtIndex(kv, 0);
    OCStringRef val0 = OCArrayGetValueAtIndex(kv, 1);
    
    // Strip $$ comments from the initial value as well
    OCMutableStringRef val0_clean = OCStringCreateMutableCopy(val0);
    OCRange commentRange = OCStringFind(val0_clean, STR("$$"), 0);
    if (commentRange.location != kOCNotFound) {
        OCStringDelete(val0_clean, OCRangeMake(commentRange.location, OCStringGetLength(val0_clean) - commentRange.location));
    }
    OCStringTrimWhitespace(val0_clean);
    
    // Trim whitespace from key
    OCMutableStringRef trimmedKey = OCStringCreateMutableCopy(key0);
    OCStringTrimWhitespace(trimmedKey);
    
    if (OCStringCompare(trimmedKey, STR("TITLE"), kOCCompareCaseInsensitive) != kOCCompareEqualTo) {
        if (error) {
            *error = OCStringCreateWithFormat(
                STR("JCAMP import: expected TITLE on line %u, got '%@'"),
                idx,
                trimmedKey);
        }
        OCRelease(trimmedKey);
        OCRelease(kv);
        return NULL;
    }
    
    OCRelease(trimmedKey);

    // build dictionary
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    OCDictionaryAddValue(dict, key0, val0_clean);
    OCRelease(val0_clean);
    OCRelease(kv);

    if (hasDTYPx) {
        OCDictionaryAddValue(dict, STR("DTYPx"), kOCBooleanTrue);
    }

    // iterate remaining lines
    for (OCIndex i = idx + 1; i < OCArrayGetCount(lines); ++i) {
        OCStringRef line = (OCStringRef)OCArrayGetValueAtIndex(lines, i);
        OCArrayRef parts = OCStringCreateArrayBySeparatingStrings(line, STR("="));

        if (OCArrayGetCount(parts) == 2) {
            OCStringRef k = OCArrayGetValueAtIndex(parts, 0);
            OCStringRef v = OCArrayGetValueAtIndex(parts, 1);
            
            // Trim whitespace from key for comparison
            OCMutableStringRef trimmedK = OCStringCreateMutableCopy(k);
            OCStringTrimWhitespace(trimmedK);
            
            if (OCStringGetLength(trimmedK) == 0) {
                // skip empty key
                OCRelease(trimmedK);
            }
            else if (OCStringCompare(trimmedK, STR("END"), kOCCompareCaseInsensitive) == kOCCompareEqualTo) {
                OCRelease(trimmedK);
                OCRelease(parts);
                *indexOut = i;
                return dict;
            }
            else {
                // normal key/value - strip $$ comments from value
                OCMutableStringRef mv = OCStringCreateMutableCopy(v);
                OCRange commentRange = OCStringFind(mv, STR("$$"), 0);
                if (commentRange.location != kOCNotFound) {
                    OCStringDelete(mv, OCRangeMake(commentRange.location, OCStringGetLength(mv) - commentRange.location));
                }
                OCStringTrimWhitespace(mv);
                // Use the trimmed key for the dictionary
                OCDictionaryAddValue(dict, trimmedK, mv);
                OCRelease(mv);
                OCRelease(trimmedK);
            }
        }
        else if (OCArrayGetCount(parts) == 1) {
            // single-token line → ambiguous record start
            if (error) {
                *error = OCStringCreateWithFormat(
                  STR("JCAMP import: malformed record at line %u: '%@' (no '=')"),
                  i, line);
            }
            OCRelease(parts);
            OCRelease(dict);
            return NULL;
        }
        // else: more than one '=', unlikely but skip
        OCRelease(parts);
    }

    // if we fall off the bottom without END
    if (error) {
        *error = STR("JCAMP import: reached EOF without encountering 'END' record");
    }
    OCRelease(dict);
    return NULL;
}
DatasetRef DatasetImportJCAMPCreateSignalWithData(OCDataRef contents, OCStringRef *error) {
    if (error && *error) return NULL;
    if (!contents) {
        if (error) *error = STR("JCAMP import: input data is NULL");
        return NULL;
    }

    OCStringRef temp = OCStringCreateWithExternalRepresentation(contents);
    if (!temp) {
        if (error) *error = STR("JCAMP import: could not convert data to string");
        return NULL;
    }
    OCMutableStringRef fileString = OCStringCreateMutableCopy(temp);
    OCRelease(temp);
    if (!fileString) {
        if (error) *error = STR("JCAMP import: could not create mutable string from file contents");
        return NULL;
    }

    OCArrayRef array = OCStringCreateArrayBySeparatingStrings(fileString, STR("##"));
    OCRelease(fileString);
    if (!array) {
        if (error) *error = STR("JCAMP import: could not split file into blocks by '##'");
        return NULL;
    }
    OCMutableArrayRef lines = OCArrayCreateMutableCopy(array);
    OCRelease(array);
    if (!lines) {
        if (error) *error = STR("JCAMP import: could not create mutable array of lines");
        return NULL;
    }
    if (OCArrayGetCount(lines) < 1) {
        if (error) *error = STR("JCAMP import: input 'lines' array is empty after splitting");
        OCRelease(lines);
        return NULL;
    }

    for (OCIndex index = 0; index < OCArrayGetCount(lines); index++) {
        OCMutableStringRef line = OCStringCreateMutableCopy(OCArrayGetValueAtIndex(lines, index));
        OCRange range = OCStringFind(line, STR("$$"), 0);
        if (range.location != kOCNotFound) OCStringDelete(line, OCRangeMake(range.location, OCStringGetLength(line) - range.location));
        OCStringFindAndReplace(line, STR("\r"), STR("\n"), OCRangeMake(0, OCStringGetLength(line)), 0);
        OCStringTrim(line, STR("\n"));
        OCStringTrimWhitespace(line);
        OCArraySetValueAtIndex(lines, index, line);
        OCRelease(line);
    }
    for (OCIndex index = OCArrayGetCount(lines) - 1; index >= 0; index--) {
        OCStringRef string = OCArrayGetValueAtIndex(lines, index);
        if (OCStringGetLength(string) == 0) OCArrayRemoveValueAtIndex(lines, index);
    }

    OCIndex index = 0;
    OCDictionaryRef dictionary = DatasetImportJCAMPCreateDictionaryWithLines(lines, &index,error);
    OCRelease(lines);
    if (NULL == dictionary) return NULL;

    OCStringRef key = STR("PEAK TABLE");
    if (OCDictionaryContainsKey(dictionary, key)) {
        if (error) {
            *error = STR("JCAMP Peak Table file is unsupported.");
        }
        OCRelease(dictionary);
        return NULL;
    }

    // Read in JCAMP Core Header
    OCMutableDictionaryRef jcampDatasetMetaData = OCDictionaryCreateMutable(0);
    OCStringRef string = NULL;
    key = STR("TITLE");
    OCStringRef title = NULL;
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) {
            OCDictionaryAddValue(jcampDatasetMetaData, key, string);
            title = OCRetain(string);
        }
    }
    string = NULL;
    key = STR("JCAMP-DX");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }
    string = NULL;
    key = STR("DATA CLASS");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }
    string = NULL;
    bool nmrSpectrumType = false;
    bool irSpectrumType = false;
    (void)irSpectrumType; // Avoid unused variable warning
    bool eprSpectrumType = false;
    key = STR("DATA TYPE");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) {
            OCDictionaryAddValue(jcampDatasetMetaData, key, string);
            if (OCStringCompare(string, STR("NMR SPECTRUM"), 0) == kOCCompareEqualTo) nmrSpectrumType = true;
            if (OCStringCompare(string, STR("INFRARED SPECTRUM"), 0) == kOCCompareEqualTo) irSpectrumType = true;
            if (OCStringCompare(string, STR("EPR SPECTRUM"), 0) == kOCCompareEqualTo) eprSpectrumType = true;
        }
    }
    string = NULL;
    key = STR("ORIGIN");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }
    string = NULL;
    key = STR("OWNER");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }
    string = NULL;
    key = STR("BLOCKS");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }
    string = NULL;
    key = STR("DATE");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }
    string = NULL;
    key = STR("TIME");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }
    string = NULL;
    key = STR("SPECTROMETER/DATA SYSTEM");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }
    string = NULL;
    key = STR("INSTRUMENT PARAMETERS");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }
    string = NULL;
    key = STR("SAMPLING PROCEDURE");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }
    string = NULL;
    key = STR("XUNITS");
    SIUnitRef xUnits = NULL;
    SIUnitRef inverseXUnits = NULL;
    OCStringRef quantityName = NULL;
    OCStringRef inverseQuantityName = NULL;
    if (OCDictionaryContainsKey(dictionary, key)) {
        OCMutableStringRef string = OCStringCreateMutableCopy((OCStringRef)OCDictionaryGetValue(dictionary, key));
        if (string) {
            OCStringTrimWhitespace(string);
            OCDictionaryAddValue(jcampDatasetMetaData, key, string);
            if (OCStringCompare(string, STR("1/CM"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("1/cm"), &unit_multiplier, error);
                inverseXUnits = SIUnitFromExpression(STR("cm"), &unit_multiplier, error);
                quantityName = kSIQuantityWavenumber;
                inverseQuantityName = kSIQuantityLength;
            }
            if (OCStringCompare(string, STR("VOLUME"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("mL"), &unit_multiplier, error);
                inverseXUnits = SIUnitFromExpression(STR("1/mL"), &unit_multiplier, error);
                quantityName = kSIQuantityVolume;
                inverseQuantityName = kSIQuantityInverseVolume;
            } else if (OCStringCompare(string, STR("m/z"), 0) == kOCCompareEqualTo || OCStringCompare(string, STR("M/Z"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("Th"), &unit_multiplier, error);
                inverseXUnits = SIUnitFromExpression(STR("(1/Th)"), &unit_multiplier, error);
                quantityName = kSIQuantityMassToChargeRatio;
                inverseQuantityName = kSIQuantityChargeToMassRatio;
            } else if (OCStringCompare(string, STR("NANOMETERS"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("nm"), &unit_multiplier, error);
                inverseXUnits = SIUnitFromExpression(STR("1/nm"), &unit_multiplier, error);
                quantityName = kSIQuantityLength;
                inverseQuantityName = kSIQuantityWavenumber;
            } else if (OCStringCompare(string, STR("GAUSS"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("G"), &unit_multiplier, error);
                inverseXUnits = SIUnitFromExpression(STR("1/G"), &unit_multiplier, error);
                quantityName = kSIQuantityMagneticFluxDensity;
                inverseQuantityName = kSIQuantityInverseMagneticFluxDensity;
            } else if (OCStringCompare(string, STR("HZ"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("Hz"), &unit_multiplier, error);
                inverseXUnits = SIUnitFindWithUnderivedSymbol(STR("s"));
                quantityName = kSIQuantityFrequency;
                inverseQuantityName = kSIQuantityTime;
            } else if (OCStringCompare(string, STR("TIME"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("min"), &unit_multiplier, error);
                inverseXUnits = SIUnitFindWithUnderivedSymbol(STR("Hz"));
                quantityName = kSIQuantityTime;
                inverseQuantityName = kSIQuantityFrequency;
            } else if (OCStringCompare(string, STR("SECONDS"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("s"), &unit_multiplier, error);
                inverseXUnits = SIUnitFindWithUnderivedSymbol(STR("Hz"));
                quantityName = kSIQuantityTime;
                inverseQuantityName = kSIQuantityFrequency;
            }
            OCRelease(string);
        }
    }
    string = NULL;
    key = STR("YUNITS");
    //    SIUnitRef yUnits = NULL;
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }
    string = NULL;
    key = STR("RESOLUTION");
    double resolution = 0;
    (void)resolution; // Avoid unused variable warning
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
        resolution = creal(OCStringGetDoubleComplexValue(string));
    }
    string = NULL;
    key = STR("COMMENT");
    OCStringRef description = NULL;
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) {
            OCDictionaryAddValue(jcampDatasetMetaData, key, string);
            description = OCRetain(string);
        }
    }
    string = NULL;
    key = STR("FIRSTX");
    double firstX = 0;
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
        firstX = creal(OCStringGetDoubleComplexValue(string));
    }
    string = NULL;
    key = STR("LASTX");
    double lastX = 0;
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
        lastX = creal(OCStringGetDoubleComplexValue(string));
    }
    string = NULL;
    key = STR("DELTAX");
    double deltaX = 0;
    (void)deltaX; // Avoid unused variable warning
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
        deltaX = creal(OCStringGetDoubleComplexValue(string));
    }
    string = NULL;
    key = STR("MAXY");
    double maxY = 0;
    (void)maxY; // Avoid unused variable warning
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
        maxY = creal(OCStringGetDoubleComplexValue(string));
    }
    string = NULL;
    key = STR("MINY");
    double minY = 0;
    (void)minY; // Avoid unused variable warning
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
        minY = creal(OCStringGetDoubleComplexValue(string));
    }
    string = NULL;
    key = STR("XFACTOR");
    double xFactor = 1;
    (void) xFactor; // Avoid unused variable warning
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
        xFactor = creal(OCStringGetDoubleComplexValue(string));
    }
    string = NULL;
    key = STR("YFACTOR");
    double yFactor = 1;
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
        yFactor = creal(OCStringGetDoubleComplexValue(string));
    }
    string = NULL;
    key = STR("NPOINTS");
    OCIndex size = 1;
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) {
            OCDictionaryAddValue(jcampDatasetMetaData, key, string);
            size = (int) creal(OCStringGetDoubleComplexValue(string));
        }
    } else {
        // NPOINTS not found - this could be an issue
    }
    
    // Ensure minimum size for linear dimension
    if (size < 2) {
        if (error) *error = STR("JCAMP import: NPOINTS must be at least 2 for linear dimension");
        OCRelease(dictionary);
        if (title) OCRelease(title);
        if (description) OCRelease(description);
        OCRelease(jcampDatasetMetaData);
        return NULL;
    }
    string = NULL;
    key = STR("FIRSTY");
    double firstY = 0;
    (void)firstY; // Avoid unused variable warning
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
        firstY = creal(OCStringGetDoubleComplexValue(string));
    }
    string = NULL;
    key = STR(".OBSERVE FREQUENCY");
    double observeFrequency = 0;
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
        observeFrequency = creal(OCStringGetDoubleComplexValue(string));
    }
    string = NULL;
    key = STR(".OBSERVE NUCLEUS");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }
    string = NULL;
    key = STR(".ACQUISITION MODE");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }
    string = NULL;
    key = STR(".AVERAGES");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }

    key = STR("XYDATA");
    float data[2 * size];
    float originOffsetValue = firstX;
    bool sqz = false;
    (void)sqz; // Avoid unused variable warning
    bool dif = false;
    
    // Ensure xUnits is not NULL - use dimensionless as fallback
    if (xUnits == NULL) {
        xUnits = SIUnitDimensionlessAndUnderived();
    }
    if (inverseXUnits == NULL) {
        inverseXUnits = SIUnitDimensionlessAndUnderived();
    }
    
    // Ensure quantity names are not NULL - use dimensionless as fallback
    if (quantityName == NULL) {
        quantityName = kSIQuantityDimensionless;
    }
    if (inverseQuantityName == NULL) {
        inverseQuantityName = kSIQuantityDimensionless;
    }
    
    // Create JCAMP character mapping table for efficient single-pass processing
    static const char* jcamp_sqz_map[256] = {0};
    static const char* jcamp_dif_map[256] = {0};
    static bool maps_initialized = false;
    
    if (!maps_initialized) {
        // SQZ format mappings
        jcamp_sqz_map['@'] = " 0";   jcamp_sqz_map['A'] = " 1";   jcamp_sqz_map['B'] = " 2";
        jcamp_sqz_map['C'] = " 3";   jcamp_sqz_map['D'] = " 4";   jcamp_sqz_map['E'] = " 5";
        jcamp_sqz_map['F'] = " 6";   jcamp_sqz_map['G'] = " 7";   jcamp_sqz_map['H'] = " 8";
        jcamp_sqz_map['I'] = " 9";   jcamp_sqz_map['a'] = " -1";  jcamp_sqz_map['b'] = " -2";
        jcamp_sqz_map['c'] = " -3";  jcamp_sqz_map['d'] = " -4";  jcamp_sqz_map['e'] = " -5";
        jcamp_sqz_map['f'] = " -6";  jcamp_sqz_map['g'] = " -7";  jcamp_sqz_map['h'] = " -8";
        jcamp_sqz_map['i'] = " -9";
        
        // DIF format mappings  
        jcamp_dif_map['%'] = " 0";   jcamp_dif_map['J'] = " 1";   jcamp_dif_map['K'] = " 2";
        jcamp_dif_map['L'] = " 3";   jcamp_dif_map['M'] = " 4";   jcamp_dif_map['N'] = " 5";
        jcamp_dif_map['O'] = " 6";   jcamp_dif_map['P'] = " 7";   jcamp_dif_map['Q'] = " 8";
        jcamp_dif_map['R'] = " 9";   jcamp_dif_map['j'] = " -1";  jcamp_dif_map['k'] = " -2";
        jcamp_dif_map['l'] = " -3";  jcamp_dif_map['m'] = " -4";  jcamp_dif_map['n'] = " -5";
        jcamp_dif_map['o'] = " -6";  jcamp_dif_map['p'] = " -7";  jcamp_dif_map['q'] = " -8";
        jcamp_dif_map['r'] = " -9";
        
        maps_initialized = true;
    }
    
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        OCArrayRef splitLines = OCStringCreateArrayBySeparatingStrings(string, STR("\n"));
        OCMutableArrayRef dataLines = OCArrayCreateMutableCopy(splitLines);
        OCRelease(splitLines);
        OCIndex i = 0;
        for (OCIndex index = 1; index < OCArrayGetCount(dataLines); index++) {
            OCStringRef originalLine = OCArrayGetValueAtIndex(dataLines, index);
            
            // Convert OCString to C string for efficient processing
            OCIndex lineLength = OCStringGetLength(originalLine);
            if (lineLength == 0) continue;
            
            // Allocate buffer for C string conversion
            char* cStringBuffer = malloc(lineLength + 1);
            if (!cStringBuffer) continue;
            
            // Convert to C string
            const char* cString = OCStringGetCString(originalLine);
            if (!cString) {
                free(cStringBuffer);
                continue;
            }
            
            // Copy to our buffer
            strncpy(cStringBuffer, cString, lineLength);
            cStringBuffer[lineLength] = '\0';
            
            // Calculate maximum possible expanded size (worst case: every char becomes " -X")
            size_t maxExpandedLen = lineLength * 4 + 1; // Conservative estimate
            char* processedLine = malloc(maxExpandedLen);
            if (!processedLine) {
                free(cStringBuffer);
                continue;
            }
            
            char* dest = processedLine;
            
            // Single-pass character processing
            for (size_t pos = 0; pos < lineLength; pos++) {
                unsigned char c = cStringBuffer[pos];
                
                if (c == '+') {
                    *dest++ = ' ';
                } else if (c == '-') {
                    *dest++ = ' ';
                    *dest++ = '-';
                } else if (jcamp_sqz_map[c]) {
                    // SQZ format character
                    const char* replacement = jcamp_sqz_map[c];
                    while (*replacement) *dest++ = *replacement++;
                    sqz = true;
                } else if (jcamp_dif_map[c]) {
                    // DIF format character  
                    const char* replacement = jcamp_dif_map[c];
                    while (*replacement) *dest++ = *replacement++;
                    dif = true;
                } else {
                    // Regular character, copy as-is
                    *dest++ = c;
                }
            }
            *dest = '\0';
            
            free(cStringBuffer);
            
            // Create OCString from processed line and split into tokens
            OCStringRef processedLineStr = OCStringCreateWithCString(processedLine);
            free(processedLine);
            
            if (!processedLineStr) continue;
            
            OCMutableStringRef trimmedLine = OCStringCreateMutableCopy(processedLineStr);
            OCStringTrimWhitespace(trimmedLine);
            OCRelease(processedLineStr);
            
            OCArrayRef splitArray = OCStringCreateArrayBySeparatingStrings(trimmedLine, STR(" "));
            OCRelease(trimmedLine);
            
            if (!splitArray) continue;
            
            // Create mutable copy to remove empty items
            OCMutableArrayRef mutableSplitArray = OCArrayCreateMutableCopy(splitArray);
            OCRelease(splitArray);
            
            for (OCIndex jndex = OCArrayGetCount(mutableSplitArray) - 1; jndex >= 0; jndex--) {
                OCStringRef item = OCArrayGetValueAtIndex(mutableSplitArray, jndex);
                if (OCStringGetLength(item) == 0) OCArrayRemoveValueAtIndex(mutableSplitArray, jndex);
            }
            for (OCIndex jndex = 0; jndex < OCArrayGetCount(mutableSplitArray); jndex++) {
                OCMutableStringRef stringNumber = OCStringCreateMutableCopy(OCArrayGetValueAtIndex(mutableSplitArray, jndex));
                int dup = 0;
                if (OCStringFindAndReplace(stringNumber, STR("S"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 1;
                if (OCStringFindAndReplace(stringNumber, STR("T"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 2;
                if (OCStringFindAndReplace(stringNumber, STR("U"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 3;
                if (OCStringFindAndReplace(stringNumber, STR("V"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 4;
                if (OCStringFindAndReplace(stringNumber, STR("W"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 5;
                if (OCStringFindAndReplace(stringNumber, STR("X"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 6;
                if (OCStringFindAndReplace(stringNumber, STR("Y"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 7;
                if (OCStringFindAndReplace(stringNumber, STR("Z"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 8;
                if (OCStringFindAndReplace(stringNumber, STR("s"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 9;
                if (jndex > 0) {
                    data[i] = creal(OCStringGetDoubleComplexValue(stringNumber));
                    if (dif && jndex > 1) data[i] += data[i - 1];
                    for (OCIndex kndex = 0; kndex < dup; kndex++) {
                        i++;
                        data[i] = data[i - 1];
                    }
                    i++;
                }
                OCRelease(stringNumber);
            }
            OCRelease(mutableSplitArray);
        }
        OCRelease(dataLines);
    }

    OCDataRef values = OCDataCreate((const uint8_t *)data, sizeof(float) * size);
    double sampleInc = (lastX - firstX) / (size - 1);
    bool reverse = false;
    (void)reverse; // Avoid unused variable warning
    if (sampleInc < 0) reverse = true;
    SIScalarRef increment = SIScalarCreateWithDouble(fabs(sampleInc), xUnits);
    SIScalarRef originOffset = SIScalarCreateWithDouble(originOffsetValue, xUnits);
    if (quantityName && OCStringCompare(quantityName, kSIQuantityFrequency, 0) == kOCCompareEqualTo) {
        OCRelease(originOffset);
        SIUnitRef megahertz = SIUnitFindWithUnderivedSymbol(STR("MHz"));
        originOffset = SIScalarCreateWithDouble(observeFrequency, megahertz);
    } else {
        OCRelease(originOffset);
        originOffset = SIScalarCreateWithDouble(observeFrequency, xUnits);
    }
    SIScalarRef referenceOffset = SIScalarCreateWithDouble(0.0, xUnits);
    SIScalarRef inverseOriginOffset = SIScalarCreateWithDouble(0.0, inverseXUnits);
    if (inverseQuantityName && OCStringCompare(inverseQuantityName, kSIQuantityFrequency, 0) == kOCCompareEqualTo) {
        OCRelease(inverseOriginOffset);
        SIUnitRef megahertz = SIUnitFindWithUnderivedSymbol(STR("MHz"));
        inverseOriginOffset = SIScalarCreateWithDouble(observeFrequency, megahertz);
    } else {
        OCRelease(inverseOriginOffset);
        inverseOriginOffset = SIScalarCreateWithDouble(observeFrequency, inverseXUnits);
    }
    SIScalarRef reciprocalReferenceOffset = SIScalarCreateWithDouble(0.0, inverseXUnits);
    OCMutableArrayRef dimensions = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    SIDimensionRef reciprocal = SIDimensionCreateWithQuantity(inverseQuantityName, error);
    if (reciprocal && inverseOriginOffset) {
        SIDimensionSetOriginOffset(reciprocal, inverseOriginOffset, error);
    }
    
    SILinearDimensionRef dim = SILinearDimensionCreateMinimal(quantityName,size,increment,reciprocal,error);
    if (dim && originOffset) {
        SIDimensionSetOriginOffset((SIDimensionRef) dim, originOffset, error);
    }
    
    // Only set coordinates offset if both dim and referenceOffset are valid
    if (dim && referenceOffset) {
        SIDimensionSetCoordinatesOffset((SIDimensionRef) dim, referenceOffset, error);
    }
    
    if (nmrSpectrumType && dim) NMRDimensionSetDimensionless((DimensionRef) dim);

    OCRelease(increment);
    OCRelease(originOffset);
    OCRelease(referenceOffset);
    OCRelease(inverseOriginOffset);
    OCRelease(reciprocalReferenceOffset);
    if (reciprocal) OCRelease(reciprocal);
    if (NULL == dim) {
        OCRelease(values);
        OCRelease(dictionary);
        if (title) OCRelease(title);
        if (description) OCRelease(description);
        OCRelease(jcampDatasetMetaData);
        return NULL;
    }
    // SIDimensionMakeNiceUnits(dim);
    OCArrayAppendValue(dimensions, dim);
    OCRelease(dim);

    DatasetRef theDataset = DatasetCreateEmpty(error);
    DatasetSetDimensions(theDataset, dimensions);
    OCRelease(dimensions);
    DependentVariableRef theDependentVariable = DatasetAddEmptyDependentVariable(theDataset, STR("scalar"), kOCNumberFloat32Type, -1);
    DependentVariableSetComponentAtIndex(theDependentVariable, values, 0);
    OCRelease(values);
    DependentVariableMultiplyValuesByDimensionlessRealConstant(theDependentVariable, -1, yFactor);

    OCStringRef yUnits = OCDictionaryGetValue(jcampDatasetMetaData, STR("YUNITS"));
    if (yUnits) {
        if (OCStringCompare(yUnits, STR("pH"), 0) == kOCCompareEqualTo) {
            DependentVariableSetQuantityName(theDependentVariable, kSIQuantityDimensionless);
            DependentVariableSetComponentLabelAtIndex(theDependentVariable, STR("pH"), 0);
        }
        if (OCStringCompare(yUnits, STR("TRANSMITTANCE"), 0) == kOCCompareEqualTo) {
            DependentVariableSetQuantityName(theDependentVariable, kSIQuantityDimensionless);
            DependentVariableSetComponentLabelAtIndex(theDependentVariable, STR("Transmittance"), 0);
        }
        if (OCStringCompare(yUnits, STR("ABSORBANCE"), 0) == kOCCompareEqualTo) {
            DependentVariableSetQuantityName(theDependentVariable, kSIQuantityDimensionless);
            DependentVariableSetComponentLabelAtIndex(theDependentVariable, STR("Absorbance"), 0);
        }
    }
    if (eprSpectrumType) {
        DependentVariableSetComponentLabelAtIndex(theDependentVariable, STR("Derivative Intensity"), 0);
    }
    string = NULL;
    key = STR("TEMPERATURE");
    SIUnitRef unit = SIUnitFromExpression(STR("°C"), NULL, error);
    quantityName = kSIQuantityTemperature;
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) {
            double value = creal(OCStringGetDoubleComplexValue(string));
            SIScalarRef scalar = SIScalarCreateWithDouble(value, unit);
            OCDictionaryAddValue(jcampDatasetMetaData, key, scalar);
            OCRelease(scalar);
        }
    }
    string = NULL;
    key = STR("PRESSURE");
    unit = SIUnitFromExpression(STR("atm"), NULL, error);
    quantityName = kSIQuantityPressure;
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) {
            double value = creal(OCStringGetDoubleComplexValue(string));
            SIScalarRef scalar = SIScalarCreateWithDouble(value, unit);
            OCDictionaryAddValue(jcampDatasetMetaData, key, scalar);
            OCRelease(scalar);
        }
    }
    string = NULL;
    key = STR("REFRACTIVE INDEX");
    unit = SIUnitFromExpression(STR("m•s/(m•s)"), NULL, error);
    quantityName = kSIQuantityRefractiveIndex;
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) {
            double value = creal(OCStringGetDoubleComplexValue(string));
            SIScalarRef scalar = SIScalarCreateWithDouble(value, unit);
            OCDictionaryAddValue(jcampDatasetMetaData, key, scalar);
            OCRelease(scalar);
        }
    }
    string = NULL;
    key = STR("DENSITY");
    unit = SIUnitFromExpression(STR("g/cm^3"), NULL, error);
    quantityName = kSIQuantityDensity;
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) {
            double value = creal(OCStringGetDoubleComplexValue(string));
            SIScalarRef scalar = SIScalarCreateWithDouble(value, unit);
            OCDictionaryAddValue(jcampDatasetMetaData, key, scalar);
            OCRelease(scalar);
        }
    }
    string = NULL;
    key = STR("MW");
    unit = SIUnitFromExpression(STR("g/mol"), NULL, error);
    quantityName = kSIQuantityMolarMass;
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) {
            double value = creal(OCStringGetDoubleComplexValue(string));
            SIScalarRef scalar = SIScalarCreateWithDouble(value, unit);
            OCDictionaryAddValue(jcampDatasetMetaData, key, scalar);
            OCRelease(scalar);
        }
    }
    DatasetSetMetaData(theDataset, jcampDatasetMetaData);
    DatasetSetDescription(theDataset, description);
    DatasetSetTitle(theDataset, title);
    OCRelease(jcampDatasetMetaData);
    
    // Release retained objects
    if (title) OCRelease(title);
    if (description) OCRelease(description);
    
    // Release the dictionary
    OCRelease(dictionary);
    
    return theDataset;
}
