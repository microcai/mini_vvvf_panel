
add_library(qtcoro STATIC QCoroHandleWrapper.cpp QCoroHandleWrapper.hpp)

target_include_directories(qtcoro INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(qtcoro Qt6::Core)
