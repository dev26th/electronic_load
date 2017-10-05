#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "stm8.h"
#include "settings.h"
#include "system.h"
#include "systemtimer.h"
#include "flash.h"
#include "uart.h"
#include "displays.h"
#include "button.h"
#include "encoder.h"
#include "encoderbutton.h"
#include "beep.h"
#include "fan.h"
#include "load.h"
#include "adc.h"

// --------------------------------------------------------------------------------------------------------------------

#define LED_V   0x01
#define LED_AH  0x02
#define LED_WH  0x04
#define LED_A   0x08
#define LED_RUN 0x10
#define LED_1   0x20
#define LED_0   0x40

enum Mode {
    Mode_Booting,
    Mode_MenuFun,
    Mode_MenuBeep,
    Mode_Fun1,
    Mode_Fun1Run,
    Mode_Fun2,
    Mode_Fun2Pre,
    Mode_Fun2Run,
    Mode_Fun2Warn,
    Mode_Fun2Res,
};
static enum Mode mode;

#define ERROR_POLARITY (1 << 0)
#define ERROR_SUPPLY   (1 << 1)
#define ERROR_OUP      (1 << 2)
#define ERROR_OTP      (1 << 3)
#define ERROR_ERT      (1 << 4)
static uint8_t  error;

static volatile uint16_t uCurRaw;
static volatile uint16_t uSenseRaw;
static volatile uint16_t uSupRaw;
static volatile uint16_t tempRaw;

static uint16_t uSet;            // mV
static uint16_t iSet;            // mA
static uint32_t powLimit;        // uW
static uint16_t uCur;            // mV
static uint16_t uSense;          // mV
static uint32_t cycleBeginMs;

const struct ValueCoef* uCoef;
static volatile bool     conn4;
static volatile uint16_t sCount;
static          uint16_t sCountCopy;
static volatile uint32_t sSum;       // raw
static          uint32_t sSumCopy;   // raw

struct MenuState {
    uint8_t fun;
    bool    beep;
};
static struct MenuState menuState;

struct Fun1State {
    bool     warning;
    bool     flip;
    uint32_t lastBeep;
};
static struct Fun1State fun1State;

enum DisplayedValue {
    DisplayedValue_V,
    DisplayedValue_Ah,
    DisplayedValue_Wh
};
struct Fun2State {
    enum DisplayedValue disp;
    uint32_t lastDisp;
    uint16_t u;
    uint32_t ah;       // 1 unit = 1 mA*h
    uint64_t sAh;      // 1 unit = 2 mA*ms
    uint32_t wh;       // 1 unit = 1 mW*h
    uint64_t sWh;      // 1 unit = 2 uW*ms, raw
    bool     flip;
    uint32_t lastBeep;
};
static struct Fun2State fun2State;

enum EncoderMode {
    EncoderMode_I1,
    EncoderMode_I0,
    EncoderMode_U1,
    EncoderMode_U0
};
static enum EncoderMode encoderMode;

enum FanState {
    FanState_Override,
    FanState_Off,
    FanState_Low,
    FanState_Mid,
    FanState_Full
};
static enum FanState fanState;

struct ValueCoef {
    uint16_t offset;
    uint16_t mul;
    uint16_t div;
};
struct Config {
    struct ValueCoef iSetCoef;
    struct ValueCoef uCurCoef;
    struct ValueCoef uSenseCoef;
    uint16_t         uSupMin;       // raw
    uint16_t         tempThreshold;
    uint16_t         tempFanLow;
    uint16_t         tempFanMid;
    uint16_t         tempFanFull;
    uint16_t         tempLimit;
    uint16_t         tempDefect;
    uint16_t         iSetMin;       // mA
    uint16_t         iSetMax;       // mA
    uint16_t         uSetMin;       // mV
    uint16_t         uSetMax;       // mV
    uint16_t         uSenseMin;     // mV
    uint16_t         uNegative;     // raw
    uint16_t         uCurLimit;     // mV
    uint32_t         powLimit;      // mW
    uint32_t         ahMax;         // mAh
    uint32_t         whMax;         // mWh
    uint8_t          fun;
    uint8_t          beepOn;
    uint16_t         uSet;          // mV
    uint16_t         iSet;          // mA
};
static_assert(sizeof(struct Config) <= 128, "Config is bigger than EEPROM");
#define CFG ((struct Config*)0x4000) // begin of the EEPROM

