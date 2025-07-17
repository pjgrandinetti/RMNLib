//
//  DatasetTecmag.c
//  RMNLib
//
//  Created by Philip J. Grandinetti on 10/23/11.
//  Copyright (c) 2011 PhySy Ltd. All rights reserved.
//

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <complex.h>
#include <math.h>
#include "../RMNLibrary.h"

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

typedef struct Tecmag
{
	// Number of points and scans in all dimensions:
    
	int32_t npts[4];				// points requested 1D, 2D, 3D, 4D
	int32_t actual_npts[4];			// points completed in each dimension
    // (actual_npts[0] is not really used)
	int32_t acq_points;				// acq_points will be number of points to acquire
    // during one acquisition icon in the sequence
    // (which may be smaller than npts[0])
	int32_t npts_start[4];			// scan or pt on which to start the acquisition
	int32_t scans;					// scans 1D requested
	int32_t actual_scans;			// scans 1D completed
	int32_t dummy_scans;			// number of scans to do prior to collecting actual data
	int32_t repeat_times;			// Number of times to repeat scan
	int32_t sadimension;			// response average dimension
	int32_t samode;					// sets behavior of the response averager for the
    // dimension specified in S.A. Dimension
    
	// Field and frequencies:
	double magnet_field;				// magnet field
	double ob_freq[4];					// observe frequency
	double base_freq[4];				// base frequency
	double offset_freq[4];				// offset from base
	double ref_freq;					// reference frequency for axis calculation 
    // (used to be freqOffset)
	double NMR_frequency;				// absolute NMR frequency
	int16_t obs_channel;				// observe channel defalut = 1;
	char space2[42];
	
	// Spectral width, dwell and filter:
	double sw[4];						// spectral width in Hz
	double dwell[4];					// dwell time in seconds
	double filter;						// filter	
	double experiment_time;				// time for whole experiment
	double acq_time;					// acquisition time - time for acquisition
    
	double last_delay;					// last delay in seconds
	
	int16_t spectrum_direction;			// 1 or -1
	int16_t hardware_sideband;
	int16_t Taps;						// number of taps on receiver filter
	int16_t Type;						// type of filter
	int32_t bDigRec;					// toggle for digital receiver
	int32_t nDigitalCenter;				// number of shift points for digital receiver
	char space3[16];	
	
	
	//	Hardware settings:
	int16_t transmitter_gain;			// transmitter gain
	int16_t receiver_gain;				// receiver gain
	int16_t NumberOfReceivers;			// number of Rx in MultiRx system
	int16_t RG2;						// receiver gain for Rx channel 2	
	double receiver_phase;				// receiver phase
	char space4[4];
	
	// Spinning speed information:
	uint16_t set_spin_rate;				// set spin rate
	uint16_t actual_spin_rate;			// actual spin rate read from the meter
	
	// Lock information:
	int16_t lock_field;					// lock field value (might be Bruker specific)
	int16_t lock_power;					// lock transmitter power
	int16_t lock_gain;					// lock receiver gain
	int16_t lock_phase;					// lock phase	
	double lock_freq_mhz;				// lock frequency in MHz
	double lock_ppm;					// lock ppm
	double H2O_freq_ref;				// H1 freq of H2O
	char space5[16];	
	
	//	VT information:
	double set_temperature;				// non-integer VT
	double actual_temperature;			// non-integer VT
	
	// Shim information:
	double shim_units;					// shim units (used to be SU)	
	int16_t shims[36];					// shim values
	double shim_FWHM;					// full width at half maximum
	
	//	Bruker specific information:
	int16_t HH_dcpl_attn;				// decoupler attenuation 
    // (0..63 or 100..163); receiver gain is above
	int16_t DF_DN;						// decoupler
	int16_t F1_tran_mode[7];			// F1 Pulse transmitter switches
	int16_t dec_BW;						// decoupler BW
	
	char grd_orientation[4];			// gradient orientation
	int32_t LatchLP;					// 990629JMB  values for lacthed LP board
	double grd_Theta;					// 990720JMB  gradient rotation angle Theta
	double grd_Phi;						// 990720JMB  gradient rotation angle Phi
	char space6[264];					// space for the middle
    
	// Time variables 
	int32_t start_time;					// starting time
	int32_t finish_time;				// finishing time
	int32_t elapsed_time;				// projected elapsed time
    // text below and variables above
	
	// Text variables:					// 96 below
	char date[32];						// experiment date
	char nucleus[16];					// nucleus
	char nucleus_2D[16];				// 2D nucleus
	char nucleus_3D[16];				// 3D nucleus
	char nucleus_4D[16];				// 4D nucleus
	char sequence[32];					// sequence name
	char lock_solvent[16];				// Lock solvent
	char lock_nucleus[16];				// Lock nucleus
} Tecmag;


