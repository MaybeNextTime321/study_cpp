cmake_minimum_required(VERSION 3.8)
project(calculation)

include(FetchContent)

FetchContent_Declare(
  	math
  	    GIT_REPOSITORY git@github.com:MaybeNextTime321/simple-math-library.git
	    GIT_TAG main
)

FetchContent_MakeAvailable(math)

add_executable(${PROJECT_NAME} main.cpp)

target_compile_options(${PROJECT_NAME}
  	PRIVATE
    	    -g
	    -fpermissive
    	    -Wall
    	    -Wextra
    	    -Werror
    	    -Wpedantic
)

set_target_properties(${PROJECT_NAME}
	PROPERTIES
    	    CXX_STANDARD 20
    	    CXX_STANDARD_REQUIRED ON
    	    CXX_EXTENSIONS OFF
)

find_program(CLANG_FORMAT_PATH NAMES clang-format-21 clang-format-18 clang-format REQUIRED)
message(STATUS "Found clang-format: ${CLANG_FORMAT_PATH}")

file(GLOB_RECURSE CLANG_FORMAT_SRC CONFIGURE_DEPENDS
	*.h
        *.cpp
        *.hpp
)

add_custom_target(clang-format-fix
	COMMAND
            ${CLANG_FORMAT_PATH} -style=file -i ${CLANG_FORMAT_SRC}
        WORKING_DIRECTORY
            ${CMAKE_SOURCE_DIR}
        COMMENT
	"Formatting ${PROJECT_NAME} with ${CLANG_FORMAT_PATH} ..."
    )

add_dependencies(${PROJECT_NAME} clang-format-fix)


find_program(CLANG_TIDY_PATH NAMES clang-tidy-21 clang-tidy-18 clang-tidy REQUIRED)
message(STATUS "Found clang-tidy: ${CLANG_TIDY_PATH}")
set(CLANG_TIDY_COMMAND ${CLANG_TIDY_PATH} "-config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")


target_link_libraries(${PROJECT_NAME} PRIVATE math)
