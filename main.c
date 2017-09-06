#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

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
    Mode_Fun2Run,
    Mode_Fun2Stop,
    Mode_Fun2Res,
};
static enum Mode mode;

#define ERROR_POLARITY (1 << 0)
#define ERROR_SUPPLY   (1 << 1)
#define ERROR_OUP      (1 << 2)
#define ERROR_OTP      (1 << 3)
#define ERROR_ERT      (1 << 4)
static uint8_t  error;

static int16_t  uSet;            // mV
static int16_t  iSet;            // mA
static int32_t  powLimit;        // uW
static uint16_t uCurRaw;
static int16_t  uCur;            // mV
static uint16_t uSenseRaw;
static int16_t  uSense;          // mV
static uint16_t uSupRaw;
static uint16_t tempRaw;

struct MenuState {
    int8_t fun;
    bool beep;
};
static struct MenuState menuState;

struct Fun1State {
    uint16_t runCount;
    uint16_t beepCount;
};
static struct Fun1State fun1State;

enum DisplayedValue {
    DisplayedValue_V,
    DisplayedValue_Ah,
    DisplayedValue_Wh
};
struct Fun2State {
    bool     conn4;
    uint8_t  dispConnCount;
    enum DisplayedValue disp;
    uint16_t dispCount;
    int16_t  u;
    uint32_t ah;   // mAs
    uint32_t wh;   // mWs
    uint16_t tick;
    uint32_t sAh;
    uint32_t sWh;
    uint16_t beepCount;
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
    int16_t  offset;
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
    int16_t          iSetMin;       // mA
    int16_t          iSetMax;       // mA
    int16_t          uSetMin;       // mV
    int16_t          uSetMax;       // mV
    int16_t          uSenseMin;     // mV
    int16_t          uNegative;     // mV
    int16_t          uCurLimit;     // mV
    int32_t          powLimit;      // mW
    uint32_t         ahMax;         // mAh
    uint32_t         whMax;         // mWh
    uint8_t          fun;
    uint8_t          beepOn;
    int16_t          uSet;          // mV
    int16_t          iSet;          // mA
};
static_assert(sizeof(struct Config) <= 128, "Config is bigger as EEPROM");
#define CFG ((struct Config*)0x4000) // begin of the EEPROM

// --------------------------------------------------------------------------------------------------------------------

static int16_t recalcValue(int16_t raw, const struct ValueCoef* coef) {
    int32_t v = (int32_t)raw + coef->offset;
    v *= coef->mul;
    v /= coef->div;
    if(v > INT16_MAX) v = INT16_MAX;
    if(v < INT16_MIN) v = INT16_MIN;
    return (int16_t)v;
}

static void recalcValues(void) {
    uCur   = recalcValue(uCurRaw,   &CFG->uCurCoef);
    uSense = recalcValue(uSenseRaw, &CFG->uSenseCoef);
}

static void updateISet(void) {
    int16_t v = recalcValue(iSet, &CFG->iSetCoef);
    if(v <= 0) v = 1;
    LOAD_set(v);
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
    if(SYSTEMTIMER_ms >= 500) {
        if(uSupRaw < CFG->uSupMin) { // cannot be reset
            error |= ERROR_SUPPLY;
        }
    }

    if(SYSTEMTIMER_ms >= 2000) {
        stopBooting();
    }
}

static void updateConfig() {
    // FIXME
    /*
    if(iSet != CFG->iSet || uSet != CFG->uSet) {
        FLASH_unlockOpt();

        CFG->uSet = uSet;
        CFG->iSet = iSet;

        FLASH_waitOpt();
        FLASH_lockOpt();
    }
    */
}