// Grid and Axis Structure	

#define TOTAL_UNIT_TYPES 12

typedef struct grid_and_axis
{
	double majorTickInc[TOTAL_UNIT_TYPES];		// Increment between major ticks
	
	int16_t minorIntNum[TOTAL_UNIT_TYPES];		// Number of intervals between major ticks 
    // (minor ticks is one less than this)
	int16_t labelPrecision[TOTAL_UNIT_TYPES];	// Number of digits after the decimal point
	
	double gaussPerCentimeter;					// Used for calculation of distance 
    // axis in frequency domain
	int16_t gridLines;							// Number of horizontal grid lines to 
    // be shown in data area 
	int16_t axisUnits;							// Type of units to show - see constants.h
	
	int32_t showGrid;							// Show or hide the grid	
	
	int32_t showGridLabels;						// Show or hide the labels on the grid lines
	
	int32_t adjustOnZoom;						// Adjust the number of ticks and the 
    // precision when zoomed in
	int32_t showDistanceUnits;					// whether to show frequency or distance 
    // units when in frequency domain
	char axisName[32];							// file name of the axis (not used as of 4/10/97)
	
	char space[52];	
	
} grid_and_axis;


typedef struct Tecmag2
{
	// Display Menu flags:
	int32_t real_flag;						// display real data				
	int32_t imag_flag;						// display imaginary data
	int32_t magn_flag;						// display magnitude data
	int32_t axis_visible;					// display axis
	int32_t auto_scale;						// auto scale mode on or off
	int32_t line_display;					// YES for lines, NO for points	
	int32_t show_shim_units;				// display shim units on the data area or not
	
	// Option Menu flags:
	int32_t integral_display;				// integrals turned on? - but not swap area
	int32_t fit_display;					// fits turned on?  - but not swap area
	int32_t show_pivot;						// show pivot point on screen; only used 
    // during interactive phasing
	int32_t label_peaks;					// show labels on the peaks?
	int32_t keep_manual_peaks;				// keep manual peaks when re-applying 
    // peak pick settings?
	int32_t label_peaks_in_units;			// peak label type
	int32_t integral_dc_average;			// use dc average for integral calculation
	int32_t integral_show_multiplier;		// show multiplier on integrals that are scaled
	int32_t int32_tean_space[9];
	
	// Processing flags:
	int32_t all_ffts_done[4];
	int32_t all_phase_done[4];
	
	// Vertical display multipliers:
	double amp;								// amplitude scale factor
	double ampbits;							// resolution of display
	double ampCtl;							// amplitude control value
	int32_t offset;							// vertical offset
	
	grid_and_axis axis_set;					// see Grid and Axis Structure below
    
	int16_t display_units[4];				// display units for swap area
	int32_t ref_point[4];					// for use in frequency offset calcs
	double ref_value[4];					// for use in frequency offset calcs
	int32_t z_start;						// beginning of data display 
    // (range: 0 to 2 * npts[0] - 2)
	int32_t z_end;							// end of data display (range: 0 to 2 * npts[0] - 2)
	int32_t z_select_start;					// beginning of zoom highlight
	int32_t z_select_end;					// end of zoom highlight
	int32_t last_zoom_start;				// last z_select_start - not used yet (4/10/97)
	int32_t last_zoom_end;					// last z_select_end - not used yet (4/10/97)
	int32_t index_2D;						// in 1D window, which 2D record we see
	int32_t index_3D;						// in 1D window, which 3D record we see
	int32_t index_4D;						// in 1D window, which 4D record we see
	
    
	int32_t apodization_done[4];		// masked value showing which processing 
    // has been done to the data; see constants.h for values
	double linebrd[4];					// line broadening value
	double gaussbrd[4];					// gaussian broadening value			
	double dmbrd[4];					// double exponential broadening value
	double sine_bell_shift[4];			// sine bell shift value
	double sine_bell_width[4];			// sine bell width value
	double sine_bell_skew[4];			// sine bell skew value
	int32_t Trapz_point_1[4];			// first trapezoid point for trapezoidal apodization
	int32_t Trapz_point_2[4];			// second trapezoid point for 
    // trapezoidal apodization	
	int32_t Trapz_point_3[4];			// third trapezoid point for trapezoidal 
    // apodization	
	int32_t Trapz_point_4[4];			// fourth trapezoid point for trapezoidal apodization
	double trafbrd[4];					// Traficante-Ziessow broadening value
	int32_t echo_center[4];				// echo center for all dimensions
    
	int32_t data_shift_points;			// number of points to use in 
    // left/right shift operations
	int16_t fft_flag[4];				// fourier transform done?  
    // NO if time domain, YES if frequency domain
	double unused[8];
	int pivot_point[4];					// for interactive phasing
	double cumm_0_phase[4];				// cummulative zero order phase applied
	double cumm_1_phase[4];				// cummulative first order phase applied
	double manual_0_phase;				// used for interactive phasing
	double manual_1_phase;				// used for interactive phasing
	double phase_0_value;				// last zero order phase value 
    // applied (not necessarily equivalent to 
    // cummulative zero order phase)
	double phase_1_value;				// last first order phase value applied 
    // (not necessarily equivalent to cummulative 
    // first order phase)
	double session_phase_0;				// used during interactive phasing
	double session_phase_1;				// used during interactive phasing
	
	int32_t max_index;					// index of max data value
	int32_t min_index;					// index of min data value
	float peak_threshold;				// threshold above which peaks are chosen
	float peak_noise;					// minimum value between two points that are 
    // above the peak threshold to distinguish two 
    // peaks from two points on the same peak
	int16_t integral_dc_points;			// number of points to use in integral 
    // calculation when dc average is used
	int16_t integral_label_type;		// how to label integrals, see constants.h
	float integral_scale_factor;		// scale factor to be used in integral draw
	int32_t auto_integrate_shoulder;	// number of points to determine 
    // where integral is cut off
	double auto_integrate_noise;		// when average of shoulder points is 
    // under this value, cut off integral
	double auto_integrate_threshold;	// threshold above which a peak 
    // is chosen in auto integrate
	int32_t s_n_peak;					// peak to be used for response to noise calculation
	int32_t s_n_noise_start;			// start of noise region for 
    // response to noise calculation
	int32_t s_n_noise_end;				// end of noise region for response to noise calculation
	float s_n_calculated;				// calculated response to noise value 
	
	int32_t Spline_point[14];			// points to be used for 
    // spline baseline fix calculation
	int16_t Spline_point_avr;			// for baseline fix
	int32_t Poly_point[8];				// points for polynomial baseline fix calculation
	int16_t Poly_point_avr;				// for baseline fix
	int16_t Poly_order;					// what order polynomial to use 
	
	// Blank Space:
	char space[610];		
    
	// Text variables:
	char line_simulation_name[32];
	char integral_template_name[32];
	char baseline_template_name[32];	
	char layout_name[32];
	char relax_information_name[32];
	
	char username[32];
	
	char user_string_1[16];
	char user_string_2[16];
	char user_string_3[16];	
	char user_string_4[16];
	
} Tecmag2;

