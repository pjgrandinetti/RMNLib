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

This produces ``libRMN.a``.

Usage
~~~~~

Basic Library Setup
^^^^^^^^^^^^^^^^^^^

Include the main library header in your C code::

    #include "RMNLibrary.h"

Link against the static library when compiling::

    gcc -o myprogram myprogram.c -L. -lRMNLib -lm

Creating Datasets
^^^^^^^^^^^^^^^^^^

RMNLib revolves around datasets containing multidimensional scientific data. Here's a simple linear dimension example:

.. code-block:: c

    OCStringRef error = NULL;
    
    // Simulate damped complex oscillation data (typical NMR/ESR experiment)
    OCIndex count = 1024;
    double complex *values = malloc(count * sizeof(double complex));
    double frequency = 100.0;    // Hz
    double decayRate = 50.0;     // s⁻¹ (decay constant)
    
    for (int i = 0; i < count; i++) {
        double time = i * 1.0e-6;  // 1 µs sampling interval
        double amplitude = exp(-decayRate * time);
        double phase = 2.0 * M_PI * frequency * time;
        values[i] = amplitude * (cos(phase) + I * sin(phase));
    }
    
    // Create RMNLib dataset and export as CSDM file
    SIDimensionRef freqDim = SIDimensionCreateWithQuantity(kSIQuantityFrequency, &error);
    SIScalarRef increment = SIScalarCreateFromExpression(STR("1.0 µs"), &error);
    SILinearDimensionRef timeDim = SILinearDimensionCreateMinimal(
        kSIQuantityTime,  // quantityName
        count,            // count  
        increment,        // increment
        freqDim,          // reciprocal
        &error);          // outError
    OCRelease(increment);
    OCRelease(freqDim);
    
    OCDataRef data = OCDataCreateWithBytes(values, count * sizeof(double complex));
    OCArrayRef components = OCArrayCreate();
    OCArrayAddValue(components, (OCTypeRef)data);
    SIUnitRef voltUnit = SIUnitFromExpression(STR("V"), NULL, &error);
    DependentVariableRef depVar = DependentVariableCreateMinimal(
        voltUnit,                    // unit
        kSIQuantityElectricPotential, // quantityName
        STR("scalar"),               // quantityType
        kOCNumberComplex128Type,     // numericType
        components,                  // components
        &error);                     // outError
    OCRelease(components);
    OCRelease(data);
    if (!voltUnit || error) goto cleanup;
    
    // Create dataset structure
    OCArrayRef dimensions = OCArrayCreate();
    OCArrayRef dependentVariables = OCArrayCreate();
    OCArrayAddValue(dimensions, (OCTypeRef)timeDim);
    OCArrayAddValue(dependentVariables, (OCTypeRef)depVar);
    OCRelease(timeDim);
    OCRelease(depVar);
    OCRelease(voltUnit);
    
    DatasetRef dataset = DatasetCreateMinimal(
        dimensions,           // dimensions
        dependentVariables,   // dependentVariables
        &error);              // outError
    OCRelease(dimensions);
    OCRelease(dependentVariables);
    
    // Export as CSDM file (binary_dir can be NULL to use same directory as JSON)
    if (dataset && !error) {
        bool success = DatasetExport(
            dataset,              // ds
            "experiment.csdf",    // json_path
            NULL,                 // binary_dir (auto-determined from json_path)
            &error);              // outError
        if (success) {
            printf("Dataset saved to experiment.csdf\n");
        }
    }
    
    // Clean up allocated memory
    free(values);
    if (dataset) OCRelease(dataset);
    if (error) { printf("Error: %s\n", OCStringGetCString(error)); OCRelease(error); }

Monotonic Dimensions
^^^^^^^^^^^^^^^^^^^^

For non-uniform sampling, use monotonic dimensions with explicit coordinates:

