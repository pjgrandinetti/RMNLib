#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

// Declare test functions
void test_RMNDatum(void);

int main(void) {
    printf("=== Running RMNDatum Tests ===\n");

    test_RMNDatum();

    printf("=== All RMNDatum tests passed ===\n");
    return 0;
}
