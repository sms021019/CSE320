#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    // double* ptr = sf_malloc(sizeof(double));
    // int *x = sf_malloc(sizeof(int));
    // void *x = sf_malloc(4048);

    // *x = 8;
    // *ptr = 320320320e-320;
    // *ptr = 123125.512315326457687879;

    // printf("%d\n", *x);

    // sf_free(x);

    void *u = sf_malloc(200);
    /* void *v = */ sf_malloc(300);
    void *w = sf_malloc(200);
    /* void *x = */ sf_malloc(500);
    void *y = sf_malloc(200);
    /* void *z = */ sf_malloc(700);

    sf_free(u);
    sf_free(w);
    sf_free(y);



    return EXIT_SUCCESS;
}
