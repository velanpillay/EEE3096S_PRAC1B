<<<<<<< HEAD
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * EEE3096S 2026 - Practical 1B
  * Tasks 4 and 5: cycle-counted phase delay, and LCD analog debugging
  *
  * Student 1 : <THAPELO MOCHEKO>  <MCHTHA046>
  * Student 2 : <VELAN PILLAY>  <PLLVEL003>
  * Date      : <24/08/2026>
  *
  * This file starts the peripherals and hands control to your Assembly.
  * The work happens in Core/Src/dsp.s (Task 4) and Core/Src/lcd.s (Task 5).
  *
  * Task 4 pins
  *   PB0  : ADC input, signal generator. Remove the POT0 jumper first.
  *   PA4  : DAC1 output, scope CH2.
  *
  * Task 5 pins
  *   PC15 : LCD Enable, 3.3 V side. Scope CH1.
  *   PC14 : LCD Register Select.
  *   PB8  : LCD D4      PB9  : LCD D5
  *   PA12 : LCD D6      PA15 : LCD D7
  *
  * Set ACTIVE_TASK below, rebuild, and flash.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN PD */

/* Pick the task to run: 4 for the phase delay, 5 for the LCD. */
#define ACTIVE_TASK 5

/*
 * ADC channel for Task 4.
 * ADC_CHANNEL_8 is PB0, the pin named in the practical sheet.
 * PB0 also drives user LED D1 through a 150 ohm resistor, so the LED loads
 * the signal generator and clips the top of the wave. Measure the input on
 * CH1 and report what you see. If your bench setup uses PA1 instead,
 * change this to ADC_CHANNEL_1 and update the .ioc to match.
 */
#define ADC_INPUT_CHANNEL   ADC_CHANNEL_8

/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;
DAC_HandleTypeDef hdac1;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC_Init(void);
static void MX_DAC1_Init(void);

/* USER CODE BEGIN PFP */
extern void DSP_Loop(void);   /* Task 4, defined in dsp.s. Never returns. */
extern void LCD_Run(void);    /* Task 5, defined in lcd.s. Never returns. */
/* USER CODE END PFP */

/**
  * @brief  The application entry point.
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_ADC_Init();
  MX_DAC1_Init();

  /* USER CODE BEGIN 2 */

#if (ACTIVE_TASK == 4)

  /*
   * TODO 1
   * Start the ADC in continuous mode and start DAC channel 1, then hand
   * over to the Assembly loop.
   *
   * HAL_ADC_Start(&hadc);
   * HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
   * DSP_Loop();
   *
   * Two settings in the .ioc decide whether this works at all:
   *   Continuous Conversion Mode must be Enabled, or the ADC converts once
   *   and stops, and your DAC output sits flat.
   *   Overrun must be set to "Overrun data overwritten", or the ADC halts
   *   the moment your Assembly reads it slower than it converts.
   */


  HAL_ADC_Start(&hadc);
  HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
  DSP_Loop();

#elif (ACTIVE_TASK == 5)

  /*
   * TODO 2
   * Hand over to the LCD routine.
   *
   * LCD_Run();
   *
   * The LCD needs its power rail settled before the initialisation
   * sequence starts. Add the wait inside lcd.s, not here.
   */

  LCD_Run();

#else
  #error "Set ACTIVE_TASK to 4 or 5"
#endif

  /* USER CODE END 2 */

  while (1)
  {
    /* Your Assembly routine never returns, so nothing runs here. */
  }
}

/**
  * @brief System Clock Configuration
  * @note  HSI at 8 MHz, no PLL. One CPU cycle is 125 ns.
  *        Every cycle count in dsp.s and lcd.s depends on this. Leave it.
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI
                                   | RCC_OSCILLATORTYPE_HSI14;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialisation. Task 4.
  */
