#include "comm.h"
#include "dwin_protocol.h"
#include "cl_event_system.h"
#include "cl_log.h"
#include "cl_serialize.h"
#include "firmware_info.h"
#include "adc.h"
#include "helper.h"
#include "systime.h"

typedef enum
{
    HHC_Sync = 0x81,
    HHC_Key = 0x8f,
} HandHeldCmd_t;

static DwinPacket_t sendPack;
static uint16_t keyCount = 0;

static bool OnMainBoardPack(void *eventArg)
{
    CL_LOG_LINE("recv mainboard pack");
    DwinPacket_t *pack = (DwinPacket_t *)eventArg;
}

static void SendSyncPack(void)
{
    sendPack.cmd = HHC_Sync;
    uint8_t offset = 0;
    sendPack.data[offset++] = firmwareInfo.verMajor;
    sendPack.data[offset++] = firmwareInfo.verMajor;
    CL_Uint16ToBytes(firmwareInfo.verPatch, sendPack.data + offset, CL_BigEndian);
    offset += 2;

    sendPack.data[offset++] = NtcAdcToTemp_10K(GetTempAdc()) + 30;
    CL_Uint16ToBytes(keyCount, sendPack.data + 5, CL_BigEndian);
    offset += 2;

    sendPack.length = offset + 3;
    DwinProtocol_SendPack(ChanIdx_ToMainBoard, &sendPack);
}

void Comm_Init(void)
{
    CL_EventSysAddListener(OnMainBoardPack, CL_Event_RecvDwinPack, ChanIdx_ToMainBoard);
}

void Comm_Process(void)
{
    static uint32_t syncTime = 0;
    if(SysTimeSpan(syncTime) >= 1000)
    {
        syncTime = GetSysTime();
        SendSyncPack();
    }
}

void Comm_SendKeyEvent(void)
{
    sendPack.cmd = HHC_Key;
    CL_Uint16ToBytes(keyCount, sendPack.data, CL_BigEndian);
    sendPack.length = 2 + 3;

    DwinProtocol_SendPack(ChanIdx_ToMainBoard, &sendPack);
    DwinProtocol_SendPack(ChanIdx_ToMainBoard, &sendPack);

    keyCount++;
}
