#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    // double* ptr = sf_malloc(sizeof(double));
    // int *x = sf_malloc(sizeof(int));
    void *x = sf_malloc(16384 - 48 - (sizeof(sf_header) + sizeof(sf_footer)));

    // *x = 8;
    // *ptr = 320320320e-320;
    // *ptr = 123125.512315326457687879;

    // printf("%d\n", *x);

    sf_free(x);

    return EXIT_SUCCESS;
}
