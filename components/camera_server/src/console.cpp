#include <array>
#include <charconv>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>

#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include "camera_server/camera_server.hpp"
#include "network.hpp"

namespace camera_server {
namespace {

constexpr const char* TAG = "camera_server";

std::optional<std::uint16_t> parse_port(std::string_view text)
{
    std::uint16_t port = 0;
    const auto [end, ec] = std::from_chars(text.begin(), text.end(), port);
    if (ec != std::errc{} || end != text.end() || port == 0) {
        return std::nullopt;
    }
    return port;
}

std::string format_bytes(double bytes)
{
    constexpr std::array units{"B", "KB", "MB", "GB"};
    std::size_t unit = 0;
    while (bytes >= 1024 && unit + 1 < units.size()) {
        bytes /= 1024;
        ++unit;
    }
    char text[24];
    std::snprintf(text, sizeof(text), unit == 0 ? "%.0f %s" : "%.1f %s", bytes, units[unit]);
    return text;
}

std::string format_duration(std::uint32_t seconds)
{
    char text[24];
    std::snprintf(text, sizeof(text), "%02lu:%02lu:%02lu", static_cast<unsigned long>(seconds / 3600),
                  static_cast<unsigned long>(seconds / 60 % 60), static_cast<unsigned long>(seconds % 60));
    return text;
}

std::string server_url(std::uint16_t port)
{
    const auto network = network_info();
    std::string url = "http://" + (network ? network->ip : std::string{"<ip>"});
    if (port != 80) {
        url += ':' + std::to_string(port);
    }
    return url + '/';
}

void print_network_status()
{
    if (const auto network = network_info(); !network) {
        std::printf("Network     : not connected\n");
    } else if (network->ssid.empty()) {
        std::printf("Network     : IP %s\n", network->ip.c_str());
    } else {
        std::printf("Network     : IP %s via \"%s\" (%d dBm)\n", network->ip.c_str(), network->ssid.c_str(),
                    network->rssi);
    }
}

int camera_start(std::optional<std::uint16_t> port)
{
    switch (const esp_err_t err = start(port)) {
    case ESP_OK:
        std::printf("Camera server started at %s\n", server_url(status().port).c_str());
        if (!network_info()) {
            std::printf("Warning: no network connection; the server will be reachable once one is available.\n");
        }
        return 0;
    case ESP_ERR_INVALID_STATE:
        std::printf("The camera server is already running.\n");
        return 1;
    case ESP_ERR_CAMERA_NOT_DETECTED:
        std::printf("Error: camera not detected. Check the module connection.\n");
        return 1;
    default:
        std::printf("Failed to start the camera server: %s\n", esp_err_to_name(err));
        return 1;
    }
}

int camera_stop()
{
    if (stop() != ESP_OK) {
        std::printf("The camera server is not running.\n");
        return 1;
    }
    std::printf("Camera server stopped.\n");
    return 0;
}

int camera_status()
{
    const auto s = status();
    if (!s.running) {
        std::printf("Server      : stopped\n");
    } else {
        std::printf("Server      : running\n");
        std::printf("Address     : %s\n", server_url(s.port).c_str());
        std::printf("Sensor      : %.*s\n", static_cast<int>(s.sensor.size()), s.sensor.data());
        std::printf("Resolution  : %dx%d\n", s.width, s.height);
        std::printf("Uptime      : %s\n", format_duration(s.uptime_s).c_str());
        std::printf("Streams     : %u/%u (%lu fps)\n", static_cast<unsigned>(s.streams),
                    static_cast<unsigned>(s.max_streams), static_cast<unsigned long>(s.fps));
        std::printf("Frames      : %llu (%s)\n", static_cast<unsigned long long>(s.frames),
                    format_bytes(static_cast<double>(s.bytes)).c_str());
    }
    print_network_status();
    std::printf("Memory      : %s free (internal), %s free (PSRAM)\n",
                format_bytes(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)).c_str(),
                format_bytes(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)).c_str());
    return 0;
}

int camera_command(int argc, char** argv)
{
    const std::string_view action = argc > 1 ? argv[1] : "status";

    if (action == "start" && argc <= 3) {
        std::optional<std::uint16_t> port;
        if (argc == 3 && !(port = parse_port(argv[2]))) {
            std::printf("Invalid port: %s\n", argv[2]);
            return 1;
        }
        return camera_start(port);
    }
    if (action == "stop" && argc == 2) {
        return camera_stop();
    }
    if (action == "status" && argc <= 2) {
        return camera_status();
    }

    std::printf("Usage: camera <start [port] | stop | status>\n");
    return 1;
}

}  // namespace

void register_commands(const Config& config)
{
    configure(config);

    const esp_console_cmd_t camera_cmd = {
        .command = "camera",
        .help = "Controls the camera web server",
        .hint = "<start [port] | stop | status>",
        .func = &camera_command,
        .argtable = nullptr,
        .func_w_context = nullptr,
        .context = nullptr,
    };
    if (const esp_err_t err = esp_console_cmd_register(&camera_cmd); err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register the 'camera' command: %s", esp_err_to_name(err));
    }
}

}  // namespace camera_server
