idf_component_register(SRCS "LiquidCrystal_I2C.cpp" "LCD.cpp" "I2CIO.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES  arduino-esp32)

project(LiquidCrystal_I2C)