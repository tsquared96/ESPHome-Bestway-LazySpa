#ifndef PORTS_H
#define PORTS_H

#include <Arduino.h>

/**
 * ESP8266 High-Speed GPIO Register Definitions
 * These allow the driver to read/write pins in nanoseconds, 
 * which is required to keep up with the Spa's clock signal.
 */

#ifndef PIN_OUT
  #define PIN_OUT         0x60000300
  #define PIN_OUT_SET     0x60000304
  #define PIN_OUT_CLEAR   0x60000308
  #define PIN_DIR_OUTPUT  0x6000030C
  #define PIN_DIR_INPUT   0x60000310
  #define PIN_IN          0x60000318
#endif

#endif // PORTS_H