#define INPUT_DISABLE_BUTTON         0x01
#define INPUT_DISABLE_ENCODER        0x02
#define INPUT_DISABLE_ENCODER_BUTTON 0x04
static bool displayOverride;
static uint8_t display[8];
static uint8_t inputDisable;
static uint16_t flowInterval = 0xFFFF;
static uint32_t lastFlow;

// --------------------------------------------------------------------------------------------------------------------
// this functions are called in interrupt context

// ~13 us
void SYSTEMTIMER_onTick(void) {
    GPIOD->ODR |= GPIO_ODR_7;
    BEEP_process();
    ENCODERBUTTON_cycle();
    BUTTON_cycle();

    ++sCount;
    sSum += (conn4 ? uSenseRaw : uCurRaw);

    GPIOD->ODR &= ~GPIO_ODR_7;
}

// ADC-cycle duration: ~515 us from start until return from onResult
void SYSTEMTIMER_onTack(void) {
    ADC_startScanCont(16);
}

// ~7 us
void ADC_onResult(const uint16_t* res) {
    // with small quasi-FIR-filter
    tempRaw   = (tempRaw   + 1) / 2 + res[0];
    uCurRaw   = 4592; //(uCurRaw   + 1) / 2 + res[1];
    uSenseRaw = (uSenseRaw + 1) / 2 + res[2];
    uSupRaw   = (uSupRaw   + 1) / 2 + res[3];  // FIXME actually, we need this value only durung startup. Optimize?
}

// --------------------------------------------------------------------------------------------------------------------

static uint16_t recalcValue(uint16_t raw, const struct ValueCoef* coef) {
    uint32_t v;
    if(raw < coef->offset) return 0;
    v = raw - coef->offset;
    v *= coef->mul;
    v /= coef->div;
    if(v > UINT16_MAX) v = UINT16_MAX;
    return (uint16_t)v;
}

static void recalcValues(void) {
    uCur   = recalcValue(uCurRaw,   &CFG->uCurCoef);
    uSense = recalcValue(uSenseRaw, &CFG->uSenseCoef);
}

static void updateISet(void) {
    LOAD_set(recalcValue(iSet, &CFG->iSetCoef));
}

static void saveMenuSettings(void) {
    FLASH_unlockOpt();

    CFG->fun    = menuState.fun;
    CFG->beepOn = menuState.beep;

    FLASH_waitOpt();
    FLASH_lockOpt();
}

static void loadMenuSettings(void) {
    menuState.fun  = CFG->fun;
    menuState.beep = CFG->beepOn;
}

static void startMenu(void) {
    mode = Mode_MenuFun;
    loadMenuSettings();
}

static void stopMenu(void) {
    saveMenuSettings();
    mode = (CFG->fun == 0) ? Mode_Fun1 : Mode_Fun2;
}

static void startBooting(void) {
    mode = Mode_Booting;

    fanState = FanState_Override;
    FAN_set(100);
    BEEP_beep(BEEP_freq_1k, 250);
}

static void stopBooting(void) {
    FAN_set(0);
    fanState = FanState_Off;

    if(BUTTON_isPressed())
        startMenu();
    else
        mode = (CFG->fun == 0) ? Mode_Fun1 : Mode_Fun2;
}

static void doBooting(void) {
    if(cycleBeginMs >= 500) {
        if(uSupRaw < CFG->uSupMin) { // cannot be reset
            error |= ERROR_SUPPLY;
        }
    }

    if(cycleBeginMs >= 2000) {
        stopBooting();
    }
}

static void saveSettings() {
    if(iSet != CFG->iSet || uSet != CFG->uSet) {
        FLASH_unlockOpt();

        CFG->uSet = uSet;
        CFG->iSet = iSet;

        FLASH_waitOpt();
        FLASH_lockOpt();
    }
}

static void copyActualValues(void) {
    // FIXME use optimistic lock to avoid sim?
    disable_irq();
    sCountCopy = sCount;
    sSumCopy   = sSum;
    sCount = 0;
    sSum   = 0;
    enable_irq();
}

static void startFun1(void) {
    saveSettings();
    mode = Mode_Fun1Run;
    if(encoderMode == EncoderMode_U1 || encoderMode == EncoderMode_U0)
        encoderMode = EncoderMode_I0;
    updateISet();
    LOAD_start();
    fun1State.warning  = false;
    fun1State.flip     = false;
    fun1State.lastBeep = 0;
}

static void stopFun1(void) {
    mode = Mode_Fun1;
    LOAD_stop();
}

