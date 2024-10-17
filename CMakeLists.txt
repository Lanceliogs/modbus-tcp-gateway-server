cmake_minimum_required(VERSION 3.5)

project(ModbusTCPGatewayServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(modbusgatewayserver
    main.cpp
    server.cpp
)

target_link_libraries(modbusgatewayserver modbus)