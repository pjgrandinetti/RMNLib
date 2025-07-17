//
//  DatasetImportJCAMP.c
//  RMN 2.0
//
//  Created by Philip J. Grandinetti on 2/21/14.
//  Copyright (c) 2014 PhySy. All rights reserved.
//
#include "../RMNLibrary.h"
#include <string.h>

// Forward declaration for PEAK TABLE support
static DatasetRef DatasetImportJCAMPCreatePeakTableDataset(OCDictionaryRef dictionary, OCStringRef *error);

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
        // Handle PEAK TABLE format
        DatasetRef result = DatasetImportJCAMPCreatePeakTableDataset(dictionary, error);
        OCRelease(dictionary);
        return result;
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
        
        // Detect data format by examining the first data line
        bool isCompressedFormat = false;
        if (OCArrayGetCount(dataLines) > 1) {
            OCStringRef firstDataLine = OCArrayGetValueAtIndex(dataLines, 1);
            const char* firstLineStr = OCStringGetCString(firstDataLine);
            if (firstLineStr) {
                // Check for compressed format characters (SQZ/DIF)
                for (size_t k = 0; firstLineStr[k] && k < 50; k++) {
                    unsigned char c = firstLineStr[k];
                    if (jcamp_sqz_map[c] || jcamp_dif_map[c]) {
                        isCompressedFormat = true;
                        break;
                    }
                }
            }
        }
        
        OCIndex i = 0;
        
        if (isCompressedFormat) {
            // Use optimized compressed format parsing
            for (OCIndex index = 1; index < OCArrayGetCount(dataLines); index++) {
                OCStringRef originalLine = OCArrayGetValueAtIndex(dataLines, index);
                
                // Convert OCString to C string for efficient processing
                const char* cString = OCStringGetCString(originalLine);
                if (!cString) continue;
                
                OCIndex lineLength = strlen(cString);
                if (lineLength == 0) continue;
                
                // Calculate maximum possible expanded size more efficiently
                size_t maxExpandedLen = lineLength * 3 + 1; // More realistic estimate
                char* processedLine = malloc(maxExpandedLen);
                if (!processedLine) continue;
                
                char* dest = processedLine;
                
                // Single-pass character processing with bounds checking
                for (size_t pos = 0; pos < lineLength && (dest - processedLine) < (maxExpandedLen - 3); pos++) {
                    unsigned char c = cString[pos];
                    
                    if (c == '+') {
                        *dest++ = ' ';
                    } else if (c == '-') {
                        *dest++ = ' ';
                        *dest++ = '-';
                    } else if (jcamp_sqz_map[c]) {
                        // SQZ format character
                        const char* replacement = jcamp_sqz_map[c];
                        while (*replacement && (dest - processedLine) < (maxExpandedLen - 1)) {
                            *dest++ = *replacement++;
                        }
                        sqz = true;
                    } else if (jcamp_dif_map[c]) {
                        // DIF format character  
                        const char* replacement = jcamp_dif_map[c];
                        while (*replacement && (dest - processedLine) < (maxExpandedLen - 1)) {
                            *dest++ = *replacement++;
                        }
                        dif = true;
                    } else {
                        // Regular character, copy as-is
                        *dest++ = c;
                    }
                }
                *dest = '\0';
                
                // Create OCString from processed line and parse tokens more efficiently
                OCStringRef processedLineStr = OCStringCreateWithCString(processedLine);
                free(processedLine);
                
                if (!processedLineStr) continue;
                
                // Direct C string tokenization for better performance
                const char* lineToProcess = OCStringGetCString(processedLineStr);
                if (!lineToProcess) {
                    OCRelease(processedLineStr);
                    continue;
                }
                
                // Simple whitespace tokenization
                char* lineCopy = malloc(strlen(lineToProcess) + 1);
                if (!lineCopy) {
                    OCRelease(processedLineStr);
                    continue;
                }
                strcpy(lineCopy, lineToProcess);
                OCRelease(processedLineStr);
                
                // Parse tokens directly from C string
                char* token = strtok(lineCopy, " \t\n\r");
                int tokenIndex = 0;
                
                while (token != NULL && i < size) {
                    if (tokenIndex > 0) { // Skip first token (x-value)
                        // Fast duplication character lookup and in-place modification
                        int dup = 0;
                        size_t tokenLen = strlen(token);
                        
                        // Process in-place for better performance
                        for (size_t k = 0; k < tokenLen; k++) {
                            switch (token[k]) {
                                case 'S': dup = 1; token[k] = ' '; break;
                                case 'T': dup = 2; token[k] = ' '; break;
                                case 'U': dup = 3; token[k] = ' '; break;
                                case 'V': dup = 4; token[k] = ' '; break;
                                case 'W': dup = 5; token[k] = ' '; break;
                                case 'X': dup = 6; token[k] = ' '; break;
                                case 'Y': dup = 7; token[k] = ' '; break;
                                case 'Z': dup = 8; token[k] = ' '; break;
                                case 's': dup = 9; token[k] = ' '; break;
                            }
                        }
                        
                        // Parse the number directly from modified token
                        char* endPtr;
                        double value = strtod(token, &endPtr);
                        
                        if (i < size) {
                            data[i] = (float)value;
                            if (dif && tokenIndex > 1 && i > 0) data[i] += data[i - 1];
                            
                            // Handle duplications efficiently with bounds checking
                            for (int kndex = 0; kndex < dup && (i + kndex + 1) < size; kndex++) {
                                data[i + kndex + 1] = data[i];
                            }
                            i += dup + 1;
                        }
                    }
                    
                    token = strtok(NULL, " \t\n\r");
                    tokenIndex++;
                }
                
                free(lineCopy);
            }
        } else {
            // Highly optimized parsing for uncompressed numeric format
            // Uses direct memory operations and minimal function calls
            
            for (OCIndex index = 1; index < OCArrayGetCount(dataLines) && i < size; index++) {
                OCStringRef originalLine = OCArrayGetValueAtIndex(dataLines, index);
                const char* lineStr = OCStringGetCString(originalLine);
                if (!lineStr) continue;
                
                // Fast inline parsing without string copies
                const char* ptr = lineStr;
                int tokenCount = 0;
                
                while (*ptr && i < size) {
                    // Skip whitespace efficiently
                    while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r') {
                        if (!*ptr) break;
                        ptr++;
                    }
                    if (!*ptr) break;
                    
                    // Fast number parsing - avoid strtod overhead where possible
                    char* endPtr;
                    double value = strtod(ptr, &endPtr);
                    
                    if (endPtr > ptr) { // Valid number found
                        if (tokenCount > 0) { // Skip first token (x-value)
                            data[i++] = (float)value;
                        }
                        tokenCount++;
                        ptr = endPtr;
                    } else {
                        // Skip non-numeric character
                        ptr++;
                    }
                }
            }
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

