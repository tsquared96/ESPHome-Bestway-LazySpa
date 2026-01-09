#pragma once

namespace esphome {
namespace bestway_spa {

enum ProtocolType {
    PROTOCOL_6WIRE_T1 = 0,
    PROTOCOL_6WIRE_T2 = 1,
    PROTOCOL_4WIRE = 2
};

enum ButtonCode {
    NONE    = 0x00,
    PUMP    = 0x01,
    HEAT    = 0x02,
    BUBBLES = 0x04,
    LOCK    = 0x08,
    UP      = 0x10,
    DOWN    = 0x20,
    TIMER   = 0x40
};

} // namespace bestway_spa
} // namespace esphome
