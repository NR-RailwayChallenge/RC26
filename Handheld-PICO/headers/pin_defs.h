// Pin Definitions file for Pico W

#ifndef pindefs_h
#define pindefs_h

#define PIN_NUCLEO_TX 8
#define PIN_NUCLEO_RX 9
//Serial 2 maps to UART 1
#define NUCLEO_UART Serial2

#define PIN_LED_LEFT 6
#define PIN_LED_RIGHT 7

#define PIN_PRIMARY_TASK_TRACE 20
#define PIN_SECONDARY_TASK_TRACE 21

#define PIN_SIM_TOOL_DIAG 22

// The LCD pinout is declared in the user_setup.h file in the library folder.

/*
// The key lines are included below incase you need to rebuild the file:
#define TFT_MISO 4
#define TFT_MOSI 3
#define TFT_SCLK 2
#define TFT_CS   20  
#define TFT_DC   13  
#define TFT_RST  14 
#define ILI9341_DRIVER 
#define TFT_RGB_ORDER TFT_BGR

#define LOAD_GLCD  
#define LOAD_FONT2 
#define LOAD_FONT4 
#define LOAD_FONT6 
#define LOAD_FONT7 
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

*/

#endif