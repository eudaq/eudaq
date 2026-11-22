find_package(fmt REQUIRED)

aux_source_directory(src COMMON_SRC)

add_library(${EUDAQ_MODULE}_common STATIC
	${COMMON_SRC}
)

target_include_directories(${EUDAQ_MODULE}_common
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${EUDAQ_INCLUDE_DIRS}
)

target_link_libraries(${EUDAQ_MODULE}_common
	PUBLIC
	${EUDAQ_CORE_LIBRARY}
	fmt::fmt
)
