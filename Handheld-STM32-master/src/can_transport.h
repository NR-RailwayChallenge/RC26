#ifndef CAN_TRANSPORT_H
#define CAN_TRANSPORT_H


#include <Arduino.h>
#include "STM32_CAN.h"
#include "constants.h"
#include "rc_can_defs.h"
#include "timing.h"

class CanTransport {

private:
    void can_irq();
    int rxDropped = 0, rxFails = 0, txFails = 0;
    volatile unsigned long lastStcmRxTime = 0;
    volatile unsigned long lastStcmTxTime = 0;
    STM32_CAN* canIf;
    CAN_message_t cachedMessage;

public:
    CanTransport(STM32_CAN* canIfInst);

    /// @brief Transmits a CAN message, and if a CAN message comes in simultaneously in interrupt mode, processes this after transmit is complete.
    /// @param frame The CAN frame to Tx.
    /// @return True if sent, false for any failure reason.
    bool transmit(CAN_message_t *frame);

    /// @brief Checks if there are any received messages waiting to be processed. If receiving in polling mode, this will transfer from the MCP2515 buffer into the internal circular buffer.
    /// @return True if messages are pending in the queue, false otherwise.
    bool readable();

    /// @brief Gets the least recently received message from the buffer and removes it from the buffer. To be used in conjunction with readable().
    /// @return The CAN frame.
    CAN_message_t read();

    int canTxFails();
    int canRxFails();
    int canRxDropped();
    bool stcmOnline();
    bool stcmTxTimerElapsed();

};

#endif