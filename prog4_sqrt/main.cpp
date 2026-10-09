#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <cmath>
#include <cstring>

#include "CycleTimer.h"
#include "sqrtAVX.h"
#include "sqrt_ispc.h"

using namespace ispc;

extern void sqrtSerial(int N, float startGuess, float* values, float* output);

static void verifyResult(int N, float* result, float* gold) {
    for (int i = 0; i < N; i++) {
        if (fabs(result[i] - gold[i]) > 1e-4) {
            printf("Error: [%d] Got %f expected %f\n", i, result[i], gold[i]);
        }
    }
}

void usage(const char* progname) {
    printf("Usage: %s [options]\n", progname);
    printf("Program Options:\n");
    printf(
        "  -v, --value <float>   set all input values (0 < value < 3, default "
        "1.0)\n");
    printf("  -h, -?, --help        This message\n");
}

int main(int argc, char** argv) {
    const unsigned int N = 20 * 1000 * 1000;
    const float initialGuess = 1.0f;
    float initVal = 1.0f;

    int opt;
    static const struct option longOptions[] = {
        {"value", required_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}};
    opterr = 0;

    while ((opt = getopt_long(argc, argv, ":v:h?", longOptions, NULL)) != -1) {
        switch (opt) {
            case 'v': {
                char* end = NULL;
                errno = 0;
                float value = strtof(optarg, &end);
                if (end == optarg || *end != '\0' || errno == ERANGE ||
                    !std::isfinite(value) || value <= 0.0f || value >= 3.0f) {
                    fprintf(stderr,
                            "Invalid input value '%s': expected a finite "
                            "number with 0 < value < 3.\n",
                            optarg);
                    return 1;
                }
                initVal = value;
                break;
            }
            case 'h':
                usage(argv[0]);
                return 0;
            case '?':
                if (optopt == 0 && strcmp(argv[optind - 1], "-?") == 0) {
                    usage(argv[0]);
                    return 0;
                }
                fprintf(stderr, "Unknown option. Use --help for usage.\n");
                return 1;
            case ':':
                fprintf(stderr, "Option --value/-v requires a number.\n");
                return 1;
            default:
                return 1;
        }
    }

    if (optind != argc) {
        fprintf(stderr, "Unexpected argument '%s'. Use --help for usage.\n",
                argv[optind]);
        return 1;
    }

    float* values = new float[N];
    float* output = new float[N];
    float* gold = new float[N];

    for (unsigned int i = 0; i < N; i++) {
        // TODO: CS149 students.  Attempt to change the values in the
        // array here to meet the instructions in the handout: we want
        // to you generate best and worse-case speedups

        // Fill the input with the value selected by --value/-v.
        values[i] = initVal;
    }

    // generate a gold version to check results
    for (unsigned int i = 0; i < N; i++) gold[i] = sqrt(values[i]);

    //
    // And run the serial implementation 3 times, again reporting the
    // minimum time.
    //
    double minSerial = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrtSerial(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minSerial = std::min(minSerial, endTime - startTime);
    }

    printf("[sqrt serial]:\t\t[%.3f] ms\n", minSerial * 1000);

    verifyResult(N, output, gold);

    //
    // Compute the image using the ispc implementation; report the minimum
    // time of three runs.
    //
    double minISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrt_ispc(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minISPC = std::min(minISPC, endTime - startTime);
    }

    printf("[sqrt ispc]:\t\t[%.3f] ms\n", minISPC * 1000);

    verifyResult(N, output, gold);

    // Clear out the buffer
    for (unsigned int i = 0; i < N; ++i) output[i] = 0;

    //
    // Tasking version of the ISPC code
    //
    double minTaskISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrt_ispc_withtasks(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minTaskISPC = std::min(minTaskISPC, endTime - startTime);
    }

    printf("[sqrt task ispc]:\t[%.3f] ms\n", minTaskISPC * 1000);

    verifyResult(N, output, gold);

    double minAVX = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrtAVX(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minAVX = std::min(minAVX, endTime - startTime);
    }

    printf("[sqrt avx]:\t\t[%.3f] ms\n", minAVX * 1000);

    verifyResult(N, output, gold);
    printf("\t\t\t\t(%.2fx speedup from ISPC)\n", minSerial / minISPC);
    printf("\t\t\t\t(%.2fx speedup from task ISPC)\n", minSerial / minTaskISPC);
    printf("\t\t\t\t(%.2fx speedup from AVX)\n", minSerial / minAVX);
    printf("\t\t\t\t(AVX is %.2fx faster than ISPC)\n", minISPC / minAVX);
    if (minISPC < minAVX) {
        printf("[FAILED]\n");
    } else {
        printf("[PASSED]\n");
    }

    delete[] values;
    delete[] output;
    delete[] gold;

    return 0;
}
