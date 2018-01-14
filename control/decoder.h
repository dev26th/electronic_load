#ifndef DECODER_H
#define DECODER_H

#include <stdint.h>

#include <QByteArray>

static const uint8_t DEVICE_ERROR_POLARITY = (1 << 0);
static const uint8_t DEVICE_ERROR_SUPPLY   = (1 << 1);
static const uint8_t DEVICE_ERROR_OUP      = (1 << 2);
static const uint8_t DEVICE_ERROR_OTP      = (1 << 3);
static const uint8_t DEVICE_ERROR_ERT      = (1 << 4);

enum class Cmd {
    Reboot            = 0x01,
    GetVersion,
    ReadConfig,
    WriteConfig,
    Display,
    Beep,
    Fan,
    InputDisable,
    ReadSettings,
    WriteSettings,
    SetMode,
    GetState,
    ResetState,
    FlowState,
    ReadRaw,
    WriteRaw,
    Bootloader,
};

enum class CmdState {
    Request           = 0x00,
    Response          = 0x40,
    Event             = 0x80,
    Error             = 0xC0
};

enum class DeviceMode {
    Booting,
    MenuFun,
    MenuBeep,
    Fun1,
    Fun1Run,
    Fun2,
    Fun2Pre,
    Fun2Run,
    Fun2Warn,
    Fun2Res,
};

struct ValueCoef {
    uint16_t offset;
    uint16_t mul;
    uint16_t div;
};

struct CmdData {
    Cmd cmd;
    CmdState state;

    CmdData(Cmd cmd_) : cmd(cmd_), state(CmdState::Request) {}
    CmdData(Cmd cmd_, CmdState state_) : cmd(cmd_), state(state_) {}
};

struct CmdConfigData : public CmdData {
    CmdConfigData(Cmd cmd_, CmdState state_) : CmdData(cmd_, state_) {}

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

struct CmdSettingData : public CmdData {
    CmdSettingData(Cmd cmd_, CmdState state_) : CmdData(cmd_, state_) {}

    uint16_t u;
    uint16_t i;
};

struct CmdStateData : public CmdData {
    CmdStateData(Cmd cmd_, CmdState state_) : CmdData(cmd_, state_) {}

    DeviceMode mode;
    uint8_t error;
    uint16_t uCurrent;
    uint16_t uSense;
    uint16_t tempRaw;
    uint16_t uSupRaw;
    uint32_t ah;
    uint32_t wh;
};

struct CmdFlowStateData : public CmdData {
    CmdFlowStateData(uint16_t interval_) : CmdData(Cmd::FlowState, CmdState::Request), interval(interval_) {}

    uint16_t interval;
};

struct CmdVersionData : public CmdData {
    CmdVersionData(Cmd cmd_, CmdState state_) : CmdData(cmd_, state_) {}

    uint32_t v;
};

QByteArray formCmdData(const CmdData& data);

CmdData* parseCmdData(const QByteArray &data);

#endif // DECODER_H
