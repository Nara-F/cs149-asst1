#include <immintrin.h>

static const float kThreshold = 0.00001f;

void sqrtAVX(int N, float startGuess, float values[], float output[]) {
    int lanes = N / 8;

    for (int i = 0; i < lanes; i++) {
        __m256 x = _mm256_loadu_ps(values + i * 8);
        __m256 act = _mm256_set1_ps(1.f);
        __m256 guess = _mm256_set1_ps(startGuess);
        __m256 one = _mm256_set1_ps(1.f);
        __m256 pred = _mm256_mul_ps(guess, guess);

        const __m256 signMask = _mm256_set1_ps(-0.0f);

        pred = _mm256_mul_ps(pred, x);
        pred = _mm256_sub_ps(pred, one);
        pred = _mm256_andnot_ps(signMask, pred);

        act = _mm256_cmp_ps(pred, _mm256_set1_ps(kThreshold), _CMP_GT_OS);

        __m256 mid = _mm256_mul_ps(guess, guess);
        mid = _mm256_mul_ps(mid, x);

        while (_mm256_movemask_ps(act)) {
            __m256 newguess = mid;
            newguess = _mm256_sub_ps(_mm256_set1_ps(3.f), newguess);
            newguess = _mm256_mul_ps(_mm256_mul_ps(newguess, guess),_mm256_set1_ps(0.5f));
            guess = _mm256_blendv_ps(guess, newguess, act);

            mid = _mm256_mul_ps(_mm256_mul_ps(guess, guess),x);
            pred = mid;
            pred = _mm256_sub_ps(pred, one);
            pred = _mm256_andnot_ps(signMask, pred);

            act = _mm256_cmp_ps(pred, _mm256_set1_ps(kThreshold), _CMP_GT_OS);
        }

        _mm256_storeu_ps(output + i * 8, _mm256_mul_ps(x, guess));
    }

    // Serial part
    for (int i = lanes * 8; i < N; i++) {
        printf("ENTER SERIAL PART %d\n", i);
        float x = values[i];
        float guess = startGuess;

        float error = fabs(guess * guess * x - 1.f);

        while (error > kThreshold) {
            guess = (3.f * guess - x * guess * guess * guess) * 0.5f;
            error = fabs(guess * guess * x - 1.f);
        }

        output[i] = x * guess;
    }
}