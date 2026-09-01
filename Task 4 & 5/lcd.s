/*
 * lcd.s
 * EEE3096S 2026 - Practical 1B, Task 5
 * 4-bit bit-banged HD44780 driver, and the level shifter timing fault
 *
 * Student 1 : <THAPELO MOCHEKO>  <MCHTHA046>
 * Student 2 : <VELAN PILLAY>  <PLLVEL003>
 */

    .syntax unified
    .thumb
    .cpu    cortex-m0
    .fpu    softvfp

    .global LCD_Run
    .type   LCD_Run, %function

@ ---------------------------------------------------------------------------
@ Register addresses. BSRR is at offset 0x18 from each port base.
@ ---------------------------------------------------------------------------
    .equ GPIOA_BSRR, 0x48000018
    .equ GPIOB_BSRR, 0x48000418
    .equ GPIOC_BSRR, 0x48000818

@ ---------------------------------------------------------------------------
@ PIN MAP
@   PC15  Enable (E)     -> PC15_S on the 5 V side
@   PC14  Register Select (RS)
@   PB8   D4      PB9   D5      PA12  D6      PA15  D7
@   R/W is tied to ground. The LCD is write only. 
@ ---------------------------------------------------------------------------

    .section .text.LCD_Run, "ax", %progbits

@ ===========================================================================
@ ENTRY POINT
@ ===========================================================================
LCD_Run:
    PUSH {LR}

    @ TODO 1: Wait for the LCD power rail to settle (consult datasheet).
    
        @ TODO 1: Wait for LCD power rail to settle.
    @ HD44780 datasheet requires >15 ms after VCC rises to 4.5 V.
    @ LCD_DelayLong will be approximately 5 ms.
    @ 4 x 5 ms = approximately 20 ms > 15 ms required.

    BL   LCD_DelayLong
    BL   LCD_DelayLong
    BL   LCD_DelayLong
    BL   LCD_DelayLong

    @ TODO 2: Call the 4-bit initialization sequence.
    
    @ initialize LCD

    BL LCD_Init

    @ TODO 3: Write the character 'A' (0x41) to the display.

    MOVS R0, #0x41
    BL   LCD_WriteData

hang:
    @ Temporary oscilloscope test: continuously toggle PC15

    LDR  R0, =GPIOC_BSRR
    LDR  R1, =0x00008000      @ PC15 HIGH mask
    LDR  R2, =0x80000000      @ PC15 LOW mask

scope_square:

    STR  R1, [R0]             @ PC15 HIGH

    BL   LCD_DelayLong        @ hold HIGH for about 5 ms

    STR  R2, [R0]             @ PC15 LOW

    BL   LCD_DelayLong        @ hold LOW for about 5 ms

    B    scope_square


    .size LCD_Run, .-LCD_Run

@ ===========================================================================
@ LCD_Init
@ Puts the controller into 4-bit mode and readies the display.
@ ===========================================================================
    .type LCD_Init, %function
LCD_Init:
    PUSH {LR}

    @ TODO 4: Send the 4-bit initialization sequence.
    @ Reference the HD44780 datasheet flowchart. 
    @ Send commands with RS low using LCD_WriteCmd.


        @ ---------------------------------------------------------------
    @ RS = LOW because initialization values are commands.
    @ PC14 reset uses bit 30 of GPIOC_BSRR.
    @ ---------------------------------------------------------------

    LDR  R1, =GPIOC_BSRR
    LDR  R2, =0x40000000
    STR  R2, [R1]

    @ ---------------------------------------------------------------
    @ Special HD44780 power-up sequence.
    @ These first values are single NIBBLES, not complete bytes.
    @ ---------------------------------------------------------------

    MOVS R0, #0x03
    BL   LCD_SendNibble
    BL   LCD_Pulse

    @ Wait several milliseconds
    BL   LCD_DelayLong


    MOVS R0, #0x03
    BL   LCD_SendNibble
    BL   LCD_Pulse

    BL   LCD_DelayShort


    MOVS R0, #0x03
    BL   LCD_SendNibble
    BL   LCD_Pulse

    BL   LCD_DelayShort


    @ Send 0x2 to enter 4-bit mode
    MOVS R0, #0x02
    BL   LCD_SendNibble
    BL   LCD_Pulse


    @ ---------------------------------------------------------------
    @ LCD is now in 4-bit mode.
    @ Commands can now be sent as complete bytes.
    @ ---------------------------------------------------------------

    @ Function set:
    @ 4-bit interface, 2 lines, 5x8 font
    MOVS R0, #0x28
    BL   LCD_WriteCmd

    @ Display OFF
    MOVS R0, #0x08
    BL   LCD_WriteCmd

    @ Clear display
    MOVS R0, #0x01
    BL   LCD_WriteCmd

    @ Clear operation takes longer
    BL   LCD_DelayLong

    @ Entry mode: increment cursor, no display shift
    MOVS R0, #0x06
    BL   LCD_WriteCmd

    @ Display ON, cursor OFF, blink OFF
    MOVS R0, #0x0C
    BL   LCD_WriteCmd

    @ Cursor to beginning of first line
    MOVS R0, #0x80
    BL   LCD_WriteCmd

    POP {PC}

