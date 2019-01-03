
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

# Add source files and dependencies to executable
set(
	heady_source_list
	"Source/Heady.cpp"
	"Source/Heady.h"
	"Source/Main.cpp"
)
add_executable(${PROJECT_NAME} ${heady_source_list})
target_link_libraries(${PROJECT_NAME} PRIVATE "stdc++fs")

# Set compiler options
set_compiler_options(${PROJECT_NAME})

# Create folder structure
foreach(source IN LISTS heady_source_list)
	get_filename_component(source_path "${source}" PATH)
	string(REPLACE "/" "\\" source_path_group "${source_path}")
	source_group("${source_path_group}" FILES "${source}")
endforeach()

# Create basic test
set(
	basic_test_source_list
	"Tests/Basic/Basic.cpp"
	"Tests/Basic/Basic.h"
	"Tests/Basic/Main.cpp"
)
add_executable(Basic ${basic_test_source_list})
target_link_libraries(Basic PRIVATE "stdc++fs")

# Set compiler options
set_compiler_options(Basic)

# Create folder structure
foreach(source IN LISTS basic_test_source_list)
	source_group("Source" FILES "${source}")
endforeach()
set_property(TARGET Basic PROPERTY FOLDER "Tests")

# Set the MSVC startup project
if(MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()


