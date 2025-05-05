set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)
set(FREERTOS_KERNEL_PATH "/Users/richard/Documents/embarcatech/FreeRTOS-Kernel")
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(semafaro-inteligente-raspberry-pico-w C CXX ASM)

pico_sdk_init()

add_executable(${PROJECT_NAME} semafaro-inteligente-raspberry-pico-w.c lib/buzzer.c lib/leds.c lib/ssd1306.c)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/pio_matrix.pio)

pico_set_program_name(${PROJECT_NAME} "semafaro-inteligente-raspberry-pico-w")
pico_set_program_version(${PROJECT_NAME}  "0.1")

pico_enable_stdio_uart(${PROJECT_NAME}   0)
pico_enable_stdio_usb(${PROJECT_NAME}  0)

include_directories(${CMAKE_SOURCE_DIR}/lib)

target_link_libraries(${PROJECT_NAME} 
pico_stdlib 
FreeRTOS-Kernel         
FreeRTOS-Kernel-Heap4  
        )

target_include_directories(${PROJECT_NAME}  PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME}  
        hardware_pwm
        hardware_pio
        hardware_clocks
        hardware_i2c
        )

pico_add_extra_outputs(${PROJECT_NAME} )

