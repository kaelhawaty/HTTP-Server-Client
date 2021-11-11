cmake_minimum_required(VERSION 3.16)
project(Network_lab)

set(CMAKE_CXX_STANDARD 17)

add_executable(Network_lab main.cpp networking.cpp http.cpp networking.h http.h debugger.h)
add_executable(Client Client/client.cpp networking.cpp http.cpp networking.h http.h debugger.h)
set_target_properties(Client PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Client")

add_executable(Evaluator Evaluator/evaluator.cpp networking.cpp http.cpp networking.h http.h debugger.h)
set_target_properties(Evaluator PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Evaluator")


if(WIN32)
    target_link_libraries(Network_lab wsock32 ws2_32)
    target_link_libraries(Client wsock32 ws2_32)
    target_link_libraries(Evaluator wsock32 ws2_32)
endif()