#pragma pack(pop)   /* restore original alignment from stack */

static OCIndex findTag(const uint8_t *buffer, OCIndex length, const char *tag)
{
    for(OCIndex index = 0; index < length; index++) {
        size_t tagLength = strlen(tag);
        char *section_tag = malloc(tagLength + 1);
        if (!section_tag) return -1;
        memcpy(section_tag, &buffer[index], tagLength);
        section_tag[tagLength] = '\0';
        if(strcmp(tag, section_tag) == 0) {
            free(section_tag);
            return index;
        }
        free(section_tag);
    }
    return -1;
}

OCStringRef ReadTecmagCreateStringFromBufferIndex(const uint8_t *buffer, OCIndex *index, OCIndex bufferLength)
{
    if(*index >= bufferLength) return NULL;
    
    uint32_t lengthOfString = 0;
    memcpy(&lengthOfString, &buffer[*index], 4);
    if(lengthOfString + *index > bufferLength) return NULL;
    
    *index += 4;
    char *string = malloc(lengthOfString + 1);
    if (!string) return NULL;
    memcpy(string, &buffer[*index], lengthOfString);
    string[lengthOfString] = '\0';
    *index += lengthOfString;
    OCStringRef result = OCStringCreateWithCString(string);
    free(string);
    return result;
}

