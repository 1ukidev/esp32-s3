#pragma once

#include <concepts>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

namespace camera_server {

class JsonObject {
public:
    JsonObject() { buffer_.reserve(512); buffer_ += '{'; }

    template <typename T>
    JsonObject& add(std::string_view key, const T& value)
    {
        write_key(key);
        if constexpr (std::same_as<T, bool>) {
            buffer_ += value ? "true" : "false";
        } else if constexpr (std::integral<T>) {
            buffer_ += std::to_string(value);
        } else if constexpr (std::floating_point<T>) {
            char text[24];
            std::snprintf(text, sizeof(text), "%.2f", static_cast<double>(value));
            buffer_ += text;
        } else {
            write_string(std::string_view{value});
        }
        return *this;
    }

    JsonObject& add_raw(std::string_view key, std::string_view json)
    {
        write_key(key);
        buffer_ += json;
        return *this;
    }

    std::string str()
    {
        buffer_ += '}';
        return std::move(buffer_);
    }

private:
    void write_key(std::string_view key)
    {
        if (buffer_.size() > 1) {
            buffer_ += ',';
        }
        write_string(key);
        buffer_ += ':';
    }

    void write_string(std::string_view text)
    {
        buffer_ += '"';
        for (const char c : text) {
            if (c == '"' || c == '\\') {
                buffer_ += '\\';
            } else if (static_cast<unsigned char>(c) < 0x20) {
                continue;
            }
            buffer_ += c;
        }
        buffer_ += '"';
    }

    std::string buffer_;
};

}  // namespace camera_server
