cmake_minimum_required(VERSION 3.25)
project(matching_engine)

set(CMAKE_CXX_STANDARD 20)

add_executable(matching_engine main.cpp src/OrderBook.cpp)
