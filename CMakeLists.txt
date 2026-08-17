cmake_minimum_required(VERSION 3.13...3.27)

include(pico_sdk_import.cmake)

set(PROJECT_NAME "pico_raven")

project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
add_compile_options(-Wall -Wextra)

add_executable(${PROJECT_NAME}
    src/main.c
    src/screen.c
    src/game.c
    src/collision.c
    src/core_msg.c
    src/core1_main.c
    src/game_data.c
    src/savegame.c
    src/player.c
    src/enemy.c
    src/draw_room.c
    src/sprite_shadow.c
    src/rooms/west__buntown_gate.c
    src/rooms/west__west_hall.c
    src/rooms/east__test.c
    src/rooms/north__test.c
    src/rooms/depths__test.c
    src/rooms/core__test.c
    lib/joy_wii_i2c.c
    lib/audio_mixer.c
    lib/mod_play.c
    lib/sound_i2s.c
    lib/vga_8bit.c
    lib/vga_font_8bit.c
    lib/vga_image_8bit.c
    lib/wii_i2c.c
    lib/flash_write.c
    lib/sysinfo.c
)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/lib/vga_8bit.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/lib/sound_i2s_8bits.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/lib/sound_i2s_16bits.pio)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_multicore
    pico_flash
    hardware_dma
    hardware_irq
    hardware_pio
    hardware_i2c
)

pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

pico_add_extra_outputs(${PROJECT_NAME})
