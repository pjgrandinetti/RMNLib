#include "RMNLibrary.h"

void RMNLibTypesShutdown(void) {
    fprintf(stderr, "Cleaning up SITypes...\n");
    SITypesShutdown();
    fprintf(stderr, "Cleaning up OCTypes...\n");
    OCTypesShutdown();
}

// If you want automatic teardown when the library is unloaded:
// __attribute__((destructor(100)))
// static void __RMNLibCleanup(void) {
//     RMNLibTypesShutdown();
// }