static void doFun1(void) {
    if(error) {
        stopFun1();
        return;
    }

    if((int32_t)uCur * iSet >= powLimit) {
        iSet = (powLimit / uCur / 10) * 10;
        updateISet();
    }

    if((uCur < uSet) || !LOAD_isStable()) {
        if((!fun1State.warning) || (cycleBeginMs - fun1State.lastBeep >= 500)) {
            BEEP_beep(BEEP_freq_1k, 52);
            fun1State.warning  = true;
            fun1State.flip     = !fun1State.flip;
            fun1State.lastBeep = cycleBeginMs;
        }
    }
    else {
        fun1State.warning  = false;
        fun1State.flip     = false;
        fun1State.lastBeep = 0;
    }
}

static void startFun2(void) {
    saveSettings();
    mode = Mode_Fun2Pre;
    if(encoderMode == EncoderMode_U1 || encoderMode == EncoderMode_U0)
        encoderMode = EncoderMode_I0;
    updateISet();
    conn4 = (uSense > CFG->uSenseMin);
    uCoef = (conn4 ? &CFG->uSenseCoef : &CFG->uCurCoef);
    fun2State.lastDisp = cycleBeginMs;
}

static void startFun2Run(void) {
    mode = Mode_Fun2Run;
    LOAD_start();
    fun2State.disp     = DisplayedValue_V;
    fun2State.lastDisp = cycleBeginMs;

    fun2State.u = 0;
    copyActualValues();
}

static void resetFun2(void) {
    fun2State.ah  = 0;
    fun2State.sAh = 0;
    fun2State.wh  = 0;
    fun2State.sWh = 0;
}

static void stopFun2(void) {
    mode = Mode_Fun2;
    LOAD_stop();
}

static void startFun2Warn(void) {
    mode = Mode_Fun2Warn;
    LOAD_stop();
    fun2State.flip     = false;
    fun2State.lastBeep = cycleBeginMs;
}

static void startFun2Res(void) {
    mode = Mode_Fun2Res;
    fun2State.disp = DisplayedValue_V;
}

static void stopFun2Res(void) {
    resetFun2();
    mode = Mode_Fun2;
}

static void doFun2Pre(void) {
    if(error) {
        stopFun2();
        return;
    }

    if(cycleBeginMs - fun2State.lastDisp >= 500) {
        startFun2Run();
    }
}

static void doFun2(void) {
    if(error) {
        stopFun2();
        return;
    }

    fun2State.u = (conn4 ? uSense : uCur);

    if((fun2State.u < uSet) || !LOAD_isStable()) {
        startFun2Warn();
        return;
    }

    if((int32_t)uCur * iSet >= powLimit) {
        iSet = (powLimit / uCur / 10) * 10;
        updateISet();
    }

    if(cycleBeginMs - fun2State.lastDisp >= 2500) {
        fun2State.disp     = (enum DisplayedValue)(((uint8_t)fun2State.disp + 1) % 3);
        fun2State.lastDisp = cycleBeginMs;
    }

    copyActualValues();
    {
        uint64_t wh;
        uint32_t off = sCountCopy * uCoef->offset;
        uint64_t sWh = (uint64_t)(sSumCopy > off ? sSumCopy - off : 0) * iSet;

        fun2State.sAh   += sCountCopy * iSet;
        fun2State.ah     = fun2State.sAh / (3600uL * 1000 / 2);
        fun2State.sWh   += sWh;

        wh = fun2State.sWh / (3600uL * 1000 * 1000 / 2);
        wh *= uCoef->mul;
        wh /= uCoef->div;
        if(wh > UINT32_MAX) wh = UINT32_MAX;
        fun2State.wh = (uint32_t)wh;

        if(fun2State.ah > CFG->ahMax || fun2State.wh > CFG->whMax) {
            startFun2Warn();
            return;
        }
    }
}

static void doFun2Warn(void) {
    if(cycleBeginMs - fun2State.lastBeep >= 100) {
        BEEP_beep(BEEP_freq_1k, 40);
        fun2State.flip     = !fun2State.flip;
        fun2State.lastBeep = cycleBeginMs;
    }
}

static inline void beepButton(void) {
    if(CFG->beepOn) BEEP_beep(BEEP_freq_1k, 100);
}

static inline void beepEncoder(void) {
    if(CFG->beepOn) BEEP_beep(BEEP_freq_1k, 2);
}

static inline void beepEncoderButton(void) {
    if(CFG->beepOn) BEEP_beep(BEEP_freq_1k, 10);
}

