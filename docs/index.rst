RMNLib Documentation
====================

.. warning::
   **🚧 DEVELOPMENT STATUS: ALPHA - NOT READY FOR USE 🚧**
   
   **⚠️ This project is in early development and is NOT suitable for production use.**
   
   - API is unstable and subject to major changes
   - Many features are incomplete or untested  
   - Documentation may be outdated or incorrect
   - Breaking changes will occur without notice
   - **DO NOT USE** in any production environment
   
   This documentation is shared for development purposes only. Check back later for stable releases.

----

RMNLib is a C library for reading, writing, and manipulating Core Scientific Dataset Model (CSDM) files.
It provides a comprehensive API for working with multidimensional scientific datasets, including sparse sampling,
geographic coordinates, and various dimension types.

Requirements
~~~~~~~~~~~~

Ensure you have installed:

- A C compiler (e.g., clang or gcc)
- Make
- Doxygen
- Python 3 with ``sphinx`` and ``breathe`` (``pip install -r docs/requirements.txt``)

Building the Library
~~~~~~~~~~~~~~~~~~~~

Compile the static library::

    make

This produces ``libRMNLib.a``.

Usage
~~~~~

Basic Library Setup
^^^^^^^^^^^^^^^^^^^

Include the main library header in your C code::

    #include "RMNLibrary.h"

Link against the static library when compiling::

    gcc -o myprogram myprogram.c -L. -lRMNLib -lm

Creating and Working with Datasets
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

RMNLib revolves around the concept of datasets that contain multidimensional scientific data. 
Let's build a dataset step by step, starting with its components:

.. code-block:: c

    // Step 1: Generate the component data first
    OCStringRef error = NULL;
    OCIndex count = 1024;  // Number of data points
    
    // First create the data component (complex data needs 2x space for real + imaginary)
    OCDataRef componentData = OCDataCreateWithBytes(NULL, count * sizeof(double complex));
    
    // Generate a damped complex valued oscillation
    double complex *data = (double complex *)OCDataGetBytesPtr(componentData);
    double frequency = 100.0;    // Hz
    double decayRate = 50.0;     // decay constant
    double samplingInterval = 1.0e-6; // 1 μs sampling interval (1 MHz sampling rate)
    
    for (OCIndex i = 0; i < count; i++) {
        double time = i * samplingInterval;
        double amplitude = exp(-decayRate * time);
        double phase = 2.0 * M_PI * frequency * time;
        
        // Store complex value directly
        data[i] = amplitude * (cos(phase) + I * sin(phase));
    }

    // Step 2: Create a dimension to define the coordinate space
    // First create a reciprocal dimension (frequency domain)
    SIDimensionRef frequencyDimension = SIDimensionCreateWithQuantity(kSIQuantityFrequency, &error);
    
    // Now create the main time dimension with the frequency dimension as reciprocal
    SIScalarRef increment = SIScalarCreateFromExpression(STR("1.0 µs"), &error);
    SILinearDimensionRef dimension = SILinearDimensionCreateMinimal(
        kSIQuantityTime, 
        count,         // count
        increment, 
        frequencyDimension, // reciprocal dimension
        &error);

    // Step 3: Create a dependent variable to hold your data
    DependentVariableRef depVar = DependentVariableCreateWithComponent(
        STR("amplitude"),      // name
        NULL,                  // description
        NULL,                  // unit (default to dimensionless)
        STR("scalar"),         // quantityName
        kOCNumberComplex128Type,  // numericType (complex double)
        NULL,                  // componentLabels
        componentData,         // component data
        &error);
    
    // Step 4: Create arrays to hold our components
    OCArrayRef dimensions = OCArrayCreate();
    OCArrayRef dependentVariables = OCArrayCreate();
    OCArrayAddValue(dimensions, (OCTypeRef)dimension);
    OCArrayAddValue(dependentVariables, (OCTypeRef)depVar);
    
    // Step 5: Create a dataset and assemble the components
    DatasetRef dataset = DatasetCreateMinimal(
        dimensions,           // dimensions array
        dependentVariables,   // dependent variables array
        &error);
    
    // Clean up when done
    OCRelease(frequencyDimension);
    OCRelease(increment);
    OCRelease(dimension);
    OCRelease(componentData);
    OCRelease(depVar);
    OCRelease(dimensions);
    OCRelease(dependentVariables);
    OCRelease(dataset);
    if (error) {
        printf("Error: %s\n", OCStringGetCStringPtr(error));
        OCRelease(error);
    }

Working with Different Dimension Types
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

RMNLib supports various dimension types for different coordinate systems:

