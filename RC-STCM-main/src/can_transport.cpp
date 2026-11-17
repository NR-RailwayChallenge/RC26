#include "can_transport.h"

//CanTransport::CanTransport(CAN_TypeDef* canPort, CAN_PINS pins)
CanTransport::CanTransport(STM32_CAN* canIfInst)
{
    rxDropped = 0;
    rxFails = 0;
    txFails = 0;
    lastRDTRxTime = 0;
    lastECMRxTime = 0;
    lastLAMRxTime = 0;
    lastHHRxTime = 0;
    canIf = canIfInst;
    canIf->begin(false);
    canIf->setBaudRate(CAN_BAUDRATE);
    canIf->setMBFilterProcessing(CAN_BANK::MB0,CAN_FILTER,CAN_MASK);
}

bool CanTransport::transmit(CAN_message_t *frame)
{
    bool isSuccess;
    if (canIf != nullptr)
    {
        isSuccess = canIf->write(*frame);
    }else{
        printf("CAN TX fail, no canIf\n");
    }
    if (!isSuccess)
    {
        txFails++;
    }
    return isSuccess;
}

bool CanTransport::HHOnline()
{
    return ms() - lastHHRxTime < KEEPALIVE_MS;
}
bool CanTransport::ECMOnline()
{
    return true;
}
bool CanTransport::RDTOnline()
{
    return true;
}
bool CanTransport::LAMOnline()
{
    return true;
    //return ms() - lastLAMRxTime < KEEPALIVE_MS;
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
    if (canIf != nullptr)
    {
        return canIf->read(cachedMessage);
    }
    return false;
}



CAN_message_t CanTransport::read()
{
    if (cachedMessage.id == HH_THROTTLE || cachedMessage.id == HH_BTN_STATE)
        {
            lastHHRxTime = ms();
        }
    else if (cachedMessage.id == ECM_CAP_ENERGY || cachedMessage.id == ECM_POWER_SUPPLY || cachedMessage.id == ECM_VESC_POWER)
    {
        lastECMRxTime = ms();
    }
    else if (cachedMessage.id == LAM_ANNOUNCE)
    {
        lastLAMRxTime = ms();
    }
    else if (cachedMessage.id == RDT_GNSS)
    {
        lastRDTRxTime = ms();
    }
    return cachedMessage;
}
