#include "wifi_manager/wifi_manager.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

namespace wifi_manager {
namespace {

constexpr const char* TAG = "wifi";
constexpr EventBits_t connected_bit = BIT0;
constexpr std::size_t max_ssid_length = sizeof(wifi_sta_config_t::ssid);
constexpr std::size_t max_password_length = sizeof(wifi_sta_config_t::password) - 1;

esp_err_t init_nvs()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "Failed to erase NVS");
        err = nvs_flash_init();
    }
    return err;
}

}  // namespace

Wifi& Wifi::instance()
{
    static Wifi wifi;
    return wifi;
}

esp_err_t Wifi::ensure_started()
{
    if (started_) {
        return ESP_OK;
    }

    if (events_ == nullptr && (events_ = xEventGroupCreate()) == nullptr) {
        return ESP_ERR_NO_MEM;
    }
    ESP_RETURN_ON_ERROR(init_nvs(), TAG, "Failed to initialize NVS");
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Failed to initialize netif");
    if (const esp_err_t err = esp_event_loop_create_default(); err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    netif_ = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(netif_, "esp32-s3");

    const wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init_config), TAG, "Failed to initialize Wi-Fi");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_event, this, nullptr),
                        TAG, "Failed to register event handler");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_event, this, nullptr),
                        TAG, "Failed to register event handler");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Failed to set Wi-Fi mode");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start Wi-Fi");
    esp_wifi_set_ps(WIFI_PS_NONE);

    started_ = true;
    return ESP_OK;
}

void Wifi::on_event(void* arg, esp_event_base_t base, std::int32_t id, void* data)
{
    auto& self = *static_cast<Wifi*>(arg);
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(self.events_, connected_bit);
        if (self.auto_reconnect_) {
            esp_wifi_connect();
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        const auto* event = static_cast<const ip_event_got_ip_t*>(data);
        ESP_LOGI(TAG, "Connected, IP " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(self.events_, connected_bit);
    }
}

esp_err_t Wifi::join(std::chrono::milliseconds timeout)
{
    auto_reconnect_ = true;
    if (const esp_err_t err = esp_wifi_connect(); err != ESP_OK) {
        auto_reconnect_ = false;
        return err;
    }

    const auto ticks = pdMS_TO_TICKS(timeout.count());
    if (xEventGroupWaitBits(events_, connected_bit, pdFALSE, pdTRUE, ticks) & connected_bit) {
        return ESP_OK;
    }

    auto_reconnect_ = false;
    esp_wifi_disconnect();
    return ESP_ERR_TIMEOUT;
}

esp_err_t Wifi::connect(std::string_view ssid, std::string_view password, std::chrono::milliseconds timeout)
{
    if (ssid.empty() || ssid.size() > max_ssid_length || password.size() > max_password_length ||
        (!password.empty() && password.size() < 8)) {
        return ESP_ERR_INVALID_ARG;
    }

    std::scoped_lock lock(mutex_);
    ESP_RETURN_ON_ERROR(ensure_started(), TAG, "Wi-Fi unavailable");

    auto_reconnect_ = false;
    esp_wifi_disconnect();
    xEventGroupClearBits(events_, connected_bit);

    wifi_config_t config{};
    std::ranges::copy(ssid, config.sta.ssid);
    std::ranges::copy(password, config.sta.password);
    config.sta.threshold.authmode = password.empty() ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &config), TAG, "Invalid configuration");

    return join(timeout);
}

esp_err_t Wifi::connect_saved(std::chrono::milliseconds timeout)
{
    if (!saved_ssid()) {
        return ESP_ERR_NOT_FOUND;
    }

    std::scoped_lock lock(mutex_);
    if (connected()) {
        return ESP_OK;
    }
    return join(timeout);
}

esp_err_t Wifi::connect_in_background()
{
    if (!saved_ssid()) {
        return ESP_ERR_NOT_FOUND;
    }

    std::scoped_lock lock(mutex_);
    if (connected()) {
        return ESP_OK;
    }
    auto_reconnect_ = true;
    return esp_wifi_connect();
}

esp_err_t Wifi::disconnect()
{
    std::scoped_lock lock(mutex_);
    if (!started_) {
        return ESP_ERR_INVALID_STATE;
    }
    auto_reconnect_ = false;
    return esp_wifi_disconnect();
}

bool Wifi::connected() const
{
    return started_ && (xEventGroupGetBits(events_) & connected_bit) != 0;
}

std::optional<Wifi::Info> Wifi::info() const
{
    wifi_ap_record_t ap{};
    esp_netif_ip_info_t ip{};
    if (!connected() || esp_wifi_sta_get_ap_info(&ap) != ESP_OK || esp_netif_get_ip_info(netif_, &ip) != ESP_OK) {
        return std::nullopt;
    }

    char address[16];
    std::snprintf(address, sizeof(address), IPSTR, IP2STR(&ip.ip));
    return Info{
        .ssid = reinterpret_cast<const char*>(ap.ssid),
        .ip = address,
        .rssi = ap.rssi,
    };
}

std::optional<std::string> Wifi::saved_ssid()
{
    std::scoped_lock lock(mutex_);
    wifi_config_t config{};
    if (ensure_started() != ESP_OK || esp_wifi_get_config(WIFI_IF_STA, &config) != ESP_OK || config.sta.ssid[0] == 0) {
        return std::nullopt;
    }

    const auto* ssid = reinterpret_cast<const char*>(config.sta.ssid);
    return std::string{ssid, strnlen(ssid, max_ssid_length)};
}

}  // namespace wifi_manager