static inline void checkErrors(void) {
    uint16_t temp = tempRaw;

    if(uCurRaw < CFG->uNegative || uSenseRaw < CFG->uNegative) {
        error |= ERROR_POLARITY;
    }
    else {
        error &= ~ERROR_POLARITY;
    }

    if(uCur >= CFG->uCurLimit) {
        error |= ERROR_OUP;
    }
    else {
        error &= ~ERROR_OUP;
    }

    if(temp >= CFG->tempDefect) {
        error |= ERROR_ERT;
    }

    if(temp <= CFG->tempLimit) {
        error |= ERROR_OTP;
    }
    else {
        error &= ~ERROR_OTP;
    }
}

static inline void controlFan(void) {
    uint16_t temp = tempRaw;
    switch(fanState) {
        case FanState_Override:
            break;

        case FanState_Off:
            if(temp < CFG->tempFanLow) {
                fanState = FanState_Low;
                FAN_set(33);
            }
            break;

        case FanState_Low:
            if(temp < CFG->tempFanMid) {
                fanState = FanState_Mid;
                FAN_set(66);
            }
            if(temp >= CFG->tempFanLow + CFG->tempThreshold) {
                fanState = FanState_Off;
                FAN_set(0);
            }
            break;

        case FanState_Mid:
            if(temp < CFG->tempFanFull) {
                fanState = FanState_Full;
                FAN_set(100);
            }
            if(temp >= CFG->tempFanMid + CFG->tempThreshold) {
                fanState = FanState_Low;
                FAN_set(33);
            }
            break;

        case FanState_Full:
            if(temp >= CFG->tempFanFull + CFG->tempThreshold) {
                fanState = FanState_Mid;
                FAN_set(66);
            }
            break;
    }
}

void BUTTON_onRelease(bool longpress) {
    if(error) return;
    if(inputDisable & INPUT_DISABLE_BUTTON) return;

    switch(mode) {
        case Mode_Booting:
            break;

        case Mode_MenuFun:
            mode = Mode_MenuBeep;
            break;

        case Mode_MenuBeep:
            stopMenu();
            break;

        case Mode_Fun1:
            startFun1();
            beepButton();
            break;

        case Mode_Fun1Run:
            stopFun1();
            beepButton();
            break;

        case Mode_Fun2:
            if(longpress) resetFun2();
            else          startFun2();
            beepButton();
            break;

        case Mode_Fun2Run:
            stopFun2();
            beepButton();
            break;

        case Mode_Fun2Warn:
            startFun2Res();
            beepButton();
            break;

        case Mode_Fun2Res:
            stopFun2Res();
            beepButton();
            break;
    }
}

static void updateValue(uint16_t* v, int16_t d, uint16_t min, uint16_t max) {
    if(d < 0) {
        if(*v < min - d)
            *v = min;
        else
            *v += d;
    }
    else {
        if(*v > max - d)
            *v = max;
        else
            *v += d;
    }
}

void ENCODER_onChange(int8_t d) {
    if(error) return;
    if(inputDisable & INPUT_DISABLE_ENCODER) return;

    switch(mode) {
        case Mode_Booting:
            break;

        case Mode_MenuFun:
            menuState.fun = (menuState.fun + 1) % 2;
            break;

        case Mode_MenuBeep:
            menuState.beep = !menuState.beep;
            break;

        case Mode_Fun1:
        case Mode_Fun1Run:
        case Mode_Fun2:
        case Mode_Fun2Run:
            switch(encoderMode) {
                case EncoderMode_I1: updateValue(&iSet,  100*d, CFG->iSetMin, CFG->iSetMax); break;
                case EncoderMode_I0: updateValue(&iSet,   10*d, CFG->iSetMin, CFG->iSetMax); break;
                case EncoderMode_U1: updateValue(&uSet, 1000*d, CFG->uSetMin, CFG->uSetMax); break;
                case EncoderMode_U0: updateValue(&uSet,  100*d, CFG->uSetMin, CFG->uSetMax); break;
            }

            beepEncoder();
            updateISet();
            break;

        case Mode_Fun2Warn:
            break;

        case Mode_Fun2Res:
            fun2State.disp = (enum DisplayedValue)(((uint8_t)fun2State.disp + 3 + d) % 3);
            break;
    }
}