OCNumberRef ReadTecmagCreate32BitNumberFromBufferIndex(const uint8_t *buffer, OCIndex *index, OCIndex bufferLength)
{
    if(*index >= bufferLength) return NULL;

    int32_t number = 0;
    memcpy(&number, &buffer[*index], 4);
    *index += 4;
    return OCNumberCreateWithSInt32(number);
}

Tecmag *ReadTecmagCreateTecmagStructureFromBufferIndex(const uint8_t *buffer, OCIndex *index, OCIndex bufferLength)
{
    char *tag = malloc(5);
    if (!tag) return NULL;
    memcpy(tag, &buffer[*index], 4);
    tag[4] = '\0';
    *index += 4;
    
    if(strcmp(tag, "TMAG") != 0) {
        free(tag);
        printf("TMAG tag missing\n");
        return NULL;
    }
    free(tag);
    
    uint32_t tmag_flag = 0;
    memcpy(&tmag_flag, &buffer[*index], 4);
    *index += 4;

    if(tmag_flag) {
        uint32_t lengthOfTecmagStructure = 0;
        memcpy(&lengthOfTecmagStructure, &buffer[*index], 4);
        *index += 4;
        
        Tecmag *tecmag = malloc(sizeof(struct Tecmag));
        if (!tecmag) return NULL;
        tecmag = memcpy(tecmag, (buffer + 20), sizeof(struct Tecmag));
        *index += lengthOfTecmagStructure;
        return tecmag;
    }
    printf("TMAG flag is false\n");
    return NULL;
}

Tecmag2 *ReadTecmagCreateTecmag2StructureFromBufferIndex(const uint8_t *buffer, OCIndex *index, OCIndex bufferLength)
{
    char *tag = malloc(5);
    if (!tag) return NULL;
    memcpy(tag, &buffer[*index], 4);
    tag[4] = '\0';
    *index += 4;
    
    if(strcmp(tag, "TMG2") != 0) {
        free(tag);
        return NULL;
    }
    free(tag);
    
    uint32_t tmag_flag = 0;
    memcpy(&tmag_flag, &buffer[*index], 4);
    *index += 4;
    if(tmag_flag) {
        uint32_t lengthOfTecmag2Structure = 0;
        memcpy(&lengthOfTecmag2Structure, &buffer[*index], 4);
        *index += 4;
        
        Tecmag2 *tecmag2 = malloc(sizeof(struct Tecmag2));
        if (!tecmag2) return NULL;
        tecmag2 = memcpy(tecmag2, (buffer + *index), sizeof(struct Tecmag2));
        *index += lengthOfTecmag2Structure;
        return tecmag2;
    }
    return NULL;
}

