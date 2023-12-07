# ioLibrary_Driver
add_library(ROLES_FILES STATIC)

target_sources(ROLES_FILES PUBLIC
        ${ROLES_DIR}/runner.c
        )

target_include_directories(ROLES_FILES PUBLIC
        ${ROLES_DIR}
        )

#target_link_libraries(ROLES_FILES PRIVATE
#        pico_stdlib
#        hardware_spi
#        hardware_dma
#        hardware_clocks
#        FREERTOS_FILES
#        )
