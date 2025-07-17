#include "../RMNLibrary.h"

void NMRDimensionSetDimensionless(DimensionRef dim) {
    OCMutableDictionaryRef metaData = DimensionGetMetadata(dim);
    if (metaData) {
        OCMutableDictionaryRef nmrMetaData = (OCMutableDictionaryRef) OCDictionaryGetValue(metaData, STR("nmr_spectroscopy"));
        if (nmrMetaData) {
            OCDictionarySetValue(nmrMetaData, STR("dimensionless"), kOCBooleanTrue);
        } else {
            nmrMetaData = OCDictionaryCreateMutable(0);
            OCDictionarySetValue(nmrMetaData, STR("dimensionless"), kOCBooleanTrue);
            OCDictionarySetValue(metaData, STR("nmr_spectroscopy"), nmrMetaData);
            OCRelease(nmrMetaData);
        }
    }
}
