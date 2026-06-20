idf_component_register(SRCS "IST8310.cpp" "vec3f.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES  arduino-esp32 driver esp_driver_spi esp_driver_gpio esp_driver_i2c)

#project(LiquidCrystal_I2C)