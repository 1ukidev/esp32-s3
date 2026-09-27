#include "chip_temperature/chip_temperature.hpp"

#include <cstdio>

#include "driver/temperature_sensor.h"
#include "esp_console.h"
#include "esp_log.h"

namespace chip_temperature {
namespace {

constexpr const char* TAG = "chip_temperature";

temperature_sensor_handle_t sensor()
{
    static const temperature_sensor_handle_t handle = [] {
        const temperature_sensor_config_t config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
        temperature_sensor_handle_t created = nullptr;
        if (temperature_sensor_install(&config, &created) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to install the temperature sensor");
            return static_cast<temperature_sensor_handle_t>(nullptr);
        }
        if (temperature_sensor_enable(created) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to enable the temperature sensor");
            temperature_sensor_uninstall(created);
            return static_cast<temperature_sensor_handle_t>(nullptr);
        }
        return created;
    }();
    return handle;
}

int temp_command(int, char**)
{
    const auto celsius = read_celsius();
    if (!celsius) {
        std::printf("Chip temperature unavailable.\n");
        return 1;
    }
    std::printf("Chip temperature: %.1f C\n", static_cast<double>(*celsius));
    return 0;
}

}  // namespace

std::optional<float> read_celsius()
{
    float celsius = 0;
    const auto handle = sensor();
    if (handle == nullptr || temperature_sensor_get_celsius(handle, &celsius) != ESP_OK) {
        return std::nullopt;
    }
    return celsius;
}

void register_commands()
{
    const esp_console_cmd_t command = {
        .command = "temp",
        .help = "Shows the chip temperature",
        .hint = nullptr,
        .func = &temp_command,
        .argtable = nullptr,
        .func_w_context = nullptr,
        .context = nullptr,
    };
    if (const esp_err_t err = esp_console_cmd_register(&command); err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register the 'temp' command: %s", esp_err_to_name(err));
    }
}

}  // namespace chip_temperature
