cmake_minimum_required(VERSION 3.15)

project(database VERSION 0.1)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCES
  src/parser.cpp
  src/server.cpp
  src/storage.cpp
  src/command_handler.cpp
  src/snapshotter.cpp
)

add_library(lib ${PROJECT_SOURCES})

target_include_directories(lib PUBLIC include)

add_executable(database src/main.cpp)
target_link_libraries(database PRIVATE lib)

include(FetchContent)
FetchContent_Declare(
	Catch2
	GIT_REPOSITORY https://github.com/catchorg/Catch2.git
	GIT_TAG        v3.8.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests tests/test.cpp)
target_link_libraries(
	tests 
	PRIVATE 
	lib
	Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(tests)

