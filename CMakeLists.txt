cmake_minimum_required(VERSION 3.15)

project(ModbusTCPGatewayServer VERSION 1.0 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(modbusgatewayserver
    main.cpp
    server.cpp
)

if(WIN32)
    target_include_directories(modbusgatewayserver PRIVATE win32/include)
    target_link_directories(modbusgatewayserver PRIVATE win32/lib)
endif()

target_link_libraries(modbusgatewayserver ws2_32 modbus-5)
