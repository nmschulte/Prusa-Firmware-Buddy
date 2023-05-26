#pragma once
#include <array>
#include <vector>
#include <stdint.h>
#include <span>
#include "common/storage.hpp"

class DummyEepromChip : public Storage {
    std::array<uint8_t, 8096> data;

public:
    DummyEepromChip() {
        data.fill(0xff);
    }
    void set(uint16_t address, const uint8_t *pdata, uint16_t len);
    void set(uint16_t address, const uint8_t pdata);
    void get(uint16_t address, uint8_t *pdata, uint16_t len);
    uint8_t get(uint16_t address);
    std::span<uint8_t> get(uint16_t address, std::size_t size);
    void check_data(uint16_t address, std::vector<uint8_t> data);
    void check_data(uint16_t address, const uint8_t *data, std::size_t len);
    void clear();
    bool is_clear();
    uint8_t read_byte(uint16_t address) override;
    void read_bytes(uint16_t address, std::span<uint8_t> buffer) override;
    void write_byte(uint16_t address, uint8_t data) override;
    void write_bytes(uint16_t address, std::span<const uint8_t> data) override;
};
extern DummyEepromChip eeprom_chip;