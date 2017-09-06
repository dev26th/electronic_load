#ifndef _STM8_H_
#define _STM8_H_

#include <stddef.h>
#include <stdint.h>
#include <assert.h>

// == Global definitions ==============================================================================================

// FIXME use asm BRES, BSET, BCPL, BTJF, BTJT for bit operations

#define _RW                 volatile           // read-write register
#define _RO                 volatile const     // read-only register
#define _WO                 volatile           // write-only register
#define _RS                                    // unused (reserved) space in registers address range

#define rim()               do { __asm__ ("rim");  } while (0) // enable interrupts
#define sim()               do { __asm__ ("sim");  } while (0) // disable interrupts
#define nop()               do { __asm__ ("nop");  } while (0) // no operation
#define trap()              do { __asm__ ("trap"); } while (0) // trap (soft interrupt)
#define wfi()               do { __asm__ ("wfi");  } while (0) // wait for interrupt
#define halt()              do { __asm__ ("halt"); } while (0) // halt

// == CPUCFG ==========================================================================================================

typedef struct {
    _RW uint8_t GCR;              // Global configuration register
} CPUCFG_t;
static_assert(offsetof(CPUCFG_t, GCR) == 0x00, "Wrong definition");

#define CPUCFG_GCR_AL                      (   1 << 1) // Activation level
#define CPUCFG_GCR_SWD                     (   1 << 0) // SWIM disable

// == FLASH ===========================================================================================================

typedef struct {
    _RW uint8_t CR1;              // Control register 1
    _RW uint8_t CR2;              // Control register 2
    _RW uint8_t NCR2;             // Complementary control register 2
    _RW uint8_t FPR;              // Flash protection register
    _RW uint8_t NFPR;             // Complementary flash protection register
    _RW uint8_t IAPSR;            // Status register
    _RS uint8_t reserved1;
    _RS uint8_t reserved2;
    _RW uint8_t PUKR;             // Flash program memory unprotecting key register
    _RS uint8_t reserved3;
    _RW uint8_t DUKR;             // Data EEPROM unprotection key register
} FLASH_t;
static_assert(offsetof(FLASH_t, DUKR) == 0x0A, "Wrong definition");

#define FLASH_CR1_HALT                     (   1 << 3) // Power-down in Halt mode
#define FLASH_CR1_AHALT                    (   1 << 2) // Power-down in Active-halt mode
#define FLASH_CR1_IE                       (   1 << 1) // Flash interrupt enable
#define FLASH_CR1_FIX                      (   1 << 0) // Fixed byte programming time

#define FLASH_CR2_OPT                      (   1 << 7) // Write option bytes
#define FLASH_CR2_WPRG                     (   1 << 6) // Word programming
#define FLASH_CR2_ERASE                    (   1 << 5) // Block erasing
#define FLASH_CR2_FPRG                     (   1 << 4) // Fast block programming
#define FLASH_CR2_PRG                      (   1 << 0) // Standard block programming

#define FLASH_NCR2_NOPT                    (   1 << 7) // Write option bytes
#define FLASH_NCR2_NWPRG                   (   1 << 6) // Word programming
#define FLASH_NCR2_NERASE                  (   1 << 5) // Block erasing
#define FLASH_NCR2_NFPRG                   (   1 << 4) // Fast block programming
#define FLASH_NCR2_NPRG                    (   1 << 0) // Standard block programming

#define FLASH_FPR_WPB5                     (   1 << 5) // User boot code area protection bit 5
#define FLASH_FPR_WPB4                     (   1 << 4) // User boot code area protection bit 4
#define FLASH_FPR_WPB3                     (   1 << 3) // User boot code area protection bit 3
#define FLASH_FPR_WPB2                     (   1 << 2) // User boot code area protection bit 2
#define FLASH_FPR_WPB1                     (   1 << 1) // User boot code area protection bit 1
#define FLASH_FPR_WPB0                     (   1 << 0) // User boot code area protection bit 0

#define FLASH_NFPR_WPB5                    (   1 << 5) // User boot code area protection bit 5
#define FLASH_NFPR_WPB4                    (   1 << 4) // User boot code area protection bit 4
#define FLASH_NFPR_WPB3                    (   1 << 3) // User boot code area protection bit 3
#define FLASH_NFPR_WPB2                    (   1 << 2) // User boot code area protection bit 2
#define FLASH_NFPR_WPB1                    (   1 << 1) // User boot code area protection bit 1
#define FLASH_NFPR_WPB0                    (   1 << 0) // User boot code area protection bit 0

#define FLASH_PUKR_PUK                     (0xFF << 0) // Main program memory unlock keys

#define FLASH_DUKR_DUK                     (0xFF << 0) // Data EEPROM write unlock keys

#define FLASH_IAPSR_HVOFF                  (   1 << 6) // End of high voltage flag
#define FLASH_IAPSR_DUL                    (   1 << 3) // Data EEPROM area unlocked flag
#define FLASH_IAPSR_EOP                    (   1 << 2) // End of programming (write or erase operation) flag
#define FLASH_IAPSR_PUL                    (   1 << 1) // Flash Program memory unlocked flag
#define FLASH_IAPSR_WRPGDIS                (   1 << 0) // Write attempted to protected page flag

#define FLASH_KEY1                         0x56
#define FLASH_KEY2                         0xAE

// == Option bytes ====================================================================================================

typedef struct {
    _RW uint8_t ROP;              // Read-out protection
    _RW uint8_t UBC;              // User boot code
    _RW uint8_t NUBC;             // Complementary user boot code
    _RW uint8_t AFR;              // Alternate function remapping
    _RW uint8_t NAFR;             // Complementary alternate function remapping
    _RW uint8_t MISC;             // Misc. option
    _RW uint8_t NMISC;            // Complementary misc. option
    _RW uint8_t CLOCK;            // Clock option
    _RW uint8_t NCLOCK;           // Complementary clock option
    _RW uint8_t HSE;              // HSE clock startup
    _RW uint8_t NHSE;             // Complementary HSE clock startup
    _RS uint8_t reserved1[115];
    _RW uint8_t BL;               // Bootloader
    _RW uint8_t NBL;              // Complementary bootloader
} OPT_t;
static_assert(offsetof(OPT_t, NBL) == 0x7F, "Wrong definition");

#define OPT_AFR_D4_BEEP                    (   1 << 7) // Port D4 alternate function = BEEP
#define OPT_AFR_B5B4_I2C                   (   1 << 6) // Port B5 alternate function = I2C_SDA; port B4 alternate function = I2C_SCL
#define OPT_AFR_B3B2B1B0_TIM1              (   1 << 5) // Port B3 alternate function = TIM1_ETR; port B2 alternate function = TIM1_NCC3; port B1 alternate function = TIM1_CH2N; port B0 alternate function = TIM1_CH1N
#define OPT_AFR_D7_TIM1                    (   1 << 4) // Port D7 alternate function = TIM1_CH4
#define OPT_AFR_D0_TIM1                    (   1 << 3) // Port D0 alternate function = TIM1_BKIN
#define OPT_AFR_D0_CLK                     (   1 << 2) // Port D0 alternate function = CLK_CCO
#define OPT_AFR_A3D2_TIM32                 (   1 << 1) // Port A3 alternate function = TIM3_CH1; port D2 alternate function TIM2_CH3
#define OPT_AFR_D3_ADC                     (   1 << 0) // Port D3 alternate function = ADC_ETR

// == ITC =============================================================================================================

typedef struct {
    _RW uint8_t SPR[8];           // Software priority registers
} ITC_t;
static_assert(sizeof(ITC_t) == 8, "Wrong definition");

// == EXTI ============================================================================================================

typedef struct {
    _RW uint8_t CR1;              // Control register 1
    _RW uint8_t CR2;              // Control register 2
} EXTI_t;
static_assert(offsetof(EXTI_t, CR2) == 0x01, "Wrong definition");

#define EXTI_CR1_PDIS                      (0x03 << 6) // Port D external interrupt sensitivity bits
#define EXTI_CR1_PCIS                      (0x03 << 4) // Port C external interrupt sensitivity bits
#define EXTI_CR1_PBIS                      (0x03 << 2) // Port B external interrupt sensitivity bits
#define EXTI_CR1_PAIS                      (0x03 << 0) // Port A external interrupt sensitivity bits

#define EXTI_CR2_TLIS                      (   1 << 2) // Top level interrupt sensitivity
#define EXTI_CR2_PEIS                      (0x03 << 0) // Port E external interrupt sensitivity bits

// == RST =============================================================================================================

typedef struct {
    _RW uint8_t SR;               // Reset status register
} RST_t;
static_assert(offsetof(RST_t, SR) == 0x00, "Wrong definition");

#define RST_SR_EMCF                        (   1 << 4) // EMC reset flag
#define RST_SR_SWIMF                       (   1 << 3) // SWIM reset flag
#define RST_SR_ILLOP                       (   1 << 2) // Illegal opcode reset flag
#define RST_SR_IWDGF                       (   1 << 1) // Independent Watchdog reset flag
#define RST_SR_WWDGF                       (   1 << 0) // Window Watchdog reset flag

