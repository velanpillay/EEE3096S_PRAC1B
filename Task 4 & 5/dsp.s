/*
 * dsp.s
 * EEE3096S 2026 - Practical 1B, Task 4
 * Cycle-counted ADC to DAC loop with a 45 degree phase delay
 *
 * Student 1 : <THAPELO MOCHEKO>  MCHTHA046>
 * Student 2 : <VELAN PILLAY>  <PLLVEL003>
 */

    .syntax unified
    .thumb
    .cpu    cortex-m0
    .fpu    softvfp

    .global DSP_Loop
    .type   DSP_Loop, %function

@ ---------------------------------------------------------------------------
@ Peripheral addresses
@ ---------------------------------------------------------------------------
    .equ ADC_DR,      0x40012440
    .equ DAC_DHR12R1, 0x40007408

    .section .text.DSP_Loop, "ax", %progbits

@ ===========================================================================
@ ENTRY POINT
@ ===========================================================================
DSP_Loop:
    @ Setup registers outside the timed loop
    LDR R0, =ADC_DR
    LDR R1, =DAC_DHR12R1

loop:
    @ --- SAMPLE AND OUTPUT ------------------------------------------------
    @ TODO 1: Read the current ADC conversion from the Data Register.
    @ Cortex_M0 load timing used for prediction: 2 cyles
    
    LDR R2, [R0]                @ 2 cycles


    @ TODO 2: Write the value straight out to the DAC Data Register.

      @ Copy ADC value directly to DAC channel 1 data register.
    @ 12-bit ADC and DAC are both right aligned.
    @ Predicted timing: 2 cycles.
    STR R2, [R1]                @ 2 cycles

    @ --- DELAY SETUP ------------------------------------------------------
    @ TODO 3: Calculate the required cycle target for a 45-degree phase 
    @         delay on a 1 kHz sine wave running at an 8 MHz system clock.
    @         Load your inner loop counter and insert any NOP padding 
    @         needed to hit your exact target.

 @ Input frequency:
    @
    @       f = 1 kHz
    @
    @ Period:
    @
    @       T = 1/f
    @         = 1/1000
    @         = 1 ms
    @
    @ Required phase shift:
    @
    @       45/360 = 1/8 cycle
    @
    @       delay = (1/8)(1 ms)
    @             = 125 us
    @
    @ CPU clock:
    @
    @       fCPU = 8 MHz
    @
    @       Tcycle = 1/8 MHz
    @              = 125 ns
    @
    @ Required repeated-loop cycle target:
    @
    @       125 us / 125 ns
    @       = 1000 CPU cycles
    @
    @ ---------------------------------------------------------------
    @ Cycle budget:
    @
    @ LDR R2,[R0]              =   2
    @ STR R2,[R1]              =   2
    @ MOVS R3,#248             =   1
    @ NOP                      =   1
    @ NOP                      =   1
    @ SUBS, 248 iterations     = 248
    @ BNE taken, 247 times     = 741
    @ BNE not taken, once      =   1
    @ B loop                   =   3
    @                            ----
    @ TOTAL                    = 1000 cycles
    @
    @ 1000 * 125 ns = 125 us
    @ ---------------------------------------------------------------
        MOVS R3, #248          @ 1 cycle
    NOP                    @ 1 cycle
    NOP                    @ 1 cycle

delay_loop:
    @ --- INNER LOOP -------------------------------------------------------
    @ TODO 4: Implement the counted delay loop.
    @         (Remember to use flag-updating arithmetic so your branch works).

 @ Subtract 1 AND update the condition flags.
    @ SUBS is essential because BNE tests the Z flag.
    SUBS R3, R3, #1            @ 1 cycle

    @ Taken while R3 != 0.
    @ 247 taken branches, then one final not-taken branch.
    BNE delay_loop


    @ --- REPEAT -----------------------------------------------------------
    @ TODO 5: Branch back to the start of the main 'loop'.
      @ Repeat the entire ADC -> DAC operation.
    B loop                     @ 3 cycles
    
    @ ----------------------------------------------------------------------
    @ NOTE: You must calculate your exact cycle budget, showing the cost 
    @ of every instruction and loop iteration, and document it in your report.
    @ ----------------------------------------------------------------------

    .size DSP_Loop, .-DSP_Loop