static void MX_ADC_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  hadc.Init.ContinuousConvMode = ENABLE;          /* keep enabled */
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;   /* keep overwritten */
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_INPUT_CHANNEL;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief DAC1 Initialisation. Task 4. Output on PA4.
  */
static void MX_DAC1_Init(void)
{
  DAC_ChannelConfTypeDef sConfig = {0};

  hdac1.Instance = DAC;
  if (HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialisation. Task 5 LCD pins.
  * @note  Output speed stays at HIGH on purpose. Drop it to LOW and the
  *        internal slew rate limiting hides the level shifter fault you
  *        are asked to find.
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* Start every LCD line low */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12 | GPIO_PIN_15, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8  | GPIO_PIN_9,  GPIO_PIN_RESET);

  /* PC14 RS, PC15 Enable */
  GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* PA12 D6, PA15 D7 */
  GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* PB8 D4, PB9 D5 */
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
=======
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * EEE3096S 2026 - Practical 1B
  * Tasks 2 and 3: fast integer square root, TIM16 timing, optimisation flags
  *
  * Student 1 : <name>  <student number>
  * Student 2 : <name>  <student number>
  * Date      : <date>
  *
  * Board pins used
  *   PC13 : scope pulse. Driven LOW for the timed section, HIGH otherwise.
  *          Broken out on Header P1.
  *   PB1  : pass or fail indicator. User LED 1. ON means all ten golden
  *          values matched.
  *
  * Search for TODO. Every TODO is a piece of work you have to complete.
  * Do not delete the USER CODE markers. STM32CubeIDE overwrites everything
  * outside them whenever you regenerate from the .ioc file.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdint.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PD */
#define PULSE_PIN    13u          /* PC13 */
#define LED_PIN      1u           /* PB1  */

#define TEST_INPUT   987654321u   /* the input named in the Task 2 question */
#define LONG_RUN_N   20000u       /* calls in the wrap-around run           */
/* USER CODE END PD */

/* USER CODE BEGIN PV */

/* The ten inputs from Task 1. Do not change these. */
static const uint32_t golden_inputs[10] = {
    0u, 1u, 15u, 16u, 4095u, 65535u,
    123456789u, 987654321u, 4294836225u, 4294967295u
};

/*
 * TODO 1
 * Fill this array with the ten outputs produced by YOUR Task 1 golden
 * measure. Copy them from your own PC run, not from a friend and not from
 * the practical sheet. The firmware self-test below compares against these.
 */
static const uint32_t golden_outputs[10] = {
    0u, 1u, 3u, 4u, 63u, 255u,
    11111u, 31426u, 65535u, 65535u
};

/*
 * Results. Keep these volatile so the optimiser leaves them alone at -O1
 * and above. Read them in the STM32CubeIDE Live Expressions view.
 */
volatile uint8_t  pass_all          = 0u;   /* 1 means all ten matched      */
volatile uint32_t single_call_span  = 0u;   /* timer counts, one call       */
volatile uint32_t long_run_span     = 0u;   /* timer counts, LONG_RUN_N     */
volatile float    mean_us_per_call  = 0.0f; /* long run divided by N        */

/* Sink for the return value. Stops the optimiser deleting the call. */
static volatile uint32_t sink = 0u;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
static void     gpio_init(void);
static void     timing_timer_init(void);
static uint32_t isqrt(uint32_t x);
static uint32_t time_one_call(uint32_t x);
static uint32_t time_n_calls(uint32_t x, uint32_t n);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* ---------------------------------------------------------------------------
 * Hardware initialisation
 * ------------------------------------------------------------------------ */
static void gpio_init(void)
{
    /*
     * TODO 2
     * Enable the peripheral clock for GPIOC and GPIOB.
     *
     * Look up the correct RCC enable register in RM0091 Section 6, Reset
     * and Clock Control, and name the register in your report.
     *
     * RCC->???ENR |= ... ;
     */
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;

    /*
     * TODO 3
     * Put PC13 and PB1 into general purpose output mode.
     * MODER holds two bits per pin. Clear both bits first, then set the
     * output pattern. Leave every other pin untouched.
     */
    GPIOC->MODER &= ~(3UL << (PULSE_PIN * 2u));
    GPIOC->MODER |=  (1UL << (PULSE_PIN * 2u));

    GPIOB->MODER &= ~(3UL << (LED_PIN * 2u));
    GPIOB->MODER |=  (1UL << (LED_PIN * 2u));

    /*
     * TODO 4
     * Set the idle states: PC13 HIGH (pulse is active low) and PB1 LOW
     * (LED off until the self-test passes).
     * BSRR sets a pin. BRR clears a pin.
     */
    GPIOC->BSRR = (1UL << PULSE_PIN);
    GPIOB->BRR  = (1UL << LED_PIN);
}

static void timing_timer_init(void)
{
    /*
     * TODO 5
     * Enable the TIM16 peripheral clock. TIM16 and the GPIO ports sit on
     * different buses on this device. Name both buses in your report.
     */
    RCC->APB2ENR |= RCC_APB2ENR_TIM16EN;

    /*
     * TODO 6
     * Set the prescaler so one timer count equals a time you choose and
     * state. The division factor is PSC + 1, so:
     *
     *     counter clock = timer clock / (PSC + 1)
     *
     * Work out the timer clock from the path HSI -> AHB prescaler ->
     * APB prescaler -> TIM16. Write the full path and every divider into
     * your report before you pick the number.
     *
     * TIM16->PSC = ??? ;
     */
    TIM16->PSC = 0u;

    /*
     * TODO 7
     * Set ARR for a free running 16-bit counter, force the prescaler to
     * load with an update event, then enable the counter.
     */
    TIM16->ARR = 0xFFFFu;
    TIM16->CNT = 0u;
    TIM16->EGR = TIM_EGR_UG;
    TIM16->CR1 |= TIM_CR1_CEN;
}

/* ---------------------------------------------------------------------------
 * Task 2 core algorithm
 * ------------------------------------------------------------------------ */

/*
 * Helper. Returns non-zero when mid * mid is at or below x.
 *
 * Keep this as a separate function. Task 3 asks you to find one
 * optimisation transformation in the disassembly, and the treatment of
 * this helper at -O2 is the easiest one to spot and name.
 *
 * Note: mid * mid overflows 32 bits for large mid. Promote before you
 * multiply.
 */
static inline uint32_t square_le(uint32_t mid, uint32_t x)
{
    /* TODO 8: return the comparison result. */
    return ((uint64_t)mid * (uint64_t)mid <= (uint64_t)x);
}

/*
 * Integer square root. Returns the largest r with r * r <= x, for every
 * x from 0 to 4294967295.
 */
static uint32_t isqrt(uint32_t x)
{
    /*
     * TODO 9
     * Implement a fast integer square root. A binary search over the
     * answer range works well and is simple to reason about.
     *
     */
    uint32_t low = 0u;
    uint32_t high = 65535u;
    uint32_t answer = 0u;

    while (low <= high)
    {
        uint32_t mid = low + ((high - low) >> 1);

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

/* ---------------------------------------------------------------------------
 * Timing harness
 * ------------------------------------------------------------------------ */

/*
 * Times one call.
 * Drives PC13 LOW for the timed window so the scope pulse and the counter
 * span cover the same code.
 * Returns the elapsed counter span.
 */
static uint32_t time_one_call(uint32_t x)
{
    uint16_t a = 0u;
    uint16_t b = 0u;

    GPIOC->BRR = (1UL << PULSE_PIN);   /* PC13 low: pulse starts */

    /* TODO 10: capture the counter into a. Which register holds the count? */
    a = (uint16_t)TIM16->CNT;

    sink = isqrt(x);                   /* the code under test */

    /* TODO 11: capture the counter into b. */
    b = (uint16_t)TIM16->CNT;

    GPIOC->BSRR = (1UL << PULSE_PIN);  /* PC13 high: pulse ends */

    /*
     * TODO 12
     * Return the elapsed span. The counter wraps at its top value during
     * long runs, so a plain b minus a is wrong once the counter rolls over.
     * Work out an expression correct across a wrap and explain it in your
     * report. Test your reasoning on a = 65500, b = 20.
     */
    return (uint32_t)((uint16_t)(b - a));
}

/*
 * Times n calls back to back so the counter crosses its overflow more
 * than once. Same arithmetic as the single call version.
 */
static uint32_t time_n_calls(uint32_t x, uint32_t n)
{
    uint16_t a = 0u;
    uint16_t b = 0u;
    uint32_t total = 0u;

    GPIOC->BRR = (1UL << PULSE_PIN);

    /* TODO 13: capture the counter into a. */
    a = (uint16_t)TIM16->CNT;

    for (uint32_t i = 0u; i < n; i++)
    {
        sink = isqrt(x);

        b = (uint16_t)TIM16->CNT;
        total += (uint32_t)((uint16_t)(b - a));
        a = b;
    }

    /* TODO 14: capture the counter into b. */
    b = (uint16_t)TIM16->CNT;
    total += (uint32_t)((uint16_t)(b - a));

    GPIOC->BSRR = (1UL << PULSE_PIN);

    /*
     * TODO 15
     * Return the elapsed span using the same wrap-safe expression.
     *
     * Careful: a 16-bit counter measures a limited window without
     * ambiguity. Work out that window from your prescaler, then pick n so
     * the total run stays inside a single unambiguous window, or track the
     * overflows yourself. State your choice in the report.
     */
    return total;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  */
int main(void)
{
  /* MCU Configuration -------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* USER CODE BEGIN 2 */
  gpio_init();
  timing_timer_init();

  /* Self-test against the ten golden values from Task 1 */
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
   * TODO 16
   * Drive PB1 from pass_all. LED on for a pass, off for a fail.
   * The demonstrator checks this LED before anything else.
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

    /* Task 2 and Task 3: single call measurement */
    single_call_span = time_one_call(TEST_INPUT);

    /*
     * TODO 17
     * Task 2 wrap case: run the long measurement, then work out the mean
     * time per call and confirm it agrees with the single call figure.
     *
     * Comment this out while you place the scope cursors on the single
     * call pulse. Two pulses of very different widths on one pin make the
     * scope trigger jump.
     */
    /* long_run_span = time_n_calls(TEST_INPUT, LONG_RUN_N); */
    /* mean_us_per_call = ((float)long_run_span * 0.125f) / (float)LONG_RUN_N; */

    /* Gap between measurements so the scope has a clean single pulse */
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

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */

/*
 * ---------------------------------------------------------------------------
 * TASK 3 CHECKLIST. No code changes needed below this line.
 * ---------------------------------------------------------------------------
 *
 * Build this same file four times, once per optimisation level.
 *
 *   Project > Properties > C/C++ Build > Settings > MCU GCC Compiler
 *     > Optimization > Optimization Level
 *
 *   None            -O0
 *   Optimize        -O1
 *   Optimize more   -O2
 *   Optimize size   -Os
 *
 * At every level record:
 *   1. Text size in bytes. Build Analyzer tab, or run
 *        arm-none-eabi-size Debug/Practical1B.elf
 *   2. single_call_span from Live Expressions.
 *   3. The PC13 pulse width from the scope, with cursors.
 *
 * At -O2 also dump the disassembly of your isqrt function:
 *        arm-none-eabi-objdump -d Debug/Practical1B.elf > disasm_O2.txt
 *   or open Debug/Practical1B.list, which the build already produces.
 * Find one transformation the compiler applied, name it, and point at the
 * source lines above it acts on.
 * ---------------------------------------------------------------------------
 */
>>>>>>> 6b4fcc0402839e416a6e9b98411845e937f59437