// == CLK =============================================================================================================

typedef struct {
    _RW uint8_t ICKR;             // Internal clock register
    _RW uint8_t ECKR;             // External clock register
    _RS uint8_t reserved1;
    _RO uint8_t CMSR;             // Clock master status register
    _RW uint8_t SWR;              // Clock master switch register
    _RW uint8_t SWCR;             // Switch control register
    _RW uint8_t CKDIVR;           // Clock divider register
    _RW uint8_t PCKENR1;          // Peripheral clock gating register 1
    _RW uint8_t CSSR;             // Clock security system register
    _RW uint8_t CCOR;             // Configurable clock output register
    _RW uint8_t PCKENR2;          // Peripheral clock gating register 2
    _RS uint8_t reserved2;
    _RW uint8_t HSITRIMR;         // HSI clock calibration trimming register
    _RW uint8_t SWIMCCR;          // SWIM clock control register
} CLK_t;
static_assert(offsetof(CLK_t, SWIMCCR) == 0x0D, "Wrong definition");

#define CLK_ICKR_REGAH                     (   1 << 5) // Regulator power off in Active-halt mode
#define CLK_ICKR_LSIRDY                    (   1 << 4) // Low speed internal oscillator ready
#define CLK_ICKR_LSIEN                     (   1 << 3) // Low speed internal RC oscillator enable
#define CLK_ICKR_FHW                       (   1 << 2) // Fast wakeup from Halt/Active-halt modes
#define CLK_ICKR_HSIRDY                    (   1 << 1) // High speed internal oscillator ready
#define CLK_ICKR_HSIEN                     (   1 << 0) // High speed internal RC oscillator enable

#define CLK_ECKR_HSERDY                    (   1 << 1) // High speed external crystal oscillator ready
#define CLK_ECKR_HSEEN                     (   1 << 0) // High speed external crystal oscillator enable

#define CLK_CMSR_CKM                       (0xFF << 0) // Clock master status bits

#define CLK_SWR_SWI                        (0xFF << 0) // Clock master selection bits
#define CLK_SWR_SWI_HSI                    (0xE1 << 0) //   - HSI selected as master clock source (reset value)
#define CLK_SWR_SWI_LSI                    (0xD2 << 0) //   - LSI selected as master clock source (only if LSI_EN option bit is set)
#define CLK_SWR_SWI_HSE                    (0xB4 << 0) //   - HSE selected as master clock source

#define CLK_SWCR_SWIF                      (   1 << 3) // Clock switch interrupt flag
#define CLK_SWCR_SWIEN                     (   1 << 2) // Clock switch interrupt enable
#define CLK_SWCR_SWEN                      (   1 << 1) // Switch start/stop
#define CLK_SWCR_SWBSY                     (   1 << 0) // Switch busy

#define CLK_CKDIVR_HSIDIV                  (0x03 << 3) // High speed internal clock prescaler
#define CLK_CKDIVR_CPUDIV                  (0x07 << 0) // CPU clock prescaler

#define CLK_PCKENR1_TIM1                   (   1 << 7) // Peripheral clock enable: TIM1
#define CLK_PCKENR1_TIM3                   (   1 << 6) // Peripheral clock enable: TIM3
#define CLK_PCKENR1_TIM25                  (   1 << 5) // Peripheral clock enable: TIM2/TIM5 (product dependent)
#define CLK_PCKENR1_TIM46                  (   1 << 4) // Peripheral clock enable: TIM4/TIM6 (product dependent)
#define CLK_PCKENR1_UARTA                  (   1 << 3) // Peripheral clock enable: UART1/2/3/4 (product dependent)
#define CLK_PCKENR1_UARTB                  (   1 << 2) // Peripheral clock enable: UART1/2/3/4 (product dependent)
#define CLK_PCKENR1_SPI                    (   1 << 1) // Peripheral clock enable: SPI
#define CLK_PCKENR1_I2C                    (   1 << 0) // Peripheral clock enable: I2C

#define CLK_PCKENR2_CAN                    (   1 << 7) // Peripheral clock enable: CAN
#define CLK_PCKENR2_ADC                    (   1 << 3) // Peripheral clock enable: ADC
#define CLK_PCKENR2_AWU                    (   1 << 2) // Peripheral clock enable: AWU

#define CLK_CSSR_CSSD                      (   1 << 3) // Clock security system detection
#define CLK_CSSR_CSSDIE                    (   1 << 2) // Clock security system detection interrupt enable
#define CLK_CSSR_AUX                       (   1 << 1) // Auxiliary oscillator connected to master clock
#define CLK_CSSR_CSSEN                     (   1 << 0) // Clock security system enable

#define CLK_CCOR_CCOBSY                    (   1 << 6) // Configurable clock output busy
#define CLK_CCOR_CCORDY                    (   1 << 5) // Configurable clock output ready
#define CLK_CCOR_CCOSEL                    (0x0F << 1) // Configurable clock output selection.
#define CLK_CCOR_CCOSEL_HSIDIV             (0x00 << 1) //   - HSIDIV
#define CLK_CCOR_CCOSEL_LSI                (0x01 << 1) //   - LSI
#define CLK_CCOR_CCOSEL_HSE                (0x02 << 1) //   - HSE
#define CLK_CCOR_CCOSEL_CPU                (0x04 << 1) //   - CPU
#define CLK_CCOR_CCOSEL_CPU2               (0x05 << 1) //   - CPU/2
#define CLK_CCOR_CCOSEL_CPU4               (0x06 << 1) //   - CPU/4
#define CLK_CCOR_CCOSEL_CPU8               (0x07 << 1) //   - CPU/8
#define CLK_CCOR_CCOSEL_CPU16              (0x08 << 1) //   - CPU/16
#define CLK_CCOR_CCOSEL_CPU32              (0x09 << 1) //   - CPU/32
#define CLK_CCOR_CCOSEL_CPU64              (0x0A << 1) //   - CPU/64
#define CLK_CCOR_CCOSEL_HSI                (0x0B << 1) //   - HSI
#define CLK_CCOR_CCOSEL_MASTER             (0x0C << 1) //   - MASTER
#define CLK_CCOR_CCOEN                     (   1 << 0) // Configurable clock output enable

#define CLK_HSIRIMR_HSITRIM                (0x0F << 0) // HSI trimming value

#define CLK_SWIMCCR_SWIMCLK                (   1 << 0) // SWIM clock divider

// == GPIO ============================================================================================================

typedef struct {
    _RW uint8_t ODR;              // Output data register
    _RW uint8_t IDR;              // Input data register
    _RW uint8_t DDR;              // Data direction register
    _RW uint8_t CR1;              // Control register 1
    _RW uint8_t CR2;              // Control register 2
} GPIO_t;
static_assert(offsetof(GPIO_t, CR2) == 0x04, "Wrong definition");

#define GPIO_ODR_7                         (   1 << 7) // Output data pin 7
#define GPIO_ODR_6                         (   1 << 6) // Output data pin 6
#define GPIO_ODR_5                         (   1 << 5) // Output data pin 5
#define GPIO_ODR_4                         (   1 << 4) // Output data pin 4
#define GPIO_ODR_3                         (   1 << 3) // Output data pin 3
#define GPIO_ODR_2                         (   1 << 2) // Output data pin 2
#define GPIO_ODR_1                         (   1 << 1) // Output data pin 1
#define GPIO_ODR_0                         (   1 << 0) // Output data pin 0

#define GPIO_IDR_7                         (   1 << 7) // Input data pin 7
#define GPIO_IDR_6                         (   1 << 6) // Input data pin 6
#define GPIO_IDR_5                         (   1 << 5) // Input data pin 5
#define GPIO_IDR_4                         (   1 << 4) // Input data pin 4
#define GPIO_IDR_3                         (   1 << 3) // Input data pin 3
#define GPIO_IDR_2                         (   1 << 2) // Input data pin 2
#define GPIO_IDR_1                         (   1 << 1) // Input data pin 1
#define GPIO_IDR_0                         (   1 << 0) // Input data pin 0

#define GPIO_DDR_7                         (   1 << 7) // Data direction pin 7
#define GPIO_DDR_6                         (   1 << 6) // Data direction pin 6
#define GPIO_DDR_5                         (   1 << 5) // Data direction pin 5
#define GPIO_DDR_4                         (   1 << 4) // Data direction pin 4
#define GPIO_DDR_3                         (   1 << 3) // Data direction pin 3
#define GPIO_DDR_2                         (   1 << 2) // Data direction pin 2
#define GPIO_DDR_1                         (   1 << 1) // Data direction pin 1
#define GPIO_DDR_0                         (   1 << 0) // Data direction pin 0

#define GPIO_CR1_7                         (   1 << 7) // Control bits (pull-up/push-pull) pin 7
#define GPIO_CR1_6                         (   1 << 6) // Control bits (pull-up/push-pull) pin 6
#define GPIO_CR1_5                         (   1 << 5) // Control bits (pull-up/push-pull) pin 5
#define GPIO_CR1_4                         (   1 << 4) // Control bits (pull-up/push-pull) pin 4
#define GPIO_CR1_3                         (   1 << 3) // Control bits (pull-up/push-pull) pin 3
#define GPIO_CR1_2                         (   1 << 2) // Control bits (pull-up/push-pull) pin 2
#define GPIO_CR1_1                         (   1 << 1) // Control bits (pull-up/push-pull) pin 1
#define GPIO_CR1_0                         (   1 << 0) // Control bits (pull-up/push-pull) pin 0

