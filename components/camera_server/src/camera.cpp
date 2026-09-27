#include "camera.hpp"

#include <algorithm>
#include <array>
#include <type_traits>

#include "esp_log.h"

#include "json.hpp"

namespace camera_server {
namespace {

constexpr const char* TAG = "camera";

template <typename Setter>
struct SetterArgument;

template <typename Argument>
struct SetterArgument<int (*)(sensor_t*, Argument)> {
    using type = Argument;
};

template <auto Field>
int apply(sensor_t* sensor, int value)
{
    const auto setter = sensor->*Field;
    if (setter == nullptr) {
        return -1;
    }
    using Argument = typename SetterArgument<std::remove_cvref_t<decltype(setter)>>::type;
    return setter(sensor, static_cast<Argument>(value));
}

constexpr int min_xclk_mhz = 8;
constexpr int max_xclk_mhz = 24;

int apply_xclk(sensor_t* sensor, int mhz)
{
    return sensor->set_xclk != nullptr ? sensor->set_xclk(sensor, LEDC_TIMER_0, mhz) : -1;
}

struct Control {
    std::string_view name;
    int (*apply)(sensor_t*, int);
};

constexpr std::array controls{
    Control{"framesize", apply<&sensor_t::set_framesize>},
    Control{"quality", apply<&sensor_t::set_quality>},
    Control{"brightness", apply<&sensor_t::set_brightness>},
    Control{"contrast", apply<&sensor_t::set_contrast>},
    Control{"saturation", apply<&sensor_t::set_saturation>},
    Control{"special_effect", apply<&sensor_t::set_special_effect>},
    Control{"hmirror", apply<&sensor_t::set_hmirror>},
    Control{"vflip", apply<&sensor_t::set_vflip>},
    Control{"awb", apply<&sensor_t::set_whitebal>},
    Control{"awb_gain", apply<&sensor_t::set_awb_gain>},
    Control{"wb_mode", apply<&sensor_t::set_wb_mode>},
    Control{"aec", apply<&sensor_t::set_exposure_ctrl>},
    Control{"aec2", apply<&sensor_t::set_aec2>},
    Control{"ae_level", apply<&sensor_t::set_ae_level>},
    Control{"aec_value", apply<&sensor_t::set_aec_value>},
    Control{"agc", apply<&sensor_t::set_gain_ctrl>},
    Control{"agc_gain", apply<&sensor_t::set_agc_gain>},
    Control{"gainceiling", apply<&sensor_t::set_gainceiling>},
    Control{"bpc", apply<&sensor_t::set_bpc>},
    Control{"wpc", apply<&sensor_t::set_wpc>},
    Control{"raw_gma", apply<&sensor_t::set_raw_gma>},
    Control{"lenc", apply<&sensor_t::set_lenc>},
    Control{"dcw", apply<&sensor_t::set_dcw>},
    Control{"colorbar", apply<&sensor_t::set_colorbar>},
    Control{"xclk", apply_xclk},
};

}  // namespace

esp_err_t Camera::init(const Config& config)
{
    if (ready()) {
        return ESP_OK;
    }

    const auto& pins = config.pins;
    camera_config_t camera{};
    camera.pin_pwdn = pins.pwdn;
    camera.pin_reset = pins.reset;
    camera.pin_xclk = pins.xclk;
    camera.pin_sccb_sda = pins.sccb_sda;
    camera.pin_sccb_scl = pins.sccb_scl;
    camera.pin_d0 = pins.d0;
    camera.pin_d1 = pins.d1;
    camera.pin_d2 = pins.d2;
    camera.pin_d3 = pins.d3;
    camera.pin_d4 = pins.d4;
    camera.pin_d5 = pins.d5;
    camera.pin_d6 = pins.d6;
    camera.pin_d7 = pins.d7;
    camera.pin_vsync = pins.vsync;
    camera.pin_href = pins.href;
    camera.pin_pclk = pins.pclk;
    camera.xclk_freq_hz = config.xclk_freq_hz;
    camera.ledc_timer = LEDC_TIMER_0;
    camera.ledc_channel = LEDC_CHANNEL_0;
    camera.pixel_format = PIXFORMAT_JPEG;
    camera.frame_size = config.max_frame_size;
    camera.jpeg_quality = config.jpeg_quality;
    camera.fb_count = config.frame_buffers;
    camera.fb_location = CAMERA_FB_IN_PSRAM;
    camera.grab_mode = CAMERA_GRAB_LATEST;
    camera.sccb_i2c_port = -1;

    if (const esp_err_t err = esp_camera_init(&camera); err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize the camera: %s", esp_err_to_name(err));
        return err;
    }

    sensor_ = esp_camera_sensor_get();
    const auto* info = esp_camera_sensor_get_info(&sensor_->id);
    model_ = info != nullptr ? info->name : "unknown";
    max_frame_size_ = info != nullptr ? std::min(info->max_size, config.max_frame_size)
                                      : config.max_frame_size;

    // Same tuning used by Espressif's CameraWebServer example.
    if (sensor_->id.PID == OV3660_PID) {
        sensor_->set_brightness(sensor_, 1);
        sensor_->set_saturation(sensor_, -2);
    }
    if (config.vertical_flip || sensor_->id.PID == OV3660_PID) {
        sensor_->set_vflip(sensor_, 1);
    }
    sensor_->set_framesize(sensor_, std::min(config.initial_frame_size, max_frame_size_));

    ESP_LOGI(TAG, "Sensor %.*s ready", static_cast<int>(model_.size()), model_.data());
    return ESP_OK;
}

void Camera::deinit()
{
    if (!ready()) {
        return;
    }
    esp_camera_deinit();
    sensor_ = nullptr;
}

framesize_t Camera::frame_size() const
{
    return ready() ? sensor_->status.framesize : FRAMESIZE_INVALID;
}

esp_err_t Camera::set(std::string_view name, int value)
{
    if (!ready()) {
        return ESP_ERR_INVALID_STATE;
    }
    if (name == "framesize" && (value < 0 || value > static_cast<int>(max_frame_size_))) {
        return ESP_ERR_INVALID_ARG;
    }
    if (name == "xclk" && (value < min_xclk_mhz || value > max_xclk_mhz)) {
        return ESP_ERR_INVALID_ARG;
    }

    const auto control = std::ranges::find(controls, name, &Control::name);
    if (control == controls.end()) {
        return ESP_ERR_NOT_FOUND;
    }
    return control->apply(sensor_, value) == 0 ? ESP_OK : ESP_FAIL;
}

std::string Camera::settings_json() const
{
    if (!ready()) {
        return "{}";
    }

    std::string framesizes = "[";
    for (int size = 0; size <= static_cast<int>(max_frame_size_); ++size) {
        const auto& info = resolution[size];
        if (size > 0) {
            framesizes += ',';
        }
        framesizes += JsonObject{}.add("id", size).add("w", info.width).add("h", info.height).str();
    }
    framesizes += ']';

    const auto& s = sensor_->status;
    return JsonObject{}
        .add("sensor", model_)
        .add("pid", sensor_->id.PID)
        .add("framesize", static_cast<int>(s.framesize))
        .add("quality", s.quality)
        .add("brightness", s.brightness)
        .add("contrast", s.contrast)
        .add("saturation", s.saturation)
        .add("special_effect", s.special_effect)
        .add("wb_mode", s.wb_mode)
        .add("awb", s.awb)
        .add("awb_gain", s.awb_gain)
        .add("aec", s.aec)
        .add("aec2", s.aec2)
        .add("ae_level", s.ae_level)
        .add("aec_value", s.aec_value)
        .add("agc", s.agc)
        .add("agc_gain", s.agc_gain)
        .add("gainceiling", s.gainceiling)
        .add("bpc", s.bpc)
        .add("wpc", s.wpc)
        .add("raw_gma", s.raw_gma)
        .add("lenc", s.lenc)
        .add("hmirror", s.hmirror)
        .add("vflip", s.vflip)
        .add("dcw", s.dcw)
        .add("colorbar", s.colorbar)
        .add("xclk", sensor_->xclk_freq_hz / 1'000'000)
        .add_raw("framesizes", framesizes)
        .str();
}

}  // namespace camera_server