@ ===========================================================================
@ LCD_WriteCmd   R0 = command byte, RS low
@ LCD_WriteData  R0 = data byte,    RS high
@ Both send the high nibble first, then the low nibble.
@ ===========================================================================
    .type LCD_WriteCmd, %function
LCD_WriteCmd:
    PUSH {R0, LR}
    @ TODO 5: Drive RS (PC14) LOW, then fall through to the shared sender.

        @ PC14 = RS.
    @ Resetting PC14 uses BSRR bit 14+16 = bit 30.

    LDR  R1, =GPIOC_BSRR
    LDR  R2, =0x40000000
    STR  R2, [R1]

    B    LCD_Send8

    .type LCD_WriteData, %function
LCD_WriteData:
    PUSH {R0, LR}
    @ TODO 6: Drive RS (PC14) HIGH, then fall through.

        @ Set PC14 HIGH.
    @ BSRR bit 14 sets GPIO PC14.

    LDR  R1, =GPIOC_BSRR
    LDR  R2, =0x00004000
    STR  R2, [R1]

    B    LCD_Send8

    @ TODO 7: Send the upper nibble of R0, pulse Enable,
    @         then the lower nibble of R0, pulse Enable again.

LCD_Send8:

    @ Original byte is already saved on the stack by
    @ LCD_WriteCmd / LCD_WriteData.

    @ ---------------------------------------------------------------
    @ Send HIGH nibble first.
    @ Example: 0x41 >> 4 = 0x04
    @ ---------------------------------------------------------------

    LSRS R0, R0, #4
    BL   LCD_SendNibble
    BL   LCD_Pulse


    @ ---------------------------------------------------------------
    @ Restore original byte and extract LOW nibble.
    @ Example: 0x41 & 0x0F = 0x01
    @ ---------------------------------------------------------------

    LDR  R0, [SP, #0]

    MOVS R1, #0x0F
    ANDS R0, R1

    BL   LCD_SendNibble
    BL   LCD_Pulse

    POP {R0, PC}

@ ===========================================================================
@ LCD_SendNibble   R0 bits 3:0 -> the four data lines
@ ===========================================================================
    .type LCD_SendNibble, %function
LCD_SendNibble:
    PUSH {R1, R2, R3, LR}

    @ TODO 8: Map the four bits of R0 onto the four data pins (across 3 ports).
    @   R0 bit 0 -> PB8   (D4)
    @   R0 bit 1 -> PB9   (D5)
    @   R0 bit 2 -> PA12  (D6)
    @   R0 bit 3 -> PA15  (D7)

    @ ---------------------------------------------------------------
    @ First clear all four LCD data pins.
    @ ---------------------------------------------------------------

    @ PB8 and PB9 reset:
    @ PB8  -> BSRR bit 24
    @ PB9  -> BSRR bit 25
    LDR  R1, =GPIOB_BSRR
    LDR  R2, =0x03000000
    STR  R2, [R1]

    @ PA12 and PA15 reset:
    @ PA12 -> BSRR bit 28
    @ PA15 -> BSRR bit 31
    LDR  R1, =GPIOA_BSRR
    LDR  R2, =0x90000000
    STR  R2, [R1]


    @ ---------------------------------------------------------------
    @ R0 bit 0 -> PB8 -> D4
    @ ---------------------------------------------------------------

    MOVS R1, #0x01
    TST   R0, R1
    BEQ   check_d5

    LDR  R1, =GPIOB_BSRR
    LDR  R2, =0x00000100
    STR  R2, [R1]


check_d5:

    @ ---------------------------------------------------------------
    @ R0 bit 1 -> PB9 -> D5
    @ ---------------------------------------------------------------

    MOVS R1, #0x02
    TST   R0, R1
    BEQ   check_d6

    LDR  R1, =GPIOB_BSRR
    LDR  R2, =0x00000200
    STR  R2, [R1]


check_d6:

    @ ---------------------------------------------------------------
    @ R0 bit 2 -> PA12 -> D6
    @ ---------------------------------------------------------------

    MOVS R1, #0x04
    TST   R0, R1
    BEQ   check_d7

    LDR  R1, =GPIOA_BSRR
    LDR  R2, =0x00001000
    STR  R2, [R1]


check_d7:

    @ ---------------------------------------------------------------
    @ R0 bit 3 -> PA15 -> D7
    @ ---------------------------------------------------------------

    MOVS R1, #0x08
    TST   R0, R1
    BEQ   nibble_done

    LDR  R1, =GPIOA_BSRR
    LDR  R2, =0x00008000
    STR  R2, [R1]


nibble_done:
    POP {R1, R2, R3, PC}

@ ===========================================================================
@ LCD_Pulse
@ ===========================================================================
    .type LCD_Pulse, %function
LCD_Pulse:
    PUSH {R0, R1, R2, LR}

    LDR  R0, =GPIOC_BSRR

    @ TODO 9: Set PC15 HIGH.
    @ PC15 SET mask = 1 << 15 = 0x00008000
    LDR  R1, =0x00008000
    LDR  R2, =0x80000000
    STR  R1, [R0]
    @ -----------------------------------------------------------------
    @ TODO 10: THE TIMING FIX
    @ Implement a calculated pad delay here to overcome the RC time 
    @ constant of the level shifter and meet the HD44780 hold time requirements.
    @ Show your cycle arithmetic in the comments.


 @ INITIAL / BEFORE-FIX TEST:
    @ No NOP instructions are inserted here yet.
    @ First measure the rise time of PC15_S from 0 V to 3.5 V.
    @
    @ CPU clock = 8 MHz
    @ Tcycle = 1 / 8 MHz = 125 ns
    @
    @ After measurement:
    @ N_NOP = ceil((t_rise + 450 ns) / 125 ns)



        @ TODO 10: THE TIMING FIX
    @ Implement a calculated pad delay here to overcome the RC time
    @ constant of the level shifter and meet the HD44780 hold time requirements.
    @ Show your cycle arithmetic in the comments.

    @ Measured PC15_S rise time to 3.5 V:
    @     t_rise = 27 ns
    @
    @ Required valid HIGH time above 3.5 V:
    @     t_high = 450 ns
    @
    @ Therefore required PC15 HIGH duration:
    @     t_required = 27 ns + 450 ns
    @                = 477 ns
    @
    @ CPU clock = 8 MHz
    @ Tcycle = 1 / 8 MHz = 125 ns
    @
    @ Required NOP count:
    @     N_NOP = ceil(477 ns / 125 ns)
    @           = ceil(3.816)
    @           = 4 NOPs
    @
    @ Added delay:
    @     4 x 125 ns = 500 ns

    NOP
    NOP
    NOP
    NOP


    @ -----------------------------------------------------------------

    @ TODO 11: Set PC15 LOW.

    @ PC15 RESET is BSRR bit 31.

    STR  R2, [R0]

    @ TODO 12: Hold Enable low long enough to meet the LCD cycle time.

        BL   LCD_DelayShort

    POP {R0, R1, R2, PC}

@ ===========================================================================
@ Delay helpers
@ ===========================================================================
    .type LCD_DelayLong, %function
LCD_DelayLong:
    @ TODO 13: Implement a millisecond-scale delay. Show cycle arithmetic.

    @ ---------------------------------------------------------------
    @ Approximate 5 ms delay at 8 MHz.
    @ CPU cycle = 125 ns.
    @
    @ N = 10000
    @
    @ LDR counter                  ~= 2 cycles
    @ SUBS              10000 x 1 = 10000
    @ BNE taken          9999 x 3 = 29997
    @ BNE final                   = 1
    @ BX LR                       = 3
    @
    @ Total ~= 40003 cycles
    @
    @ 40003 x 125 ns ~= 5.000 ms
    @ ---------------------------------------------------------------

    LDR  R3, =10000

delay_long_loop:

    SUBS R3, R3, #1
    BNE  delay_long_loop
    BX   LR

    .type LCD_DelayShort, %function
LCD_DelayShort:
    @ TODO 14: Implement a microsecond-scale delay. Show cycle arithmetic.

    LCD_DelayShort:

    @ ---------------------------------------------------------------
    @ Approximate 100 us delay at 8 MHz.
    @
    @ N = 200
    @
    @ MOVS                    = 1 cycle
    @ SUBS          200 x 1  = 200
    @ BNE taken     199 x 3  = 597
    @ final BNE               = 1
    @ BX LR                   = 3
    @
    @ Total ~= 802 cycles
    @
    @ 802 x 125 ns
    @     ~= 100.25 us
    @ ---------------------------------------------------------------

    MOVS R3, #200

delay_short_loop:

    SUBS R3, R3, #1
    BNE  delay_short_loop

    BX   LR