#define GPIO_CR2_7                         (   1 << 7) // Control bits (external interrupt/high output speed) pin 7
#define GPIO_CR2_6                         (   1 << 6) // Control bits (external interrupt/high output speed) pin 6
#define GPIO_CR2_5                         (   1 << 5) // Control bits (external interrupt/high output speed) pin 5
#define GPIO_CR2_4                         (   1 << 4) // Control bits (external interrupt/high output speed) pin 4
#define GPIO_CR2_3                         (   1 << 3) // Control bits (external interrupt/high output speed) pin 3
#define GPIO_CR2_2                         (   1 << 2) // Control bits (external interrupt/high output speed) pin 2
#define GPIO_CR2_1                         (   1 << 1) // Control bits (external interrupt/high output speed) pin 1
#define GPIO_CR2_0                         (   1 << 0) // Control bits (external interrupt/high output speed) pin 0

// == AWU =============================================================================================================

typedef struct {
    _RW uint8_t CSR;              // Control/status register
    _RW uint8_t APR;              // Asynchronous prescaler register
    _RW uint8_t TBR;              // Timebase selection register
} AWU_t;
static_assert(offsetof(AWU_t, TBR) == 0x02, "Wrong definition");

#define AWU_CSR_AWUF                       (   1 << 5) // Auto-wakeup flag
#define AWU_CSR_AWUEN                      (   1 << 4) // Auto-wakeup enable
#define AWU_CSR_MSR                        (   1 << 0) // Measurement enable

#define AWU_APR_APR                        (0x3F << 0) // Asynchronous prescaler divider

#define AWU_TBR_AWUTB                      (0x0F << 0) // Auto-wakeup timebase selection

// == BEEP ============================================================================================================

typedef struct {
    _RW uint8_t CSR;              // Control/status register
} BEEP_t;
static_assert(offsetof(BEEP_t, CSR) == 0x00, "Wrong definition");

#define BEEP_CSR_BEEPSEL                   (0x03 << 6) // Beep selection
#define BEEP_CSR_BEEPSEL_1KHZ              (0x00 << 6) //   - 1 kHz
#define BEEP_CSR_BEEPSEL_2KHZ              (0x01 << 6) //   - 2 kHz
#define BEEP_CSR_BEEPSEL_4KHZ              (0x02 << 6) //   - 4 kHz
#define BEEP_CSR_BEEPEN                    (   1 << 5) // Beep enable
#define BEEP_CSR_BEEPDIV                   (0x1F << 0) // Beep prescaler divider (-2)
#define BEEP_CSR_BEEPDIV_INVALID           (0x1F << 0) //   - invalid value

// == IWDG ============================================================================================================

typedef struct {
    _RW uint8_t KR;               // Key register
    _RW uint8_t PR;               // Prescaler register
    _RW uint8_t RLR;              // Reload register
} IWDG_t;
static_assert(offsetof(IWDG_t, RLR) == 0x02, "Wrong definition");

#define IWDG_KR_KEY                        (0xFF << 0) // Key value
#define IWDG_KR_KEY_ENABLE                 (0xCC << 0) //   - Start the IWDG
#define IWDG_KR_KEY_REFRESH                (0xAA << 0) //   - Refresh the IWDG
#define IWDG_KR_KEY_ACCESS                 (0x55 << 0) //   - Enables the access to the IWDG_PR and IWDG_RLR

#define IWDG_PR_PR                         (0x07 << 0) // Prescaler divider

#define IWDG_RLR_RL                        (0xFF << 0) // Watchdog counter reload value

// == WWDG ============================================================================================================

typedef struct {
    _RW uint8_t CR;               // Control register
    _RW uint8_t WR;               // Window register
} WWDG_t;
static_assert(offsetof(WWDG_t, WR) == 0x01, "Wrong definition");

#define WWDG_CR_WDGA                       (   1 << 7) // Activation bit
#define WWDG_CR_T                          (0x7F << 0) // 7-bit counter (MSB to LSB)

#define WWDG_WR_W                          (0x07 << 0) // 7-bit window value

// == TIM1 ============================================================================================================

typedef struct {
    _RW uint8_t CR1;              // Control register 1
    _RW uint8_t CR2;              // Control register 2
    _RW uint8_t SMCR;             // Slave mode control register
    _RW uint8_t ETR;              // External trigger register
    _RW uint8_t IER;              // Interrupt enable register
    _RW uint8_t SR1;              // Status register 1
    _RW uint8_t SR2;              // Status register 2
    _WO uint8_t EGR;              // Event generation register
    _RW uint8_t CCMR1;            // Capture/compare mode register 1
    _RW uint8_t CCMR2;            // Capture/compare mode register 2
    _RW uint8_t CCMR3;            // Capture/compare mode register 3
    _RW uint8_t CCMR4;            // Capture/compare mode register 4
    _RW uint8_t CCER1;            // Capture/compare enable register 1
    _RW uint8_t CCER2;            // Capture/compare enable register 2
    _RW uint8_t CNTRH;            // Counter high
    _RW uint8_t CNTRL;            // Counter low
    _RW uint8_t PSCRH;            // Prescaler register high
    _RW uint8_t PSCRL;            // Prescaler register low
    _RW uint8_t ARRH;             // Auto-reload register high
    _RW uint8_t ARRL;             // Auto-reload register low
    _RW uint8_t RCR;              // Repetition counter register
    _RW uint8_t CCR1H;            // Capture/compare register 1 high
    _RW uint8_t CCR1L;            // Capture/compare register 1 low
    _RW uint8_t CCR2H;            // Capture/compare register 2 high
    _RW uint8_t CCR2L;            // Capture/compare register 2 low
    _RW uint8_t CCR3H;            // Capture/compare register 3 high
    _RW uint8_t CCR3L;            // Capture/compare register 3 low
    _RW uint8_t CCR4H;            // Capture/compare register 4 high
    _RW uint8_t CCR4L;            // Capture/compare register 4 low
    _RW uint8_t BKR;              // Break register
    _RW uint8_t DTR;              // Deadtime register
    _RW uint8_t OISR;             // Output idle state register
} TIM1_t;
static_assert(offsetof(TIM1_t, OISR) == 0x1F, "Wrong definition");

#define TIM1_CR1_ARPE                      (   1 << 7) // Auto-reload preload enable
#define TIM1_CR1_CMS                       (0x03 << 5) // Center-aligned mode selection
#define TIM1_CR1_DIR                       (   1 << 4) // Direction
#define TIM1_CR1_OPM                       (   1 << 3) // One-pulse mode
#define TIM1_CR1_URS                       (   1 << 2) // Update request source
#define TIM1_CR1_UDIS                      (   1 << 1) // Update disable
#define TIM1_CR1_CEN                       (   1 << 0) // Counter enable

#define TIM1_CR2_MMS                       (0x07 << 4) // Master mode selection
#define TIM1_CR2_COMS                      (   1 << 2) // Capture/compare control update selection
#define TIM1_CR2_CCPC                      (   1 << 0) // Capture/compare preloaded control

#define TIM1_SMCR_MSM                      (   1 << 7) // Master/slave mode
#define TIM1_SMCR_TS                       (0x07 << 4) // Trigger selection
#define TIM1_SMCR_SMS                      (0x07 << 0) // Clock/trigger/slave mode selection

#define TIM1_ETR_ETP                       (   1 << 7) // External trigger polarity
#define TIM1_ETR_ECE                       (   1 << 6) // External clock enable
#define TIM1_ETR_ETPS                      (0x03 << 4) // External trigger prescaler
#define TIM1_ETR_ETF                       (0x0F << 0) // External trigger filter

#define TIM1_IER_BIE                       (   1 << 7) // Break interrupt enable
#define TIM1_IER_TIE                       (   1 << 6) // Trigger interrupt enable
#define TIM1_IER_COMIE                     (   1 << 5) // Commutation interrupt enable
#define TIM1_IER_CC4IE                     (   1 << 4) // Capture/compare 4 interrupt enable
#define TIM1_IER_CC3IE                     (   1 << 3) // Capture/compare 3 interrupt enable
#define TIM1_IER_CC2IE                     (   1 << 2) // Capture/compare 2 interrupt enable
#define TIM1_IER_CC1IE                     (   1 << 1) // Capture/compare 1 interrupt enable
#define TIM1_IER_UIE                       (   1 << 0) // Update interrupt enable

