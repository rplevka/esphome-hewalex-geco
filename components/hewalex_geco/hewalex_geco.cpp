#include "hewalex_geco.h"
#include "crc.h"
#include "sensor/hewalex_geco_sensor.h"

#include "esphome/core/log.h"

namespace esphome {
namespace hewalex_geco {

static const char *const TAG = "hewalex_geco";
static const uint32_t RESPONSE_TIMEOUT_MS = 500;

void HewalexGecoHub::setup() {
  if (flow_control_pin_ != nullptr) {
    flow_control_pin_->setup();
    flow_control_pin_->digital_write(false);
  }
  rx_buffer_.reserve(256);
}

void HewalexGecoHub::dump_config() {
  ESP_LOGCONFIG(TAG, "Hewalex Geco Hub:");
  ESP_LOGCONFIG(TAG, "  Target address: %u", target_addr_);
  ESP_LOGCONFIG(TAG, "  Source address: %u", source_addr_);
  ESP_LOGCONFIG(TAG, "  Function code:  0x%02X", function_code_);
  ESP_LOGCONFIG(TAG, "  Register ranges: %u", static_cast<unsigned>(ranges_.size()));
  for (auto &r : ranges_) {
    ESP_LOGCONFIG(TAG, "    %u..%u (%u bytes)", r.start, r.start + r.count - 1, r.count);
  }
  ESP_LOGCONFIG(TAG, "  Sensors registered: %u", static_cast<unsigned>(sensors_.size()));
  LOG_PIN("  Flow control pin: ", flow_control_pin_);
  this->check_uart_settings(38400);
}

void HewalexGecoHub::update() {
  if (state_ != State::IDLE) {
    ESP_LOGW(TAG, "Previous poll cycle still running, skipping");
    return;
  }
  if (ranges_.empty()) {
    ESP_LOGW(TAG, "No register ranges configured");
    return;
  }
  current_range_idx_ = 0;
  send_request_(ranges_[0].start, ranges_[0].count);
}

void HewalexGecoHub::send_request_(uint16_t start, uint8_t count) {
  uint8_t buf[20];

  // Hard header
  buf[0] = 0x69;
  buf[1] = target_addr_;
  buf[2] = source_addr_;
  buf[3] = 0x84;
  buf[4] = 0x00;
  buf[5] = 0x00;
  buf[6] = 0x0C;  // soft frame length
  buf[7] = crc8_dvb_s2(0, buf, 7);

  // Soft frame
  buf[8] = target_addr_;
  buf[9] = 0x00;
  buf[10] = source_addr_;
  buf[11] = 0x00;
  buf[12] = function_code_;  // 0x40 read
  buf[13] = 0x80;
  buf[14] = 0x00;
  buf[15] = count;
  buf[16] = static_cast<uint8_t>(start & 0xFF);
  buf[17] = static_cast<uint8_t>((start >> 8) & 0xFF);
  uint16_t pcrc = crc16_ccitt(0, buf + 8, 10);
  buf[18] = static_cast<uint8_t>((pcrc >> 8) & 0xFF);
  buf[19] = static_cast<uint8_t>(pcrc & 0xFF);

  // Drain stale RX
  while (this->available()) {
    uint8_t junk;
    this->read_byte(&junk);
  }
  rx_buffer_.clear();

  if (flow_control_pin_ != nullptr) flow_control_pin_->digital_write(true);
  this->write_array(buf, sizeof(buf));
  this->flush();
  if (flow_control_pin_ != nullptr) flow_control_pin_->digital_write(false);

  // Response = 8 (hard hdr) + 10 (soft hdr) + count (data) + 2 (payload CRC)
  expected_response_len_ = static_cast<uint16_t>(20 + count);
  current_start_reg_ = start;
  state_ = State::AWAITING_RESPONSE;
  request_started_ms_ = millis();

  ESP_LOGD(TAG, "Sent request: start=%u count=%u (expecting %u bytes)", start, count,
           expected_response_len_);
}

void HewalexGecoHub::loop() {
  if (state_ != State::AWAITING_RESPONSE) return;

  while (this->available()) {
    uint8_t b;
    if (!this->read_byte(&b)) break;
    if (rx_buffer_.empty() && b != 0x69) continue;  // hunt for frame start
    rx_buffer_.push_back(b);
    if (rx_buffer_.size() >= expected_response_len_) {
      if (validate_response_(rx_buffer_.data(), rx_buffer_.size())) {
        parse_response_(rx_buffer_.data(), rx_buffer_.size());
      } else {
        ESP_LOGW(TAG, "Invalid response frame for start=%u", current_start_reg_);
      }
      advance_or_idle_();
      return;
    }
  }

  if (millis() - request_started_ms_ > RESPONSE_TIMEOUT_MS) {
    ESP_LOGW(TAG, "Timeout waiting for response (start=%u, got %u/%u bytes)", current_start_reg_,
             static_cast<unsigned>(rx_buffer_.size()), expected_response_len_);
    advance_or_idle_();
  }
}

void HewalexGecoHub::advance_or_idle_() {
  current_range_idx_++;
  if (current_range_idx_ >= ranges_.size()) {
    state_ = State::IDLE;
    return;
  }
  auto &r = ranges_[current_range_idx_];
  send_request_(r.start, r.count);
}

bool HewalexGecoHub::validate_response_(const uint8_t *buf, size_t len) const {
  if (len < 8) return false;
  if (buf[0] != 0x69) return false;

  uint8_t hdr_crc = crc8_dvb_s2(0, buf, 7);
  if (hdr_crc != buf[7]) {
    ESP_LOGW(TAG, "Header CRC mismatch: expected 0x%02X, got 0x%02X", buf[7], hdr_crc);
    return false;
  }

  uint8_t payload_len = buf[6];
  if (len != static_cast<size_t>(8 + payload_len)) {
    ESP_LOGW(TAG, "Length mismatch: expected %u, got %u", 8 + payload_len,
             static_cast<unsigned>(len));
    return false;
  }

  uint16_t got = (static_cast<uint16_t>(buf[len - 2]) << 8) | buf[len - 1];
  uint16_t want = crc16_ccitt(0, buf + 8, payload_len - 2);
  if (got != want) {
    ESP_LOGW(TAG, "Payload CRC mismatch: expected 0x%04X, got 0x%04X", got, want);
    return false;
  }
  return true;
}

void HewalexGecoHub::parse_response_(const uint8_t *buf, size_t len) {
  if (len < 18) return;
  uint8_t data_len = buf[15];
  uint16_t start_reg = static_cast<uint16_t>(buf[16]) | (static_cast<uint16_t>(buf[17]) << 8);
  const uint8_t *data = buf + 18;

  ESP_LOGD(TAG, "Response: start=%u data_len=%u", start_reg, data_len);

  for (auto *s : sensors_) {
    uint16_t reg = s->get_register();
    if (reg < start_reg) continue;
    uint16_t off = reg - start_reg;
    if (off + 1u >= data_len) continue;
    // Wire layout per Python source: high byte at data[off+1], low byte at data[off]
    uint16_t raw = (static_cast<uint16_t>(data[off + 1]) << 8) | data[off];
    s->publish_decoded(raw);
  }
}

}  // namespace hewalex_geco
}  // namespace esphome
