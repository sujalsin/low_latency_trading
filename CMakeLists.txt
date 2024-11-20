cmake_minimum_required(VERSION 3.10)
project(LowLatencyTrading)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")

# Add executable
add_executable(trading_system
    src/main.cpp
    src/order_book/order_book.cpp
    src/network/udp_socket.cpp
)

# Include directories
target_include_directories(trading_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Find and link threads
find_package(Threads REQUIRED)
target_link_libraries(trading_system PRIVATE Threads::Threads)