static void startFun1(void) {
    updateConfig();
    mode = Mode_Fun1Run;
    if(encoderMode == EncoderMode_U1 || encoderMode == EncoderMode_U0)
        encoderMode = EncoderMode_I0;
    updateISet();
    LOAD_start();
    fun1State.runCount  = 0;
    fun1State.beepCount = 0;
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

    if(!LOAD_isStable()) {
        stopFun1();
        return;
    }

    if((int32_t)uCur * iSet >= powLimit) {
        iSet = (powLimit / uCur / 10) * 10;
        updateISet();
    }

    if(fun1State.runCount < (500 / SYSTEMTIMER_MS_PER_TICK)) {
        ++fun1State.runCount;
        return;
    }

    if(uCur < uSet) {
        if(!fun1State.beepCount)
            fun1State.beepCount = 1; // start
    }
    else {
        fun1State.beepCount = 0; // stop
    }

    if(fun1State.beepCount) {
        ++fun1State.beepCount;
        if(fun1State.beepCount == (1000 / SYSTEMTIMER_MS_PER_TICK)) {
            fun1State.beepCount = 1;        // reload
        }
        else if(fun1State.beepCount < (500 / SYSTEMTIMER_MS_PER_TICK)) { // on
            BEEP_beep(BEEP_freq_1k, 100);
        }
        else {                              // off
            BEEP_beep(BEEP_freq_None, 100);
        }
    }
}

static void startFun2(void) {
    updateConfig();
    mode = Mode_Fun2Run;
    if(encoderMode == EncoderMode_U1 || encoderMode == EncoderMode_U0)
        encoderMode = EncoderMode_I0;
    updateISet();
    LOAD_start();
    fun2State.conn4         = (uSense > CFG->uSenseMin);
    fun2State.dispConnCount = (SYSTEMTIMER_TICKS_PER_S / 2);
    fun2State.disp          = DisplayedValue_V;
    fun2State.dispCount     = 0;
    fun2State.u             = 0;
    fun2State.tick          = 0;
    fun2State.sAh           = 0;
    fun2State.sWh           = 0;
}

static void resetFun2(void) {
    fun2State.ah = 0;
    fun2State.wh = 0;
}

static void stopFun2(void) {
    mode = Mode_Fun2;
    LOAD_stop();
}

static void startFun2Stop(void) {
    mode = Mode_Fun2Stop;
    LOAD_stop();
    fun2State.beepCount = 1;
}

static void startFun2Res(void) {
    mode = Mode_Fun2Res;
    fun2State.disp = DisplayedValue_V;
}

static void stopFun2Res(void) {
    resetFun2();
    mode = Mode_Fun2;
}

static void doFun2(void) {
    if(error) {
        stopFun2();
        return;
    }

    fun2State.u = (fun2State.conn4 ? uSense : uCur);

    if(fun2State.u < uSet) {
        startFun2Stop();
        return;
    }

    if(!LOAD_isStable()) {
        startFun2Stop();
        return;
    }

    if((int32_t)uCur * iSet >= powLimit) {
        iSet = (powLimit / uCur / 10) * 10;
        updateISet();
    }

    fun2State.sAh += (uint32_t)iSet;
    fun2State.sWh += (uint32_t)iSet * fun2State.u / 64;
    ++fun2State.tick;
    if(fun2State.tick == SYSTEMTIMER_TICKS_PER_S) {
        fun2State.ah += fun2State.sAh / SYSTEMTIMER_TICKS_PER_S;
        fun2State.wh += fun2State.sWh / (1000ul * SYSTEMTIMER_TICKS_PER_S / 64);

        fun2State.tick = 0;
        fun2State.sAh  = 0;
        fun2State.sWh  = 0;

        if(fun2State.ah > CFG->ahMax || fun2State.wh > CFG->whMax) {
            startFun2Stop();
            return;
        }
    }

    if(fun2State.dispConnCount) {
        --fun2State.dispConnCount;
    }
    else {
        ++fun2State.dispCount;
        if(fun2State.dispCount == (SYSTEMTIMER_TICKS_PER_S * 25 / 10)) {
            fun2State.dispCount = 0;
            fun2State.disp = (enum DisplayedValue)(((uint8_t)fun2State.disp + 1) % 3);
        }
    }
}

