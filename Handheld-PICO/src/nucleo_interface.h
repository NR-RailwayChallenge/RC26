#ifndef NUCLEO_INTERFACE_H
#define NUCLEO_INTERFACE_H

#include "constants.h"
#include "pin_defs.h"
#include "b64.h"
#include <cstdint>
#include <Arduino.h>

// The order is important. Go largest to smallest, otherwise padding bytes may be inserted.
#pragma pack(1)
struct PicoPacket
{
    float speed;           // 4 Bytes
    float tractionCurrent; // 4 Bytes
    float regenEnergy;     // 4 Bytes
    uint32_t flags;        // 4 Bytes
    uint16_t bus12;        // 2 Bytes
    uint16_t bus24;        // 2 Bytes
    uint16_t bus36;        // 2 Bytes
    uint16_t buscap;       // 2 Bytes
    uint16_t busTrac;      // 2 Bytes
    uint8_t message;       // 1 Byte
    // Total Payload of 27 Bytes -> 36 Chars of Base 64 Encoded text
};

#pragma pack (1)
struct PicoDiag{
    uint16_t brakeAnalogue;
    uint16_t throttleAnalogue;
    uint8_t flags;
    uint8_t rand;
    // 6 bytes -> 8 chars of B64.
};

#define PICO_PACK_PAYLOAD_SIZE 25
#define PICO_PACK_ENCODED_PAYLOAD_SIZE 36
#define PICO_PACK_PREFIX "D1"

#define PICO_DIAG_PAYLOAD_SIZE 6
#define PICO_DIAG_ENCODED_PAYLOAD_SIZE 8
#define PICO_DIAG_PREFIX "Dx"

#define PARSER_BUF_LEN 42

#define PICO_PACK_FLAG_DIR 0
#define PICO_PACK_FLAG_KSR 1
#define PICO_PACK_FLAG_ES 2
#define PICO_PACK_FLAG_BK_BR1 3
#define PICO_PACK_FLAG_BK_BR2 4
#define PICO_PACK_FLAG_BK_BR3 5
#define PICO_PACK_FLAG_BK_MBR 6
#define PICO_PACK_FLAG_BF 7
#define PICO_PACK_FLAG_AS_EN 8
#define PICO_PACK_FLAG_BK_BH 9
#define PICO_PACK_FLAG_RG_EN 10
#define PICO_PACK_FLAG_AS_MD 11
#define PICO_PACK_FLAG_DSD_STAT 12
#define PICO_PACK_FLAG_FIRE 13
#define PICO_PACK_FLAG_SOFT_ES 14
#define PICO_PACK_FLAG_CAN_STAT 15
#define PICO_PACK_FLAG_ATE 16
#define PICO_PACK_FLAG_DTE 17
#define PICO_PACK_FLAG_SHUNT_MODE 18
#define PICO_PACK_COUPLER 19 // + 20


#define PICO_DIAG_FLAG_AS 2
#define PICO_DIAG_FLAG_AC 3
#define PICO_DIAG_FLAG_CD 4
#define PICO_DIAG_FLAG_HR 5
#define PICO_DIAG_FLAG_RG 6
#define PICO_DIAG_FLAG_DSD 7

enum MessageType{
    Unknown,
    Pico_Pack,
    Pico_Diag
};

class NucleoInterface{
private:
    SerialUART *ser;
    PicoPacket pBuf;
    PicoDiag dBuf;
    uint8_t siBuf[64];
    int serialInBufferStart = 0, serialInBufferLen = 0;
    uint8_t parserBuf[PARSER_BUF_LEN];
    uint8_t parsedBytes = 0;
    uint8_t checksum = 0;

    void resetParser();
public:
    NucleoInterface(SerialUART* uart);

    void signalReady();

    bool processNextChunk();

    bool doParsing();

    MessageType checkMessageType();
    
    PicoPacket getNextPacket();

    PicoDiag getNextDiagPacket();
};

#endif