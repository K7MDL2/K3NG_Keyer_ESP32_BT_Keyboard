idf_component_register(SRCS "LiquidCrystal_I2C.cpp" "LCD.cpp" "I2CIO.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES  arduino-esp32 driver esp_driver_spi esp_driver_gpio esp_driver_i2c)

project(LiquidCrystal_I2C)