static void doFun2Stop(void) {
    fun2State.dispConnCount = 0;
    ++fun2State.beepCount;
    if(fun2State.beepCount == (200 / SYSTEMTIMER_MS_PER_TICK)) {
        fun2State.beepCount = 0;        // reload
        BEEP_beep(BEEP_freq_1k, 100);
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

void ADC_onResult(const uint16_t* res) {
    // with small quasi-FIR-filter
    tempRaw   = (tempRaw   + 1) / 2 + res[0];
    uCurRaw   = (uCurRaw   + 1) / 2 + res[1];
    uSenseRaw = (uSenseRaw + 1) / 2 + res[2];
    uSupRaw   = (uSupRaw   + 1) / 2 + res[3];
}

static inline void checkErrors(void) {
    if(uCur < CFG->uNegative || uSense < CFG->uNegative) {
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

    if(tempRaw >= CFG->tempDefect) {
        error |= ERROR_ERT;
    }

    if(tempRaw <= CFG->tempLimit) {
        error |= ERROR_OTP;
    }
    else {
        error &= ~ERROR_OTP;
    }
}

static inline void controlFan(void) {
    switch(fanState) {
        case FanState_Override:
            break;

        case FanState_Off:
            if(tempRaw < CFG->tempFanLow) {
                fanState = FanState_Low;
                FAN_set(33);
            }
            break;

        case FanState_Low:
            if(tempRaw < CFG->tempFanMid) {
                fanState = FanState_Mid;
                FAN_set(66);
            }
            if(tempRaw >= CFG->tempFanLow + CFG->tempThreshold) {
                fanState = FanState_Off;
                FAN_set(0);
            }
            break;

        case FanState_Mid:
            if(tempRaw < CFG->tempFanFull) {
                fanState = FanState_Full;
                FAN_set(100);
            }
            if(tempRaw >= CFG->tempFanMid + CFG->tempThreshold) {
                fanState = FanState_Low;
                FAN_set(33);
            }
            break;

        case FanState_Full:
            if(tempRaw >= CFG->tempFanFull + CFG->tempThreshold) {
                fanState = FanState_Mid;
                FAN_set(66);
            }
            break;
    }
}

void SYSTEMTIMER_onTick(void) {
    GPIOD->ODR |= GPIO_ODR_7;
    BEEP_process();
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
        case Mode_Fun2Run:  doFun2();        break;
        case Mode_Fun2Stop: doFun2Stop();    break;
        case Mode_Fun2Res:                   break;
    }
    GPIOD->ODR &= ~GPIO_ODR_7;
}

void SYSTEMTIMER_onTack(void) {
    ADC_startScanCont(16);
}

void BUTTON_onRelease(bool longpress) {
    if(error) return;

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

        case Mode_Fun2Stop:
            startFun2Res();
            beepButton();
            break;

        case Mode_Fun2Res:
            stopFun2Res();
            beepButton();
            break;
    }
}

static void updateValue(int16_t* v, int16_t d, uint16_t min, uint16_t max) {
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

        case Mode_Fun2Stop:
            break;

        case Mode_Fun2Res:
            fun2State.disp = (enum DisplayedValue)(((int8_t)fun2State.disp + 3 + d) % 3);
            break;
    }
}

