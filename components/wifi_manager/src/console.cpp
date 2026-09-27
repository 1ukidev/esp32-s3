#include <chrono>
#include <cstdio>
#include <string_view>

#include "esp_console.h"
#include "esp_log.h"

#include "wifi_manager/wifi_manager.hpp"

namespace wifi_manager {
namespace {

using namespace std::chrono_literals;

constexpr const char* TAG = "wifi_manager";
constexpr auto connect_timeout = 15s;

void print_status()
{
    auto& wifi = Wifi::instance();
    if (const auto info = wifi.info()) {
        std::printf("Wi-Fi: connected to \"%s\" (%d dBm), IP %s\n", info->ssid.c_str(), info->rssi, info->ip.c_str());
    } else if (const auto ssid = wifi.saved_ssid()) {
        std::printf("Wi-Fi: disconnected (saved network: \"%s\")\n", ssid->c_str());
    } else {
        std::printf("Wi-Fi: not configured\n");
    }
}

int report_connection(esp_err_t err)
{
    switch (err) {
    case ESP_OK:
        print_status();
        return 0;
    case ESP_ERR_NOT_FOUND:
        std::printf("No saved network. Use: wifi connect <ssid> [password]\n");
        return 1;
    case ESP_ERR_INVALID_ARG:
        std::printf("Invalid SSID or password (the password must have 8 to 63 characters).\n");
        return 1;
    case ESP_ERR_TIMEOUT:
        std::printf("Timed out. Check the network name and password.\n");
        return 1;
    default:
        std::printf("Connection failed: %s\n", esp_err_to_name(err));
        return 1;
    }
}

int wifi_command(int argc, char** argv)
{
    auto& wifi = Wifi::instance();
    const std::string_view action = argc > 1 ? argv[1] : "status";

    if (action == "connect" && argc == 2) {
        std::printf("Connecting to the saved network...\n");
        return report_connection(wifi.connect_saved(connect_timeout));
    }
    if (action == "connect" && (argc == 3 || argc == 4)) {
        const std::string_view password = argc == 4 ? argv[3] : "";
        std::printf("Connecting to \"%s\"...\n", argv[2]);
        return report_connection(wifi.connect(argv[2], password, connect_timeout));
    }
    if (action == "disconnect" && argc == 2) {
        wifi.disconnect();
        std::printf("Wi-Fi disconnected.\n");
        return 0;
    }
    if (action == "status" && argc <= 2) {
        print_status();
        return 0;
    }

    std::printf("Usage: wifi <connect [ssid] [password] | disconnect | status>\n");
    return 1;
}

}  // namespace

void register_commands(bool auto_connect)
{
    const esp_console_cmd_t command = {
        .command = "wifi",
        .help = "Manages the Wi-Fi connection (without ssid, joins the saved network)",
        .hint = "<connect [ssid] [password] | disconnect | status>",
        .func = &wifi_command,
        .argtable = nullptr,
        .func_w_context = nullptr,
        .context = nullptr,
    };
    if (const esp_err_t err = esp_console_cmd_register(&command); err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register the 'wifi' command: %s", esp_err_to_name(err));
    }

    if (auto_connect && Wifi::instance().connect_in_background() == ESP_OK) {
        ESP_LOGI(TAG, "Connecting to the saved network in the background");
    }
}

}  // namespace wifi_manager
