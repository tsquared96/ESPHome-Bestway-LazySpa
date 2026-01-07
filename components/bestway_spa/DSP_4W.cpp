/**
 * DSP 4-Wire Protocol Handler Implementation
 *
 * Adapted from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/dsp/DSP_4W.cpp
 *
 * Original work Copyright (c) visualapproach
 * Licensed under GPL v3
 */

#include "DSP_4W.h"
#ifdef ESP8266
#include <umm_malloc/umm_heap_select.h>
#endif

namespace bestway_va {

// Model-specific jump tables and allowed states (same as CIO_4W)

// 54123 tables
const uint8_t DSP_54123::JUMPTABLE[4][4] = {
    {1, 0, 2, 3},
    {0, 1, 2, 3},
    {1, 2, 0, 3},
    {1, 3, 0, 2}
};
const uint8_t DSP_54123::ALLOWEDSTATES[4][4] = {
    {0, 0, 0, 0},
    {1, 0, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 2}
};

// 54138 tables
const uint8_t DSP_54138::JUMPTABLE[5][4] = {
    {1, 2, 3, 4},
    {0, 2, 3, 4},
    {1, 0, 3, 4},
    {1, 2, 0, 4},
    {1, 2, 0, 3}
};
const uint8_t DSP_54138::ALLOWEDSTATES[5][4] = {
    {0, 0, 0, 0},
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 2}
};

// 54144 tables
const uint8_t DSP_54144::JUMPTABLE[5][4] = {
    {1, 2, 3, 4},
    {0, 2, 3, 4},
    {1, 0, 3, 4},
    {1, 2, 0, 4},
    {1, 2, 0, 3}
};
const uint8_t DSP_54144::ALLOWEDSTATES[5][4] = {
    {0, 0, 0, 0},
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 2}
};

// 54154 tables
const uint8_t DSP_54154::JUMPTABLE[6][4] = {
    {3, 0, 1, 2},
    {4, 1, 0, 2},
    {5, 2, 0, 1},
    {0, 3, 4, 5},
    {1, 4, 3, 5},
    {2, 5, 3, 4}
};
const uint8_t DSP_54154::ALLOWEDSTATES[6][4] = {
    {0, 0, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 2},
    {1, 0, 0, 0},
    {1, 0, 1, 0},
    {1, 0, 1, 1}
};

// 54173 tables
const uint8_t DSP_54173::JUMPTABLE[8][4] = {
    {6, 4, 1, 3},
    {7, 4, 0, 3},
    {3, 5, 6, 7},
    {2, 4, 0, 1},
    {5, 0, 1, 3},
    {4, 6, 1, 3},
    {0, 5, 7, 2},
    {1, 5, 6, 2}
};
const uint8_t DSP_54173::ALLOWEDSTATES[8][4] = {
    {0, 0, 0, 0},
    {0, 0, 1, 0},
    {1, 0, 1, 1},
    {0, 0, 1, 2},
    {0, 1, 0, 0},
    {1, 1, 0, 0},
    {1, 0, 0, 0},
    {1, 0, 1, 0}
};

// Jumptable accessors
uint8_t DSP_54123::getJumptable(int row, int col) { return JUMPTABLE[row][col]; }
uint8_t DSP_54123::getAllowedstates(int row, int col) { return ALLOWEDSTATES[row][col]; }
uint8_t DSP_54138::getJumptable(int row, int col) { return JUMPTABLE[row][col]; }
uint8_t DSP_54138::getAllowedstates(int row, int col) { return ALLOWEDSTATES[row][col]; }
uint8_t DSP_54144::getJumptable(int row, int col) { return JUMPTABLE[row][col]; }
uint8_t DSP_54144::getAllowedstates(int row, int col) { return ALLOWEDSTATES[row][col]; }
uint8_t DSP_54154::getJumptable(int row, int col) { return JUMPTABLE[row][col]; }
uint8_t DSP_54154::getAllowedstates(int row, int col) { return ALLOWEDSTATES[row][col]; }
uint8_t DSP_54173::getJumptable(int row, int col) { return JUMPTABLE[row][col]; }
uint8_t DSP_54173::getAllowedstates(int row, int col) { return ALLOWEDSTATES[row][col]; }

DSP_4W::DSP_4W() {
    _bubbles = 0;
    _pump = 0;
    _jets = 0;
}

DSP_4W::~DSP_4W() {
    if (_dsp_serial) {
        delete _dsp_serial;
        _dsp_serial = nullptr;
    }
}

void DSP_4W::setup(int dsp_tx, int dsp_rx, int dummy, int dummy2) {
#ifdef ESP8266
    HeapSelectIram ephemeral;
#endif
    _dsp_serial = new EspSoftwareSerial::UART;
    _dsp_serial->begin(9600, SWSERIAL_8N1, dsp_tx, dsp_rx, false, 24);
    _dsp_serial->setTimeout(20);

    dsp_toggles.locked_pressed = 0;
    dsp_toggles.power_change = 0;
    dsp_toggles.unit_change = 0;
    dsp_toggles.pressed_button = NOBTN;
    dsp_toggles.no_of_heater_elements_on = 2;
    dsp_toggles.godmode = false;

    _dsp_serial->write(_to_DSP_buf, PAYLOADSIZE);
}

