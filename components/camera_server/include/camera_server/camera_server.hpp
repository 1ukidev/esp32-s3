#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "esp_camera.h"
#include "esp_err.h"

namespace camera_server {

// Default pinout: ESP32-S3-EYE compatible camera connector.
struct CameraPins {
    int pwdn = -1;
    int reset = -1;
    int xclk = 15;
    int sccb_sda = 4;
    int sccb_scl = 5;
    int d0 = 11;
    int d1 = 9;
    int d2 = 8;
    int d3 = 10;
    int d4 = 12;
    int d5 = 18;
    int d6 = 17;
    int d7 = 16;
    int vsync = 6;
    int href = 7;
    int pclk = 13;
};

struct Config {
    // Shown in the web interface title and used to name downloaded photos.
    std::string name = "ESP32 Camera";
    CameraPins pins{};
    int xclk_freq_hz = 20'000'000;
    // Frame buffers are sized for this resolution, so it also caps what the UI offers.
    framesize_t max_frame_size = FRAMESIZE_UXGA;
    framesize_t initial_frame_size = FRAMESIZE_VGA;
    int jpeg_quality = 10;
    // A third buffer lets the sensor keep capturing while two frames are being sent.
    std::size_t frame_buffers = 3;
    bool vertical_flip = true;
    std::uint16_t port = 80;
    std::size_t max_streams = 3;
    // Endpoint of an external YOLO API, e.g. "http://192.168.0.10:8000/detect".
    // A URL saved later from the web interface or the console takes precedence.
    std::string detector_url{};
};

struct Status {
    bool running = false;
    std::uint16_t port = 0;
    std::uint32_t uptime_s = 0;
    std::size_t streams = 0;
    std::size_t max_streams = 0;
    std::uint32_t fps = 0;
    std::uint64_t frames = 0;
    std::uint64_t bytes = 0;
    std::string_view sensor{};
    int width = 0;
    int height = 0;
};

void configure(const Config& config);

esp_err_t start(std::optional<std::uint16_t> port = std::nullopt);
esp_err_t stop();
Status status();

// Restores the image settings to their defaults and forgets the saved ones.
esp_err_t reset_settings();

// Empty disables object detection. The value is persisted in NVS.
std::string detector_url();
esp_err_t set_detector_url(std::string_view url);

// Registers the "camera" command in the ESP-IDF console.
void register_commands(const Config& config = {});

}  // namespace camera_server
