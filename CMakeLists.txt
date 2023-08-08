cmake_minimum_required(VERSION 3.26)
project(testc C)

set(CMAKE_C_STANDARD 11)

add_executable(testc main.c)

set(LIBS Ws2_32)
target_link_libraries(testc ${LIBS})
