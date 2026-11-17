#ifndef INCOMING_STRUCT_H
#define INCOMING_STRUCT_H

#include <cstdint>

enum drive_mode_t{
    Shunting = 0,
    Standard = 1,
    Regen = 2,
    Autostop = 3,
    Neutral = 4
};

struct incoming_values{
    bool self_online;
    bool handheld_online;
    drive_mode_t drive_mode;
};

#endif