#define TIM1_SR1_BIF                       (   1 << 7) // Break interrupt flag
#define TIM1_SR1_TIF                       (   1 << 6) // Trigger interrupt flag
#define TIM1_SR1_COMIF                     (   1 << 5) // Commutation interrupt flag
#define TIM1_SR1_CC4IF                     (   1 << 4) // Capture/compare 4 interrupt flag
#define TIM1_SR1_CC3IF                     (   1 << 3) // Capture/compare 3 interrupt flag
#define TIM1_SR1_CC2IF                     (   1 << 2) // Capture/compare 2 interrupt flag
#define TIM1_SR1_CC1IF                     (   1 << 1) // Capture/compare 1 interrupt flag
#define TIM1_SR1_UIF                       (   1 << 0) // Update interrupt flag

#define TIM1_SR2_CC4OF                     (   1 << 4) // Capture/compare 4 overcapture flag
#define TIM1_SR2_CC3OF                     (   1 << 3) // Capture/compare 3 overcapture flag
#define TIM1_SR2_CC2OF                     (   1 << 2) // Capture/compare 2 overcapture flag
#define TIM1_SR2_CC1OF                     (   1 << 1) // Capture/compare 1 overcapture flag

#define TIM1_EGR_BG                        (   1 << 7) // Break generation
#define TIM1_EGR_TG                        (   1 << 6) // Trigger generation
#define TIM1_EGR_COMG                      (   1 << 5) // Commutation generation
#define TIM1_EGR_CC4G                      (   1 << 4) // Capture/compare 4 generation
#define TIM1_EGR_CC3G                      (   1 << 3) // Capture/compare 3 generation
#define TIM1_EGR_CC2G                      (   1 << 2) // Capture/compare 2 generation
#define TIM1_EGR_CC1G                      (   1 << 1) // Capture/compare 1 generation
#define TIM1_EGR_UG                        (   1 << 0) // Update generation

#define TIM1_CCMR1_OC1CE                   (   1 << 7) // Output compare 1 clear enable
#define TIM1_CCMR1_OC1M                    (0x07 << 4) // Output compare 1 mode
#define TIM1_CCMR1_OC1M_FROSEN             (0x00 << 4) //   - Frozen
#define TIM1_CCMR1_OC1M_ACTIVE             (0x01 << 4) //   - Set channel to active level on match
#define TIM1_CCMR1_OC1M_INACTIVE           (0x02 << 4) //   - Set channel to inactive level on match
#define TIM1_CCMR1_OC1M_TOGGLE             (0x03 << 4) //   - Toggle
#define TIM1_CCMR1_OC1M_FORCELOW           (0x04 << 4) //   - Force inactive level
#define TIM1_CCMR1_OC1M_FORCEHIGH          (0x05 << 4) //   - Force active level
#define TIM1_CCMR1_OC1M_PWM1               (0x06 << 4) //   - PWM mode 1
#define TIM1_CCMR1_OC1M_PWM2               (0x07 << 4) //   - PWM mode 2
#define TIM1_CCMR1_OC1PE                   (   1 << 3) // Output compare 1 preload enable
#define TIM1_CCMR1_OC1FE                   (   1 << 2) // Output compare 1 fast enable
#define TIM1_CCMR1_IC1F                    (0x0F << 4) // Input capture 1 filter
#define TIM1_CCMR1_IC1PSC                  (0x03 << 2) // Input capture 1 prescaler
#define TIM1_CCMR1_CC1S                    (0x03 << 0) // Capture/compare 1 selection
#define TIM1_CCMR1_CC1S_OUT                (0x00 << 0) //   - output
#define TIM1_CCMR1_CC1S_TI1                (0x01 << 0) //   - input, IC1 is mapped on TI1FP1
#define TIM1_CCMR1_CC1S_TI2                (0x02 << 0) //   - input, IC1 is mapped on TI2FP1
#define TIM1_CCMR1_CC1S_TRC                (0x03 << 0) //   - input, IC1 is mapped on TRC

#define TIM1_CCMR2_OC2CE                   (   1 << 7) // Output compare 2 clear enable
#define TIM1_CCMR2_OC2M                    (0x07 << 4) // Output compare 2 mode
#define TIM1_CCMR2_OC2M_FROSEN             (0x00 << 4) //   - Frozen
#define TIM1_CCMR2_OC2M_ACTIVE             (0x01 << 4) //   - Set channel to active level on match
#define TIM1_CCMR2_OC2M_INACTIVE           (0x02 << 4) //   - Set channel to inactive level on match
#define TIM1_CCMR2_OC2M_TOGGLE             (0x03 << 4) //   - Toggle
#define TIM1_CCMR2_OC2M_FORCELOW           (0x04 << 4) //   - Force inactive level
#define TIM1_CCMR2_OC2M_FORCEHIGH          (0x05 << 4) //   - Force active level
#define TIM1_CCMR2_OC2M_PWM1               (0x06 << 4) //   - PWM mode 1
#define TIM1_CCMR2_OC2M_PWM2               (0x07 << 4) //   - PWM mode 2
#define TIM1_CCMR2_OC2PE                   (   1 << 3) // Output compare 2 preload enable
#define TIM1_CCMR2_OC2FE                   (   1 << 2) // Output compare 2 fast enable
#define TIM1_CCMR2_IC2F                    (0x0F << 4) // Input capture 2 filter
#define TIM1_CCMR2_IC2PSC                  (0x03 << 2) // Input capture 2 prescaler
#define TIM1_CCMR2_CC2S                    (0x03 << 0) // Capture/compare 2 selection
#define TIM1_CCMR2_CC2S_OUT                (0x00 << 0) //   - output
#define TIM1_CCMR2_CC2S_TI1                (0x01 << 0) //   - input, IC2 is mapped on TI2FP2
#define TIM1_CCMR2_CC2S_TI2                (0x02 << 0) //   - input, IC2 is mapped on TI1FP2
#define TIM1_CCMR2_CC2S_TRC                (0x03 << 0) //   - input, IC2 is mapped on TRC

#define TIM1_CCMR3_OC3CE                   (   1 << 7) // Output compare 3 clear enable
#define TIM1_CCMR3_OC3M                    (0x07 << 4) // Output compare 3 mode
#define TIM1_CCMR3_OC3M_FROSEN             (0x00 << 4) //   - Frozen
#define TIM1_CCMR3_OC3M_ACTIVE             (0x01 << 4) //   - Set channel to active level on match
#define TIM1_CCMR3_OC3M_INACTIVE           (0x02 << 4) //   - Set channel to inactive level on match
#define TIM1_CCMR3_OC3M_TOGGLE             (0x03 << 4) //   - Toggle
#define TIM1_CCMR3_OC3M_FORCELOW           (0x04 << 4) //   - Force inactive level
#define TIM1_CCMR3_OC3M_FORCEHIGH          (0x05 << 4) //   - Force active level
#define TIM1_CCMR3_OC3M_PWM1               (0x06 << 4) //   - PWM mode 1
#define TIM1_CCMR3_OC3M_PWM2               (0x07 << 4) //   - PWM mode 2
#define TIM1_CCMR3_OC3PE                   (   1 << 3) // Output compare 3 preload enable
#define TIM1_CCMR3_OC3FE                   (   1 << 2) // Output compare 3 fast enable
#define TIM1_CCMR3_IC3F                    (0x0F << 4) // Input capture 3 filter
#define TIM1_CCMR3_IC3PSC                  (0x03 << 2) // Input capture 3 prescaler
#define TIM1_CCMR3_CC3S                    (0x03 << 0) // Capture/compare 3 selection
#define TIM1_CCMR3_CC3S_OUT                (0x00 << 0) //   - output
#define TIM1_CCMR3_CC3S_TI1                (0x01 << 0) //   - input, IC3 is mapped on TI3FP3
#define TIM1_CCMR3_CC3S_TI2                (0x02 << 0) //   - input, IC3 is mapped on TI4FP3

#define TIM1_CCMR4_OC4CE                   (   1 << 7) // Output compare 4 clear enable
#define TIM1_CCMR4_OC4M                    (0x07 << 4) // Output compare 4 mode
#define TIM1_CCMR4_OC4M_FROSEN             (0x00 << 4) //   - Frozen
#define TIM1_CCMR4_OC4M_ACTIVE             (0x01 << 4) //   - Set channel to active level on match
#define TIM1_CCMR4_OC4M_INACTIVE           (0x02 << 4) //   - Set channel to inactive level on match
#define TIM1_CCMR4_OC4M_TOGGLE             (0x03 << 4) //   - Toggle
#define TIM1_CCMR4_OC4M_FORCELOW           (0x04 << 4) //   - Force inactive level
#define TIM1_CCMR4_OC4M_FORCEHIGH          (0x05 << 4) //   - Force active level
#define TIM1_CCMR4_OC4M_PWM1               (0x06 << 4) //   - PWM mode 1
#define TIM1_CCMR4_OC4M_PWM2               (0x07 << 4) //   - PWM mode 2
#define TIM1_CCMR4_OC4PE                   (   1 << 3) // Output compare 4 preload enable
#define TIM1_CCMR4_OC4FE                   (   1 << 2) // Output compare 4 fast enable
#define TIM1_CCMR4_IC4F                    (0x0F << 4) // Input capture 4 filter
#define TIM1_CCMR4_IC4PSC                  (0x03 << 2) // Input capture 4 prescaler
#define TIM1_CCMR4_CC4S                    (0x03 << 0) // Capture/compare 4 selection
#define TIM1_CCMR4_CC4S_OUT                (0x00 << 0) //   - output
#define TIM1_CCMR4_CC4S_TI1                (0x01 << 0) //   - input, IC4 is mapped on TI4FP4
#define TIM1_CCMR4_CC4S_TI2                (0x02 << 0) //   - input, IC4 is mapped on TI3FP4

