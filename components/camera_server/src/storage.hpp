#pragma once

#include <optional>
#include <utility>

#include "nvs.h"

namespace camera_server {

// Open NVS namespace that closes itself; the NVS partition is initialized on first use.
class NvsNamespace {
public:
    [[nodiscard]] static std::optional<NvsNamespace> open(const char* name, nvs_open_mode_t mode);

    NvsNamespace(NvsNamespace&& other) noexcept : handle_(std::exchange(other.handle_, 0)) {}
    NvsNamespace& operator=(NvsNamespace&&) = delete;
    ~NvsNamespace();

    [[nodiscard]] nvs_handle_t get() const { return handle_; }

private:
    explicit NvsNamespace(nvs_handle_t handle) : handle_(handle) {}

    nvs_handle_t handle_ = 0;
};

}  // namespace camera_server
