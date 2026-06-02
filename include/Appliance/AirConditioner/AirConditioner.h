#pragma once
#include <Arduino.h>
#include "Appliance/ApplianceBase.h"
#include "Appliance/AirConditioner/Capabilities.h"
#include "Appliance/AirConditioner/StatusData.h"
#include "Helpers/Helpers.h"

namespace dudanov {
namespace midea {
namespace ac {

// Air conditioner control command
struct Control {
  Optional<float> targetTemp{};
  Optional<Mode> mode{};
  Optional<Preset> preset{};
  Optional<FanMode> fanMode{};
  Optional<SwingMode> swingMode{};
};

class AirConditioner : public ApplianceBase {
 public:
  AirConditioner() : ApplianceBase(AIR_CONDITIONER) {}
  void m_setup() override;
  void m_onIdle() override { this->m_getStatus(); }
  void control(const Control &control);
  void setPowerState(bool state);
  bool getPowerState() const { return this->m_mode != Mode::MODE_OFF; }
  void togglePowerState() { this->setPowerState(this->m_mode == Mode::MODE_OFF); }
  float getTargetTemp() const { return this->m_targetTemp; }
  float getIndoorTemp() const { return this->m_indoorTemp; }
  float getOutdoorTemp() const { return this->m_outdoorTemp; }
  float getIndoorHum() const { return this->m_indoorHumidity; }
  float getPowerUsage() const { return this->m_powerUsage; }
  Mode getMode() const { return this->m_mode; }
  SwingMode getSwingMode() const { return this->m_swingMode; }
  FanMode getFanMode() const { return this->m_fanMode; }
  Preset getPreset() const { return this->m_preset; }
  const Capabilities &getCapabilities() const { return this->m_capabilities; }
  void displayToggle() { this->m_displayToggle(); }

  // --- New-protocol (0xB0) feature setters: Breezeless / indirect-wind ---
  // Property tags & encoding taken from the Midea LAN protocol implementation
  // (midea_ac_lan): breezeless tag=0x0018 (0x01/0x00), indirect-wind ("Breeze
  // Away") tag=0x0042 (0x02/0x01). Frame body = [0xB0, pack_count, tag_lo,
  // tag_hi, len, value...]; CRC8 appended by FrameData; sent as DEVICE_CONTROL.
  // BREEZE_CONTROL (tag 0x0043) enum: 1=OFF, 2=Breeze Away, 3=Breeze Mild, 4=Breezeless
  void setBreezeMode(uint8_t mode) {
    FrameData data{0xB0, 0x01, 0x43, 0x00, 0x01, mode};
    data.appendCRC();
    this->m_queueRequestPriority(FrameType::DEVICE_CONTROL, std::move(data),
        [](FrameData d) -> ResponseStatus { return d.hasID(0xB0) ? RESPONSE_OK : RESPONSE_WRONG; });
  }
  void setBreezeless(bool state) { this->setBreezeMode(state ? 4 : 1); }
  void setIndirectWind(bool state) {
    FrameData data{0xB0, 0x01, 0x42, 0x00, 0x01, static_cast<uint8_t>(state ? 0x02 : 0x01)};
    data.appendCRC();
    this->m_queueRequestPriority(FrameType::DEVICE_CONTROL, std::move(data),
        [](FrameData d) -> ResponseStatus { return d.hasID(0xB0) ? RESPONSE_OK : RESPONSE_WRONG; });
  }

 protected:
  void m_getPowerUsage();
  void m_getCapabilities();
  void m_getStatus();
  void m_setStatus(StatusData status);
  void m_displayToggle();
  ResponseStatus m_readStatus(FrameData data);
  Capabilities m_capabilities{};
  Timer m_powerUsageTimer;
  float m_indoorHumidity{};
  float m_indoorTemp{};
  float m_outdoorTemp{};
  float m_targetTemp{};
  float m_powerUsage{};
  Mode m_mode{Mode::MODE_OFF};
  Preset m_preset{Preset::PRESET_NONE};
  FanMode m_fanMode{FanMode::FAN_AUTO};
  SwingMode m_swingMode{SwingMode::SWING_OFF};
  Preset m_lastPreset{Preset::PRESET_NONE};
  StatusData m_status{};
  bool m_sendControl{};
};

}  // namespace ac
}  // namespace midea
}  // namespace dudanov
