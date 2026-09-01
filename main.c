/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * EEE3096S 2026 - Practical 1B
  * Tasks 2 and 3: fast integer square root, TIM16 timing, optimisation flags
  *
  * Student 1 : Thapelo MCHTHA046
  * Student 2 : Velan PLLVEL003
  * Date      : 1 September 2026
  *
  * Board pins used
  *   PC13 : scope pulse. Driven LOW for the timed section, HIGH otherwise.
  *          Broken out on Header P1.
  *   PB1  : pass or fail indicator. User LED 1.
  *          ON means all ten golden values matched.
  ******************************************************************************
  */
/* USER CODE END Header */


/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdint.h>
/* USER CODE END Includes */


/* USER CODE BEGIN PD */

#define PULSE_PIN      13u
#define LED_PIN        1u
#define TEST_INPUT     987654321u
#define LONG_RUN_N     20000u

/* USER CODE END PD */


/* USER CODE BEGIN PV */

/* Ten inputs from Task 1. */
static const uint32_t golden_inputs[10] =
{
    0u,
    1u,
    15u,
    16u,
    4095u,
    65535u,
    123456789u,
    987654321u,
    4294836225u,
    4294967295u
};


/* Golden outputs obtained from Task 1. */
static const uint32_t golden_outputs[10] =
{
    0u,
    1u,
    3u,
    4u,
    63u,
    255u,
    11111u,
    31426u,
    65535u,
    65535u
};


/*
 * Results visible in STM32CubeIDE Live Expressions.
 */
volatile uint8_t  pass_all         = 0u;
volatile uint32_t single_call_span = 0u;
volatile uint32_t long_run_span    = 0u;
volatile float    mean_us_per_call = 0.0f;


/*
 * Volatile sink prevents the return value from being discarded.
 */
static volatile uint32_t sink = 0u;


/*
 * Runtime input.
 *
 * Volatile prevents TEST_INPUT from being propagated as a compile-time
 * constant through the timed measurement.
 */
static volatile uint32_t test_input = TEST_INPUT;

/* USER CODE END PV */


/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);

/* USER CODE BEGIN PFP */

static void gpio_init(void);
static void timing_timer_init(void);

static uint32_t isqrt(uint32_t x);
static uint32_t time_one_call(uint32_t x);
static uint32_t time_n_calls(uint32_t x, uint32_t n);

/* USER CODE END PFP */


/* USER CODE BEGIN 0 */


/* -------------------------------------------------------------------------- */
/* GPIO INITIALISATION                                                        */
/* -------------------------------------------------------------------------- */

static void gpio_init(void)
{
    /*
     * Enable GPIOB and GPIOC peripheral clocks.
     */
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN |
                   RCC_AHBENR_GPIOCEN;


    /*
     * PC13 = general-purpose output.
     */
    GPIOC->MODER &= ~(3UL << (PULSE_PIN * 2u));
    GPIOC->MODER |=  (1UL << (PULSE_PIN * 2u));


    /*
     * PB1 = general-purpose output.
     */
    GPIOB->MODER &= ~(3UL << (LED_PIN * 2u));
    GPIOB->MODER |=  (1UL << (LED_PIN * 2u));


    /*
     * Initial states:
     *
     * PC13 HIGH = timing pulse inactive.
     * PB1 LOW   = LED off until self-test passes.
     */
    GPIOC->BSRR = (1UL << PULSE_PIN);
    GPIOB->BRR  = (1UL << LED_PIN);
}


/* -------------------------------------------------------------------------- */
/* TIM16 INITIALISATION                                                       */
/* -------------------------------------------------------------------------- */

static void timing_timer_init(void)
{
    /*
     * Enable TIM16 peripheral clock.
     */
    RCC->APB2ENR |= RCC_APB2ENR_TIM16EN;


    /*
     * Clock configuration:
     *
     * HSI               = 8 MHz
     * AHB divider       = 1
     * APB divider       = 1
     * TIM16 PSC         = 0
     *
     * Therefore:
     *
     * timer clock
     *      = 8 MHz / (0 + 1)
     *      = 8 MHz
     *
     * One timer count
     *      = 1 / 8 MHz
     *      = 0.125 us
     *      = 125 ns
     */
    TIM16->PSC = 0u;


    /*
     * Full 16-bit free-running counter.
     */
    TIM16->ARR = 0xFFFFu;


    /*
     * Start counter from zero.
     */
    TIM16->CNT = 0u;


    /*
     * Generate update event so PSC is loaded.
     */
    TIM16->EGR = TIM_EGR_UG;


    /*
     * Update event can alter CNT, therefore clear it again.
     */
    TIM16->CNT = 0u;


    /*
     * Enable TIM16.
     */
    TIM16->CR1 |= TIM_CR1_CEN;
}


