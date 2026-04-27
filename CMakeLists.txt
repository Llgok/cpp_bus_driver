set(
        src_dirs 
        "./src"

        "./src/bus"
        "./src/bus/i2c"
        "./src/bus/spi"
        "./src/bus/uart"
        "./src/bus/i2s"
        "./src/bus/sdio"
        "./src/bus/mipi"

        "./src/chip"
        "./src/chip/i2c"
        "./src/chip/spi"
        "./src/chip/uart"
        "./src/chip/i2c_i2s"
        "./src/chip/sdio"
        "./src/chip/mipi"
)
    
idf_component_register(
        SRC_DIRS 
        ${src_dirs} 
        INCLUDE_DIRS 
        ${src_dirs} 
        REQUIRES
        "driver"
        "esp_timer"
        "esp_lcd"
    )