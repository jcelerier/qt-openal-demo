cmake_minimum_required (VERSION 3.2)
project (oaltest)

add_executable(oaltest main.cpp)
target_link_libraries(oaltest openal alut)
set_property(TARGET oaltest PROPERTY CXX_STANDARD 14)
