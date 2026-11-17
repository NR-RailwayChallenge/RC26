#include "can_transport.h"

// CanTransport::CanTransport(CAN_TypeDef* canPort, CAN_PINS pins)
CanTransport::CanTransport(STM32_CAN *canIfInst)
{
    rxDropped = 0;
    rxFails = 0;
    txFails = 0;
    lastStcmRxTime = 0;
    canIf = canIfInst;
    canIf->begin(false);
    canIf->setBaudRate(CAN_BAUDRATE);
    canIf->setMBFilterProcessing(CAN_BANK::MB0, CAN_FILTER, CAN_MASK);
}

bool CanTransport::transmit(CAN_message_t *frame)
{
    bool isSuccess;
    if (canIf != nullptr)
    {
        isSuccess = canIf->write(*frame);
    }
    else
    {
        printf("CAN TX fail, no canIf\n");
    }
    if (!isSuccess)
    {
        txFails++;
    }
    else if ((frame->id & 0x8) == 0)
    {
        lastStcmTxTime = ms();
    }
    return isSuccess;
}

bool CanTransport::stcmOnline()
{
    bool result = (ms() - lastStcmRxTime) < KEEPALIVE_MS;
    return result;
}

bool CanTransport::stcmTxTimerElapsed()
{
    return ms() - lastStcmTxTime > TX_KEEPALIVE_MS;
}

int CanTransport::canRxDropped()
{
    return rxDropped;
}

int CanTransport::canRxFails()
{
    return rxFails;
}

int CanTransport::canTxFails()
{
    return txFails;
}

bool CanTransport::readable()
{
    static uint32_t lt = 0;
    if (canIf != nullptr)
    {
        auto er = canIf->getLastError();
        if (er.esrTime != lt)
        {
            lt = er.esrTime;
            Serial.printf("TEC: %d, REC:%d, LEC:%d, BOFF: %d\n", er.TEC, er.REC, er.LEC, er.BOFF);
            Serial.flush();
        }
        return canIf->read(cachedMessage);
    }
    return false;
}

CAN_message_t CanTransport::read()
{
    //Serial.printf("CAN-R: 0x%03x\n",cachedMessage.id);
    if (cachedMessage.id == CAN_STCM_LINKREPORT || cachedMessage.id == CAN_STCM_TRACTION_STATE || cachedMessage.id == CAN_STCM_AUX_STATE || cachedMessage.id == CAN_STCM_THROTTLE_STAT || cachedMessage.id == CAN_STCM_MSG_PUSH)
    {
        lastStcmRxTime = ms();
    }
    return cachedMessage;
}