void ENCODERBUTTON_onRelease(void) {
    if(error) return;
    if(inputDisable & INPUT_DISABLE_ENCODER_BUTTON) return;

    switch(mode) {
        case Mode_Booting:
            break;

        case Mode_MenuFun:
            break;

        case Mode_MenuBeep:
            break;

        case Mode_Fun1:
        case Mode_Fun2:
            switch(encoderMode) {
                case EncoderMode_I1: encoderMode = EncoderMode_I0; break;
                case EncoderMode_I0: encoderMode = EncoderMode_U1; break;
                case EncoderMode_U1: encoderMode = EncoderMode_U0; break;
                case EncoderMode_U0: encoderMode = EncoderMode_I1; break;
            }
            beepEncoderButton();
            break;

        case Mode_Fun1Run:
        case Mode_Fun2Run:
            switch(encoderMode) {
                case EncoderMode_I1: encoderMode = EncoderMode_I0; break;
                case EncoderMode_I0: encoderMode = EncoderMode_I1; break;
            }
            beepEncoderButton();
            break;

        case Mode_Fun2Warn:
            startFun2Res();
            beepEncoderButton();
            break;

        case Mode_Fun2Res:
            break;
    }
}

static void displayNothing(uint8_t* buf) {
    buf[0] = DISPLAYS_SYM_SPACE;
    buf[1] = DISPLAYS_SYM_SPACE;
    buf[2] = DISPLAYS_SYM_SPACE;
    buf[3] = DISPLAYS_SYM_SPACE;
}

static void displayDashes(uint8_t* buf) {
    buf[0] = DISPLAYS_SYM_DASH;
    buf[1] = DISPLAYS_SYM_DASH;
    buf[2] = DISPLAYS_SYM_DASH;
    buf[3] = DISPLAYS_SYM_DASH;
}

static void displayInt(uint16_t v, uint8_t dot, uint8_t* buf) {
    while(v > 9999) {
        v /= 10;
        ++dot;
    }

    buf[3] = DISPLAYS_DIGITS[v%10];
    v /= 10;
    buf[2] = (dot > 2 && v == 0) ? DISPLAYS_SYM_SPACE : DISPLAYS_DIGITS[v%10];
    v /= 10;
    buf[1] = (dot > 1 && v == 0) ? DISPLAYS_SYM_SPACE : DISPLAYS_DIGITS[v%10];
    v /= 10;
    buf[0] = (dot > 0 && v == 0) ? DISPLAYS_SYM_SPACE : DISPLAYS_DIGITS[v%10];
    if(dot < 4) buf[dot] |= DISPLAYS_DOT;
}

static uint8_t encoderStateToLeds(void) {
    uint8_t encLeds = 0;
    switch(encoderMode) {
        case EncoderMode_I1: encLeds = LED_A | LED_1; break;
        case EncoderMode_I0: encLeds = LED_A | LED_0; break;
        case EncoderMode_U1: encLeds = LED_V | LED_1; break;
        case EncoderMode_U0: encLeds = LED_V | LED_0; break;
    }
    return encLeds;
}

