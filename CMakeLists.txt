set(
        src_dirs 
        "./src"

        "./src/bus"
        "./src/bus/iic"
        "./src/bus/spi"
        "./src/bus/uart"
        "./src/bus/iis"
        "./src/bus/sdio"
        "./src/bus/mipi"

        "./src/chip"
        "./src/chip/iic"
        "./src/chip/spi"
        "./src/chip/uart"
        "./src/chip/iic_iis"
        "./src/chip/sdio"
        "./src/chip/mipi"

        "./src/resources"
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