void DSP_4W::stop() {
    if (_dsp_serial) {
        _dsp_serial->stopListening();
        delete _dsp_serial;
        _dsp_serial = nullptr;
    }
}

void DSP_4W::pause_all(bool action) {
    if (!_dsp_serial) return;
    if (action) {
        _dsp_serial->stopListening();
    } else {
        _dsp_serial->listen();
    }
}

void DSP_4W::updateToggles() {
    // Copy state values to toggles
    dsp_toggles.godmode = dsp_states.godmode;
    dsp_toggles.target = dsp_states.target;
    dsp_toggles.no_of_heater_elements_on = dsp_states.no_of_heater_elements_on;

    if (!_dsp_serial || !_dsp_serial->available()) return;

    uint8_t tempbuffer[PAYLOADSIZE];
    int msglen = _dsp_serial->readBytes(tempbuffer, PAYLOADSIZE);
    if (msglen != PAYLOADSIZE) return;

    // Verify checksum
    uint8_t calculatedChecksum = tempbuffer[1] + tempbuffer[2] + tempbuffer[3] + tempbuffer[4];
    if (tempbuffer[DSP_CHECKSUMINDEX] != calculatedChecksum) {
        bad_packets_count++;
        return;
    }

    good_packets_count++;

    // Copy to buffers
    for (int i = 0; i < PAYLOADSIZE; i++) {
        _from_DSP_buf[i] = tempbuffer[i];
        _raw_payload_from_dsp[i] = tempbuffer[i];
    }

    uint8_t bubbles = (_from_DSP_buf[COMMANDINDEX] & getBubblesBitmask()) > 0;
    uint8_t pump = (_from_DSP_buf[COMMANDINDEX] & getPumpBitmask()) > 0;
    uint8_t jets = (_from_DSP_buf[COMMANDINDEX] & getJetsBitmask()) > 0;

    if (dsp_states.godmode) {
        // Detect changes
        dsp_toggles.bubbles_change = _bubbles != bubbles;
        dsp_toggles.heat_change = 0;
        dsp_toggles.jets_change = _jets != jets;
        dsp_toggles.locked_pressed = 0;
        dsp_toggles.power_change = 0;
        dsp_toggles.pump_change = _pump != pump;
        dsp_toggles.unit_change = 0;
        dsp_toggles.pressed_button = NOBTN;
    }

    _bubbles = bubbles;
    _pump = pump;
    _jets = jets;

    _serialreceived = true;
}

void DSP_4W::handleStates() {
    static unsigned long lastmillis = millis();
    int elapsedtime = millis() - lastmillis;
    lastmillis += elapsedtime;
    _time_since_last_transmission_ms += elapsedtime;

    // Generate payload based on godmode
    if (dsp_states.godmode) {
        generatePayload();
    } else {
        if (PAYLOADSIZE > _raw_payload_to_dsp.size()) return;
        for (int i = 0; i < PAYLOADSIZE; i++) {
            _to_DSP_buf[i] = _raw_payload_to_dsp[i];
        }
    }

    if (_readyToTransmit || (_time_since_last_transmission_ms > _max_allowed_time_between_transmissions_ms)) {
        _readyToTransmit = false;
        if (_dsp_serial) {
            _dsp_serial->write(_to_DSP_buf, PAYLOADSIZE);
        }
        write_msg_count++;
        if (_time_since_last_transmission_ms > max_time_between_transmissions_ms) {
            max_time_between_transmissions_ms = _time_since_last_transmission_ms;
        }
        _time_since_last_transmission_ms = 0;
    }
}

bool DSP_4W::getSerialReceived() {
    bool result = _serialreceived;
    _serialreceived = false;
    return result;
}

void DSP_4W::setSerialReceived(bool txok) {
    _readyToTransmit = txok;
}

void DSP_4W::generatePayload() {
    int tempC;
    tempC = dsp_states.unit ? dsp_states.temperature : F2C(dsp_states.temperature);

    _to_DSP_buf[0] = _raw_payload_to_dsp[0];  // SoF
    _to_DSP_buf[1] = _raw_payload_to_dsp[1];  // Unknown, usually 2
    _to_DSP_buf[2] = tempC;
    _to_DSP_buf[3] = dsp_states.error;
    _to_DSP_buf[4] = _raw_payload_to_dsp[4];  // Ready flags?
    _to_DSP_buf[5] = _to_DSP_buf[1] + _to_DSP_buf[2] + _to_DSP_buf[3] + _to_DSP_buf[4];  // Checksum
    _to_DSP_buf[6] = _raw_payload_to_dsp[6];  // EoF
}

}  // namespace bestway_va
