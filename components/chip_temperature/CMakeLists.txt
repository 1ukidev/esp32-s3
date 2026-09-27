idf_component_register(
    SRCS "src/chip_temperature.cpp"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES
        console
        esp_driver_tsens
)
