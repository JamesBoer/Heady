
# Require minimum standard version of CMake
cmake_minimum_required (VERSION 3.10)

function(set_compiler_options project_name)
	if(MSVC)
		target_compile_options(${project_name} PRIVATE /W4 /WX)
	else()
		target_compile_options(${project_name} PRIVATE -Wall -Wextra -pedantic -Werror)
	endif()
endfunction()

# Set C++ 17 compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable IDE folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set project name
project(Heady)

# Option to enable/disable building the Heady test project
option(BUILD_HEADY_TESTS "Build the Heady test project" ON)

# Add source files and dependencies to executable
set(
	heady_source_list
	"Source/Heady.cpp"
	"Source/Heady.h"
	"Source/Main.cpp"
)
add_executable(${PROJECT_NAME} ${heady_source_list})
if(UNIX AND NOT APPLE)
	target_link_libraries(${PROJECT_NAME} PRIVATE "stdc++fs")
else()
	target_link_libraries(${PROJECT_NAME})
endif()

# Set compiler options
set_compiler_options(${PROJECT_NAME})

# Create folder structure
foreach(source IN LISTS heady_source_list)
	get_filename_component(source_path "${source}" PATH)
	string(REPLACE "/" "\\" source_path_group "${source_path}")
	source_group("${source_path_group}" FILES "${source}")
endforeach()

# Create tests
if(BUILD_HEADY_TESTS)
	set(
		basic_test_source_list
		"Tests/SelfTest/SelfTest.cpp"
		"Tests/SelfTest/SelfTest.h"
		"Tests/SelfTest/Main.cpp"
	)
	add_executable(SelfTest ${basic_test_source_list})
	if(UNIX AND NOT APPLE)
		target_link_libraries(SelfTest PRIVATE "stdc++fs")
	else()
		target_link_libraries(SelfTest)
	endif()

	# Set compiler options
	set_compiler_options(SelfTest)

	# Create folder structure
	foreach(source IN LISTS basic_test_source_list)
		source_group("Source" FILES "${source}")
	endforeach()
	set_property(TARGET SelfTest PROPERTY FOLDER "Tests")
endif()

# Set the MSVC startup project
if(MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()