const uint8_t *SetTecmagDataLocationAndNpts(const uint8_t *buffer, OCIndex *index, OCIndex *npts, OCIndex bufferLength)
{
    char *tag = malloc(5);
    if (!tag) return NULL;
    memcpy(tag, &buffer[*index], 4);
    tag[4] = '\0';
    *index += 4;
    if(strcmp(tag, "DATA") != 0) {
        free(tag);
        return NULL;
    }
    free(tag);
    
    uint32_t data_flag = 0;
    memcpy(&data_flag, &buffer[*index], 4);
    *index += 4;
    if(data_flag) {
        uint32_t lengthOfData = 0;
        memcpy(&lengthOfData, &buffer[*index], 4);
        *index += 4;
        *npts = lengthOfData;
        *npts = *npts / sizeof(float complex);
        *index += lengthOfData;
        return (const uint8_t *) (buffer + 1056);
    }
    return NULL;
}

OCMutableDictionaryRef CreateTecmagStructureMetaData(Tecmag *tecmag, OCStringRef *error)
{
    OCStringRef err = NULL;
    SIUnitRef hertz = SIUnitFromExpression(STR("Hz"), NULL, &err);
    if (err) { OCRelease(err); err = NULL; }
    SIUnitRef seconds = SIUnitFromExpression(STR("s"), NULL, &err);
    if (err) { OCRelease(err); err = NULL; }

    OCMutableDictionaryRef tmagMetaData = OCDictionaryCreateMutable(0);
    
    OCStringRef date = OCStringCreateWithCString(tecmag->date);
    OCDictionarySetValue(tmagMetaData, STR("date"), date);
    OCRelease(date);
    
    OCStringRef sequence = OCStringCreateWithCString(tecmag->sequence);
    OCDictionarySetValue(tmagMetaData, STR("sequence"), sequence);
    OCRelease(sequence);
    
    OCStringRef lock_solvent = OCStringCreateWithCString(tecmag->lock_solvent);
    OCDictionarySetValue(tmagMetaData, STR("lock solvent"), lock_solvent);
    OCRelease(lock_solvent);
    
    OCNumberRef actual_scans = OCNumberCreateWithSInt32(tecmag->actual_scans);
    OCStringRef temp = OCNumberCreateStringValue(actual_scans);
    OCDictionarySetValue(tmagMetaData, STR("actual scans"), temp);
    OCRelease(temp);
    OCRelease(actual_scans);
    
    OCNumberRef scans = OCNumberCreateWithSInt32(tecmag->scans);
    temp = OCNumberCreateStringValue(scans);
    OCDictionarySetValue(tmagMetaData, STR("scans"), temp);
    OCRelease(temp);
    OCRelease(scans);
    
    OCNumberRef dummy_scans = OCNumberCreateWithSInt32(tecmag->dummy_scans);
    temp = OCNumberCreateStringValue(dummy_scans);
    OCDictionarySetValue(tmagMetaData, STR("dummy scans"), temp);
    OCRelease(temp);
    OCRelease(dummy_scans);
    
    SIScalarRef acquisitionTime = SIScalarCreateWithDouble(tecmag->acq_time, seconds);
    temp = SIScalarCreateStringValue(acquisitionTime);
    OCDictionarySetValue(tmagMetaData, STR("acquisition time"), temp);
    OCRelease(temp);
    OCRelease(acquisitionTime);
    
    SIScalarRef experiment_time = SIScalarCreateWithDouble(tecmag->experiment_time, seconds);
    temp = SIScalarCreateStringValue(experiment_time);
    OCDictionarySetValue(tmagMetaData, STR("experiment time"), temp);
    OCRelease(temp);
    OCRelease(experiment_time);
    
    SIScalarRef last_delay = SIScalarCreateWithDouble(tecmag->last_delay, seconds);
    temp = SIScalarCreateStringValue(last_delay);
    OCDictionarySetValue(tmagMetaData, STR("last delay"), temp);
    OCRelease(temp);
    OCRelease(last_delay);
    
    SIUnitRef radians = SIUnitFromExpression(STR("rad"), NULL, &err);
    if (err) { OCRelease(err); err = NULL; }
    SIScalarRef receiver_phase = SIScalarCreateWithDouble(tecmag->receiver_phase, radians);
    temp = SIScalarCreateStringValue(receiver_phase);
    OCDictionarySetValue(tmagMetaData, STR("receiver phase"), temp);
    OCRelease(temp);
    OCRelease(receiver_phase);
    
    SIScalarRef filter = SIScalarCreateWithDouble(tecmag->filter, hertz);
    temp = SIScalarCreateStringValue(filter);
    OCDictionarySetValue(tmagMetaData, STR("filter"), temp);
    OCRelease(temp);
    OCRelease(filter);
    
    OCNumberRef transmitter_gain = OCNumberCreateWithSInt16(tecmag->transmitter_gain);
    temp = OCNumberCreateStringValue(transmitter_gain);
    OCDictionarySetValue(tmagMetaData, STR("transmitter gain"), temp);
    OCRelease(temp);
    OCRelease(transmitter_gain);
    
    OCNumberRef receiver_gain = OCNumberCreateWithSInt16(tecmag->receiver_gain);
    temp = OCNumberCreateStringValue(receiver_gain);
    OCDictionarySetValue(tmagMetaData, STR("receiver gain"), temp);
    OCRelease(temp);
    OCRelease(receiver_gain);

    return tmagMetaData;
}

