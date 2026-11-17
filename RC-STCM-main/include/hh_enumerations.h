#ifndef HH_ENUMERATIONS_H
#define HH_ENUMERATIONS_H

enum DriveModeEnum {
    SHUNTING,
    STANDARD,
    REGEN,
    AUTOSTOP,
    NEUTRAL
};

enum BrakeDemand{
    LVL_0,
    LVL_1,
    LVL_2,
    LVL_3
};

enum CouplerStatus{
    UNCOUPLING = 0,
    UNCOUPLED = 1,
    COUPLING = 2,
    COUPLED = 3
};




#endif
