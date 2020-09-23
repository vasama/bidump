cmake_minimum_required(VERSION 3.15)

project(bidmp)

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
	add_compile_options(/W4)
else()
	set(CMAKE_CXX_STANDARD 17)
	add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

add_executable(bidump
	src/main.cpp
	src/bidmp.cpp
	src/mdmp.cpp
)