.. code-block:: c

    OCStringRef error = NULL;
    
    // T1 inversion recovery with non-uniform time spacing
    // Recovery times span multiple orders of magnitude (µs to seconds)
    OCStringRef timeExpressions[] = {
        STR("10.0 µs"), STR("50.0 µs"), STR("100.0 µs"), STR("500.0 µs"),   
        STR("1.0 ms"), STR("5.0 ms"), STR("10.0 ms"), STR("50.0 ms"), STR("100.0 ms"), STR("500.0 ms"),   
        STR("1.0 s"), STR("2.0 s"), STR("5.0 s"), STR("10.0 s")
    };
    int numPoints = sizeof(timeExpressions) / sizeof(timeExpressions[0]);
    
    // Convert string expressions to SIScalar objects and store in OCArray
    OCMutableArrayRef recoveryTimes = OCArrayCreateMutable(numPoints, &kOCTypeArrayCallBacks);
    for (int i = 0; i < numPoints; i++) {
        SIScalarRef timeScalar = SIScalarCreateFromExpression(timeExpressions[i], &error);
        if (!timeScalar || error) goto cleanup;
        OCArrayAppendValue(recoveryTimes, (const void*)timeScalar);
        OCRelease(timeScalar);  // Array now owns it
    }
    
    // Calculate experimental inversion recovery data: M(t) = M0 * (1 - 2*exp(-t/T1))
    double T1_seconds = 0.5;  // 500 ms relaxation time
    double M0 = 1000.0;       // Initial magnetization
    double *magnetization = malloc(numPoints * sizeof(double));
    
    // Create second unit once for efficiency (reused in loop)
    SIUnitRef secondUnit = SIUnitFromExpression(STR("s"), NULL, &error);
    if (!secondUnit || error) goto cleanup;
    
    // Calculate magnetization values for each recovery time
    for (int i = 0; i < numPoints; i++) {
        // Get time value in seconds from SIScalar in the array
        SIScalarRef timeScalar = (SIScalarRef)OCArrayGetValueAtIndex(recoveryTimes, i);
        double timeInSeconds = SIScalarDoubleValueInUnit(timeScalar, secondUnit);
        magnetization[i] = M0 * (1.0 - 2.0 * exp(-timeInSeconds / T1_seconds));
    }
    
    // Create RMNLib dataset from experimental data
    // We can use the recoveryTimes array directly as coordinates
    SIMonotonicDimensionRef timeDim = SIMonotonicDimensionCreateMinimal(
        kSIQuantityTime,             // quantityName
        (OCArrayRef)recoveryTimes,   // coordinates
        NULL,                        // reciprocal
        &error);                     // outError
    
    OCDataRef magnetizationData = OCDataCreateWithBytes(magnetization, numPoints * sizeof(double));
    OCMutableArrayRef components = OCArrayCreateMutable(1, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(components, (const void*)magnetizationData);
    
    SIUnitRef dimensionlessUnit = SIUnitFromExpression(STR("dimensionless"), NULL, &error);
    DependentVariableRef magnetizationVar = DependentVariableCreateMinimal(
        dimensionlessUnit,       // unit
        kSIQuantityDimensionless, // quantityName
        STR("scalar"),           // quantityType
        kOCNumberFloat64Type,    // numericType
        (OCArrayRef)components,  // components
        &error);                 // outError
    OCRelease(components);
    OCRelease(magnetizationData);
    if (!dimensionlessUnit || error) goto cleanup;
    
    OCMutableArrayRef dimensions = OCArrayCreateMutable(1, &kOCTypeArrayCallBacks);
    OCMutableArrayRef dependentVariables = OCArrayCreateMutable(1, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(dimensions, (const void*)timeDim);
    OCArrayAppendValue(dependentVariables, (const void*)magnetizationVar);
    OCRelease(timeDim);
    OCRelease(magnetizationVar);
    OCRelease(dimensionlessUnit);
    
    DatasetRef inversionRecoveryDataset = DatasetCreateMinimal(
        (OCArrayRef)dimensions,       // dimensions
        (OCArrayRef)dependentVariables, // dependentVariables
        &error);                      // outError
    OCRelease(dimensions);
    OCRelease(dependentVariables);
    
    if (inversionRecoveryDataset && !error) {
        printf("Successfully created T1 inversion recovery dataset\n");
        printf("Time range: 10 µs to 10 s (6 orders of magnitude)\n");
        printf("T1 relaxation time: %.1f ms\n", T1_seconds * 1000);
        
        // Export as CSDM file
        bool success = DatasetExport(
            inversionRecoveryDataset,  // ds
            "T1_recovery.csdf",        // json_path
            NULL,                      // binary_dir (auto-determined)
            &error);                   // outError
        if (success) {
            printf("Dataset saved to T1_recovery.csdf\n");
        }
    }
    
    // Clean up allocated memory
    free(magnetization);
    OCRelease(recoveryTimes);
    if (inversionRecoveryDataset) OCRelease(inversionRecoveryDataset);
    
cleanup:
    if (error) { printf("Error: %s\n", OCStringGetCString(error)); OCRelease(error); }

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

**Best Practice: Release objects immediately when ownership transfers**

.. code-block:: c

    SIScalarRef increment = SIScalarCreateFromExpression(STR("1.0 s"), &error);
    SILinearDimensionRef dimension = SILinearDimensionCreateMinimal(
        kSIQuantityTime, 100, increment, NULL, &error);
    
    OCRelease(increment);  // Release immediately - dimension now owns it
    
    OCArrayRef dimensions = OCArrayCreate();
    OCArrayAddValue(dimensions, (OCTypeRef)dimension);
    
    OCRelease(dimension);  // Release immediately - array now owns it

This pattern provides lower memory pressure, clearer ownership semantics, and easier debugging.

Metadata and Error Handling
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

    // Metadata with JSON serialization
    OCDictionaryRef metadata = OCDictionaryCreate();
    OCDictionarySetValue(metadata, STR("experiment"), STR("T1_measurement"));
    
    cJSON *json = OCMetadataCopyJSON(metadata);
    char *jsonString = cJSON_Print(json);
    
    // Error handling - always check return values
    DatasetRef dataset = DatasetCreate();
    if (!dataset) {
        fprintf(stderr, "Failed to create dataset\n");
        return -1;
    }
    
    // Clean up
    free(jsonString);
    cJSON_Delete(json);
    OCRelease(metadata);
    OCRelease(dataset);
    
    // Library shutdown for leak detection
    RMNLibTypesShutdown();

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
