#pragma once

#include <optional>
#include <string>

namespace camera_server {

struct NetworkInfo {
    std::string ip;
    std::string ssid;
    int rssi = 0;
};

// Reads the default network interface, whoever brought it up.
std::optional<NetworkInfo> network_info();

}  // namespace camera_server
