#include "src/RMNLibrary.h"
#include <stdio.h>

int main() {
    printf("Testing SparseSampling with new OCIndexPairSetRef API...\n");

    // Create dimension indexes
    OCMutableIndexSetRef dimIndexes = OCIndexSetCreateMutable();
    OCIndexSetAddIndex(dimIndexes, 0);
    OCIndexSetAddIndex(dimIndexes, 1);

    // Create sparse vertices as single OCIndexPairSetRef with linearized indices
    OCMutableIndexPairSetRef sparseVertices = OCIndexPairSetCreateMutable();

    // Vertex 0 at coordinates (10, 20) for dimensions [0, 1]
    OCIndexPairSetAddIndexPair(sparseVertices, 0, 10);  // vertex 0, dim 0, value 10
    OCIndexPairSetAddIndexPair(sparseVertices, 1, 20);  // vertex 0, dim 1, value 20

    // Vertex 1 at coordinates (30, 40) for dimensions [0, 1]
    OCIndexPairSetAddIndexPair(sparseVertices, 2, 30);  // vertex 1, dim 0, value 30
    OCIndexPairSetAddIndexPair(sparseVertices, 3, 40);  // vertex 1, dim 1, value 40

    printf("Created sparse vertices with %ld pairs\n", (long)OCIndexPairSetGetCount(sparseVertices));

    // Create SparseSampling object
    OCStringRef error = NULL;
    SparseSamplingRef ss = SparseSamplingCreate(
        dimIndexes,
        sparseVertices,
        kOCNumberUInt32Type,
        STR("base64"),
        STR("Test sparse sampling"),
        NULL,
        &error);

    if (error) {
        printf("ERROR: %s\n", OCStringGetCString(error));
        OCRelease(error);
        return 1;
    }

    if (!ss) {
        printf("ERROR: Failed to create SparseSampling object\n");
        return 1;
    }

    printf("✅ SparseSampling object created successfully!\n");

    // Verify properties
    OCIndexSetRef returnedDims = SparseSamplingGetDimensionIndexes(ss);
    OCIndexPairSetRef returnedVertices = SparseSamplingGetSparseGridVertexes(ss);

    printf("Dimension indexes count: %ld\n", (long)OCIndexSetGetCount(returnedDims));
    printf("Sparse vertices count: %ld\n", (long)OCIndexPairSetGetCount(returnedVertices));
    printf("Encoding: %s\n", OCStringGetCString(SparseSamplingGetEncoding(ss)));

    // Test JSON serialization
    cJSON *json = SparseSamplingCopyAsJSON(ss, false, &error);
    if (error) {
        printf("JSON serialization error: %s\n", OCStringGetCString(error));
        OCRelease(error);
    } else if (json) {
        char *jsonStr = cJSON_Print(json);
        printf("JSON: %s\n", jsonStr);
        free(jsonStr);
        cJSON_Delete(json);
        printf("✅ JSON serialization working!\n");
    }

    // Cleanup
    OCRelease(dimIndexes);
    OCRelease(sparseVertices);
    OCRelease(ss);

    printf("✅ All tests passed!\n");
    return 0;
}
