cmake_minimum_required(VERSION 3.15)

project(database VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(database
  src/main.cpp
  src/parser.cpp
  src/server.cpp
  src/storage.cpp
  src/command_handler.cpp
)

target_include_directories(database PUBLIC include)
