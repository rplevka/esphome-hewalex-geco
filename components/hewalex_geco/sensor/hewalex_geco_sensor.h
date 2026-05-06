#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

#include <cstdint>

namespace esphome {
namespace hewalex_geco {

enum DecodeType : uint8_t {
  DECODE_U16 = 0,
  DECODE_S16 = 1,
};

class HewalexGecoSensor : public sensor::Sensor, public Component {
 public:
  void dump_config() override;

  void set_register(uint16_t r) { register_ = r; }
  void set_decode(DecodeType d) { decode_ = d; }
  void set_divisor(float d) { divisor_ = d; }
  uint16_t get_register() const { return register_; }

  void publish_decoded(uint16_t raw);

 protected:
  uint16_t register_{0};
  DecodeType decode_{DECODE_U16};
  float divisor_{1.0f};
};

}  // namespace hewalex_geco
}  // namespace esphome
