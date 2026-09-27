#include "storage.hpp"

#include "nvs_flash.h"

namespace camera_server {

std::optional<NvsNamespace> NvsNamespace::open(const char* name, nvs_open_mode_t mode)
{
    // A partition that needs erasing belongs to the application, so it is left untouched here.
    if (const esp_err_t err = nvs_flash_init(); err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        return std::nullopt;
    }

    nvs_handle_t handle = 0;
    if (nvs_open(name, mode, &handle) != ESP_OK) {
        return std::nullopt;
    }
    return NvsNamespace{handle};
}

NvsNamespace::~NvsNamespace()
{
    if (handle_ != 0) {
        nvs_close(handle_);
    }
}

}  // namespace camera_server
