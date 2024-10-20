#include "respeakerlite.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace respeakerlite {

static const char* const TAG = "respeaker_lite";

float RespeakerLite::get_setup_priority() const {
  return setup_priority::IO;
}

void RespeakerLite::setup() {
  ESP_LOGI(TAG, "Setting up RespeakerLite...");

  uint8_t command[3] = {0xF1, 0x81, 0x03};
  this->write(command, 3);

  uint8_t data[4];
  if (this->read(data, 4) !=i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Unable to read firmware version");
    this->mark_failed();
    return;
  }
  std::string firmware = std::to_string(arr[0]) + "." + std::to_string(arr[1]) + "." + std::to_string(arr[2]);
  ESP_LOGI(TAG, "Firmware version: %s", firmware);
}

void RespeakerLite::loop() {
  uint8_t command[3] = {0xF1, 0x81, 0x01};
  this->write(command, 3);

  uint8_t data[2];
  if (this->read(data, 2) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Unable to read mute state");
    return;
  }
  bool new_mute_state = data[1] == 0x01;
  if (this->mute_state_ != nullptr) {
    if (!this->mute_state_->has_state() || (this->mute_state_->state != new_mute_state)) {
      ESP_LOGD(TAG, "Mic mute state: %d", new_mute_state);
      this->mute_state_->publish_state(new_mute_state);
    }
  }
}

}  // namespace respeakerlite
}  // namespace esphome