.. code-block:: c

    OCStringRef error = NULL;
    
    // Linear dimension (evenly spaced points)
    SIScalarRef increment = SIScalarCreateFromExpression(STR("0.5 s"), &error);
    SILinearDimensionRef linearDim = SILinearDimensionCreateMinimal(
        kSIQuantityTime,
        200,        // count
        increment,  // step size
        NULL,       // reciprocal
        &error);
    
    // Monotonic dimension (arbitrary spacing, but ordered)
    double values[] = {0.0, 1.5, 3.7, 8.2, 15.0};
    OCArrayRef valueArray = OCArrayCreate();
    for (int i = 0; i < 5; i++) {
        OCStringRef valueStr = OCStringCreateWithFormat("%.1f m", values[i]);
        SIScalarRef scalar = SIScalarCreateFromExpression(valueStr, &error);
        OCArrayAddValue(valueArray, (OCTypeRef)scalar);
        OCRelease(scalar);
        OCRelease(valueStr);
    }
    SIMonotonicDimensionRef monotonicDim = SIMonotonicDimensionCreate(
        kSIQuantityLength,
        NULL,       // description
        NULL,       // metadata
        valueArray, // coordinates
        NULL,       // reciprocal
        &error);
    
    // Labeled dimension (categorical data)
    OCArrayRef labels = OCArrayCreate();
    OCArrayAddValue(labels, (OCTypeRef)STR("Sample A"));
    OCArrayAddValue(labels, (OCTypeRef)STR("Sample B"));
    OCArrayAddValue(labels, (OCTypeRef)STR("Sample C"));
    
    LabeledDimensionRef labeledDim = LabeledDimensionCreateWithCoordinateLabels(labels);
    
    // Clean up
    OCRelease(increment);
    OCRelease(linearDim);
    OCRelease(valueArray);
    OCRelease(monotonicDim);
    OCRelease(labels);
    OCRelease(labeledDim);
    if (error) {
        printf("Error: %s\n", OCStringGetCStringPtr(error));
        OCRelease(error);
    }

Memory Management
^^^^^^^^^^^^^^^^^

RMNLib uses reference counting for memory management. Follow these rules:

.. code-block:: c

    // Functions with "Create" or "Copy" return objects you own
    DatasetRef dataset = DatasetCreate();           // retain count = 1, you own it
    DatasetRef copy = DatasetCopy(dataset);         // retain count = 1, you own the copy
    
    // Retain an object to keep it alive
    OCRetain(dataset);                              // retain count = 2
    
    // Release when done (decrements retain count)
    OCRelease(dataset);                             // retain count = 1
    OCRelease(dataset);                             // retain count = 0, object is deallocated
    OCRelease(copy);                                // Clean up the copy

Metadata and JSON Serialization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Store and retrieve metadata using dictionaries and JSON:

.. code-block:: c

    // Create metadata dictionary
    OCDictionaryRef metadata = OCDictionaryCreate();
    OCDictionarySetValue(metadata, 
                        STR("experiment_name"),
                        STR("NMR_Experiment_001"));
    
    // Convert to JSON for storage/transmission
    cJSON *json = OCMetadataCopyJSON(metadata);
    char *jsonString = cJSON_Print(json);
    printf("Metadata JSON: %s\n", jsonString);
    
    // Convert back from JSON
    OCStringRef error = NULL;
    OCDictionaryRef restoredMetadata = OCMetadataCreateFromJSON(json, &error);
    if (error) {
        printf("Error: %s\n", OCStringGetCStringPtr(error));
        OCRelease(error);
    }
    
    // Clean up
    free(jsonString);
    cJSON_Delete(json);
    OCRelease(metadata);
    OCRelease(restoredMetadata);

Error Handling
^^^^^^^^^^^^^^

Most RMNLib functions return NULL on error. Always check return values:

.. code-block:: c

    DatasetRef dataset = DatasetCreate();
    if (!dataset) {
        fprintf(stderr, "Failed to create dataset\n");
        return -1;
    }
    
    DimensionRef dimension = DimensionCreateLinear(0.0, 1.0, 100);
    if (!dimension) {
        fprintf(stderr, "Failed to create dimension\n");
        OCRelease(dataset);
        return -1;
    }
    
    // Use the objects...
    
    // Always clean up
    OCRelease(dimension);
    OCRelease(dataset);

Library Cleanup
^^^^^^^^^^^^^^^^

For clean shutdown and accurate leak detection, call the shutdown function:

.. code-block:: c

    int main() {
        // Your program logic here...
        
        // At the end of your program
        RMNLibTypesShutdown();
        return 0;
    }

Complete Example
^^^^^^^^^^^^^^^^

Here's a complete example that creates a simple 1D dataset with reciprocal dimensions:

