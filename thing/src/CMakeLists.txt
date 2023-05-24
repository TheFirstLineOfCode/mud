cmake_minimum_required(VERSION 3.21)

add_library(thing STATIC
	thing.h
	thing.c
)

target_link_libraries(thing PUBLIC tacp)

target_include_directories(thing PUBLIC
	"${CMAKE_SOURCE_DIR}/tacp/src"
)