#define TIM1_CCER1_CC2NP                   (   1 << 7) // Capture/compare 2 complementary output polarity
#define TIM1_CCER1_CC2NE                   (   1 << 6) // Capture/compare 2 complementary output enable
#define TIM1_CCER1_CC2P                    (   1 << 5) // Capture/compare 2 output polarity
#define TIM1_CCER1_CC2E                    (   1 << 4) // Capture/compare 2 output enable
#define TIM1_CCER1_CC1NP                   (   1 << 3) // Capture/compare 1 complementary output polarity
#define TIM1_CCER1_CC1NE                   (   1 << 2) // Capture/compare 1 complementary output enable
#define TIM1_CCER1_CC1P                    (   1 << 1) // Capture/compare 1 output polarity
#define TIM1_CCER1_CC1E                    (   1 << 0) // Capture/compare 1 output enable

#define TIM1_CCER2_CC4P                    (   1 << 5) // Capture/compare 4 output polarity
#define TIM1_CCER2_CC4E                    (   1 << 4) // Capture/compare 4 output enable
#define TIM1_CCER2_CC3NP                   (   1 << 3) // Capture/compare 3 complementary output polarity
#define TIM1_CCER2_CC3NE                   (   1 << 2) // Capture/compare 3 complementary output enable
#define TIM1_CCER2_CC3P                    (   1 << 1) // Capture/compare 3 output polarity
#define TIM1_CCER2_CC3E                    (   1 << 0) // Capture/compare 3 output enable

#define TIM1_CNTRH_CNT                     (0xFF << 0) // Counter value (MSB)

#define TIM1_CNTRL_CNT                     (0xFF << 0) // Counter value (LSB)

#define TIM1_PSCRH_PSC                     (0xFF << 0) // Prescaler value (MBS)

#define TIM1_PSCRL_PSC                     (0xFF << 0) // Prescaler value (LBS)

#define TIM1_ARRH_ARR                      (0xFF << 0) // Auto-reload value (MSB)

#define TIM1_ARRL_ARR                      (0xFF << 0) // Auto-reload value (LSB)

#define TIM1_RCR_REP                       (0xFF << 0) // Repetition counter value

#define TIM1_CCR1H_CCR1                    (0xFF << 0) // Capture/compare 1 value (MSB)

#define TIM1_CCR1L_CCR1                    (0xFF << 0) // Capture/compare 1 value (LSB)

#define TIM1_CCR2H_CCR2                    (0xFF << 0) // Capture/compare 2 value (MSB)

#define TIM1_CCR2L_CCR2                    (0xFF << 0) // Capture/compare 2 value (LSB)

#define TIM1_CCR3H_CCR3                    (0xFF << 0) // Capture/compare 3 value (MSB)

#define TIM1_CCR3L_CCR3                    (0xFF << 0) // Capture/compare 3 value (LSB)

#define TIM1_CCR4H_CCR4                    (0xFF << 0) // Capture/compare 4 value (MSB)

#define TIM1_CCR4L_CCR4                    (0xFF << 0) // Capture/compare 4 value (LSB)

#define TIM1_BKR_MOE                       (   1 << 7) // Main output enable
#define TIM1_BKR_AOE                       (   1 << 6) // Automatic output enable
#define TIM1_BKR_BKP                       (   1 << 5) // Break polarity
#define TIM1_BKR_BKE                       (   1 << 4) // Break enable
#define TIM1_BKR_OSSR                      (   1 << 3) // Off state selection for Run mode
#define TIM1_BKR_OSSI                      (   1 << 2) // Off state selection for Idle mode
#define TIM1_BKR_LOCK                      (0x03 << 0) // Lock configuration

#define TIM1_DTR_DTG                       (0xFF << 0) // Deadtime generator set-up

#define TIM1_OISR_OIS4                     (   1 << 6) // Output idle state 4 (OC4 output)
#define TIM1_OISR_OIS3N                    (   1 << 5) // Output idle state 3 (OC3N output)
#define TIM1_OISR_OIS3                     (   1 << 4) // Output idle state 3 (OC3 output)
#define TIM1_OISR_OIS2N                    (   1 << 3) // Output idle state 2 (OC2N output)
#define TIM1_OISR_OIS2                     (   1 << 2) // Output idle state 2 (OC2 output)
#define TIM1_OISR_OIS1N                    (   1 << 1) // Output idle state 1 (OC1N output)
#define TIM1_OISR_OIS1                     (   1 << 0) // Output idle state 1 (OC1 output)

// == TIM2 ============================================================================================================

typedef struct {
    _RW uint8_t CR1;              // Control register 1
    _RW uint8_t IER;              // Interrupt enable register
    _RW uint8_t SR1;              // Status register 1
    _RW uint8_t SR2;              // Status register 2
    _WO uint8_t EGR;              // Event generation register
    _RW uint8_t CCMR1;            // Capture/compare mode register 1
    _RW uint8_t CCMR2;            // Capture/compare mode register 2
    _RW uint8_t CCMR3;            // Capture/compare mode register 3
    _RW uint8_t CCER1;            // Capture/compare enable register 1
    _RW uint8_t CCER2;            // Capture/compare enable register 2
    _RW uint8_t CNTRH;            // Counter high
    _RW uint8_t CNTRL;            // Counter low
    _RW uint8_t PSCR;             // Prescaler register
    _RW uint8_t ARRH;             // Auto-reload register high
    _RW uint8_t ARRL;             // Auto-reload register low
    _RW uint8_t CCR1H;            // Capture/compare register 1 high
    _RW uint8_t CCR1L;            // Capture/compare register 1 low
    _RW uint8_t CCR2H;            // Capture/compare register 2 high
    _RW uint8_t CCR2L;            // Capture/compare register 2 low
    _RW uint8_t CCR3H;            // Capture/compare register 3 high
    _RW uint8_t CCR3L;            // Capture/compare register 3 low
} TIM2_t;
static_assert(offsetof(TIM2_t, CCR3L) == 0x14, "Wrong definition");

#define TIM2_CR1_ARPE                      (   1 << 7) // Auto-reload preload enable
#define TIM2_CR1_OPM                       (   1 << 3) // One-pulse mode
#define TIM2_CR1_URS                       (   1 << 2) // Update request source
#define TIM2_CR1_UDIS                      (   1 << 1) // Update disable
#define TIM2_CR1_CEN                       (   1 << 0) // Counter enable

#define TIM2_IER_TIE                       (   1 << 6) // Trigger interrupt enable
#define TIM2_IER_CC3IE                     (   1 << 3) // Capture/compare 3 interrupt enable
#define TIM2_IER_CC2IE                     (   1 << 2) // Capture/compare 2 interrupt enable
#define TIM2_IER_CC1IE                     (   1 << 1) // Capture/compare 1 interrupt enable
#define TIM2_IER_UIE                       (   1 << 0) // Update interrupt enable

#define TIM2_SR1_TIF                       (   1 << 6) // Trigger interrupt flag
#define TIM2_SR1_CC3IF                     (   1 << 3) // Capture/compare 3 interrupt flag
#define TIM2_SR1_CC2IF                     (   1 << 2) // Capture/compare 2 interrupt flag
#define TIM2_SR1_CC1IF                     (   1 << 1) // Capture/compare 1 interrupt flag
#define TIM2_SR1_UIF                       (   1 << 0) // Update interrupt flag

#define TIM2_SR2_CC3OF                     (   1 << 3) // Capture/compare 3 overcapture flag
#define TIM2_SR2_CC2OF                     (   1 << 2) // Capture/compare 2 overcapture flag
#define TIM2_SR2_CC1OF                     (   1 << 1) // Capture/compare 1 overcapture flag

#define TIM2_EGR_TG                        (   1 << 6) // Trigger generation
#define TIM2_EGR_CC3G                      (   1 << 3) // Capture/compare 3 generation
#define TIM2_EGR_CC2G                      (   1 << 2) // Capture/compare 2 generation
#define TIM2_EGR_CC1G                      (   1 << 1) // Capture/compare 1 generation
#define TIM2_EGR_UG                        (   1 << 0) // Update generation

