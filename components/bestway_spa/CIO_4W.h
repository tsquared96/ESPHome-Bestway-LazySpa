/**
 * CIO 4-Wire Protocol Handler
 *
 * Adapted from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/cio/CIO_4W.h
 *
 * For models: 54123, 54138, 54144, 54154, 54173 (4-wire UART protocol)
 *
 * Original work Copyright (c) visualapproach
 * Licensed under GPL v3
 */

#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "enums.h"

namespace bestway_va {

class CIO_4W {
public:
    CIO_4W();
    virtual ~CIO_4W();

    void setup(int cio_rx, int cio_tx, int dummy);
    void stop();
    void pause_all(bool action);
    void handleToggles();
    void updateStates();

    bool getHasgod() { return true; }
    virtual bool getHasjets() = 0;
    virtual bool getHasair() = 0;
    bool getSerialReceived();
    void setSerialReceived(bool txok);

    sStates cio_states;
    sToggles cio_toggles;
    uint32_t good_packets_count = 0;
    uint32_t bad_packets_count = 0;
    int write_msg_count = 0;

    std::vector<uint8_t> _raw_payload_to_cio = {0, 0, 0, 0, 0, 0, 0};
    std::vector<uint8_t> _raw_payload_from_cio = {0, 0, 0, 0, 0, 0, 0};

protected:
    void regulateTemp();
    void antifreeze();
    void antiboil();
    void togglestates();
    void generatePayload();

    virtual uint8_t getPumpBitmask() = 0;
    virtual uint8_t getBubblesBitmask() = 0;
    virtual uint8_t getJetsBitmask() = 0;
    virtual uint8_t getHeatBitmask1() = 0;
    virtual uint8_t getHeatBitmask2() = 0;
    virtual uint8_t getPowerBitmask() = 0;
    virtual uint8_t getJumptable(int row, int col) = 0;
    virtual uint8_t getAllowedstates(int row, int col) = 0;

    // Helper functions
    int F2C(int f) { return (f - 32) * 5 / 9; }
    int C2F(int c) { return c * 9 / 5 + 32; }

private:
    uint64_t _prev_ms;
    int _time_since_last_transmission_ms = 0;
    const int _max_time_between_transmissions_ms = 2000;
    EspSoftwareSerial::UART* _cio_serial = nullptr;
    uint8_t _heat_bitmask = 0;
    uint8_t _from_CIO_buf[7] = {};
    uint8_t _to_CIO_buf[7] = {};
    uint8_t _currentStateIndex = 0;

    // Packet indices
    static const uint8_t TEMPINDEX = 2;
    static const uint8_t ERRORINDEX = 3;
    static const uint8_t CIO_CHECKSUMINDEX = 5;
    static const uint8_t COMMANDINDEX = 2;
    static const uint8_t DSP_CHECKSUMINDEX = 5;
    static const uint8_t PAYLOADSIZE = 7;

    // Timing constants
    static const uint32_t _HEATER2_DELAY_MS = 10000;
    static const uint32_t _HEATERCOOLING_DELAY_MS = 5000;
    int32_t _heater2_countdown_ms = 0;
    int32_t _cool_heater_countdown_ms = 0;
    bool _turn_off_pump_flag = false;
    bool _serialreceived = false;
    bool _readyToTransmit = false;
};

// Model-specific implementations

class CIO_54123 : public CIO_4W {
public:
    bool getHasjets() override { return false; }
    bool getHasair() override { return true; }
    uint8_t getPumpBitmask() override { return 0x10; }      // B00010000
    uint8_t getBubblesBitmask() override { return 0x20; }   // B00100000
    uint8_t getJetsBitmask() override { return 0x00; }
    uint8_t getHeatBitmask1() override { return 0x02; }     // B00000010
    uint8_t getHeatBitmask2() override { return 0x08; }     // B00001000
    uint8_t getPowerBitmask() override { return 0x01; }
    uint8_t getJumptable(int row, int col) override;
    uint8_t getAllowedstates(int row, int col) override;

private:
    static const uint8_t JUMPTABLE[4][4];
    static const uint8_t ALLOWEDSTATES[4][4];
};

class CIO_54138 : public CIO_4W {
public:
    bool getHasjets() override { return true; }
    bool getHasair() override { return true; }
    uint8_t getPumpBitmask() override { return 0x05; }      // B00000101
    uint8_t getBubblesBitmask() override { return 0x02; }   // B00000010
    uint8_t getJetsBitmask() override { return 0x08; }      // B00001000
    uint8_t getHeatBitmask1() override { return 0x30; }     // B00110000
    uint8_t getHeatBitmask2() override { return 0x40; }     // B01000000
    uint8_t getPowerBitmask() override { return 0x80; }     // B10000000
    uint8_t getJumptable(int row, int col) override;
    uint8_t getAllowedstates(int row, int col) override;

private:
    static const uint8_t JUMPTABLE[5][4];
    static const uint8_t ALLOWEDSTATES[5][4];
};

class CIO_54144 : public CIO_4W {
public:
    bool getHasjets() override { return true; }
    bool getHasair() override { return false; }
    uint8_t getPumpBitmask() override { return 0x05; }
    uint8_t getBubblesBitmask() override { return 0x02; }
    uint8_t getJetsBitmask() override { return 0x08; }
    uint8_t getHeatBitmask1() override { return 0x30; }
    uint8_t getHeatBitmask2() override { return 0x40; }
    uint8_t getPowerBitmask() override { return 0x80; }
    uint8_t getJumptable(int row, int col) override;
    uint8_t getAllowedstates(int row, int col) override;

private:
    static const uint8_t JUMPTABLE[5][4];
    static const uint8_t ALLOWEDSTATES[5][4];
};

class CIO_54154 : public CIO_4W {
public:
    bool getHasjets() override { return false; }
    bool getHasair() override { return true; }
    uint8_t getPumpBitmask() override { return 0x10; }
    uint8_t getBubblesBitmask() override { return 0x20; }
    uint8_t getJetsBitmask() override { return 0x00; }
    uint8_t getHeatBitmask1() override { return 0x02; }
    uint8_t getHeatBitmask2() override { return 0x08; }
    uint8_t getPowerBitmask() override { return 0x01; }
    uint8_t getJumptable(int row, int col) override;
    uint8_t getAllowedstates(int row, int col) override;

private:
    static const uint8_t JUMPTABLE[6][4];
    static const uint8_t ALLOWEDSTATES[6][4];
};

class CIO_54173 : public CIO_4W {
public:
    bool getHasjets() override { return true; }
    bool getHasair() override { return true; }
    uint8_t getPumpBitmask() override { return 0x05; }
    uint8_t getBubblesBitmask() override { return 0x02; }
    uint8_t getJetsBitmask() override { return 0x08; }
    uint8_t getHeatBitmask1() override { return 0x30; }
    uint8_t getHeatBitmask2() override { return 0x40; }
    uint8_t getPowerBitmask() override { return 0x80; }
    uint8_t getJumptable(int row, int col) override;
    uint8_t getAllowedstates(int row, int col) override;

private:
    static const uint8_t JUMPTABLE[8][4];
    static const uint8_t ALLOWEDSTATES[8][4];
};

}  // namespace bestway_va
