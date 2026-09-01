/*
 * Task 1: The Golden Measure on the PC
 *
 * Integer square root of a 32-bit unsigned input x: the largest integer
 * whose square does not exceed x. Golden version uses double precision
 * arithmetic and the standard library square root.
 *
 * Prediction (written before running):
 *   On a ~3 GHz x86-64 CPU, sqrt + floor + conversion is roughly
 *   20..40 instructions. Estimate: 40 instructions * 0.33 ns/instr
 *   ~= 13 ns per call. One call alone cannot be timed reliably because
 *   clock_gettime resolution and normal operating-system scheduling noise
 *   can be comparable to or larger than the execution time of one call.
 *   Therefore many calls are timed and the total is divided by the number
 *   of repetitions.
 *
 * Build:   gcc -O2 -o golden_measure golden_measure.c -lm
 * Run:     ./golden_measure
 */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>

static const uint32_t inputs[10] = {
    0u, 1u, 15u, 16u, 4095u, 65535u,
    123456789u, 987654321u, 4294836225u, 4294967295u
};

static uint32_t golden_isqrt(uint32_t x)
{
    return (uint32_t)floor(sqrt((double)x));
}

static double timestamp_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ((double)ts.tv_sec * 1000000.0)
         + ((double)ts.tv_nsec / 1000.0);
}

/* Hand check: r^2 <= x < (r+1)^2. */
static int hand_check(uint32_t x, uint32_t r)
{
    uint64_t r_squared =
        (uint64_t)r * (uint64_t)r;

    uint64_t next_squared =
        (uint64_t)(r + 1u) * (uint64_t)(r + 1u);

    return (r_squared <= (uint64_t)x)
        && (next_squared > (uint64_t)x);
}

static double time_n_calls(long reps)
{
    volatile uint32_t x = 987654321u;
    volatile uint32_t result = 0u;

    double start = timestamp_us();

    for (long i = 0; i < reps; i++)
    {
        result = golden_isqrt(x);
    }

    double end = timestamp_us();

    /*
     * result is volatile so the compiler cannot simply remove
     * the calls because their return values are otherwise unused.
     */
    (void)result;

    /* timestamp_us() gives microseconds; convert to ns per call. */
    return ((end - start) * 1000.0) / (double)reps;
}

int main(void)
{
    printf("Golden output table\n");
    printf("-------------------\n");
    printf("%12s %10s %8s\n",
           "Input", "isqrt", "Check");

    for (int i = 0; i < 10; i++)
    {
        uint32_t r = golden_isqrt(inputs[i]);

        printf("%12" PRIu32 " %10" PRIu32 " %8s\n",
               inputs[i],
               r,
               hand_check(inputs[i], r) ? "PASS" : "FAIL");
    }

    /*
     * Full hand check for the input specifically named
     * in the practical question.
     */
    uint32_t x = 987654321u;
    uint32_t r = golden_isqrt(x);

    uint64_t r_squared =
        (uint64_t)r * (uint64_t)r;

    uint64_t next_squared =
        (uint64_t)(r + 1u) * (uint64_t)(r + 1u);

    printf("\nHand check for %" PRIu32 "\n", x);
    printf("-------------------------\n");

    printf("%" PRIu32 "^2 = %" PRIu64
           " <= %" PRIu32 "\n",
           r,
           r_squared,
           x);

    printf("%" PRIu32 "^2 = %" PRIu64
           " > %" PRIu32 "\n",
           r + 1u,
           next_squared,
           x);

    /*
     * Two timing runs with different repetition counts.
     */
    const long reps1 = 1000000L;
    const long reps2 = 5000000L;

    double time1 = time_n_calls(reps1);
    double time2 = time_n_calls(reps2);

    double mean = (time1 + time2) / 2.0;
    double spread = fabs(time1 - time2);

    printf("\nTiming results\n");
    printf("--------------\n");

    printf("Run 1: %ld repetitions\n", reps1);
    printf("       %.3f ns/call\n", time1);

    printf("Run 2: %ld repetitions\n", reps2);
    printf("       %.3f ns/call\n", time2);

    printf("Mean:   %.3f ns/call\n", mean);
    printf("Spread: %.3f ns/call\n", spread);

    printf("\nTask 1 answer:\n");
    printf("Golden output for 987654321 = %" PRIu32 "\n", r);
    printf("Repetition counts = %ld and %ld\n", reps1, reps2);

    return 0;
}