cmake_minimum_required(VERSION 3.21)

add_executable(thing_test
	thing_test.c
	${CMAKE_SOURCE_DIR}/Unity/unity.c
)
target_include_directories(thing_test PRIVATE
	"${CMAKE_SOURCE_DIR}/Unity"
	"${CMAKE_SOURCE_DIR}/thing/src"
)
target_link_libraries(thing_test PRIVATE thing)

add_test(thing_test thing_test)
