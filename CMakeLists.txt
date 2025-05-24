cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# cmake_policy(SET CMP0091 NEW)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(vfd_panel CXX)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-foptimize-sibling-calls)
endif()

if (MSVC)
    add_link_options(/NODEFAULTLIB:libucrt$<$<CONFIG:Debug>:d>.lib)
    link_libraries(ucrt$<$<CONFIG:Debug>:d>.lib)
endif(MSVC)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SerialPort)

qt_standard_project_setup()

include_directories(include)
include_directories(src)
include_directories(libs/qtcoro)

qt_add_executable(vfd_panel
    src/mainwindow.ui
    src/Tab1.ui

    src/Tab1.hpp
    src/mainwindow.hpp
    src/vfd_ctl.hpp

    src/Tab1.cpp
    src/mainwindow.cpp
    src/main.cpp
    src/vfd_ctl.cpp
)

target_link_libraries(vfd_panel PRIVATE Qt6::Widgets Qt6::SerialPort)

if (MSVC)
target_link_options(vfd_panel PRIVATE "/NODEFAULTLIB")
target_link_libraries(vfd_panel PRIVATE ucrt$<$<CONFIG:Debug>:d>.lib msvcrt.lib libvcruntime.lib libcpmt$<$<CONFIG:Debug>:d>.lib)
endif()

set_target_properties(vfd_panel PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

qt_add_resources(vfd_panel imageresources
    PREFIX "/images"
    FILES res/logo.png
)

install(TARGETS vfd_panel DESTINATION bin)