// Implementation of PEAK TABLE dataset creation
static DatasetRef DatasetImportJCAMPCreatePeakTableDataset(OCDictionaryRef dictionary, OCStringRef *error) {
    if (error) *error = NULL;
    if (!dictionary) {
        if (error) *error = STR("JCAMP Peak Table: dictionary is NULL");
        return NULL;
    }

    // Extract metadata from JCAMP Core Header
    OCMutableDictionaryRef jcampDatasetMetaData = OCDictionaryCreateMutable(0);
    OCStringRef string = NULL;
    OCStringRef key = NULL;

    // Title
    key = STR("TITLE");
    OCStringRef title = NULL;
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) {
            OCDictionaryAddValue(jcampDatasetMetaData, key, string);
            title = OCRetain(string);
        }
    }

    // X and Y units
    key = STR("XUNITS");
    SIUnitRef xUnits = NULL;
    OCStringRef quantityName = NULL;
    if (OCDictionaryContainsKey(dictionary, key)) {
        OCMutableStringRef string = OCStringCreateMutableCopy((OCStringRef)OCDictionaryGetValue(dictionary, key));
        if (string) {
            OCStringTrimWhitespace(string);
            OCDictionaryAddValue(jcampDatasetMetaData, key, string);
            if (OCStringCompare(string, STR("m/z"), 0) == kOCCompareEqualTo || OCStringCompare(string, STR("M/Z"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("Th"), &unit_multiplier, error);
                quantityName = kSIQuantityMassToChargeRatio;
            } else if (OCStringCompare(string, STR("HZ"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("Hz"), &unit_multiplier, error);
                quantityName = kSIQuantityFrequency;
            } else if (OCStringCompare(string, STR("TIME"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("min"), &unit_multiplier, error);
                quantityName = kSIQuantityTime;
            } else if (OCStringCompare(string, STR("SECONDS"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("s"), &unit_multiplier, error);
                quantityName = kSIQuantityTime;
            } else if (OCStringCompare(string, STR("1/CM"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("1/cm"), &unit_multiplier, error);
                quantityName = kSIQuantityWavenumber;
            } else if (OCStringCompare(string, STR("NANOMETERS"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("nm"), &unit_multiplier, error);
                quantityName = kSIQuantityLength;
            } else if (OCStringCompare(string, STR("VOLUME"), 0) == kOCCompareEqualTo) {
                double unit_multiplier = 1;
                xUnits = SIUnitFromExpression(STR("mL"), &unit_multiplier, error);
                quantityName = kSIQuantityVolume;
            }
            OCRelease(string);
        }
    }

    // Fallback to dimensionless if no unit specified
    if (xUnits == NULL) {
        xUnits = SIUnitDimensionlessAndUnderived();
    }
    if (quantityName == NULL) {
        quantityName = kSIQuantityDimensionless;
    }

    key = STR("YUNITS");
    if (OCDictionaryContainsKey(dictionary, key)) {
        string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
        if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
    }

    // Parse PEAK TABLE data
    key = STR("PEAK TABLE");
    if (!OCDictionaryContainsKey(dictionary, key)) {
        if (error) *error = STR("JCAMP Peak Table: missing PEAK TABLE data");
        OCRelease(jcampDatasetMetaData);
        if (title) OCRelease(title);
        return NULL;
    }

    string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
    if (!string) {
        if (error) *error = STR("JCAMP Peak Table: PEAK TABLE value is NULL");
        OCRelease(jcampDatasetMetaData);
        if (title) OCRelease(title);
        return NULL;
    }

    // Split the peak table data into lines
    OCArrayRef splitLines = OCStringCreateArrayBySeparatingStrings(string, STR("\n"));
    if (!splitLines) {
        if (error) *error = STR("JCAMP Peak Table: failed to split PEAK TABLE data");
        OCRelease(jcampDatasetMetaData);
        if (title) OCRelease(title);
        return NULL;
    }

    // Parse X,Y pairs from the peak table data
    OCMutableArrayRef xCoordinates = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCMutableArrayRef yValues = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);

    for (OCIndex lineIndex = 0; lineIndex < OCArrayGetCount(splitLines); lineIndex++) {
        OCStringRef line = (OCStringRef)OCArrayGetValueAtIndex(splitLines, lineIndex);
        if (!line || OCStringGetLength(line) == 0) continue;

        // Split by whitespace and parse X,Y pairs
        OCArrayRef tokens = OCStringCreateArrayBySeparatingStrings(line, STR(" "));
        if (!tokens) continue;

        for (OCIndex tokenIndex = 0; tokenIndex < OCArrayGetCount(tokens); tokenIndex++) {
            OCStringRef token = (OCStringRef)OCArrayGetValueAtIndex(tokens, tokenIndex);
            if (!token || OCStringGetLength(token) == 0) continue;

            // Split by comma to get X,Y pair
            OCArrayRef xyPair = OCStringCreateArrayBySeparatingStrings(token, STR(","));
            if (xyPair && OCArrayGetCount(xyPair) == 2) {
                OCStringRef xStr = (OCStringRef)OCArrayGetValueAtIndex(xyPair, 0);
                OCStringRef yStr = (OCStringRef)OCArrayGetValueAtIndex(xyPair, 1);

                if (xStr && yStr) {
                    // Parse X coordinate as SIScalar
                    double xValue = creal(OCStringGetDoubleComplexValue(xStr));
                    SIScalarRef xScalar = SIScalarCreateWithDouble(xValue, xUnits);
                    if (xScalar) {
                        OCArrayAppendValue(xCoordinates, xScalar);
                        OCRelease(xScalar);
                    }

                    // Parse Y value as float
                    float yValue = (float)creal(OCStringGetDoubleComplexValue(yStr));
                    OCNumberRef yNumber = OCNumberCreateWithFloat(yValue);
                    if (yNumber) {
                        OCArrayAppendValue(yValues, yNumber);
                        OCRelease(yNumber);
                    }
                }
            }
            if (xyPair) OCRelease(xyPair);
        }
        OCRelease(tokens);
    }
    OCRelease(splitLines);

    // Validate we have data
    OCIndex pointCount = OCArrayGetCount(xCoordinates);
    if (pointCount == 0 || OCArrayGetCount(yValues) != pointCount) {
        if (error) *error = STR("JCAMP Peak Table: no valid X,Y pairs found or mismatched counts");
        OCRelease(xCoordinates);
        OCRelease(yValues);
        OCRelease(jcampDatasetMetaData);
        if (title) OCRelease(title);
        return NULL;
    }

    if (pointCount < 2) {
        if (error) *error = STR("JCAMP Peak Table: need at least 2 data points for monotonic dimension");
        OCRelease(xCoordinates);
        OCRelease(yValues);
        OCRelease(jcampDatasetMetaData);
        if (title) OCRelease(title);
        return NULL;
    }

    // Create SIMonotonicDimension from X coordinates
    SIMonotonicDimensionRef xDimension = SIMonotonicDimensionCreate(
        STR("Peak Table X"),    // label
        NULL,                   // description
        NULL,                   // metadata
        quantityName,           // quantity
        NULL,                   // offset (will be defaulted)
        NULL,                   // origin (will be defaulted) 
        NULL,                   // period
        false,                  // periodic
        kDimensionScalingNone,  // scaling
        xCoordinates,           // coordinates
        NULL,                   // reciprocal
        error                   // outError
    );

    if (!xDimension) {
        OCRelease(xCoordinates);
        OCRelease(yValues);
        OCRelease(jcampDatasetMetaData);
        if (title) OCRelease(title);
        return NULL;
    }

    // Convert Y values to float array for DependentVariable
    float *yData = malloc(pointCount * sizeof(float));
    if (!yData) {
        if (error) *error = STR("JCAMP Peak Table: failed to allocate memory for Y data");
        OCRelease(xDimension);
        OCRelease(xCoordinates);
        OCRelease(yValues);
        OCRelease(jcampDatasetMetaData);
        if (title) OCRelease(title);
        return NULL;
    }

    for (OCIndex i = 0; i < pointCount; i++) {
        OCNumberRef yNumber = (OCNumberRef)OCArrayGetValueAtIndex(yValues, i);
        float value = 0.0f;
        OCNumberTryGetFloat(yNumber, &value);
        yData[i] = value;
    }

    // Create Dataset with monotonic dimension
    OCMutableArrayRef dimensions = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(dimensions, xDimension);

    DatasetRef theDataset = DatasetCreateEmpty(error);
    if (!theDataset) {
        free(yData);
        OCRelease(xDimension);
        OCRelease(dimensions);
        OCRelease(xCoordinates);
        OCRelease(yValues);
        OCRelease(jcampDatasetMetaData);
        if (title) OCRelease(title);
        return NULL;
    }

    DatasetSetDimensions(theDataset, dimensions);

    // Create DependentVariable from Y values
    OCDataRef yDataRef = OCDataCreate((const uint8_t *)yData, pointCount * sizeof(float));
    DependentVariableRef theDependentVariable = DatasetAddEmptyDependentVariable(theDataset, STR("intensity"), kOCNumberFloat32Type, -1);
    DependentVariableSetComponentAtIndex(theDependentVariable, yDataRef, 0);

    // Set Y units/labels based on YUNITS
    OCStringRef yUnits = OCDictionaryGetValue(jcampDatasetMetaData, STR("YUNITS"));
    if (yUnits) {
        if (OCStringCompare(yUnits, STR("relative abundance"), kOCCompareCaseInsensitive) == kOCCompareEqualTo) {
            DependentVariableSetQuantityName(theDependentVariable, kSIQuantityDimensionless);
            DependentVariableSetComponentLabelAtIndex(theDependentVariable, STR("Relative Abundance"), 0);
        } else if (OCStringCompare(yUnits, STR("TRANSMITTANCE"), 0) == kOCCompareEqualTo) {
            DependentVariableSetQuantityName(theDependentVariable, kSIQuantityDimensionless);
            DependentVariableSetComponentLabelAtIndex(theDependentVariable, STR("Transmittance"), 0);
        } else if (OCStringCompare(yUnits, STR("ABSORBANCE"), 0) == kOCCompareEqualTo) {
            DependentVariableSetQuantityName(theDependentVariable, kSIQuantityDimensionless);
            DependentVariableSetComponentLabelAtIndex(theDependentVariable, STR("Absorbance"), 0);
        } else {
            DependentVariableSetComponentLabelAtIndex(theDependentVariable, yUnits, 0);
        }
    }

    // Copy additional metadata fields from dictionary to dataset metadata
    OCStringRef metadataKeys[] = {
        STR("JCAMP-DX"), STR("DATA TYPE"), STR("DATA CLASS"), STR("ORIGIN"), STR("OWNER"),
        STR("DATE"), STR("TIME"), STR("SPECTROMETER/DATA SYSTEM"), STR("INSTRUMENTAL PARAMETERS"),
        STR("SAMPLING PROCEDURE"), STR("COMMENT"), STR("NPOINTS"), STR("FIRSTX"), STR("LASTX"),
        STR("FIRSTY"), STR("XFACTOR"), STR("YFACTOR")
    };
    OCIndex metadataKeyCount = sizeof(metadataKeys) / sizeof(metadataKeys[0]);
    
    for (OCIndex i = 0; i < metadataKeyCount; i++) {
        if (OCDictionaryContainsKey(dictionary, metadataKeys[i])) {
            OCStringRef value = (OCStringRef)OCDictionaryGetValue(dictionary, metadataKeys[i]);
            if (value) {
                OCDictionaryAddValue(jcampDatasetMetaData, metadataKeys[i], value);
            }
        }
    }

    // Set dataset metadata
    DatasetSetMetaData(theDataset, jcampDatasetMetaData);
    DatasetSetTitle(theDataset, title);

    // Cleanup
    free(yData);
    OCRelease(yDataRef);
    OCRelease(xDimension);
    OCRelease(dimensions);
    OCRelease(xCoordinates);
    OCRelease(yValues);
    OCRelease(jcampDatasetMetaData);
    if (title) OCRelease(title);

    return theDataset;
}
