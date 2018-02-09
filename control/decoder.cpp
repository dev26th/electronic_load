#include "decoder.h"

#include <assert.h>
#include <QDataStream>

QByteArray formCmdData(const CmdData& data)
{
    QByteArray res;

    QDataStream stream(&res, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::ByteOrder::BigEndian);

    stream << (uint8_t)((uint8_t)data.cmd | (uint8_t)data.state);

    switch(data.cmd) {
        case Cmd::WriteConfig:
            {
                const CmdConfigData& d = static_cast<const CmdConfigData&>(data);
                stream << d.iSetCoef.offset;
                stream << d.iSetCoef.mul;
                stream << d.iSetCoef.div;
                stream << d.uCurCoef.offset;
                stream << d.uCurCoef.mul;
                stream << d.uCurCoef.div;
                stream << d.uSenseCoef.offset;
                stream << d.uSenseCoef.mul;
                stream << d.uSenseCoef.div;
                stream << d.uSupMin;
                stream << d.tempThreshold;
                stream << d.tempFanLow;
                stream << d.tempFanMid;
                stream << d.tempFanFull;
                stream << d.tempLimit;
                stream << d.tempDefect;
                stream << d.iSetMin;
                stream << d.iSetMax;
                stream << d.uSetMin;
                stream << d.uSetMax;
                stream << d.uSenseMin;
                stream << d.uNegative;
                stream << d.uCurLimit;
                stream << d.powLimit;
                stream << d.ahMax;
                stream << d.whMax;
                stream << d.fun;
                stream << d.beepOn;
                stream << d.uSet;
                stream << d.iSet;
            }
            break;

        case Cmd::WriteSettings:
            {
                const CmdSettingData& d = static_cast<const CmdSettingData&>(data);
                stream << d.u;
                stream << d.i;
            }
            break;

        case Cmd::FlowState:
            {
                const CmdFlowStateData& d = static_cast<const CmdFlowStateData&>(data);
                stream << d.interval;
            }
            break;

        case Cmd::Bootloader:
            {
                const CmdBootloaderData& d = static_cast<const CmdBootloaderData&>(data);
                stream << (uint8_t)(d.enable ? 1 : 0);
            }
            break;

        case Cmd::ReadConfig:
        case Cmd::ReadSettings:
        case Cmd::GetVersion:
        case Cmd::ResetState:
        case Cmd::Reboot:
            break;

        default:
            assert(false);
    }

    return res;
}

CmdData* parseCmdData(const QByteArray& data)
{
    if(data.isEmpty()) return nullptr;

    QDataStream stream(data);
    stream.setByteOrder(QDataStream::ByteOrder::BigEndian);

    uint8_t c;
    stream >> c;
    Cmd      cmd   = (Cmd)(c & 0x1F);
    CmdState state = (CmdState)(c & 0xE0);

    switch(cmd) {
        case Cmd::ReadConfig:
            {
                if(data.size() != 65) return nullptr;

                CmdConfigData* res = new CmdConfigData(cmd, state);
                stream >> res->iSetCoef.offset;
                stream >> res->iSetCoef.mul;
                stream >> res->iSetCoef.div;
                stream >> res->uCurCoef.offset;
                stream >> res->uCurCoef.mul;
                stream >> res->uCurCoef.div;
                stream >> res->uSenseCoef.offset;
                stream >> res->uSenseCoef.mul;
                stream >> res->uSenseCoef.div;
                stream >> res->uSupMin;
                stream >> res->tempThreshold;
                stream >> res->tempFanLow;
                stream >> res->tempFanMid;
                stream >> res->tempFanFull;
                stream >> res->tempLimit;
                stream >> res->tempDefect;
                stream >> res->iSetMin;
                stream >> res->iSetMax;
                stream >> res->uSetMin;
                stream >> res->uSetMax;
                stream >> res->uSenseMin;
                stream >> res->uNegative;
                stream >> res->uCurLimit;
                stream >> res->powLimit;
                stream >> res->ahMax;
                stream >> res->whMax;
                stream >> res->fun;
                stream >> res->beepOn;
                stream >> res->uSet;
                stream >> res->iSet;
                return res;
            }

        case Cmd::ReadSettings:
            {
                if(data.size() != 5) return nullptr;

                CmdSettingData* res = new CmdSettingData(cmd, state);
                stream >> res->u;
                stream >> res->i;
                return res;
            }

        case Cmd::GetVersion:
            {
                if(data.size() != 5) return nullptr;

                CmdVersionData* res = new CmdVersionData(cmd, state);
                stream >> res->v;
                return res;
            }

        case Cmd::GetState:
            {
                if(data.size() != 19) return nullptr;

                CmdStateData* res = new CmdStateData(cmd, state);
                uint8_t mode;
                stream >> mode;
                stream >> res->error;
                stream >> res->uMain;
                stream >> res->uSense;
                stream >> res->tempRaw;
                stream >> res->uSupRaw;
                stream >> res->ah;
                stream >> res->wh;
                res->mode = (DeviceMode)mode;
                return res;
            }

        default:
            return nullptr;
    }
}
