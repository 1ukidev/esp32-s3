#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "camera_server/camera_server.hpp"
#include "chip_temperature/chip_temperature.hpp"
#include "wifi_manager/wifi_manager.hpp"

constexpr const char* TAG = "MAIN";

static int cmd_info(int argc, char** argv)
{
    std::printf("ESP32-S3 N16R8\n");
    std::printf("Reset reason: %d\n", esp_reset_reason());
    std::printf("Free heap: %lu bytes\n", esp_get_free_heap_size());

    return 0;
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Initializing system");

    esp_console_repl_config_t repl_config =
        ESP_CONSOLE_REPL_CONFIG_DEFAULT();

    repl_config.prompt = "esp32> ";
    repl_config.task_stack_size = 8192;

    const esp_console_cmd_t info_cmd = {
        .command = "info",
        .help = "Shows system information",
        .hint = nullptr,
        .func = &cmd_info,
        .argtable = nullptr,
        .func_w_context = nullptr,
        .context = nullptr
    };

    ESP_ERROR_CHECK(
        esp_console_cmd_register(&info_cmd)
    );

    wifi_manager::register_commands();
    camera_server::register_commands({.name = "Camera Server"});
    chip_temperature::register_commands();

    ESP_ERROR_CHECK(
        esp_console_register_help_command()
    );

    esp_console_repl_t* repl = nullptr;

    ESP_ERROR_CHECK(
        esp_console_new_repl_stdio(
            &repl_config,
            &repl
        )
    );

    ESP_ERROR_CHECK(
        esp_console_start_repl(repl)
    );

    ESP_LOGI(TAG, "Shell started");
}