DatasetRef DatasetImportTecmagCreateWithFileData(OCDataRef contents, OCStringRef *error)
{
    IF_NO_OBJECT_EXISTS_RETURN(contents, NULL);
    if(error && *error) return NULL;
    
    OCIndex totalFileLength = OCDataGetLength(contents);
    if(totalFileLength == 0) return NULL;
    const uint8_t *buffer = OCDataGetBytesPtr(contents);

    OCStringRef err = NULL;
    SIUnitRef megahertz = SIUnitFromExpression(STR("MHz"), NULL, &err);
    if (err) { OCRelease(err); err = NULL; }
    SIUnitRef seconds = SIUnitFromExpression(STR("s"), NULL, &err);
    if (err) { OCRelease(err); err = NULL; }
    
    OCIndex count = 0;
    char *tntTag = malloc(5);
    if (!tntTag) return NULL;
    memcpy(tntTag, &buffer[count], 4);
    tntTag[4] = '\0';
    if(strcmp(tntTag, "TNT1") != 0) {
        free(tntTag);
        return NULL;
    }
    free(tntTag);

    count = 0;
    char *versionID = malloc(9);
    if (!versionID) return NULL;
    memcpy(versionID, &buffer[count], 8);
    versionID[8] = '\0';
    OCStringRef versionIDString = OCStringCreateWithCString(versionID);
    free(versionID);
    
    count = 8;
    Tecmag *tecmag = ReadTecmagCreateTecmagStructureFromBufferIndex(buffer, &count, totalFileLength);
    if(NULL == tecmag) {
        OCRelease(versionIDString);
        if(error) {
            *error = STR("Error reading tecmag structure.");
        }
        return NULL;
    }
    
    OCIndex size = 0;
    const uint8_t *data = SetTecmagDataLocationAndNpts(buffer, &count, &size, totalFileLength);
    if(NULL == data) {
        if(error) {
            *error = STR("Error reading tecmag data.");
        }
        if(tecmag) free(tecmag);
        OCRelease(versionIDString);
        return NULL;
    }

    Tecmag2 *tecmag2 = ReadTecmagCreateTecmag2StructureFromBufferIndex(buffer, &count, totalFileLength);
    if(NULL == tecmag2) {
        if(error) {
            *error = STR("Error reading tecmag2 structure.");
        }
        if(tecmag) free(tecmag);
        OCRelease(versionIDString);
        return NULL;
    }
    
    // Essential structures read
    
    // Create NMR meta-data
    OCMutableDictionaryRef tecmagDatasetMetaData = OCDictionaryCreateMutable(0);
    OCDictionarySetValue(tecmagDatasetMetaData, STR("versionID"), versionIDString);
    OCRelease(versionIDString);
    
    OCMutableDictionaryRef tmagMetaData = CreateTecmagStructureMetaData(tecmag, error);
    OCDictionarySetValue(tecmagDatasetMetaData, STR("tmag"), tmagMetaData);
    OCRelease(tmagMetaData);
    
    int numberOfDimensions = 0;
    for(int i = 0; i < 4; i++) {
        if(tecmag->actual_npts[i] > 1) numberOfDimensions++;
    }
    
    if(numberOfDimensions == 0) numberOfDimensions = 1;
    
    OCMutableArrayRef dimensions = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    
    for(OCIndex iDim = 0; iDim < numberOfDimensions; iDim++) {
        double observe_frequency = tecmag->ob_freq[tecmag->obs_channel - 1];
        observe_frequency *= 1e7;
        observe_frequency = floor(observe_frequency);
        observe_frequency /= 1e7;
        
        OCStringRef quantityName, inverseQuantityName;
        SIScalarRef increment, originOffset, inverseOriginOffset;
        
        if(tecmag2 && tecmag2->fft_flag[iDim]) {
            quantityName = OCStringCreateCopy(kSIQuantityFrequency);
            inverseQuantityName = OCStringCreateCopy(kSIQuantityTime);
            SIScalarRef dwell = SIScalarCreateWithDouble(tecmag->dwell[iDim], seconds);
            SIScalarRef temp = SIScalarCreateByRaisingToPower(dwell, -1, &err);
            if (err) { OCRelease(err); err = NULL; }
            increment = SIScalarCreateByMultiplyingByDimensionlessRealConstant(temp, 1.0 / (double) tecmag->actual_npts[iDim]);
            OCRelease(temp);
            OCRelease(dwell);
            originOffset = SIScalarCreateWithDouble(observe_frequency, megahertz);
            inverseOriginOffset = SIScalarCreateWithDouble(0.0, seconds);
        }
        else {
            quantityName = OCStringCreateCopy(kSIQuantityTime);
            inverseQuantityName = OCStringCreateCopy(kSIQuantityFrequency);
            increment = SIScalarCreateWithDouble(tecmag->dwell[iDim], seconds);
            originOffset = SIScalarCreateWithDouble(0.0, seconds);
            inverseOriginOffset = SIScalarCreateWithDouble(observe_frequency, megahertz);
        }
        
        // Create reciprocal dimension with inverse variables
        SIDimensionRef reciprocalDimension = SIDimensionCreate(
            STR("reciprocal"),          // label
            NULL,                       // description
            NULL,                       // metadata
            inverseQuantityName,        // quantity
            inverseOriginOffset,        // offset
            NULL,                       // origin
            NULL,                       // period
            false,                      // periodic
            kDimensionScalingNone,      // scaling
            &err                        // outError
        );
        
        if (err) { OCRelease(err); err = NULL; }
        // Set madeDimensionless flag in metadata if inverse origin offset is non-zero
        if(!SIScalarIsZero(inverseOriginOffset)) {
            NMRDimensionSetDimensionless((DimensionRef)reciprocalDimension);
        }
        
        SILinearDimensionRef theDimension = SILinearDimensionCreate(
            STR("dimension"),           // label
            NULL,                       // description
            NULL,                       // metadata
            quantityName,               // quantity
            originOffset,               // offset
            NULL,                       // origin
            NULL,                       // period
            false,                      // periodic
            kDimensionScalingNone,      // scaling
            tecmag->actual_npts[iDim],  // count
            increment,                  // increment
            false,                      // fft
            reciprocalDimension,        // reciprocal
            &err                        // outError
        );
        
        if (err) { OCRelease(err); err = NULL; }
        
        OCArrayAppendValue(dimensions, theDimension);
        OCRelease(theDimension);
        OCRelease(reciprocalDimension);
        OCRelease(quantityName);
        OCRelease(inverseQuantityName);
        OCRelease(increment);
        OCRelease(originOffset);
        OCRelease(inverseOriginOffset);
    }
    
    // Create dependent variables array
    OCMutableArrayRef dependentVariables = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    
    DependentVariableRef theDependentVariable = DependentVariableCreate(
        STR("signal"),                    // name
        STR("NMR Signal"),                // description
        NULL,                             // unit
        STR("signal"),                    // quantityName
        STR("scalar"),                    // quantityType
        kOCNumberComplex64Type,           // elementType
        NULL,                             // componentLabels
        NULL,                             // components
        &err                              // outError
    );
    
    if (err) { OCRelease(err); err = NULL; }
    
    OCArrayAppendValue(dependentVariables, theDependentVariable);
    
    // Create the dataset
    DatasetRef theDataset = DatasetCreate(
        dimensions,           // dimensions
        NULL,                 // dimensionPrecedence
        dependentVariables,   // dependentVariables
        NULL,                 // tags
        NULL,                 // description
        STR("Tecmag Dataset"), // title
        NULL,                 // focus
        NULL,                 // previousFocus
        NULL,                 // metadata
        &err                  // outError
    );
    
    if (err) { OCRelease(err); err = NULL; }
    
    OCRelease(dimensions);
    OCRelease(dependentVariables);
    
    if (!theDataset) {
        if(error) *error = STR("Failed to create dataset");
        free(tecmag);
        free(tecmag2);
        OCRelease(theDependentVariable);
        return NULL;
    }
    
    // Set the data
    OCIndex sizeFromDimensions = DependentVariableGetSize(theDependentVariable);
    OCMutableDataRef values = OCDataCreateMutable(0);
    OCDataAppendBytes(values, data, size * sizeof(float complex));
    if(sizeFromDimensions != size) {
        OCDataSetLength(values, sizeFromDimensions * sizeof(float complex));
    }
    DependentVariableSetComponentAtIndex(theDependentVariable, values, 0);
    OCRelease(values);
    
    // Needed to get frequency signs correct in rotating frame
    DependentVariableConjugate(theDependentVariable, 0);
    
    // Create metadata
    OCMutableDictionaryRef nmrDatasetMetaData = OCDictionaryCreateMutable(0);
    OCDictionarySetValue(nmrDatasetMetaData, STR("Tecmag"), tecmagDatasetMetaData);
    OCRelease(tecmagDatasetMetaData);
    
    OCMutableDictionaryRef datasetMetaData = OCDictionaryCreateMutable(0);
    OCDictionarySetValue(datasetMetaData, STR("NMR"), nmrDatasetMetaData);
    OCRelease(nmrDatasetMetaData);
    
    SIUnitRef kelvin = SIUnitFromExpression(STR("K"), NULL, &err);
    if (err) { OCRelease(err); err = NULL; }
    SIScalarRef metaCoordinate = SIScalarCreateWithDouble(tecmag->actual_temperature, kelvin);
    OCDictionarySetValue(datasetMetaData, kSIQuantityTemperature, metaCoordinate);
    OCRelease(metaCoordinate);
    
    SIUnitRef tesla = SIUnitFromExpression(STR("T"), NULL, &err);
    if (err) { OCRelease(err); err = NULL; }
    metaCoordinate = SIScalarCreateWithDouble(tecmag->magnet_field, tesla);
    OCDictionarySetValue(datasetMetaData, kSIQuantityMagneticFluxDensity, metaCoordinate);
    OCRelease(metaCoordinate);
    
    DatasetSetMetaData(theDataset, datasetMetaData);
    OCRelease(datasetMetaData);
    
    OCRelease(theDependentVariable);
    free(tecmag);
    free(tecmag2);
    
    return theDataset;
}
