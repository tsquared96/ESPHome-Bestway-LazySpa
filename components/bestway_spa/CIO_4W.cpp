/**
 * CIO 4-Wire Protocol Handler Implementation
 *
 * Adapted from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/cio/CIO_4W.cpp
 *
 * Original work Copyright (c) visualapproach
 * Licensed under GPL v3
 */

#include "CIO_4W.h"
#ifdef ESP8266
#include <umm_malloc/umm_heap_select.h>
#endif

namespace bestway_va {

// Model-specific jump tables and allowed states

// 54123 tables
const uint8_t CIO_54123::JUMPTABLE[4][4] = {
    {1, 0, 2, 3},
    {0, 1, 2, 3},
    {1, 2, 0, 3},
    {1, 3, 0, 2}
};
const uint8_t CIO_54123::ALLOWEDSTATES[4][4] = {
    {0, 0, 0, 0},
    {1, 0, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 2}
};

// 54138 tables
const uint8_t CIO_54138::JUMPTABLE[5][4] = {
    {1, 2, 3, 4},
    {0, 2, 3, 4},
    {1, 0, 3, 4},
    {1, 2, 0, 4},
    {1, 2, 0, 3}
};
const uint8_t CIO_54138::ALLOWEDSTATES[5][4] = {
    {0, 0, 0, 0},
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 2}
};

// 54144 tables
const uint8_t CIO_54144::JUMPTABLE[5][4] = {
    {1, 2, 3, 4},
    {0, 2, 3, 4},
    {1, 0, 3, 4},
    {1, 2, 0, 4},
    {1, 2, 0, 3}
};
const uint8_t CIO_54144::ALLOWEDSTATES[5][4] = {
    {0, 0, 0, 0},
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 2}
};

// 54154 tables
const uint8_t CIO_54154::JUMPTABLE[6][4] = {
    {3, 0, 1, 2},
    {4, 1, 0, 2},
    {5, 2, 0, 1},
    {0, 3, 4, 5},
    {1, 4, 3, 5},
    {2, 5, 3, 4}
};
const uint8_t CIO_54154::ALLOWEDSTATES[6][4] = {
    {0, 0, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 2},
    {1, 0, 0, 0},
    {1, 0, 1, 0},
    {1, 0, 1, 1}
};

