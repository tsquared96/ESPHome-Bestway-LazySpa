/**
 * DSP 4-Wire Protocol Handler
 *
 * Adapted from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
 * https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
 * Original file: Code/lib/dsp/DSP_4W.h
 *
 * For models: 54123, 54138, 54144, 54154, 54173 (4-wire UART protocol)
 *
 * DSP handles communication with the physical display panel
 *
 * Original work Copyright (c) visualapproach
 * Licensed under GPL v3
 */

#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "enums.h"

namespace bestway_va {

class DSP_4W {
public:
    DSP_4W();
    virtual ~DSP_4W();

    void setup(int dsp_tx, int dsp_rx, int dummy, int dummy2);
    void stop();
    void pause_all(bool action);
    void updateToggles();
    void handleStates();

    bool getSerialReceived();
    void setSerialReceived(bool txok);

    virtual bool getHasjets() = 0;
    virtual bool getHasair() = 0;

    sToggles dsp_toggles;
    sStates dsp_states;
    String text = "";
    int audiofrequency = 0;
    uint32_t good_packets_count = 0;
    uint32_t bad_packets_count = 0;
    int write_msg_count = 0;
    int max_time_between_transmissions_ms = -1;

    std::vector<uint8_t> _raw_payload_to_dsp = {0, 0, 0, 0, 0, 0, 0};
    std::vector<uint8_t> _raw_payload_from_dsp = {0, 0, 0, 0, 0, 0, 0};

    bool EnabledButtons[11] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    EspSoftwareSerial::UART* _dsp_serial = nullptr;

protected:
    virtual uint8_t getPumpBitmask() = 0;
    virtual uint8_t getBubblesBitmask() = 0;
    virtual uint8_t getJetsBitmask() = 0;
    virtual uint8_t getHeatBitmask1() = 0;
    virtual uint8_t getHeatBitmask2() = 0;
    virtual uint8_t getPowerBitmask() = 0;
    virtual uint8_t getJumptable(int row, int col) = 0;
    virtual uint8_t getAllowedstates(int row, int col) = 0;

    void generatePayload();

    // Helper functions
    int F2C(int f) { return (f - 32) * 5 / 9; }
    int C2F(int c) { return c * 9 / 5 + 32; }

private:
    int _time_since_last_transmission_ms = 0;
    const int _max_allowed_time_between_transmissions_ms = 2000;

    uint8_t _to_DSP_buf[7] = {};
    uint8_t _from_DSP_buf[7] = {};

    static const uint8_t COMMANDINDEX = 2;
    static const uint8_t DSP_CHECKSUMINDEX = 5;
    static const uint8_t PAYLOADSIZE = 7;

    uint8_t _bubbles, _pump, _jets;
    bool _serialreceived = false;
    bool _readyToTransmit = false;
};

// Model-specific implementations

class DSP_54123 : public DSP_4W {
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
    static const uint8_t JUMPTABLE[4][4];
    static const uint8_t ALLOWEDSTATES[4][4];
};

class DSP_54138 : public DSP_4W {
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
    static const uint8_t JUMPTABLE[5][4];
    static const uint8_t ALLOWEDSTATES[5][4];
};

class DSP_54144 : public DSP_4W {
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

class DSP_54154 : public DSP_4W {
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

class DSP_54173 : public DSP_4W {
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