static void updateDisplays(void) {
    uint8_t bufA[4];
    uint8_t bufALeds = 0;
    uint8_t bufB[4];

    displayDashes(bufA);

    if(displayOverride) {
        bufB[0]  = display[0];
        bufB[1]  = display[1];
        bufB[2]  = display[2];
        bufB[3]  = display[3];
        bufA[0]  = display[4];
        bufA[1]  = display[5];
        bufA[2]  = display[6];
        bufALeds = display[7];
    }
    else if(error) {
        bufB[0] = DISPLAYS_SYM_E;
        bufB[1] = DISPLAYS_SYM_r;
        bufB[2] = DISPLAYS_SYM_r;
        if(error & ERROR_SUPPLY) {
            bufB[3] = DISPLAYS_SYM_6;
        }
        else if(error & ERROR_POLARITY) {
            bufB[3] = DISPLAYS_SYM_2;
        }
        else if(error & ERROR_OUP) {
            bufB[0] = DISPLAYS_SYM_SPACE;
            bufB[1] = DISPLAYS_SYM_o;
            bufB[2] = DISPLAYS_SYM_u;
            bufB[3] = DISPLAYS_SYM_P;
        }
        else {
            bufB[3] = DISPLAYS_SYM_0;
        }
    }
    else {
        switch(mode) {
            case Mode_Booting:
                if(cycleBeginMs >= 1500) {
                    bufB[0] = DISPLAYS_SYM_F;
                    bufB[1] = DISPLAYS_SYM_u;
                    bufB[2] = DISPLAYS_SYM_n;
                    bufB[3] = (CFG->fun == 0) ? DISPLAYS_SYM_1 : DISPLAYS_SYM_2;
                }
                else {
                    bufA[0]  = 0xFF;
                    bufA[1]  = 0xFF;
                    bufA[2]  = 0xFF;
                    bufALeds = 0xFF;
                    bufB[0]  = 0xFF;
                    bufB[1]  = 0xFF;
                    bufB[2]  = 0xFF;
                    bufB[3]  = 0xFF;
                }
                break;

            case Mode_MenuFun:
                bufB[0] = DISPLAYS_SYM_F;
                bufB[1] = DISPLAYS_SYM_u;
                bufB[2] = DISPLAYS_SYM_n;
                bufB[3] = (menuState.fun == 0) ? DISPLAYS_SYM_1 : DISPLAYS_SYM_2;
                break;

            case Mode_MenuBeep:
                bufB[0] = DISPLAYS_SYM_b;
                bufB[1] = DISPLAYS_SYM_E;
                bufB[2] = DISPLAYS_SYM_o;
                bufB[3] = (menuState.beep ? DISPLAYS_SYM_n : DISPLAYS_SYM_F);
                break;

            case Mode_Fun1:
            case Mode_Fun2:
                displayInt(iSet, 0, bufA);
                bufALeds = encoderStateToLeds();
                displayInt(uSet/10, 1, bufB);
                bufB[3] = DISPLAYS_SYM_u;
                break;

            case Mode_Fun1Run:
                displayInt(iSet, 0, bufA);
                bufALeds = encoderStateToLeds() | LED_V;
                if(!fun1State.flip) bufALeds |= LED_RUN;
                displayInt(uCur/10, 1, bufB);
                break;

            case Mode_Fun2Pre:
                bufB[0] = DISPLAYS_SYM_J;
                bufB[1] = DISPLAYS_SYM_5;
                bufB[2] = DISPLAYS_SYM_DASH;
                bufB[3] = conn4 ? DISPLAYS_SYM_4 : DISPLAYS_SYM_2;
                break;

            case Mode_Fun2Run:
                bufALeds = LED_RUN;
                // fallthrough

            case Mode_Fun2Res:
                displayInt(iSet, 0, bufA);
                bufALeds |= encoderStateToLeds();
                switch(fun2State.disp) {
                    case DisplayedValue_V:
                        displayInt(fun2State.u/10, 1, bufB);
                        bufALeds |= LED_V;
                        break;

                    case DisplayedValue_Ah:
                        displayInt(fun2State.ah, 0, bufB);
                        bufALeds |= LED_AH;
                        break;

                    case DisplayedValue_Wh:
                        displayInt(fun2State.wh, 0, bufB);
                        bufALeds |= LED_WH;
                        break;
                }
                break;

            case Mode_Fun2Warn:
                if(!fun2State.flip) {
                    displayInt(fun2State.u/10, 1, bufB);
                    bufALeds |= LED_V;
                }
                else {
                    displayNothing(bufB);
                }
                break;
        }
    }

    bufA[3] = bufALeds;
    DISPLAYS_display(bufA, bufB);
}

static inline void initialState(void) {
/*
    FLASH_unlockOpt();

    CFG->iSetCoef.offset   = 66;
    CFG->iSetCoef.mul      = 5329;
    CFG->iSetCoef.div      = 1000;

    CFG->uCurCoef.offset   = 606;
    CFG->uCurCoef.mul      = 30000;
    CFG->uCurCoef.div      = 23912;

    CFG->uSenseCoef.offset = 670;
    CFG->uSenseCoef.mul    = 30000;
    CFG->uSenseCoef.div    = 27215;

    CFG->uSupMin           = 0x52A0; // ~10V

    CFG->tempThreshold     = 0x0200;
    CFG->tempFanLow        = 0x3000;
    CFG->tempFanMid        = 0x2800;
    CFG->tempFanFull       = 0x2000;
    CFG->tempLimit         = 0x1000;
    CFG->tempDefect        = 0x6000;

    CFG->iSetMin           =     200;
    CFG->iSetMax           =   10000;
    CFG->uSetMin           =    1000;
    CFG->uSetMax           =   25000;
    CFG->uSenseMin         =      50;
    CFG->uNegative         =     500;
    CFG->uCurLimit         =   31000;
    CFG->powLimit          =  150000uL;
    CFG->ahMax             =  999900uL;
    CFG->whMax             = 9999000uL;

    FLASH_waitOpt();
    FLASH_lockOpt();
*/
    error       = 0;
    encoderMode = EncoderMode_I1;
    fanState    = FanState_Off;
    powLimit    = (int32_t)CFG->powLimit * 1000;

    uSet = CFG->uSet;
    if(uSet < CFG->uSetMin)      uSet = CFG->uSetMin;
    else if(uSet > CFG->uSetMax) uSet = CFG->uSetMax;

    iSet = CFG->iSet;
    if(iSet < CFG->iSetMin)      iSet = CFG->iSetMin;
    else if(iSet > CFG->iSetMax) iSet = CFG->iSetMax;
}

