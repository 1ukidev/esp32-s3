#include "camera_server/camera_server.hpp"

#include <mutex>

#include "camera.hpp"
#include "detector.hpp"
#include "web_server.hpp"

namespace camera_server {
namespace {

struct Service {
    std::mutex mutex;
    Config config;
    Camera camera;
    Detector detector;
    WebServer server{camera, detector};
};

Service& service()
{
    static Service instance;
    return instance;
}

}  // namespace

void configure(const Config& config)
{
    auto& s = service();
    std::scoped_lock lock(s.mutex);
    s.config = config;
    s.detector.set_default_url(config.detector_url);
}

esp_err_t start(std::optional<std::uint16_t> port)
{
    auto& s = service();
    std::scoped_lock lock(s.mutex);
    if (s.server.running()) {
        return ESP_ERR_INVALID_STATE;
    }

    if (const esp_err_t err = s.camera.init(s.config); err != ESP_OK) {
        return err;
    }
    if (const esp_err_t err = s.server.start(port.value_or(s.config.port), s.config.max_streams, s.config.name); err != ESP_OK) {
        s.camera.deinit();
        return err;
    }
    return ESP_OK;
}

esp_err_t stop()
{
    auto& s = service();
    std::scoped_lock lock(s.mutex);
    if (!s.server.running()) {
        return ESP_ERR_INVALID_STATE;
    }

    s.server.stop();
    s.camera.deinit();
    return ESP_OK;
}

Status status()
{
    auto& s = service();
    std::scoped_lock lock(s.mutex);
    if (!s.server.running()) {
        return {};
    }

    const auto stats = s.server.stats();
    const auto frame_size = s.camera.frame_size();
    const bool known_size = frame_size < FRAMESIZE_INVALID;
    return {
        .running = true,
        .port = stats.port,
        .uptime_s = stats.uptime_s,
        .streams = stats.streams,
        .max_streams = stats.max_streams,
        .fps = stats.fps,
        .frames = stats.frames,
        .bytes = stats.bytes,
        .sensor = s.camera.model(),
        .width = known_size ? resolution[frame_size].width : 0,
        .height = known_size ? resolution[frame_size].height : 0,
    };
}

esp_err_t reset_settings()
{
    auto& s = service();
    std::scoped_lock lock(s.mutex);
    return s.camera.reset();
}

std::string detector_url()
{
    return service().detector.url();
}

esp_err_t set_detector_url(std::string_view url)
{
    return service().detector.set_url(url);
}

}  // namespace camera_server