void ENCODERBUTTON_onRelease(void) {
    if(error) return;

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

        case Mode_Fun2Stop:
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

static void displayInt(int16_t v, uint8_t dot, uint8_t* buf) {
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

    if(error) {
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
                if(SYSTEMTIMER_ms >= 1500) {
                    bufB[0] = DISPLAYS_SYM_F;
                    bufB[1] = DISPLAYS_SYM_u;
                    bufB[2] = DISPLAYS_SYM_n;
                    bufB[3] = (CFG->fun == 0) ? DISPLAYS_SYM_1 : DISPLAYS_SYM_2;
                }
                else {
                    bufA[0] = 0xFF;
                    bufA[1] = 0xFF;
                    bufA[2] = 0xFF;
                    bufALeds = 0xFF;
                    bufB[0] = 0xFF;
                    bufB[1] = 0xFF;
                    bufB[2] = 0xFF;
                    bufB[3] = 0xFF;
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
                if(fun1State.beepCount < (500 / SYSTEMTIMER_MS_PER_TICK)) bufA[3] |= LED_RUN;
                displayInt(uCur/10, 1, bufB);
                break;

            case Mode_Fun2Run:
                bufALeds = LED_RUN;
                // continue

            case Mode_Fun2Res:
                displayInt(iSet, 0, bufA);
                bufALeds |= encoderStateToLeds();
                if(fun2State.dispConnCount) {
                    bufB[0] = DISPLAYS_SYM_J;
                    bufB[1] = DISPLAYS_SYM_5;
                    bufB[2] = DISPLAYS_SYM_DASH;
                    bufB[3] = fun2State.conn4 ? DISPLAYS_SYM_4 : DISPLAYS_SYM_2;
                }
                else {
                    switch(fun2State.disp) {
                        case DisplayedValue_V:
                            displayInt(fun2State.u/10, 1, bufB);
                            bufALeds |= LED_V;
                            break;

                        case DisplayedValue_Ah:
                            displayInt(fun2State.ah / 3600, 0, bufB);
                            bufALeds |= LED_AH;
                            break;

                        case DisplayedValue_Wh:
                            displayInt(fun2State.wh / 3600, 0, bufB);
                            bufALeds |= LED_WH;
                            break;
                    }
                }
                break;

            case Mode_Fun2Stop:
                if(fun2State.beepCount > (100 / SYSTEMTIMER_MS_PER_TICK)) {
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

    CFG->iSetCoef.offset   = -66;
    CFG->iSetCoef.mul      = 5329;
    CFG->iSetCoef.div      = 1000;

    CFG->uCurCoef.offset   = -606;
    CFG->uCurCoef.mul      = 30000;
    CFG->uCurCoef.div      = 23912;

    CFG->uSenseCoef.offset = -670;
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
    CFG->uNegative         =     -50;
    CFG->uCurLimit         =   31000;
    CFG->powLimit          =  150000l;
    CFG->ahMax             =  999900l;
    CFG->whMax             = 9999000l;

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

// PD2, PD7
static void initDebug(void) {
    GPIOD->DDR |= GPIO_DDR_2 | GPIO_DDR_7;  // output
    GPIOD->CR1 |= GPIO_CR1_2 | GPIO_CR1_7;  // pull-push
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
    UART_write("== start ==\r\n");

    startBooting();

    {
        uint32_t lastDump   = SYSTEMTIMER_ms;
        uint32_t lastUpdate = SYSTEMTIMER_ms;
        while(1) {
            if(SYSTEMTIMER_ms - lastUpdate >= 100) {
                updateDisplays();
                lastUpdate = SYSTEMTIMER_ms;

                if(SYSTEMTIMER_ms - lastDump >= 1000) {
                    UART_write("err=");
                    UART_writeHexU8(error);
                    UART_write(" mode=");
                    UART_writeHexU8(mode);
                    UART_write(" temp=");
                    UART_writeHexU16(tempRaw);
                    UART_write(" uCurRaw=");
                    UART_writeHexU16(uCurRaw);
                    UART_write(" uCur=");
                    UART_writeHexU16(uCur);
                    UART_write(" uSense=");
                    UART_writeHexU16(uSense);
                    UART_write(" uSenseRaw=");
                    UART_writeHexU16(uSenseRaw);
                    UART_write(" uSup=");
                    UART_writeHexU16(uSupRaw);
                    UART_write(" PC2=");
                    UART_write(GPIOC->IDR & GPIO_IDR_2 ? "1" : "0");
                    UART_write(" load=");
                    UART_writeHexU16(CFG->iSetCoef.offset);
                    UART_writeHexU16(CFG->iSetCoef.mul);
                    UART_writeHexU16(CFG->iSetCoef.div);
                    UART_write("\r\n");

                    lastDump = SYSTEMTIMER_ms;
                }
            }
        }
    }
}

