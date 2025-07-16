// //
// //  DatasetImportJCAMP.c
// //  RMN 2.0
// //
// //  Created by Philip J. Grandinetti on 2/21/14.
// //  Copyright (c) 2014 PhySy. All rights reserved.
// //
// #include "RMNLibrary.h"
// OCDictionaryRef DatasetImportJCAMPCreateDictionaryWithLines(OCArrayRef lines, OCIndex *index) {
//     // Make sure first line is TITLE or DTYPx
//     OCStringRef line = (OCStringRef)OCArrayGetValueAtIndex(lines, 0);
//     bool dtypx = false;
//     if (OCStringCompare(line, STR("DTYPx"), 0) == kOCCompareEqualTo) {
//         line = (OCStringRef)OCArrayGetValueAtIndex(lines, 1);
//         *index = 1;
//         dtypx = true;
//     }
//     OCArrayRef array = OCStringCreateArrayBySeparatingStrings(line, STR("="));
//     OCStringRef key0 = OCArrayGetValueAtIndex(array, 0);
//     if (OCStringCompare(key0, STR("TITLE"), 0) != kOCCompareEqualTo) {
//         OCRelease(array);
//         return NULL;
//     }
//     // Valid first line, let's continue processing title and making first entry in dictionary.
//     // Create dictionary with each entrying holding labeled-data-records or blocks
//     OCMutableDictionaryRef dictionary = OCDictionaryCreateMutable(0);
//     OCDictionaryAddValue(dictionary, key0, OCArrayGetValueAtIndex(array, 1));
//     OCRelease(array);
//     if (dtypx) OCDictionaryAddValue(dictionary, STR("DTYPx"), kOCBooleanTrue);
//     OCIndex start = *index + 1;
//     for (*index = start; *index < OCArrayGetCount(lines); (*index)++) {
//         OCStringRef line = OCArrayGetValueAtIndex(lines, (*index));
//         OCArrayRef array = OCStringCreateArrayBySeparatingStrings(line, STR("="));
//         if (OCArrayGetCount(array) == 2) {
//             OCStringRef key = OCArrayGetValueAtIndex(array, 0);
//             if (OCStringGetLength(key)) {
//                 if (OCStringCompare(key, STR("TITLE"), kOCCompareCaseInsensitive) == kOCCompareEqualTo) {
//                     key = STR("BLOCK_ARRAY");
//                     OCMutableArrayRef blockArray = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
//                     OCDictionaryRef blockDictionary = DatasetImportJCAMPCreateDictionaryWithLines(lines, index);
//                     OCArrayAppendValue(blockArray, blockDictionary);
//                     OCRelease(blockDictionary);
//                 } else if (OCStringCompare(key, STR("END"), kOCCompareCaseInsensitive) == kOCCompareEqualTo) {
//                     OCRelease(array);
//                     return dictionary;
//                 } else {
//                     OCStringRef value = OCArrayGetValueAtIndex(array, 1);
//                     OCMutableStringRef mutValue = OCStringCreateMutableCopy(value);
//                     OCStringTrimWhitespace(mutValue);
//                     OCDictionaryAddValue(dictionary, key, mutValue);
//                     OCRelease(mutValue);
//                 }
//             }
//         }
//         OCRelease(array);
//     }
//     return dictionary;
// }
// DatasetRef DatasetImportJCAMPCreateSignalWithData(OCDataRef contents, OCStringRef *error) {
//     if (error)
//         if (*error) return NULL;
//     OCStringRef temp = OCStringCreateWithExternalRepresentation(contents);
//     OCMutableStringRef fileString = OCStringCreateMutableCopy(temp);
//     OCRelease(temp);
//     OCArrayRef array = OCStringCreateArrayBySeparatingStrings(fileString, STR("##"));
//     OCRelease(fileString);
//     OCMutableArrayRef lines = OCArrayCreateMutableCopy(array);
//     if (NULL == lines) return NULL;
//     if (OCArrayGetCount(lines) < 1) {
//         OCRelease(lines);
//         return NULL;
//     }
//     for (OCIndex index = 0; index < OCArrayGetCount(lines); index++) {
//         OCMutableStringRef line = OCStringCreateMutableCopy(OCArrayGetValueAtIndex(lines, index));
//         OCRange range = OCStringFind(line, STR("$$"), 0);
//         if (range.location != kOCNotFound) OCStringDelete(line, OCRangeMake(range.location, OCStringGetLength(line) - range.location));
//         OCStringFindAndReplace(line, STR("\r"), STR("\n"), OCRangeMake(0, OCStringGetLength(line)), 0);
//         OCStringTrim(line, STR("\n"));
//         OCStringTrimWhitespace(line);
//         OCArraySetValueAtIndex(lines, index, line);
//         OCRelease(line);
//     }
//     for (OCIndex index = OCArrayGetCount(lines) - 1; index >= 0; index--) {
//         OCStringRef string = OCArrayGetValueAtIndex(lines, index);
//         if (OCStringGetLength(string) == 0) OCArrayRemoveValueAtIndex(lines, index);
//     }
//     OCIndex index = 0;
//     OCDictionaryRef dictionary = DatasetImportJCAMPCreateDictionaryWithLines(lines, &index);
//     OCRelease(lines);
//     if (NULL == dictionary) return NULL;
//     OCStringRef key = STR("PEAK TABLE");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         if (error) {
//             *error = STR("JCAMP Peak Table file is unsupported.");
//         }
//         OCRelease(dictionary);
//         return NULL;
//     }
//     // Read in JCAMP Core Header
//     OCMutableDictionaryRef jcampDatasetMetaData = OCDictionaryCreateMutable(0);
//     OCStringRef string = NULL;
//     key = STR("TITLE");
//     OCStringRef title = NULL;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) {
//             OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//             title = OCRetain(string);
//         }
//     }
//     string = NULL;
//     key = STR("JCAMP-DX");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//     }
//     string = NULL;
//     key = STR("DATA CLASS");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//     }
//     string = NULL;
//     bool nmrSpectrumType = false;
//     bool irSpectrumType = false;
//     bool eprSpectrumType = false;
//     key = STR("DATA TYPE");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) {
//             OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//             if (OCStringCompare(string, STR("NMR SPECTRUM"), 0) == kOCCompareEqualTo) nmrSpectrumType = true;
//             if (OCStringCompare(string, STR("INFRARED SPECTRUM"), 0) == kOCCompareEqualTo) irSpectrumType = true;
//             if (OCStringCompare(string, STR("EPR SPECTRUM"), 0) == kOCCompareEqualTo) eprSpectrumType = true;
//         }
//     }
//     string = NULL;
//     key = STR("ORIGIN");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//     }
//     string = NULL;
//     key = STR("OWNER");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//     }
//     string = NULL;
//     key = STR("BLOCKS");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//     }
//     string = NULL;
//     key = STR("DATE");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//     }
//     string = NULL;
//     key = STR("TIME");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//     }
//     string = NULL;
//     key = STR("SPECTROMETER/DATA SYSTEM");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//     }
//     string = NULL;
//     key = STR("INSTRUMENT PARAMETERS");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//     }
//     string = NULL;
//     key = STR("SAMPLING PROCEDURE");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//     }
//     string = NULL;
//     key = STR("XUNITS");
//     SIUnitRef xUnits = NULL;
//     SIUnitRef inverseXUnits = NULL;
//     OCStringRef quantityName = NULL;
//     OCStringRef inverseQuantityName = NULL;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         OCMutableStringRef string = OCStringCreateMutableCopy((OCStringRef)OCDictionaryGetValue(dictionary, key));
//         if (string) {
//             OCStringTrimWhitespace(string);
//             OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//             if (OCStringCompare(string, STR("1/CM"), 0) == kOCCompareEqualTo) {
//                 double unit_multiplier = 1;
//                 xUnits = SIUnitFromExpression(STR("1/cm"), &unit_multiplier, error);
//                 inverseXUnits = SIUnitFromExpression(STR("cm"), &unit_multiplier, error);
//                 quantityName = kSIQuantityWavenumber;
//                 inverseQuantityName = kSIQuantityLength;
//             }
//             if (OCStringCompare(string, STR("VOLUME"), 0) == kOCCompareEqualTo) {
//                 double unit_multiplier = 1;
//                 xUnits = SIUnitFromExpression(STR("mL"), &unit_multiplier, error);
//                 inverseXUnits = SIUnitFromExpression(STR("1/mL"), &unit_multiplier, error);
//                 quantityName = kSIQuantityVolume;
//                 inverseQuantityName = kSIQuantityInverseVolume;
//             } else if (OCStringCompare(string, STR("m/z"), 0) == kOCCompareEqualTo || OCStringCompare(string, STR("M/Z"), 0) == kOCCompareEqualTo) {
//                 double unit_multiplier = 1;
//                 xUnits = SIUnitFromExpression(STR("Th"), &unit_multiplier, error);
//                 inverseXUnits = SIUnitFromExpression(STR("(1/Th)"), &unit_multiplier, error);
//                 quantityName = kSIQuantityMassToChargeRatio;
//                 inverseQuantityName = kSIQuantityChargeToMassRatio;
//             } else if (OCStringCompare(string, STR("NANOMETERS"), 0) == kOCCompareEqualTo) {
//                 double unit_multiplier = 1;
//                 xUnits = SIUnitFromExpression(STR("nm"), &unit_multiplier, error);
//                 inverseXUnits = SIUnitFromExpression(STR("1/nm"), &unit_multiplier, error);
//                 quantityName = kSIQuantityLength;
//                 inverseQuantityName = kSIQuantityWavenumber;
//             } else if (OCStringCompare(string, STR("GAUSS"), 0) == kOCCompareEqualTo) {
//                 double unit_multiplier = 1;
//                 xUnits = SIUnitFromExpression(STR("G"), &unit_multiplier, error);
//                 inverseXUnits = SIUnitFromExpression(STR("1/G"), &unit_multiplier, error);
//                 quantityName = kSIQuantityMagneticFluxDensity;
//                 inverseQuantityName = kSIQuantityInverseMagneticFluxDensity;
//             } else if (OCStringCompare(string, STR("HZ"), 0) == kOCCompareEqualTo) {
//                 double unit_multiplier = 1;
//                 xUnits = SIUnitFromExpression(STR("Hz"), &unit_multiplier, error);
//                 inverseXUnits = SIUnitFindWithUnderivedSymbol(STR("s"));
//                 quantityName = kSIQuantityFrequency;
//                 inverseQuantityName = kSIQuantityTime;
//             } else if (OCStringCompare(string, STR("TIME"), 0) == kOCCompareEqualTo) {
//                 double unit_multiplier = 1;
//                 xUnits = SIUnitFromExpression(STR("min"), &unit_multiplier, error);
//                 inverseXUnits = SIUnitFindWithUnderivedSymbol(STR("Hz"));
//                 quantityName = kSIQuantityTime;
//                 inverseQuantityName = kSIQuantityFrequency;
//             } else if (OCStringCompare(string, STR("SECONDS"), 0) == kOCCompareEqualTo) {
//                 double unit_multiplier = 1;
//                 xUnits = SIUnitFromExpression(STR("s"), &unit_multiplier, error);
//                 inverseXUnits = SIUnitFindWithUnderivedSymbol(STR("Hz"));
//                 quantityName = kSIQuantityTime;
//                 inverseQuantityName = kSIQuantityFrequency;
//             }
//             OCRelease(string);
//         }
//     }
//     string = NULL;
//     key = STR("YUNITS");
//     //    SIUnitRef yUnits = NULL;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//     }
//     string = NULL;
//     key = STR("RESOLUTION");
//     double resolution = 0;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//         resolution = creal(OCStringGetDoubleComplexValue(string));
//     }
//     string = NULL;
//     key = STR("COMMENT");
//     OCStringRef description = NULL;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) {
//             OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//             description = OCRetain(string);
//         }
//     }
//     string = NULL;
//     key = STR("FIRSTX");
//     double firstX = 0;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//         firstX = creal(OCStringGetDoubleComplexValue(string));
//     }
//     string = NULL;
//     key = STR("LASTX");
//     double lastX = 0;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//         lastX = creal(OCStringGetDoubleComplexValue(string));
//     }
//     string = NULL;
//     key = STR("DELTAX");
//     double deltaX = 0;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//         deltaX = creal(OCStringGetDoubleComplexValue(string));
//     }
//     string = NULL;
//     key = STR("MAXY");
//     double maxY = 0;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//         maxY = creal(OCStringGetDoubleComplexValue(string));
//     }
//     string = NULL;
//     key = STR("MINY");
//     double minY = 0;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//         minY = creal(OCStringGetDoubleComplexValue(string));
//     }
//     string = NULL;
//     key = STR("XFACTOR");
//     double xFactor = 1;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//         xFactor = creal(OCStringGetDoubleComplexValue(string));
//     }
//     string = NULL;
//     key = STR("YFACTOR");
//     double yFactor = 1;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//         yFactor = creal(OCStringGetDoubleComplexValue(string));
//     }
//     string = NULL;
//     key = STR("NPOINTS");
//     OCIndex size = 1;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//         size = (int) creal(OCStringGetDoubleComplexValue(string));
//     }
//     string = NULL;
//     key = STR("FIRSTY");
//     double firstY = 0;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//         firstY = creal(OCStringGetDoubleComplexValue(string));
//     }
//     string = NULL;
//     key = STR(".OBSERVE FREQUENCY");
//     double observeFrequency = 0;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//         observeFrequency = creal(OCStringGetDoubleComplexValue(string));
//     }
//     string = NULL;
//     key = STR(".OBSERVE NUCLEUS");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//     }
//     string = NULL;
//     key = STR(".ACQUISITION MODE");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//     }
//     string = NULL;
//     key = STR(".AVERAGES");
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) OCDictionaryAddValue(jcampDatasetMetaData, key, string);
//     }
//     key = STR("XYDATA");
//     float data[2 * size];
//     float originOffsetValue = firstX;
//     bool sqz = false;
//     bool dif = false;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         OCArrayRef splitLines = OCStringCreateArrayBySeparatingStrings(string, STR("\n"));
//         OCMutableArrayRef dataLines = OCArrayCreateMutableCopy(splitLines);
//         OCRelease(splitLines);
//         OCIndex i = 0;
//         for (OCIndex index = 1; index < OCArrayGetCount(dataLines); index++) {
//             OCMutableStringRef line = OCStringCreateMutableCopy(OCArrayGetValueAtIndex(dataLines, index));
//             OCStringFindAndReplace(line, STR("+"), STR(" "), OCRangeMake(0, OCStringGetLength(line)), 0);
//             OCStringFindAndReplace(line, STR("-"), STR(" -"), OCRangeMake(0, OCStringGetLength(line)), 0);
//             if (OCStringFindAndReplace(line, STR("@"), STR(" 0"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("A"), STR(" 1"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("B"), STR(" 2"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("C"), STR(" 3"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("D"), STR(" 4"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("E"), STR(" 5"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("F"), STR(" 6"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("G"), STR(" 7"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("H"), STR(" 8"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("I"), STR(" 9"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("a"), STR(" -1"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("b"), STR(" -2"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("c"), STR(" -3"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("d"), STR(" -4"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("e"), STR(" -5"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("f"), STR(" -6"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("g"), STR(" -7"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("h"), STR(" -8"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("i"), STR(" -9"), OCRangeMake(0, OCStringGetLength(line)), 0)) sqz = true;
//             if (OCStringFindAndReplace(line, STR("%"), STR(" 0"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("J"), STR(" 1"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("K"), STR(" 2"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("L"), STR(" 3"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("M"), STR(" 4"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("N"), STR(" 5"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("O"), STR(" 6"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("P"), STR(" 7"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("Q"), STR(" 8"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("R"), STR(" 9"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("j"), STR(" -1"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("k"), STR(" -2"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("l"), STR(" -3"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("m"), STR(" -4"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("n"), STR(" -5"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("o"), STR(" -6"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("p"), STR(" -7"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("q"), STR(" -8"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             if (OCStringFindAndReplace(line, STR("r"), STR(" -9"), OCRangeMake(0, OCStringGetLength(line)), 0)) dif = true;
//             OCStringTrimWhitespace(line);
//             OCArrayRef splitArray = OCStringCreateArrayBySeparatingStrings(line, STR(" "));
//             OCMutableArrayRef array = OCArrayCreateMutableCopy(splitArray);
//             OCRelease(splitArray);
//             OCRelease(line);
//             for (OCIndex jndex = OCArrayGetCount(array) - 1; jndex >= 0; jndex--) {
//                 OCStringRef item = OCArrayGetValueAtIndex(array, jndex);
//                 if (OCStringGetLength(item) == 0) OCArrayRemoveValueAtIndex(array, jndex);
//             }
//             for (OCIndex jndex = 0; jndex < OCArrayGetCount(array); jndex++) {
//                 OCMutableStringRef stringNumber = OCStringCreateMutableCopy(OCArrayGetValueAtIndex(array, jndex));
//                 int dup = 0;
//                 if (OCStringFindAndReplace(stringNumber, STR("S"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 1;
//                 if (OCStringFindAndReplace(stringNumber, STR("T"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 2;
//                 if (OCStringFindAndReplace(stringNumber, STR("U"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 3;
//                 if (OCStringFindAndReplace(stringNumber, STR("V"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 4;
//                 if (OCStringFindAndReplace(stringNumber, STR("W"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 5;
//                 if (OCStringFindAndReplace(stringNumber, STR("X"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 6;
//                 if (OCStringFindAndReplace(stringNumber, STR("Y"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 7;
//                 if (OCStringFindAndReplace(stringNumber, STR("Z"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 8;
//                 if (OCStringFindAndReplace(stringNumber, STR("s"), STR(" "), OCRangeMake(0, OCStringGetLength(stringNumber)), 0)) dup = 9;
//                 if (jndex > 0) {
//                     data[i] = creal(OCStringGetDoubleComplexValue(stringNumber));
//                     if (dif && jndex > 1) data[i] += data[i - 1];
//                     for (OCIndex kndex = 0; kndex < dup; kndex++) {
//                         i++;
//                         data[i] = data[i - 1];
//                     }
//                     i++;
//                 }
//                 OCRelease(stringNumber);
//             }
//             OCRelease(array);
//         }
//         OCRelease(dataLines);
//     }
//     OCDataRef values = OCDataCreate((const u_int8_t *)data, sizeof(float) * size);
//     double sampleInc = (lastX - firstX) / (size - 1);
//     bool reverse = false;
//     if (sampleInc < 0) reverse = true;
//     SIScalarRef increment = SIScalarCreateWithDouble(fabs(sampleInc), xUnits);
//     SIScalarRef originOffset = SIScalarCreateWithDouble(originOffsetValue, xUnits);
//     if (quantityName && OCStringCompare(quantityName, kSIQuantityFrequency, 0) == kOCCompareEqualTo) {
//         OCRelease(originOffset);
//         SIUnitRef megahertz = SIUnitFindWithUnderivedSymbol(STR("MHz"));
//         originOffset = SIScalarCreateWithDouble(observeFrequency, megahertz);
//     } else {
//         originOffset = SIScalarCreateWithDouble(observeFrequency, xUnits);
//     }
//     SIScalarRef referenceOffset = SIScalarCreateWithDouble(0.0, xUnits);
//     SIScalarRef inverseOriginOffset = SIScalarCreateWithDouble(0.0, inverseXUnits);
//     if (inverseQuantityName && OCStringCompare(inverseQuantityName, kSIQuantityFrequency, 0) == kOCCompareEqualTo) {
//         OCRelease(inverseOriginOffset);
//         SIUnitRef megahertz = SIUnitFindWithUnderivedSymbol(STR("MHz"));
//         inverseOriginOffset = SIScalarCreateWithDouble(observeFrequency, megahertz);
//     } else {
//         inverseOriginOffset = SIScalarCreateWithDouble(observeFrequency, inverseXUnits);
//     }
//     SIScalarRef reciprocalReferenceOffset = SIScalarCreateWithDouble(0.0, inverseXUnits);
//     OCMutableArrayRef dimensions = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
//     SIDimensionRef dim = SILinearDimensionCreateDefault(size, increment, quantityName, inverseQuantityName);
//     SIDimensionSetOriginOffset(dim, originOffset, error);
//     SIDimensionSetInverseOriginOffset(dim, inverseOriginOffset);
//     SIDimensionSetCoordinatesOffset(dim, referenceOffset);
//     SIDimensionSetInverseQuantityName(dim, inverseQuantityName);
//     if (nmrSpectrumType) {
//         SIDimensionSetMadeDimensionless(dim, true);
//     }
//     OCRelease(increment);
//     OCRelease(originOffset);
//     OCRelease(referenceOffset);
//     OCRelease(inverseOriginOffset);
//     OCRelease(reciprocalReferenceOffset);
//     if (NULL == dim) {
//         OCRelease(values);
//         return NULL;
//     }
//     SIDimensionMakeNiceUnits(dim);
//     OCArrayAppendValue(dimensions, dim);
//     OCRelease(dim);
//     DatasetRef theDataset = DatasetCreateDefault();
//     DatasetSetDimensions(theDataset, dimensions);
//     OCRelease(dimensions);
//     DependentVariableRef theDependentVariable = DatasetAddDefaultDependentVariable(theDataset, STR("scalar"), kSINumberFloat32Type, -1);
//     DependentVariableSetValues(theDependentVariable, 0, values);
//     OCRelease(values);
//     DependentVariableMultiplyValuesByDimensionlessRealConstant(theDependentVariable, -1, yFactor);
//     OCStringRef yUnits = OCDictionaryGetValue(jcampDatasetMetaData, STR("YUNITS"));
//     if (yUnits) {
//         if (OCStringCompare(yUnits, STR("pH"), 0) == kOCCompareEqualTo) {
//             DependentVariableSetQuantityName(theDependentVariable, kSIQuantityDimensionless);
//             DependentVariableSetComponentLabelAtIndex(theDependentVariable, STR("pH"), 0);
//         }
//         if (OCStringCompare(yUnits, STR("TRANSMITTANCE"), 0) == kOCCompareEqualTo) {
//             DependentVariableSetQuantityName(theDependentVariable, kSIQuantityDimensionless);
//             DependentVariableSetComponentLabelAtIndex(theDependentVariable, STR("Transmittance"), 0);
//         }
//         if (OCStringCompare(yUnits, STR("ABSORBANCE"), 0) == kOCCompareEqualTo) {
//             DependentVariableSetQuantityName(theDependentVariable, kSIQuantityDimensionless);
//             DependentVariableSetComponentLabelAtIndex(theDependentVariable, STR("Absorbance"), 0);
//         }
//     }
//     if (eprSpectrumType) {
//         DependentVariableSetComponentLabelAtIndex(theDependentVariable, STR("Derivative Intensity"), 0);
//     }
//     string = NULL;
//     key = STR("TEMPERATURE");
//     SIUnitRef unit = SIUnitFromExpression(STR("°C"), NULL, error);
//     quantityName = kSIQuantityTemperature;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) {
//             double value = creal(OCStringGetDoubleComplexValue(string));
//             SIScalarRef scalar = SIScalarCreateWithDouble(value, unit);
//             OCDictionaryAddValue(jcampDatasetMetaData, key, scalar);
//             OCRelease(scalar);
//         }
//     }
//     string = NULL;
//     key = STR("PRESSURE");
//     unit = SIUnitFromExpression(STR("atm"), NULL, error);
//     quantityName = kSIQuantityPressure;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) {
//             double value = creal(OCStringGetDoubleComplexValue(string));
//             SIScalarRef scalar = SIScalarCreateWithDouble(value, unit);
//             OCDictionaryAddValue(jcampDatasetMetaData, key, scalar);
//             OCRelease(scalar);
//         }
//     }
//     string = NULL;
//     key = STR("REFRACTIVE INDEX");
//     unit = SIUnitFromExpression(STR("m•s/(m•s)"), NULL, error);
//     quantityName = kSIQuantityRefractiveIndex;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) {
//             double value = creal(OCStringGetDoubleComplexValue(string));
//             SIScalarRef scalar = SIScalarCreateWithDouble(value, unit);
//             OCDictionaryAddValue(jcampDatasetMetaData, key, scalar);
//             OCRelease(scalar);
//         }
//     }
//     string = NULL;
//     key = STR("DENSITY");
//     unit = SIUnitFromExpression(STR("g/cm^3"), NULL, error);
//     quantityName = kSIQuantityDensity;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) {
//             double value = creal(OCStringGetDoubleComplexValue(string));
//             SIScalarRef scalar = SIScalarCreateWithDouble(value, unit);
//             OCDictionaryAddValue(jcampDatasetMetaData, key, scalar);
//             OCRelease(scalar);
//         }
//     }
//     string = NULL;
//     key = STR("MW");
//     unit = SIUnitFromExpression(STR("g/mol"), NULL, error);
//     quantityName = kSIQuantityMolarMass;
//     if (OCDictionaryContainsKey(dictionary, key)) {
//         string = (OCStringRef)OCDictionaryGetValue(dictionary, key);
//         if (string) {
//             double value = creal(OCStringGetDoubleComplexValue(string));
//             SIScalarRef scalar = SIScalarCreateWithDouble(value, unit);
//             OCDictionaryAddValue(jcampDatasetMetaData, key, scalar);
//             OCRelease(scalar);
//         }
//     }
//     DatasetSetMetaData(theDataset, jcampDatasetMetaData);
//     DatasetSetDescription(theDataset, description);
//     DatasetSetTitle(theDataset, title);
//     OCRelease(jcampDatasetMetaData);
//     return theDataset;
// }
