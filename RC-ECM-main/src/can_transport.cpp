#include "can_transport.h"

// CanTransport::CanTransport(CAN_TypeDef* canPort, CAN_PINS pins)
CanTransport::CanTransport(STM32_CAN *canIfInst)
{
    rxDropped = 0;
    rxFails = 0;
    txFails = 0;
    lastStcmRxTime = 0;
    canIf = canIfInst;
    canIf->begin(true);
    canIf->setBaudRate(CAN_BAUDRATE);
    canIf->setMBFilterProcessing(CAN_BANK::MB0, CAN_FILTER, CAN_MASK);
}

bool CanTransport::transmit(CAN_message_t *frame)
{
    // This code is a wrapper to add some statistics tracking.
    // This can be ignored.
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
    else if ((frame->id & 0x4) == 0)
    {
        lastStcmTxTime = ms();
    }
    return isSuccess;
}

bool CanTransport::stcmOnline()
{
    // If the last time we received a message from the
    // STCM is less than KEEPALIVE_MS ago, we consider the STCM to be online.
    bool result = ms() - lastStcmRxTime < KEEPALIVE_MS;
    return result;
}

bool CanTransport::stcmTxTimerElapsed()
{
    // This function tells us if we've transmitted to the STCM recently enough.
    // If we don't the STCM might think we've fallen offline.
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
    // Again, more debugging code to report CAN errors.
    // Only important if the CAN bus isn't behaving and
    // we need to invesitgate why. Refer to RM0390 by ST-Micro.
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
    // This function updates the timer for 'last time we received from the STCM', so we can track if
    // we are still 'connected' to it.
    if (cachedMessage.id == CAN_STCM_LINKREPORT || cachedMessage.id == CAN_STCM_AUX_STATE)
    {
        lastStcmRxTime = ms();
    }
    return cachedMessage;
}