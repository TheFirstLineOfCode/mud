cmake_minimum_required(VERSION 3.21)

add_executable(things_tiny_id_test
	things_tiny_id_test.c
	${CMAKE_SOURCE_DIR}/Unity/unity.c
)
target_include_directories(things_tiny_id_test PRIVATE
	"${CMAKE_SOURCE_DIR}/Unity"
	"${CMAKE_SOURCE_DIR}/tuxp/src"
)
target_link_libraries(things_tiny_id_test PRIVATE tuxp)

add_test(things_tiny_id_test things_tiny_id_test)

add_executable(tuxp_test
	tuxp_test.c
	${CMAKE_SOURCE_DIR}/Unity/unity.c
)
target_include_directories(tuxp_test PRIVATE
	"${CMAKE_SOURCE_DIR}/Unity"
	"${CMAKE_SOURCE_DIR}/tuxp/src"
)
target_link_libraries(tuxp_test PRIVATE tuxp)

add_test(tuxp_test tuxp_test)
