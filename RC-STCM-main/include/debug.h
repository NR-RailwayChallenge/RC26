#ifndef debug_h
#define debug_h

#define DEBUG_ENABLED
#define CAN_RC_DEBUG
//#define TRACE_PINS_ENABLED

#ifdef DEBUG_ENABLED
#define trace(str)          \
  Serial.print("[TRACE] "); \
  Serial.println(str)
#else
#define trace(str)
#endif

#ifdef TRACE_PINS_ENABLED
#define TDW(p1,p2) digitalWrite(p1,p2)
#else
#define TDW(p1,p2)
#endif

#endif