/* -------------------------------------------------------------------------- */
/* INTEGER SQUARE ROOT                                                        */
/* -------------------------------------------------------------------------- */

/*
 * Return non-zero when:
 *
 *      mid^2 <= x
 *
 * The multiplication is promoted to 64 bits so that mid * mid cannot
 * overflow a 32-bit integer.
 *
 * Keeping this helper inline allows the optimiser to potentially inline it
 * into isqrt(), which can be inspected in the -O2 disassembly.
 */
static inline uint32_t square_le(uint32_t mid, uint32_t x)
{
    return ((uint64_t)mid * (uint64_t)mid <= (uint64_t)x);
}


/*
 * Prevent isqrt() itself from being absorbed into the timing harness.
 *
 * The same source code must be used for all optimisation levels.
 */
__attribute__((noinline))
static uint32_t isqrt(uint32_t x)
{
    uint32_t low = 0u;
    uint32_t high = 65535u;
    uint32_t answer = 0u;


    while (low <= high)
    {
        uint32_t mid;

        mid = low + ((high - low) >> 1);


        if (square_le(mid, x))
        {
            answer = mid;
            low = mid + 1u;
        }
        else
        {
            if (mid == 0u)
            {
                break;
            }

            high = mid - 1u;
        }
    }


    return answer;
}


/* -------------------------------------------------------------------------- */
/* SINGLE-CALL TIMING                                                         */
/* -------------------------------------------------------------------------- */

/*
 * Time one isqrt() call.
 *
 * PC13 goes LOW before the timing window and HIGH afterwards.
 *
 * TIM16 is read immediately before and immediately after the calculation.
 *
 * The empty inline-assembly statements are COMPILER barriers. They generate
 * no useful runtime work but prevent high optimisation levels such as -Os
 * from moving the pure calculation outside the timer-read boundaries.
 */
static uint32_t time_one_call(uint32_t x)
{
    uint16_t a;
    uint16_t b;

    uint32_t result;


    /*
     * Start oscilloscope pulse.
     */
    GPIOC->BRR = (1UL << PULSE_PIN);


    /*
     * Starting TIM16 count.
     */
    a = (uint16_t)TIM16->CNT;


    /*
     * Compiler ordering barrier.
     *
     * x is specified as an input/output operand, which creates a dependency
     * preventing isqrt(x) from being calculated before this point.
     */
    __asm volatile(
        ""
        : "+r" (x)
        :
        : "memory"
    );


    /*
     * Code under test.
     */
    result = isqrt(x);


    /*
     * result is an input to this compiler barrier, forcing the complete
     * isqrt() calculation to occur before this point.
     */
    __asm volatile(
        ""
        :
        : "r" (result)
        : "memory"
    );


    /*
     * Preserve result so optimiser cannot remove calculation.
     */
    sink = result;


    /*
     * Ending TIM16 count.
     */
    b = (uint16_t)TIM16->CNT;


    /*
     * End oscilloscope pulse.
     */
    GPIOC->BSRR = (1UL << PULSE_PIN);


    /*
     * 16-bit subtraction automatically performs modulo-65536 arithmetic.
     *
     * Example:
     *
     * a = 65500
     * b = 20
     *
     * elapsed =
     *      (uint16_t)(20 - 65500)
     *      = 56 counts
     */
    return (uint32_t)((uint16_t)(b - a));
}


/* -------------------------------------------------------------------------- */
/* LONG-RUN / WRAP-AROUND TIMING                                              */
/* -------------------------------------------------------------------------- */

/*
 * Time N calls.
 *
 * Every individual isqrt() call is measured using TIM16.
 *
 * The individual modulo-65536 differences are accumulated into a 32-bit
 * software total. Therefore the hardware timer may overflow many times over
 * the complete experiment.
 *
 * One complete TIM16 period:
 *
 *      65536 counts x 0.125 us/count
 *      = 8192 us
 *      = 8.192 ms
 *
 * A single isqrt() call is much shorter than this period, therefore each
 * individual difference remains unambiguous.
 */
