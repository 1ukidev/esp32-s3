#pragma once

#include <optional>

namespace chip_temperature {

// Die temperature of the ESP32 itself, not of any attached peripheral.
std::optional<float> read_celsius();

// Registers the "temp" command in the ESP-IDF console.
void register_commands();

}  // namespace chip_temperature
