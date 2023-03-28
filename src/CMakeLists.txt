# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.
#. /home/jim/.platformio/packages/framework-espidf/export.sh

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
set(include_dirs ${CMAKE_SOURCE_DIR}/include)

idf_component_register(SRCS "main.cpp" "bt_keyboard.cpp" "${app_sources}"
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES esp_hid)
