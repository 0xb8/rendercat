set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/catch/cmake/")

add_library(Catch catch_main.cpp)
target_include_directories(Catch PUBLIC catch/)
target_compile_definitions(Catch PUBLIC
	CATCH_CONFIG_FAST_COMPILE
	CATCH_CONFIG_COLOUR_NONE
	CATCH_CONFIG_DISABLE_MATCHERS
)

if(CMAKE_COMPILER_IS_GNUCXX)
	target_compile_options(Catch PRIVATE -O2 -g0)
endif()

add_executable(unique_handle_test unique_handle.cpp)
target_link_libraries(unique_handle_test PRIVATE Catch)
target_include_directories(unique_handle_test PRIVATE ${PROJECT_SOURCE_DIR}/include)

set_target_properties(unique_handle_test Catch
	PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
)

include(ParseAndAddCatchTests)
ParseAndAddCatchTests(unique_handle_test)