#define TIM2_CCMR1_OC1M                    (0x07 << 4) // Output compare 1 mode
#define TIM2_CCMR1_OC1M_FROSEN             (0x00 << 4) //   - Frozen
#define TIM2_CCMR1_OC1M_ACTIVE             (0x01 << 4) //   - Set channel to active level on match
#define TIM2_CCMR1_OC1M_INACTIVE           (0x02 << 4) //   - Set channel to inactive level on match
#define TIM2_CCMR1_OC1M_TOGGLE             (0x03 << 4) //   - Toggle
#define TIM2_CCMR1_OC1M_FORCELOW           (0x04 << 4) //   - Force inactive level
#define TIM2_CCMR1_OC1M_FORCEHIGH          (0x05 << 4) //   - Force active level
#define TIM2_CCMR1_OC1M_PWM1               (0x06 << 4) //   - PWM mode 1
#define TIM2_CCMR1_OC1M_PWM2               (0x07 << 4) //   - PWM mode 2
#define TIM2_CCMR1_OC1PE                   (   1 << 3) // Output compare 1 preload enable
#define TIM2_CCMR1_IC1F                    (0x0F << 4) // Input capture 1 filter
#define TIM2_CCMR1_IC1PSC                  (0x03 << 2) // Input capture 1 prescaler
#define TIM2_CCMR1_CC1S                    (0x03 << 0) // Capture/compare 1 selection

#define TIM2_CCMR2_OC2M                    (0x07 << 4) // Output compare 2 mode
#define TIM2_CCMR2_OC2M_FROSEN             (0x00 << 4) //   - Frozen
#define TIM2_CCMR2_OC2M_ACTIVE             (0x01 << 4) //   - Set channel to active level on match
#define TIM2_CCMR2_OC2M_INACTIVE           (0x02 << 4) //   - Set channel to inactive level on match
#define TIM2_CCMR2_OC2M_TOGGLE             (0x03 << 4) //   - Toggle
#define TIM2_CCMR2_OC2M_FORCELOW           (0x04 << 4) //   - Force inactive level
#define TIM2_CCMR2_OC2M_FORCEHIGH          (0x05 << 4) //   - Force active level
#define TIM2_CCMR2_OC2M_PWM1               (0x06 << 4) //   - PWM mode 1
#define TIM2_CCMR2_OC2M_PWM2               (0x07 << 4) //   - PWM mode 2
#define TIM2_CCMR2_OC2PE                   (   1 << 3) // Output compare 2 preload enable
#define TIM2_CCMR2_IC2F                    (0x0F << 4) // Input capture 2 filter
#define TIM2_CCMR2_IC2PSC                  (0x03 << 2) // Input capture 2 prescaler
#define TIM2_CCMR2_CC2S                    (0x03 << 0) // Capture/compare 2 selection

#define TIM2_CCMR3_OC3M                    (0x07 << 4) // Output compare 3 mode
#define TIM2_CCMR3_OC3M_FROSEN             (0x00 << 4) //   - Frozen
#define TIM2_CCMR3_OC3M_ACTIVE             (0x01 << 4) //   - Set channel to active level on match
#define TIM2_CCMR3_OC3M_INACTIVE           (0x02 << 4) //   - Set channel to inactive level on match
#define TIM2_CCMR3_OC3M_TOGGLE             (0x03 << 4) //   - Toggle
#define TIM2_CCMR3_OC3M_FORCELOW           (0x04 << 4) //   - Force inactive level
#define TIM2_CCMR3_OC3M_FORCEHIGH          (0x05 << 4) //   - Force active level
#define TIM2_CCMR3_OC3M_PWM1               (0x06 << 4) //   - PWM mode 1
#define TIM2_CCMR3_OC3M_PWM2               (0x07 << 4) //   - PWM mode 2
#define TIM2_CCMR3_OC3PE                   (   1 << 3) // Output compare 3 preload enable
#define TIM2_CCMR3_IC3F                    (0x0F << 4) // Input capture 3 filter
#define TIM2_CCMR3_IC3PSC                  (0x03 << 2) // Input capture 3 prescaler
#define TIM2_CCMR3_CC3S                    (0x03 << 0) // Capture/compare 3 selection

#define TIM2_CCER1_CC2P                    (   1 << 5) // Capture/compare 2 output polarity
#define TIM2_CCER1_CC2E                    (   1 << 4) // Capture/compare 2 output enable
#define TIM2_CCER1_CC1P                    (   1 << 1) // Capture/compare 1 output polarity
#define TIM2_CCER1_CC1E                    (   1 << 0) // Capture/compare 1 output enable

#define TIM2_CCER2_CC3P                    (   1 << 1) // Capture/compare 3 output polarity
#define TIM2_CCER2_CC3E                    (   1 << 0) // Capture/compare 3 output enable

#define TIM2_CNTRH_CNT                     (0xFF << 0) // Counter value (MSB)

#define TIM2_CNTRL_CNT                     (0xFF << 0) // Counter value (LSB)

#define TIM2_PSCR_PSC                      (0x0F << 0) // Prescaler value

#define TIM2_ARRH_ARR                      (0xFF << 0) // Auto-reload value (MSB)

#define TIM2_ARRL_ARR                      (0xFF << 0) // Auto-reload value (LSB)

#define TIM2_CCR1H_CCR1                    (0xFF << 0) // Capture/compare 1 value (MSB)

#define TIM2_CCR1L_CCR1                    (0xFF << 0) // Capture/compare 1 value (LSB)

#define TIM2_CCR2H_CCR2                    (0xFF << 0) // Capture/compare 2 value (MSB)

#define TIM2_CCR2L_CCR2                    (0xFF << 0) // Capture/compare 2 value (LSB)

#define TIM2_CCR3H_CCR3                    (0xFF << 0) // Capture/compare 3 value (MSB)

#define TIM2_CCR3L_CCR3                    (0xFF << 0) // Capture/compare 3 value (LSB)

// == TIM3 ============================================================================================================

typedef struct {
    _RW uint8_t CR1;              // Control register 1
    _RW uint8_t IER;              // Interrupt enable register
    _RW uint8_t SR1;              // Status register 1
    _RW uint8_t SR2;              // Status register 2
    _WO uint8_t EGR;              // Event generation register
    _RW uint8_t CCMR1;            // Capture/compare mode register 1
    _RW uint8_t CCMR2;            // Capture/compare mode register 2
    _RW uint8_t CCER1;            // Capture/compare enable register 1
    _RW uint8_t CNTRH;            // Counter high
    _RW uint8_t CNTRL;            // Counter low
    _RW uint8_t PSCR;             // Prescaler register
    _RW uint8_t ARRH;             // Auto-reload register high
    _RW uint8_t ARRL;             // Auto-reload register low
    _RW uint8_t CCR1H;            // Capture/compare register 1 high
    _RW uint8_t CCR1L;            // Capture/compare register 1 low
    _RW uint8_t CCR2H;            // Capture/compare register 2 high
    _RW uint8_t CCR2L;            // Capture/compare register 2 low
} TIM3_t;
static_assert(offsetof(TIM3_t, CCR2L) == 0x10, "Wrong definition");

#define TIM3_CR1_ARPE                      (   1 << 7) // Auto-reload preload enable
#define TIM3_CR1_OPM                       (   1 << 3) // One-pulse mode
#define TIM3_CR1_URS                       (   1 << 2) // Update request source
#define TIM3_CR1_UDIS                      (   1 << 1) // Update disable
#define TIM3_CR1_CEN                       (   1 << 0) // Counter enable

#define TIM3_IER_TIE                       (   1 << 6) // Trigger interrupt enable
#define TIM3_IER_CC3IE                     (   1 << 3) // Capture/compare 3 interrupt enable
#define TIM3_IER_CC2IE                     (   1 << 2) // Capture/compare 2 interrupt enable
#define TIM3_IER_CC1IE                     (   1 << 1) // Capture/compare 1 interrupt enable
#define TIM3_IER_UIE                       (   1 << 0) // Update interrupt enable

#define TIM3_SR1_TIF                       (   1 << 6) // Trigger interrupt flag
#define TIM3_SR1_CC3IF                     (   1 << 3) // Capture/compare 3 interrupt flag
#define TIM3_SR1_CC2IF                     (   1 << 2) // Capture/compare 2 interrupt flag
#define TIM3_SR1_CC1IF                     (   1 << 1) // Capture/compare 1 interrupt flag
#define TIM3_SR1_UIF                       (   1 << 0) // Update interrupt flag

#define TIM3_SR2_CC3OF                     (   1 << 3) // Capture/compare 3 overcapture flag
#define TIM3_SR2_CC2OF                     (   1 << 2) // Capture/compare 2 overcapture flag
#define TIM3_SR2_CC1OF                     (   1 << 1) // Capture/compare 1 overcapture flag

#define TIM3_EGR_TG                        (   1 << 6) // Trigger generation
#define TIM3_EGR_CC3G                      (   1 << 3) // Capture/compare 3 generation
#define TIM3_EGR_CC2G                      (   1 << 2) // Capture/compare 2 generation
#define TIM3_EGR_CC1G                      (   1 << 1) // Capture/compare 1 generation
#define TIM3_EGR_UG                        (   1 << 0) // Update generation

