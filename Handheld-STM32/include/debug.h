#ifndef debug_h
#define debug_h

#define DEBUG_ENABLED
#define CAN_RC_DEBUG

#ifdef DEBUG_ENABLED
#define trace(str)          \
  Serial.print("[TRACE] "); \
  Serial.println(str)
#else
#define trace(str)
#endif

#endif