static uint8_t crc8(uint8_t crc, uint8_t b) __naked {
    // Algorithm:
    //  crc ^= b;
    //  for(uint8_t i = 0; i < 8; ++i)
    //      crc = crc & 0x80 ? (crc << 1) ^ 0x07 : crc << 1;
    //  return crc;

    (void)crc; (void)b;
    __asm

    LD      A, (0x03, SP)   ;        A == crc
    XOR     A, (0x04, SP)
    LDW     Y, #8           ;        Y == i
	            		    ; 11
00001$:
    LD      XL, A           ; 1   1
    RCF                     ; 1   1
    RLCW    X               ; 2   1  X == (crc << 1)
    AND     A, #0x80        ; 1   2
    LD      A, XL           ; 1   1
    JREQ    00002$          ; 1/2 2
    XOR     A, #0x07        ; 1   2
00002$:
    DECW    Y               ; 1   2
    JRNE    00001$          ; 2   2

    RET
     __endasm;
}

enum Command {
    Command_ReadCal           = 0x01,
    Command_WriteCal,
    Command_Display,
    Command_Beep,
    Command_Fan,
    Command_InputDisable,
    Command_ReadSettings,
    Command_WriteSettings,
    Command_Go,
    Command_GetState,
    Command_StartFlow,
    Command_StartBootloader,

    Command_Flow              = 0x2B,
    Command_Response          = 0x40,
    Command_Error             = 0x80
};

static void sendUartCommand(uint8_t cmd, const uint8_t* data, uint8_t size) {
    uint8_t crc;
    UART_send('S');
    crc = crc8(0, cmd);
    UART_writeHexU8(cmd);

    for(; size > 0; --size, ++data) {
        crc = crc8(crc, *data);
        UART_writeHexU8(*data);
    }
    UART_writeHexU8(crc);
    UART_send('\r');
    UART_send('\n');
}

static void commitUartCommand(uint8_t cmd) {
    cmd |= (uint8_t)Command_Response;
    sendUartCommand(cmd, NULL, 0);
}

static void processUartCommand(const uint8_t* buf, uint8_t size) {
    static uint8_t reply[4];

    switch(buf[0]) {
        case Command_Display:
            if(size == 10) {
                displayOverride = buf[1];
                memcpy(display, buf+2, sizeof(display));
                commitUartCommand(buf[0]);
            }
            break;

        case Command_Beep:
            if(size == 3) {
                BEEP_beep(buf[1], buf[2]);
                commitUartCommand(buf[0]);
            }
            break;

        case Command_Fan:
            if(size == 2) {
                if(buf[1] == 0xFF) {
                    FAN_set(0);
                    fanState = FanState_Off;
                }
                else {
                    fanState = FanState_Override;
                    FAN_set(buf[1]);
                }
                commitUartCommand(buf[0]);
            }
            break;

        case Command_InputDisable:
            if(size == 2) {
                inputDisable = buf[1];
                commitUartCommand(buf[0]);
            }
            break;

        case Command_ReadSettings:
            if(size == 1) {
                reply[0] = (uint8_t)(uSet >> 8);
                reply[1] = (uint8_t)uSet;
                reply[2] = (uint8_t)(iSet >> 8);
                reply[3] = (uint8_t)iSet;
                sendUartCommand(Command_ReadSettings | Command_Response, reply, 4);
            }
            break;

        case Command_WriteSettings:
            if(size == 5) {
                uSet = ((uint16_t)buf[1] << 8) | buf[2];
                iSet = ((uint16_t)buf[3] << 8) | buf[4];
                commitUartCommand(buf[0]);
            }
            break;

        case Command_GetState:
            if(size == 1) {
                // FIXME sendUartCommand(Command_GetState | Command_Response, (const uint8_t*)&actualValues, sizeof(actualValues));
            }
            break;

        case Command_StartFlow:
            if(size == 3) {
                flowInterval = ((uint16_t)buf[1] << 8) | buf[2];
                commitUartCommand(buf[0]);
            }
            break;

        default:
            UART_write("->");
            for(; size > 0; --size, ++buf) UART_writeHexU8(*buf);
    }
    UART_write("cmd done\r\n");
}

