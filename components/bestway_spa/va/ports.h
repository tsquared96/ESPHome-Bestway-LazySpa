/**
 * Port definitions for ESP8266 direct GPIO register access
 *
 * From VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 *
 * Licensed under GPL v3
 */

#pragma once

#ifdef ESP8266
// Direct port manipulation memory addresses for ESP8266
#define PIN_OUT         0x60000300
#define PIN_OUT_SET     0x60000304
#define PIN_OUT_CLEAR   0x60000308

#define PIN_DIR         0x6000030C
#define PIN_DIR_OUTPUT  0x60000310
#define PIN_DIR_INPUT   0x60000314

#define PIN_IN          0x60000318
#endif
