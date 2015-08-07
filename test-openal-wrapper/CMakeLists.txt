cmake_minimum_required (VERSION 3.2)
project (oaltest)

set(CMAKE_AUTOMOC On)
set(CMAKE_INCLUDE_CURRENT_DIR On)


find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

add_subdirectory(API)


add_executable(oaltest main.cpp SoundObj.hpp)
target_link_libraries(oaltest
    openal alut
    APIJamoma
    Qt5::Core Qt5::Widgets)

set_property(TARGET oaltest PROPERTY CXX_STANDARD 14)
