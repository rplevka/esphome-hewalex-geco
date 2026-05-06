#include "crc.h"

namespace esphome {
namespace hewalex_geco {

uint8_t crc8_dvb_s2(uint8_t crc, const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0xD5)
                         : static_cast<uint8_t>(crc << 1);
    }
  }
  return crc;
}

uint16_t crc16_ccitt(uint16_t crc, const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    uint8_t msb = (crc >> 8) & 0xFF;
    uint8_t lsb = crc & 0xFF;
    uint8_t x = data[i] ^ msb;
    x ^= (x >> 4);
    msb = static_cast<uint8_t>(lsb ^ (x >> 3) ^ (x << 4));
    lsb = static_cast<uint8_t>(x ^ (x << 5));
    crc = (static_cast<uint16_t>(msb) << 8) | lsb;
  }
  return crc;
}

}  // namespace hewalex_geco
}  // namespace esphome
