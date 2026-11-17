#ifndef TIMING_H
#define TIMING_H

#include "Arduino.h"

// VS Code doesn't recognise the millis or micros functions for some reason,
// but has no issues compiling them. This file exists to hide the error
// squiggles.

unsigned long us();

unsigned long ms();

#endif