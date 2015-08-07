cmake_minimum_required (VERSION 3.2)
project (oaltest)

add_subdirectory(API)

add_executable(oaltest main.cpp)
target_link_libraries(oaltest openal alut APIJamoma)
set_property(TARGET oaltest PROPERTY CXX_STANDARD 14)