#define TIM3_CCMR1_OC1M                    (0x07 << 4) // Output compare 1 mode
#define TIM3_CCMR1_OC1M_FROSEN             (0x00 << 4) //   - Frozen
#define TIM3_CCMR1_OC1M_ACTIVE             (0x01 << 4) //   - Set channel to active level on match
#define TIM3_CCMR1_OC1M_INACTIVE           (0x02 << 4) //   - Set channel to inactive level on match
#define TIM3_CCMR1_OC1M_TOGGLE             (0x03 << 4) //   - Toggle
#define TIM3_CCMR1_OC1M_FORCELOW           (0x04 << 4) //   - Force inactive level
#define TIM3_CCMR1_OC1M_FORCEHIGH          (0x05 << 4) //   - Force active level
#define TIM3_CCMR1_OC1M_PWM1               (0x06 << 4) //   - PWM mode 1
#define TIM3_CCMR1_OC1M_PWM2               (0x07 << 4) //   - PWM mode 2
#define TIM3_CCMR1_OC1PE                   (   1 << 3) // Output compare 1 preload enable
#define TIM3_CCMR1_IC1F                    (0x0F << 4) // Input capture 1 filter
#define TIM3_CCMR1_IC1PSC                  (0x03 << 2) // Input capture 1 prescaler
#define TIM3_CCMR1_CC1S                    (0x03 << 0) // Capture/compare 1 selection

#define TIM3_CCMR2_OC2M                    (0x07 << 4) // Output compare 2 mode
#define TIM3_CCMR2_OC2M_FROSEN             (0x00 << 4) //   - Frozen
#define TIM3_CCMR2_OC2M_ACTIVE             (0x01 << 4) //   - Set channel to active level on match
#define TIM3_CCMR2_OC2M_INACTIVE           (0x02 << 4) //   - Set channel to inactive level on match
#define TIM3_CCMR2_OC2M_TOGGLE             (0x03 << 4) //   - Toggle
#define TIM3_CCMR2_OC2M_FORCELOW           (0x04 << 4) //   - Force inactive level
#define TIM3_CCMR2_OC2M_FORCEHIGH          (0x05 << 4) //   - Force active level
#define TIM3_CCMR2_OC2M_PWM1               (0x06 << 4) //   - PWM mode 1
#define TIM3_CCMR2_OC2M_PWM2               (0x07 << 4) //   - PWM mode 2
#define TIM3_CCMR2_OC2PE                   (   1 << 3) // Output compare 2 preload enable
#define TIM3_CCMR2_IC2F                    (0x0F << 4) // Input capture 2 filter
#define TIM3_CCMR2_IC2PSC                  (0x03 << 2) // Input capture 2 prescaler
#define TIM3_CCMR2_CC2S                    (0x03 << 0) // Capture/compare 2 selection

#define TIM3_CCER1_CC2P                    (   1 << 5) // Capture/compare 2 output polarity
#define TIM3_CCER1_CC2E                    (   1 << 4) // Capture/compare 2 output enable
#define TIM3_CCER1_CC1P                    (   1 << 1) // Capture/compare 1 output polarity
#define TIM3_CCER1_CC1E                    (   1 << 0) // Capture/compare 1 output enable

#define TIM3_CNTRH_CNT                     (0xFF << 0) // Counter value (MSB)

#define TIM3_CNTRL_CNT                     (0xFF << 0) // Counter value (LSB)

#define TIM3_PSCR_PSC                      (0x0F << 0) // Prescaler value

#define TIM3_ARRH_ARR                      (0xFF << 0) // Auto-reload value (MSB)

#define TIM3_ARRL_ARR                      (0xFF << 0) // Auto-reload value (LSB)

#define TIM3_CCR1H_CCR1                    (0xFF << 0) // Capture/compare 1 value (MSB)

#define TIM3_CCR1L_CCR1                    (0xFF << 0) // Capture/compare 1 value (LSB)

#define TIM3_CCR2H_CCR2                    (0xFF << 0) // Capture/compare 2 value (MSB)

#define TIM3_CCR2L_CCR2                    (0xFF << 0) // Capture/compare 2 value (LSB)

// == TIM4 ============================================================================================================

typedef struct {
    _RW uint8_t CR1;              // Control register 1
    _RW uint8_t IER;              // Interrupt enable register
    _RW uint8_t SR1;              // Status register
    _WO uint8_t EGR;              // Event generation register
    _RW uint8_t CNTR;             // Counter
    _RW uint8_t PSCR;             // Prescaler register
    _RW uint8_t ARR;              // Auto-reload register
} TIM4_t;
static_assert(offsetof(TIM4_t, ARR) == 0x06, "Wrong definition");

#define TIM4_CR1_ARPE                      (   1 << 7) // Auto-reload preload enable
#define TIM4_CR1_OPM                       (   1 << 3) // One-pulse mode
#define TIM4_CR1_URS                       (   1 << 2) // Update request source
#define TIM4_CR1_UDIS                      (   1 << 1) // Update disable
#define TIM4_CR1_CEN                       (   1 << 0) // Counter enable

#define TIM4_IER_TIE                       (   1 << 6) // Trigger interrupt enable
#define TIM4_IER_UIE                       (   1 << 0) // Update interrupt enable

#define TIM4_SR1_TIF                       (   1 << 6) // Trigger interrupt flag
#define TIM4_SR1_UIF                       (   1 << 0) // Update interrupt flag

#define TIM4_EGR_TG                        (   1 << 6) // Trigger generation
#define TIM4_EGR_UG                        (   1 << 0) // Update generation

#define TIM4_CNTR_CNT                      (0xFF << 0) // Counter value

#define TIM4_PSCR_PSC                      (0x07 << 0) // Prescaler value

#define TIM4_ARR_ARR                       (0xFF << 0) // Auto-reload value

// == UART2 ===========================================================================================================

typedef struct {
    _RW uint8_t SR;               // Status register
    _RW uint8_t DR;               // Data register
    _RW uint8_t BRR1;             // Baud rate register 1
    _RW uint8_t BRR2;             // Baud rate register 2
    _RW uint8_t CR1;              // Control register 1
    _RW uint8_t CR2;              // Control register 2
    _RW uint8_t CR3;              // Control register 3
    _RW uint8_t CR4;              // Control register 4
    _RW uint8_t CR5;              // Control register 5
    _RW uint8_t CR6;              // Control register 6
    _RW uint8_t GTR;              // Guard time register
    _RW uint8_t PSCR;             // Prescaler register
} UART2_t;
static_assert(offsetof(UART2_t, PSCR) == 0x0B, "Wrong definition");

#define UART_SR_TXE                        (   1 << 7) // Transmit data register empty
#define UART_SR_TC                         (   1 << 6) // Transmission complete
#define UART_SR_RXNE                       (   1 << 5) // Read data register not empty
#define UART_SR_IDLE                       (   1 << 4) // IDLE line detected
#define UART_SR_OR                         (   1 << 3) // Overrun error/LIN Header Error
#define UART_SR_NF                         (   1 << 2) // Noise flag
#define UART_SR_FE                         (   1 << 1) // Framing error
#define UART_SR_PE                         (   1 << 0) // Parity error

#define UART_DR_DR                         (0xFF << 0) // Data value

#define UART_BRR1_DIV2                     (0xFF << 0) // These 8 bits define the 2nd and 3rd nibbles of the 16-bit UART divide

#define UART_BRR2_DIV1                     (0x0F << 4) // These 4 bits define the MSB of the UART Divide
#define UART_BRR2_DIV3                     (0x0F << 0) // These 4 bits define the LSB of the UART Divide

#define UART_CR1_R8                        (   1 << 7) // Receive Data bit 8
#define UART_CR1_T8                        (   1 << 6) // Transmit data bit 8
#define UART_CR1_UARTD                     (   1 << 5) // UART Disable (for low power consumption)
#define UART_CR1_M                         (   1 << 4) // Word length
#define UART_CR1_WAKE                      (   1 << 3) // Wakeup method
#define UART_CR1_PCEN                      (   1 << 2) // Parity control enable
#define UART_CR1_PS                        (   1 << 1) // Parity selection
#define UART_CR1_PIEN                      (   1 << 0) // Parity interrupt enable

#define UART_CR2_TIEN                      (   1 << 7) // Transmitter interrupt enable
#define UART_CR2_TCIEN                     (   1 << 6) // Transmission complete interrupt enable
#define UART_CR2_RIEN                      (   1 << 5) // Receiver interrupt enable
#define UART_CR2_ILIEN                     (   1 << 4) // IDLE Line interrupt enable
#define UART_CR2_TEN                       (   1 << 3) // Transmitter enable
#define UART_CR2_REN                       (   1 << 2) // Receiver enable
#define UART_CR2_RWU                       (   1 << 1) // Receiver wakeup
#define UART_CR2_SBK                       (   1 << 0) // Send break

#define UART_CR3_LINEN                     (   1 << 6) // LIN mode enable
#define UART_CR3_STOP                      (0x03 << 4) // STOP bits
#define UART_CR3_CLKEN                     (   1 << 3) // Clock enable
#define UART_CR3_CPOL                      (   1 << 2) // Clock polarity
#define UART_CR3_CPHA                      (   1 << 1) // Clock phase
#define UART_CR3_LBCL                      (   1 << 0) // Last bit clock pulse

#define UART_CR4_LBDIEN                    (   1 << 6) // LIN break detection interrupt enable
#define UART_CR4_LBDL                      (   1 << 5) // LIN break detection length
#define UART_CR4_LBDF                      (   1 << 4) // LIN break detection flag
#define UART_CR4_ADD                       (0x0F << 0) // Address of the UART node