.. code-block:: c

    #include "RMNLibrary.h"
    
    int main() {
        OCStringRef error = NULL;
        OCIndex count = 1024;  // Number of data points
        
        // Step 1: Create reciprocal dimension (frequency domain)
        SIDimensionRef frequencyDimension = SIDimensionCreateWithQuantity(kSIQuantityFrequency, &error);
        if (!frequencyDimension || error) {
            printf("Failed to create frequency dimension: %s\n", 
                   error ? OCStringGetCStringPtr(error) : "unknown error");
            if (error) OCRelease(error);
            return -1;
        }
        
        // Step 2: Create main time dimension with reciprocal
        SIScalarRef increment = SIScalarCreateFromExpression(STR("1.0 µs"), &error);
        if (!increment || error) {
            printf("Failed to create time increment: %s\n", 
                   error ? OCStringGetCStringPtr(error) : "unknown error");
            if (error) OCRelease(error);
            OCRelease(frequencyDimension);
            return -1;
        }
        
        SILinearDimensionRef dimension = SILinearDimensionCreateMinimal(
            kSIQuantityTime, 
            count,        // count
            increment, 
            frequencyDimension, // reciprocal dimension
            &error);
        if (!dimension || error) {
            printf("Failed to create time dimension: %s\n", 
                   error ? OCStringGetCStringPtr(error) : "unknown error");
            if (error) OCRelease(error);
            OCRelease(frequencyDimension);
            OCRelease(increment);
            return -1;
        }
        
        // Step 3: Create dependent variable for data
        // First create the data component (complex data needs 2x space for real + imaginary)
        OCDataRef componentData = OCDataCreateWithBytes(NULL, count * sizeof(double complex));
        
        // Generate a damped complex valued oscillation
        double complex *data = (double complex *)OCDataGetBytesPtr(componentData);
        double frequency = 100.0;    // Hz
        double decayRate = 50.0;     // decay constant
        double samplingInterval = 1.0e-6; // 1 μs sampling interval (1 MHz sampling rate)
        
        for (OCIndex i = 0; i < count; i++) {
            double time = i * samplingInterval;
            double amplitude = exp(-decayRate * time);
            double phase = 2.0 * M_PI * frequency * time;
            
            // Store complex value directly
            data[i] = amplitude * (cos(phase) + I * sin(phase));
        }
        
        DependentVariableRef depVar = DependentVariableCreateWithComponent(
            STR("intensity"),      // name
            NULL,                  // description
            NULL,                  // unit (dimensionless)
            STR("scalar"),         // quantityName
            kOCNumberComplex128Type,  // numericType (complex double)
            NULL,                  // componentLabels
            componentData,         // component data
            &error);
        if (!depVar || error) {
            printf("Failed to create dependent variable: %s\n", 
                   error ? OCStringGetCStringPtr(error) : "unknown error");
            if (error) OCRelease(error);
            OCRelease(frequencyDimension);
            OCRelease(increment);
            OCRelease(dimension);
            OCRelease(componentData);
            return -1;
        }
        
        // Step 4: Create arrays to hold components
        OCArrayRef dimensions = OCArrayCreate();
        OCArrayRef dependentVariables = OCArrayCreate();
        if (!dimensions || !dependentVariables) {
            printf("Failed to create arrays\n");
            OCRelease(frequencyDimension);
            OCRelease(increment);
            OCRelease(dimension);
            OCRelease(componentData);
            OCRelease(depVar);
            if (dimensions) OCRelease(dimensions);
            if (dependentVariables) OCRelease(dependentVariables);
            return -1;
        }
        
        OCArrayAddValue(dimensions, (OCTypeRef)dimension);
        OCArrayAddValue(dependentVariables, (OCTypeRef)depVar);
        
        // Step 5: Create dataset
        DatasetRef dataset = DatasetCreateMinimal(
            dimensions,           // dimensions array
            dependentVariables,   // dependent variables array
            &error);
        
        if (!dataset || error) {
            printf("Failed to create dataset: %s\n", 
                   error ? OCStringGetCStringPtr(error) : "unknown error");
            if (error) OCRelease(error);
        } else {
            printf("Dataset created successfully with reciprocal time/frequency dimensions!\n");
        }
        
        // Clean up
        OCRelease(frequencyDimension);
        OCRelease(increment);
        OCRelease(dimension);
        OCRelease(componentData);
        OCRelease(depVar);
        OCRelease(dimensions);
        OCRelease(dependentVariables);
        if (dataset) OCRelease(dataset);
        
        // Shutdown library
        RMNLibTypesShutdown();
        return dataset ? 0 : -1;
    }

.. toctree::
   :maxdepth: 2
   :caption: API Reference

   api/Dataset
   api/Dimension
   api/DependentVariable
   api/Datum
   api/SparseSampling
   api/GeographicCoordinate
   api/RMNGridUtils
   api/RMNLibrary

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
