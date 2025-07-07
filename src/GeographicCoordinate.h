#ifndef GEOGRAPHIC_COORDINATE_H
#define GEOGRAPHIC_COORDINATE_H
#include "RMNLibrary.h"
#ifdef __cplusplus
extern "C" {
#endif
/// Opaque type for geographic coordinate
typedef struct impl_GeographicCoordinate *GeographicCoordinateRef;
/// Create a GeographicCoordinate with required latitude and longitude, optional altitude, and metadata
GeographicCoordinateRef GeographicCoordinateCreate(
    SIScalarRef latitude,     ///< Required latitude (° north positive)
    SIScalarRef longitude,    ///< Required longitude (° east positive)
    SIScalarRef altitude,     ///< Optional altitude (m above sea level), NULL for unspecified
    OCDictionaryRef metadata  ///< Optional application-specific metadata, NULL for none
);
/// Create a copy from a serialized dictionary (round-trip)
/// @param dict   Dictionary representation of a GeographicCoordinate
/// @param outError Optional output for error message
/// @return New GeographicCoordinateRef or NULL on error
GeographicCoordinateRef GeographicCoordinateCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError);
GeographicCoordinateRef GeographicCoordinateCreateFromJSON(cJSON *json, OCStringRef *outError);
/// Serialize to a dictionary for JSON conversion
/// @param gc GeographicCoordinateRef to serialize
/// @return Newly created dictionary (caller must release)
OCDictionaryRef GeographicCoordinateCopyAsDictionary(GeographicCoordinateRef gc);
/// Getters
SIScalarRef GeographicCoordinateGetLatitude(GeographicCoordinateRef gc);      ///< Latitude
SIScalarRef GeographicCoordinateGetLongitude(GeographicCoordinateRef gc);     ///< Longitude
SIScalarRef GeographicCoordinateGetAltitude(GeographicCoordinateRef gc);      ///< Altitude or NULL
OCDictionaryRef GeographicCoordinateGetMetaData(GeographicCoordinateRef gc);  ///< User metadata (never NULL)
/// Setters
bool GeographicCoordinateSetLatitude(GeographicCoordinateRef gc, SIScalarRef latitude);
bool GeographicCoordinateSetLongitude(GeographicCoordinateRef gc, SIScalarRef longitude);
bool GeographicCoordinateSetAltitude(GeographicCoordinateRef gc, SIScalarRef altitude);
bool GeographicCoordinateSetMetaData(GeographicCoordinateRef gc, OCDictionaryRef metadata);
/// Deep copy
GeographicCoordinateRef GeographicCoordinateCreateCopy(GeographicCoordinateRef gc);
/// Type identification
OCTypeID GeographicCoordinateGetTypeID(void);
#ifdef __cplusplus
}
#endif
#endif  // GEOGRAPHIC_COORDINATE_H
