#include "hewalex_geco_sensor.h"

#include "esphome/core/log.h"

namespace esphome {
namespace hewalex_geco {

static const char *const TAG = "hewalex_geco.sensor";

void HewalexGecoSensor::publish_decoded(uint16_t raw) {
  float v;
  if (decode_ == DECODE_S16) {
    int16_t s = static_cast<int16_t>(raw);
    v = static_cast<float>(s);
  } else {
    v = static_cast<float>(raw);
  }
  if (divisor_ != 1.0f) v /= divisor_;
  this->publish_state(v);
}

void HewalexGecoSensor::dump_config() {
  LOG_SENSOR("", "Hewalex Geco Sensor", this);
  ESP_LOGCONFIG(TAG, "  Register: %u", register_);
  ESP_LOGCONFIG(TAG, "  Decode: %s", decode_ == DECODE_S16 ? "signed" : "unsigned");
  ESP_LOGCONFIG(TAG, "  Divisor: %.2f", divisor_);
}

}  // namespace hewalex_geco
}  // namespace esphome