static uint32_t time_n_calls(uint32_t x, uint32_t n)
{
    uint16_t a;
    uint16_t b;

    uint32_t result;
    uint32_t total = 0u;


    /*
     * Long-run pulse starts.
     */
    GPIOC->BRR = (1UL << PULSE_PIN);


    for (uint32_t i = 0u; i < n; i++)
    {
        /*
         * Starting count for this individual call.
         */
        a = (uint16_t)TIM16->CNT;


        /*
         * Prevent isqrt() moving before the first timer read.
         */
        __asm volatile(
            ""
            : "+r" (x)
            :
            : "memory"
        );


        /*
         * Code under test.
         */
        result = isqrt(x);


        /*
         * Prevent isqrt() moving beyond the second measurement boundary.
         */
        __asm volatile(
            ""
            :
            : "r" (result)
            : "memory"
        );


        /*
         * Preserve result.
         */
        sink = result;


        /*
         * Ending count for this call.
         */
        b = (uint16_t)TIM16->CNT;


        /*
         * Wrap-safe elapsed count.
         */
        total += (uint32_t)((uint16_t)(b - a));
    }


    /*
     * Long-run pulse ends.
     */
    GPIOC->BSRR = (1UL << PULSE_PIN);


    return total;
}


/* USER CODE END 0 */


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* MCU Configuration ------------------------------------------------------*/

    HAL_Init();

    SystemClock_Config();


    /* USER CODE BEGIN 2 */

    gpio_init();

    timing_timer_init();


    /* ---------------------------------------------------------------------- */
    /* SELF TEST                                                              */
    /* ---------------------------------------------------------------------- */

    pass_all = 1u;


    for (int i = 0; i < 10; i++)
    {
        if (isqrt(golden_inputs[i]) != golden_outputs[i])
        {
            pass_all = 0u;
            break;
        }
    }


    /*
     * PB1 ON when all ten golden values pass.
     */
    if (pass_all)
    {
        GPIOB->BSRR = (1UL << LED_PIN);
    }
    else
    {
        GPIOB->BRR = (1UL << LED_PIN);
    }


    /* USER CODE END 2 */


    /* Infinite loop */
    while (1)
    {
        /* USER CODE BEGIN 3 */


        /* ------------------------------------------------------------------ */
        /* TASK 2 / TASK 3 SINGLE-CALL MEASUREMENT                           */
        /* ------------------------------------------------------------------ */

        single_call_span = time_one_call(test_input);


        /*
         * Conversion:
         *
         * time in microseconds
         *
         *      = single_call_span x 0.125 us
         */


        /* ------------------------------------------------------------------ */
        /* TASK 2 LONG-RUN WRAP TEST                                         */
        /* ------------------------------------------------------------------ */

        /*
         * Leave these COMMENTED OUT while taking the single-call
         * oscilloscope screenshots.
         *
         * Uncomment them only when performing the Task 2 long-run test.
         */


        long_run_span = time_n_calls(test_input, LONG_RUN_N);

        mean_us_per_call =
            ((float)long_run_span * 0.125f)
            / (float)LONG_RUN_N;



        /*
         * Gap between measurements so the scope can trigger on one clean
         * single-call pulse.
         */
        for (volatile int d = 0; d < 100000; d++)
        {
        }


        /* USER CODE END 3 */
    }
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};


    /*
     * Use 8 MHz internal HSI.
     */
    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSI;

    RCC_OscInitStruct.HSIState =
        RCC_HSI_ON;

    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;

    RCC_OscInitStruct.PLL.PLLState =
        RCC_PLL_NONE;


    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }


    /*
     * SYSCLK = HSI = 8 MHz
     * AHB    = SYSCLK / 1
     * APB    = HCLK / 1
     */
    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1;


    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_HSI;


    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;


    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV1;


    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();


    while (1)
    {
    }
}


#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}

#endif /* USE_FULL_ASSERT */


/*
 * ---------------------------------------------------------------------------
 * TASK 3 CHECKLIST
 * ---------------------------------------------------------------------------
 *
 * USE THIS EXACT SAME SOURCE FILE FOR:
 *
 *      -O0
 *      -O1
 *      -O2
 *      -Os
 *
 *
 * At every optimisation level record:
 *
 *      1. Text size in bytes
 *      2. single_call_span
 *      3. Counter-derived time
 *      4. PC13 scope pulse width
 *
 *
 * Timer conversion:
 *
 *      timer clock = 8 MHz
 *
 *      one count =
 *
 *          1 / 8 MHz
 *
 *          = 0.125 us
 *
 *
 *      time_us =
 *
 *          single_call_span x 0.125
 *
 *
 * At -O2 inspect:
 *
 *      Debug/Practical1B.list
 *
 * or run:
 *
 * arm-none-eabi-objdump -d Debug/Practical1B.elf > disasm_O2.txt
 *
 *
 * Search for:
 *
 *      isqrt
 *
 * and determine one genuine optimisation transformation from YOUR build.
 *
 * Do not claim square_le() inlining unless the disassembly actually shows it.
 *
 * ---------------------------------------------------------------------------
 */
