#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

namespace wifi_manager {

class Wifi {
public:
    struct Info {
        std::string ssid;
        std::string ip;
        int rssi = 0;
    };

    static Wifi& instance();

    esp_err_t connect(std::string_view ssid, std::string_view password,
                      std::chrono::milliseconds timeout);
    esp_err_t connect_saved(std::chrono::milliseconds timeout);
    esp_err_t connect_in_background();
    esp_err_t disconnect();

    [[nodiscard]] bool connected() const;
    [[nodiscard]] std::optional<Info> info() const;
    [[nodiscard]] std::optional<std::string> saved_ssid();

private:
    Wifi() = default;

    esp_err_t ensure_started();
    esp_err_t join(std::chrono::milliseconds timeout);
    static void on_event(void* self, esp_event_base_t base, std::int32_t id, void* data);

    std::mutex mutex_;
    EventGroupHandle_t events_ = nullptr;
    esp_netif_t* netif_ = nullptr;
    std::atomic<bool> started_{false};
    std::atomic<bool> auto_reconnect_{false};
};

// Registers the "wifi" command in the ESP-IDF console. With auto_connect, the
// saved network (if any) is joined in the background right away.
void register_commands(bool auto_connect = true);

}  // namespace wifi_manager
