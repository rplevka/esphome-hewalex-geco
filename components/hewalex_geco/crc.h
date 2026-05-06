#pragma once

#include <cstdint>
#include <cstddef>

namespace esphome {
namespace hewalex_geco {

uint8_t crc8_dvb_s2(uint8_t crc, const uint8_t *data, size_t len);
uint16_t crc16_ccitt(uint16_t crc, const uint8_t *data, size_t len);

}  // namespace hewalex_geco
}  // namespace esphome