static void processUartRx(void) {
    uint8_t rxSize;
    uint8_t n;
    uint8_t crc;
    const uint8_t* rx;
    const uint8_t* p;

    UART_process();
    rx = UART_getRx(&rxSize);
    if(!rx) return;

    crc = 0;
    if(UART_hasChecksum()) {
        for(p = rx, n = rxSize; n > 0; --n, ++p) crc = crc8(crc, *p);
    }

    if(crc == 0 && rxSize > 1)
        processUartCommand(rx, rxSize-1);
    else
        UART_write("checksum mismatch\r\n");
    UART_rxDone();
}

static void processFlow(void) {
    if(flowInterval == 0xFFFF) return;
    if(cycleBeginMs - lastFlow < flowInterval) return;

    // FIXME sendUartCommand(Command_Flow, (const uint8_t*)&actualValues, sizeof(actualValues));
    lastFlow = cycleBeginMs;
}

// PD2, PD7
static void initDebug(void) {
    GPIOD->DDR |= GPIO_DDR_2 | GPIO_DDR_7;  // output
    GPIOD->CR1 |= GPIO_CR1_2 | GPIO_CR1_7;  // pull-push
    GPIOD->CR2 |= GPIO_CR2_2 | GPIO_CR2_7;  // high speed
    // GPIOD->ODR |= GPIO_ODR_2;
    // GPIOD->ODR &= ~GPIO_ODR_2;
}

int main(void) {
    SYSTEM_toHseClock();
    initialState();
    SYSTEMTIMER_init();
    UART_init();
    BEEP_init();
    LOAD_init();
    FAN_init();
    ADC_init();
    ENCODER_init();
    ENCODERBUTTON_init();
    BUTTON_init();
    DISPLAYS_init();
    initDebug();

    enable_irq();
    DISPLAYS_start();
    UART_write("== start ==\r\n");

    startBooting();

    {
        uint32_t lastDump   = SYSTEMTIMER_ms;
        uint32_t lastUpdate = SYSTEMTIMER_ms;
        while(1) {
            GPIOD->ODR ^= GPIO_ODR_2;
            cycleBeginMs = SYSTEMTIMER_ms;
            ENCODER_process();
            ENCODERBUTTON_process();
            BUTTON_process();
            recalcValues();
            checkErrors();
            controlFan();

            switch(mode) {
                case Mode_Booting:  doBooting();     break;
                case Mode_MenuFun:                   break;
                case Mode_MenuBeep:                  break;
                case Mode_Fun1:                      break;
                case Mode_Fun1Run:  doFun1();        break;
                case Mode_Fun2:                      break;
                case Mode_Fun2Pre:  doFun2Pre();     break;
                case Mode_Fun2Run:  doFun2();        break;
                case Mode_Fun2Warn: doFun2Warn();    break;
                case Mode_Fun2Res:                   break;
            }

            processUartRx();
            processFlow();
            if(cycleBeginMs - lastUpdate >= 100) {
                updateDisplays();
                lastUpdate = cycleBeginMs;

                if(cycleBeginMs - lastDump >= 1000) {
                    //UART_write("err=");
                    //UART_writeHexU8(error);
                    //UART_write(" mode=");
                    //UART_writeHexU8(mode);
                    //UART_write(" temp=");
                    //UART_writeHexU16(tempRaw);
                    UART_write(" uCurRaw=");
                    UART_writeHexU16(uCurRaw);
                    UART_write(" uCur=");
                    UART_writeDecU32(uCur);
                    //UART_write(" uSenseRaw=");
                    //UART_writeHexU16(uSenseRaw);
                    //UART_write(" uSense=");
                    //UART_writeDecU32(uSense);
                    //UART_write(" uSup=");
                    //UART_writeHexU16(uSupRaw);
                    UART_write(" sAh=");
                    UART_writeDecU64(fun2State.sAh, 21);
                    UART_write(" sWh=");
                    UART_writeDecU64(fun2State.sWh, 21);
                    UART_write(" wh=");
                    UART_writeDecU32(fun2State.wh);
                    //UART_write(" PC2=");
                    //UART_write(GPIOC->IDR & GPIO_IDR_2 ? "1" : "0");
                    //UART_write(" load=");
                    //UART_writeHexU16(CFG->iSetCoef.offset);
                    //UART_writeHexU16(CFG->iSetCoef.mul);
                    //UART_writeHexU16(CFG->iSetCoef.div);
                    UART_write("\r\n");

                    lastDump = cycleBeginMs;
                }
            }
        }
    }
}

