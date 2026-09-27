#include "network.hpp"

#include <cstdio>
#include <cstring>

#include "esp_netif.h"
#include "esp_wifi.h"

namespace camera_server {

std::optional<NetworkInfo> network_info()
{
    esp_netif_t* netif = esp_netif_get_default_netif();
    esp_netif_ip_info_t ip{};
    if (netif == nullptr || esp_netif_get_ip_info(netif, &ip) != ESP_OK || ip.ip.addr == 0) {
        return std::nullopt;
    }

    char address[16];
    std::snprintf(address, sizeof(address), IPSTR, IP2STR(&ip.ip));
    NetworkInfo info{.ip = address, .ssid = {}, .rssi = 0};

    if (wifi_ap_record_t ap{}; esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        const auto* ssid = reinterpret_cast<const char*>(ap.ssid);
        info.ssid.assign(ssid, strnlen(ssid, sizeof(ap.ssid)));
        info.rssi = ap.rssi;
    }
    return info;
}

}  // namespace camera_server
