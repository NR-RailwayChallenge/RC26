#include "nucleo_interface.h"


void NucleoInterface::resetParser()
{
    parsedBytes = 0;
    memset(parserBuf, 0, sizeof(parserBuf));
    checksum = 0;
}

NucleoInterface::NucleoInterface(SerialUART *uart)
{
    memset(&pBuf,0,sizeof(pBuf));
    memset(&dBuf,0,sizeof(dBuf));
    ser = uart;
    ser->setTX(PIN_NUCLEO_TX);
    ser->setRX(PIN_NUCLEO_RX);
    ser->begin(NUCLEO_SERIAL_BAUD);
}

void NucleoInterface::signalReady()
{
    ser->write(0x06);
}

PicoPacket NucleoInterface::getNextPacket()
{
    if (parserBuf[0] == 'D' && parserBuf[1] == '1')
    {
        decode_base64_cl(&parserBuf[2], PICO_PACK_ENCODED_PAYLOAD_SIZE, (unsigned char *)&pBuf);
        resetParser();
    }
    return pBuf;
}

PicoDiag NucleoInterface::getNextDiagPacket()
{
    if (parserBuf[0] == 'D' && parserBuf[1] == 'x')
    {
        decode_base64_cl(&parserBuf[2], PICO_DIAG_ENCODED_PAYLOAD_SIZE, (unsigned char *)&dBuf);
        resetParser();
    }
    return dBuf;
}

bool NucleoInterface::processNextChunk()
{
    // If there are remaining fragments in the rx buffer, run the parser
    while (serialInBufferLen > 0)
    {
        if (doParsing())
            return true;
    }
    // Transfer the UART buffer here
    int toRx = ser->available();
    if (toRx == 0)
        return false;
    serialInBufferStart = 0;
    serialInBufferLen = ser->readBytes(siBuf, toRx);
    // Serial.printf("ln%d, ar%d, %p\n", serialInBufferLen, toRx, siBuf);
    //  Run the parser again
    while (serialInBufferLen > 0)
    {
        if (doParsing())
            return true;
    }
    return false;
}

bool NucleoInterface::doParsing()
{
    // Check that there's actually serial data to act on
    if (serialInBufferLen < 1)
    {
        return false;
    }
    uint8_t iChar = siBuf[serialInBufferStart++];
    serialInBufferLen--;
    // printf("pb%d, ic%2x\n",parsedBytes,iChar);
    if (iChar >= 0x20 && iChar <= 0x7e && parsedBytes < PARSER_BUF_LEN)
    {
        // Printable character, add it to the parser buffer
        parserBuf[parsedBytes++] = iChar;
        checksum += iChar;
    }
    else if (iChar == 0x0A || iChar == 0x0D)
    {
        if (parsedBytes < 0)
            return false;
        if (checksum != 0xAA)
        {
            Serial.printf("dip, ac: %2x\n", checksum);
            resetParser();
            return false;
        }
        return true;
    }
    else
    {
        checksum += iChar;
    }
    return false;
}

MessageType NucleoInterface::checkMessageType()
{
    switch (parserBuf[0])
    {
    case 'D':
    {
        switch (parserBuf[1])
        {
        case '1':
            return MessageType::Pico_Pack;
            break;
        case 'x':
            return MessageType::Pico_Diag;
        case 'R':
            rp2040.reboot();
        default:
            resetParser();
            return MessageType::Unknown;
            break;
        }
    }
    break;

    default:
        resetParser();
        return MessageType::Unknown;
        break;
    }
}
