#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/uart/uart.h"

#include <cstdint>
#include <vector>

namespace esphome {
namespace hewalex_geco {

class HewalexGecoSensor;

struct RegisterRange {
  uint16_t start;
  uint8_t count;
};

class HewalexGecoHub : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  void set_flow_control_pin(GPIOPin *pin) { flow_control_pin_ = pin; }
  void set_target_address(uint8_t a) { target_addr_ = a; }
  void set_source_address(uint8_t a) { source_addr_ = a; }
  void set_function_code(uint8_t fc) { function_code_ = fc; }
  void add_register_range(uint16_t start, uint8_t count) { ranges_.push_back({start, count}); }
  void add_sensor(HewalexGecoSensor *s) { sensors_.push_back(s); }

 protected:
  enum class State { IDLE, AWAITING_RESPONSE };

  void send_request_(uint16_t start, uint8_t count);
  bool validate_response_(const uint8_t *buf, size_t len) const;
  void parse_response_(const uint8_t *buf, size_t len);
  void advance_or_idle_();

  GPIOPin *flow_control_pin_{nullptr};
  uint8_t target_addr_{2};
  uint8_t source_addr_{1};
  uint8_t function_code_{0x40};
  std::vector<RegisterRange> ranges_;
  std::vector<HewalexGecoSensor *> sensors_;

  State state_{State::IDLE};
  size_t current_range_idx_{0};
  uint16_t current_start_reg_{0};
  uint16_t expected_response_len_{0};
  uint32_t request_started_ms_{0};
  std::vector<uint8_t> rx_buffer_;
};

}  // namespace hewalex_geco
}  // namespace esphome