// 54173 tables
const uint8_t CIO_54173::JUMPTABLE[8][4] = {
    {6, 4, 1, 3},
    {7, 4, 0, 3},
    {3, 5, 6, 7},
    {2, 4, 0, 1},
    {5, 0, 1, 3},
    {4, 6, 1, 3},
    {0, 5, 7, 2},
    {1, 5, 6, 2}
};
const uint8_t CIO_54173::ALLOWEDSTATES[8][4] = {
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
uint8_t CIO_54123::getJumptable(int row, int col) { return JUMPTABLE[row][col]; }
uint8_t CIO_54123::getAllowedstates(int row, int col) { return ALLOWEDSTATES[row][col]; }
uint8_t CIO_54138::getJumptable(int row, int col) { return JUMPTABLE[row][col]; }
uint8_t CIO_54138::getAllowedstates(int row, int col) { return ALLOWEDSTATES[row][col]; }
uint8_t CIO_54144::getJumptable(int row, int col) { return JUMPTABLE[row][col]; }
uint8_t CIO_54144::getAllowedstates(int row, int col) { return ALLOWEDSTATES[row][col]; }
uint8_t CIO_54154::getJumptable(int row, int col) { return JUMPTABLE[row][col]; }
uint8_t CIO_54154::getAllowedstates(int row, int col) { return ALLOWEDSTATES[row][col]; }
uint8_t CIO_54173::getJumptable(int row, int col) { return JUMPTABLE[row][col]; }
uint8_t CIO_54173::getAllowedstates(int row, int col) { return ALLOWEDSTATES[row][col]; }

CIO_4W::CIO_4W() {
    _prev_ms = 0;
    _currentStateIndex = 0;
    _heat_bitmask = 0;
}

CIO_4W::~CIO_4W() {
    if (_cio_serial) {
        delete _cio_serial;
        _cio_serial = nullptr;
    }
}

void CIO_4W::setup(int cio_rx, int cio_tx, int dummy) {
#ifdef ESP8266
    HeapSelectIram ephemeral;
#endif
    _cio_serial = new EspSoftwareSerial::UART;
    _cio_serial->begin(9600, SWSERIAL_8N1, cio_tx, cio_rx, false, 24);
    _cio_serial->setTimeout(20);

    cio_states.target = 20;
    cio_states.locked = false;
    cio_states.power = true;
    cio_states.unit = true;
    cio_states.char1 = ' ';
    cio_states.char2 = ' ';
    cio_states.char3 = ' ';

    _currentStateIndex = 0;
    _cio_serial->write(_to_CIO_buf, PAYLOADSIZE);
}

void CIO_4W::stop() {
    if (_cio_serial) {
        _cio_serial->stopListening();
        delete _cio_serial;
        _cio_serial = nullptr;
    }
}

void CIO_4W::pause_all(bool action) {
    if (!_cio_serial) return;
    if (action) {
        _cio_serial->stopListening();
    } else {
        _cio_serial->listen();
    }
}

void CIO_4W::handleToggles() {
    uint32_t elapsed_time_ms = millis() - _prev_ms;
    _prev_ms = millis();
    _time_since_last_transmission_ms += elapsed_time_ms;

    cio_states.target = cio_toggles.target;

    if (_heater2_countdown_ms > 0) _heater2_countdown_ms -= elapsed_time_ms;
    if (_cool_heater_countdown_ms > 0) _cool_heater_countdown_ms -= elapsed_time_ms;

    // After countdown, turn off pump just once
    if (_cool_heater_countdown_ms <= 0 && _turn_off_pump_flag) {
        _currentStateIndex = getJumptable(_currentStateIndex, PUMPTOGGLE);
        togglestates();
        _turn_off_pump_flag = false;
    }

    // Handle godmode (manual control from web/MQTT)
    if (!cio_toggles.godmode) {
        // Copy raw payload to CIO
        for (unsigned int i = 0; i < sizeof(_to_CIO_buf); i++) {
            _to_CIO_buf[i] = _raw_payload_to_cio[i];
        }
        cio_states.godmode = false;

        if (_readyToTransmit) {
            _readyToTransmit = false;
            _cio_serial->write(_to_CIO_buf, PAYLOADSIZE);
        }
        return;
    }

    cio_states.godmode = true;

    if (cio_toggles.unit_change) {
        cio_states.unit = !cio_states.unit;
        cio_states.unit ? cio_states.temperature = F2C(cio_states.temperature) :
                          cio_states.temperature = C2F(cio_states.temperature);
    }

    if (cio_toggles.heat_change) {
        _currentStateIndex = getJumptable(_currentStateIndex, HEATTOGGLE);
        togglestates();
    }

    if (cio_toggles.bubbles_change && getHasair()) {
        _currentStateIndex = getJumptable(_currentStateIndex, BUBBLETOGGLE);
        togglestates();
    }

    if (cio_toggles.pump_change) {
        if (!cio_states.pump) {
            _currentStateIndex = getJumptable(_currentStateIndex, PUMPTOGGLE);
            togglestates();
        } else {
            // Pump turning OFF -> turn off heaters first
            if (cio_states.heat) {
                _currentStateIndex = getJumptable(_currentStateIndex, HEATTOGGLE);
                togglestates();
                _cool_heater_countdown_ms = _HEATERCOOLING_DELAY_MS;
                _turn_off_pump_flag = true;
            } else {
                _currentStateIndex = getJumptable(_currentStateIndex, PUMPTOGGLE);
                togglestates();
            }
        }
    }

    if (cio_toggles.jets_change && getHasjets()) {
        _currentStateIndex = getJumptable(_currentStateIndex, JETSTOGGLE);
        togglestates();
    }

    if (cio_toggles.no_of_heater_elements_on < 2) {
        _heat_bitmask = getHeatBitmask1();
    }

    regulateTemp();
    antifreeze();
    antiboil();
    generatePayload();

    if (_readyToTransmit || (_time_since_last_transmission_ms > _max_time_between_transmissions_ms)) {
        _readyToTransmit = false;
        _time_since_last_transmission_ms = 0;
        _cio_serial->write(_to_CIO_buf, PAYLOADSIZE);
        write_msg_count++;
    }
}

void CIO_4W::generatePayload() {
    _to_CIO_buf[0] = _raw_payload_to_cio[0];  // SoF
    _to_CIO_buf[1] = _raw_payload_to_cio[1];  // Unknown, usually 1

    cio_states.heatgrn = !cio_states.heatred && cio_states.heat;

    _to_CIO_buf[COMMANDINDEX] = (cio_states.heatred * _heat_bitmask) |
                                 (cio_states.jets * getJetsBitmask()) |
                                 (cio_states.bubbles * getBubblesBitmask()) |
                                 (cio_states.pump * getPumpBitmask());

    if (_to_CIO_buf[COMMANDINDEX] > 0) {
        _to_CIO_buf[COMMANDINDEX] |= getPowerBitmask();
    }

    _to_CIO_buf[3] = _raw_payload_to_cio[3];  // Unknown
    _to_CIO_buf[4] = _raw_payload_to_cio[4];  // Unknown

    // Calculate checksum
    _to_CIO_buf[CIO_CHECKSUMINDEX] = _to_CIO_buf[1] + _to_CIO_buf[2] + _to_CIO_buf[3] + _to_CIO_buf[4];
    _to_CIO_buf[6] = _raw_payload_to_cio[6];  // EoF
}

void CIO_4W::updateStates() {
    if (!_cio_serial || !_cio_serial->available()) return;

    uint8_t tempbuffer[PAYLOADSIZE];
    int msglen = _cio_serial->readBytes(tempbuffer, PAYLOADSIZE);
    if (msglen != PAYLOADSIZE) return;

    // Verify checksum
    uint8_t calculatedChecksum = tempbuffer[1] + tempbuffer[2] + tempbuffer[3] + tempbuffer[4];
    if (tempbuffer[CIO_CHECKSUMINDEX] != calculatedChecksum) {
        bad_packets_count++;
        return;
    }

    good_packets_count++;

    // Copy to buffers
    for (int i = 0; i < PAYLOADSIZE; i++) {
        _from_CIO_buf[i] = tempbuffer[i];
        _raw_payload_from_cio[i] = tempbuffer[i];
    }

    cio_states.temperature = _from_CIO_buf[TEMPINDEX];
    if (!cio_states.unit) cio_states.temperature = C2F(cio_states.temperature);
    cio_states.error = _from_CIO_buf[ERRORINDEX];

    // Show temp in display chars
    cio_states.char1 = 48 + (cio_states.temperature) / 100;
    cio_states.char2 = 48 + (cio_states.temperature % 100) / 10;
    cio_states.char3 = 48 + (cio_states.temperature % 10);

    // Check for error
    if (cio_states.error) {
        _to_CIO_buf[COMMANDINDEX] = 0;
        cio_states.godmode = false;
        cio_states.char1 = 'E';
        cio_states.char2 = (char)(48 + (_from_CIO_buf[ERRORINDEX] / 10));
        cio_states.char3 = (char)(48 + (_from_CIO_buf[ERRORINDEX] % 10));
    }

    _serialreceived = true;
}

bool CIO_4W::getSerialReceived() {
    bool result = _serialreceived;
    _serialreceived = false;
    return result;
}

void CIO_4W::setSerialReceived(bool txok) {
    _readyToTransmit = txok;
}

void CIO_4W::togglestates() {
    cio_states.bubbles = getAllowedstates(_currentStateIndex, BUBBLETOGGLE);
    cio_states.jets = getAllowedstates(_currentStateIndex, JETSTOGGLE);
    cio_states.pump = getAllowedstates(_currentStateIndex, PUMPTOGGLE);
    cio_states.heat = getAllowedstates(_currentStateIndex, HEATTOGGLE) > 0;
}

void CIO_4W::regulateTemp() {
    static uint8_t hysteresis = 0;

    if (!cio_states.heat) {
        cio_states.heatred = 0;
        return;
    }

    if ((cio_states.temperature + hysteresis) <= cio_states.target) {
        if (!cio_states.heatred) {
            _heat_bitmask = getHeatBitmask1();
            cio_states.heatred = 1;
            _heater2_countdown_ms = _HEATER2_DELAY_MS;
        }
        hysteresis = 0;
    } else {
        cio_states.heatred = 0;
        hysteresis = 1;
    }

    // Start 2nd heater element
    if ((_heater2_countdown_ms <= 0) && (cio_toggles.no_of_heater_elements_on == 2)) {
        _heat_bitmask = getHeatBitmask1() | getHeatBitmask2();
    }
}

void CIO_4W::antifreeze() {
    int tempC = cio_states.temperature;
    if (!cio_states.unit) tempC = F2C(tempC);

    if (tempC < 10) {
        if (!cio_states.pump) {
            _currentStateIndex = getJumptable(_currentStateIndex, PUMPTOGGLE);
            togglestates();
        }
        if (!cio_states.heat) {
            _currentStateIndex = getJumptable(_currentStateIndex, HEATTOGGLE);
            togglestates();
        }
        int targetC = 10;
        if (cio_states.unit) cio_states.target = targetC;
        else cio_states.target = C2F(targetC);
    }
}

void CIO_4W::antiboil() {
    int tempC = cio_states.temperature;
    if (!cio_states.unit) tempC = F2C(tempC);

    if (tempC > 41) {
        if (!cio_states.pump) {
            _currentStateIndex = getJumptable(_currentStateIndex, PUMPTOGGLE);
            togglestates();
        }
        if (cio_states.heat) {
            _currentStateIndex = getJumptable(_currentStateIndex, HEATTOGGLE);
            togglestates();
        }
    }
}

}  // namespace bestway_va