#define UART_CR5_SCEN                      (   1 << 5) // Smartcard mode enable
#define UART_CR5_NACK                      (   1 << 4) // Smartcard NACK enable
#define UART_CR5_HDSEL                     (   1 << 3) // Half-Duplex Selection
#define UART_CR5_IRLP                      (   1 << 2) // IrDA Low Power
#define UART_CR5_IREN                      (   1 << 1) // IrDA mode Enable

#define UART_CR6_LDUM                      (   1 << 7) // LIN divider update method
#define UART_CR6_LSLV                      (   1 << 5) // LIN slave enable
#define UART_CR6_LASE                      (   1 << 4) // LIN automatic resynchronisation enable
#define UART_CR6_LHDIEN                    (   1 << 2) // LIN header detection interrupt enable
#define UART_CR6_LHDF                      (   1 << 1) // LIN header detection flag
#define UART_CR6_LSF                       (   1 << 0) // LIN sync field

#define UART_GTR_GT                        (0xFF << 0) // Guard time value

#define UART_PSCR_PSC                      (0xFF << 0) // Prescaler value

// == ADC1 ============================================================================================================

typedef struct {
    _RO uint8_t H;                // Data buffer register high
    _RO uint8_t L;                // Data buffer register low
} ADC1buffer_t;

typedef struct {
    ADC1buffer_t DB[10];
    _RS uint8_t reserved2[12];
    _RW uint8_t CSR;              // Control/status register
    _RW uint8_t CR1;              // Configuration register 1
    _RW uint8_t CR2;              // Configuration register 2
    _RW uint8_t CR3;              // Configuration register 3
    _RO uint8_t DRH;              // Data register high
    _RO uint8_t DRL;              // Data register low
    _RW uint8_t TDRH;             // Schmitt trigger disable register high
    _RW uint8_t TDRL;             // Schmitt trigger disable register low
    _RW uint8_t HTRH;             // High threshold register high
    _RW uint8_t HTRL;             // High threshold register low
    _RW uint8_t LTRH;             // Low threshold register high
    _RW uint8_t LTRL;             // Low threshold register low
    _RW uint8_t AWSRH;            // Watchdog status register high
    _RW uint8_t AWSRL;            // Watchdog status register low
    _RW uint8_t AWCRH;            // Watchdog control register high
    _RW uint8_t AWCRL;            // Watchdog control register low
} ADC1_t;
static_assert(offsetof(ADC1_t, AWCRL) == 0x2F, "Wrong definition");

#define ADC1_DBxRH_DBH                     (0xFF << 0) // Data bits high

#define ADC1_DBxRL_DBL                     (0xFF << 0) // Data bits low

#define ADC1_CSR_EOC                       (   1 << 7) // End of conversion
#define ADC1_CSR_AWD                       (   1 << 6) // Analog Watchdog flag
#define ADC1_CSR_EOCIE                     (   1 << 5) // Interrupt enable for EOC
#define ADC1_CSR_AWDIE                     (   1 << 4) // Analog watchdog interrupt enable
#define ADC1_CSR_CH                        (0x0F << 0) // Channel selection bits

#define ADC1_CR1_SPSEL                     (0x07 << 4) // Prescaler selection
#define ADC1_CR1_SPSEL_2                   (0x00 << 4) //   - Fmaster/2
#define ADC1_CR1_SPSEL_3                   (0x01 << 4) //   - Fmaster/3
#define ADC1_CR1_SPSEL_4                   (0x02 << 4) //   - Fmaster/4
#define ADC1_CR1_SPSEL_6                   (0x03 << 4) //   - Fmaster/6
#define ADC1_CR1_SPSEL_8                   (0x04 << 4) //   - Fmaster/8
#define ADC1_CR1_SPSEL_10                  (0x05 << 4) //   - Fmaster/10
#define ADC1_CR1_SPSEL_12                  (0x06 << 4) //   - Fmaster/12
#define ADC1_CR1_SPSEL_18                  (0x07 << 4) //   - Fmaster/18
#define ADC1_CR1_CONT                      (   1 << 1) // Continuous conversion
#define ADC1_CR1_ADON                      (   1 << 0) // A/D Converter on/off

#define ADC1_CR2_EXTTRIG                   (   1 << 6) // External trigger enable
#define ADC1_CR2_EXTSEL                    (0x03 << 4) // External event selection
#define ADC1_CR2_ALIGN                     (   1 << 3) // Data alignment
#define ADC1_CR2_SCAN                      (   1 << 1) // Scan mode enable

#define ADC1_CR3_DBUF                      (   1 << 7) // Data buffer enable
#define ADC1_CR3_OVR                       (   1 << 6) // Overrun flag

#define ADC1_DRH_DH                        (0xFF << 0) // Data bits high

#define ADC1_DRL_DL                        (0xFF << 0) // Data bits low

#define ADC1_TDRH_TD                       (0xFF << 0) // Schmitt trigger disable high

#define ADC1_TDRL_TD                       (0xFF << 0) // Schmitt trigger disable low

#define ADC1_HTRH_HT                       (0xFF << 0) // Analog watchdog high voltage threshold (MSB)

#define ADC1_HTRL_HT                       (0x03 << 0) // Analog watchdog high voltage threshold (LBS)

#define ADC1_LTRH_HT                       (0xFF << 0) // Analog watchdog low voltage threshold (MSB)

#define ADC1_LTRL_HT                       (0x03 << 0) // Analog watchdog low voltage threshold (LBS)

#define ADC1_AWSRH_AWS                     (0x03 << 0) // Analog watchdog status flags 9:8

#define ADC1_AWSRL_AWS                     (0xFF << 0) // Analog watchdog status flags 7:0

#define ADC1_AWCRH_AWS                     (0x03 << 0) // Analog watchdog enable bits 9:8

#define ADC1_AWCRL_AWS                     (0xFF << 0) // Analog watchdog enable bits 7:0

// == Interrupts ======================================================================================================

#define IRQN_TLI                                    0  // External top level interrupt
#define IRQN_AWU                                    1  // Auto wake up from halt
#define IRQN_CLK                                    2  // Clock controller
#define IRQN_EXTI0                                  3  // Port A external interrupts
#define IRQN_EXTI1                                  4  // Port B external interrupts
#define IRQN_EXTI2                                  5  // Port C external interrupts
#define IRQN_EXTI3                                  6  // Port D external interrupts
#define IRQN_EXTI4                                  7  // Port E external interrupts
#define IRQN_SPI                                   10  // SPI end of transfer
#define IRQN_TIM1_UP                               11  // TIM1 update/overflow/underflow/trigger/break
#define IRQN_TIM1_CMP                              12  // TIM1 capture/compare
#define IRQN_TIM2_UP                               13  // TIM2 update/overflow
#define IRQN_TIM2_CMP                              14  // TIM2 capture/compare
#define IRQN_TIM3_UP                               15  // TIM3 update/overflow
#define IRQN_TIM3_CMP                              16  // TIM3 capture/compare
#define IRQN_I2C                                   19  // I2C interrupt
#define IRQN_UART2_TX                              20  // UART Tx complete
#define IRQN_UART2_RX                              21  // UART Rx full
#define IRQN_ADC1_EOC                              22  // ADC1 end of conversion/analog watchdog interrupt
#define IRQN_TIM4_UP                               23  // TIM4 update/overflow
#define IRQN_FLASH                                 24  // Flash EOP/WR_PG_DIS

// == Hardware ========================================================================================================

#define OPT             ((OPT_t *)      0x4800)
#define GPIOA           ((GPIO_t *)     0x5000)
#define GPIOB           ((GPIO_t *)     0x5005)
#define GPIOC           ((GPIO_t *)     0x500A)
#define GPIOD           ((GPIO_t *)     0x500F)
#define GPIOE           ((GPIO_t *)     0x5014)
#define GPIOF           ((GPIO_t *)     0x5019)
#define GPIOG           ((GPIO_t *)     0x501E)
#define GPIOH           ((GPIO_t *)     0x5023)
#define GPIOI           ((GPIO_t *)     0x5028)
#define FLASH           ((FLASH_t *)    0x505A)
#define EXTI            ((EXTI_t *)     0x50A0)
#define RST             ((RST_t *)      0x50B3)
#define CLK             ((CLK_t *)      0x50C0)
#define WWDG            ((WWDG_t *)     0x50D1)
#define IWDG            ((IWDG_t *)     0x50E0)
#define AWU             ((AWU_t *)      0x50F0)
#define BEEP            ((BEEP_t *)     0x50F3)
#define UART2           ((UART2_t *)    0x5240)
#define TIM1            ((TIM1_t *)     0x5250)
#define TIM2            ((TIM2_t *)     0x5300)
#define TIM3            ((TIM3_t *)     0x5320)
#define TIM4            ((TIM4_t *)     0x5340)
#define ADC1            ((ADC1_t *)     0x53E0)
#define CPUCFG          ((CPUCFG_t *)   0x7F60)
#define ITC             ((ITC_t *)      0x7F70)

#endif // _STM8_H_

