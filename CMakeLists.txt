idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    return() # This component is not supported by the POSIX/Linux simulator
endif()

set(srcs "src/convert.cpp"
         "src/growable_buffer.cpp"
         "src/fnv.cpp"
         "src/onboard_led.cpp"
         "src/reset_button.cpp"
         "src/run_once.cpp"
         "src/rundown_protection.cpp"
         "src/task.cpp"
         "src/time.cpp"
         "src/lightstd/allocator.cpp"
         "src/storage/nvs.cpp"
)

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_gpio nvs_flash
                       PRIV_REQUIRES esp_timer)
