#ifndef MARSHALLED_DATA_H
#define MARSHALLED_DATA_H

#include "nucleo_interface.h"

struct MarshalledData{
    PicoPacket packet;
    PicoDiag diagPacket;
    bool diagMode;
};

#endif //MARSHALLED_DATA_H