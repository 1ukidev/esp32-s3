idf_component_register(
    SRCS
        "src/console.cpp"
        "src/wifi.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_event
        esp_netif
    PRIV_REQUIRES
        console
        esp_wifi
        nvs_flash
)
