cmake_minimum_required(VERSION 3.8)
project(calculation)

set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Installation prefix" FORCE)

include(FetchContent)
include(CTest)
find_package(Boost REQUIRED COMPONENTS system)

option(CALC_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

string(REPLACE "-Werror=tsan" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE "-Wno-tsan" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE "-Werror=tsan" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-Wno-tsan" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "C flags" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "C++ flags" FORCE)

if(CALC_ENABLE_TSAN)
	if(NOT MSVC)
        add_compile_options(-fsanitize=thread -g -O1)
        add_link_options(-fsanitize=thread)
    else()
        message(FATAL_ERROR "ThreadSanitizer is only supported with GCC/Clang on non-MSVC toolchains.")
    endif()
endif()

set(CMAKE_SKIP_INSTALL_RULES ON)

FetchContent_Declare(
  	math
  	    GIT_REPOSITORY https://github.com/MaybeNextTime321/simple-math-library.git
	    GIT_TAG main
	)

FetchContent_MakeAvailable(math)

FetchContent_Declare(
  		spdlog
  	    GIT_REPOSITORY https://github.com/gabime/spdlog.git
		GIT_TAG v1.14.1
		GIT_SHALLOW TRUE
)

	set(SPDLOG_INSTALL OFF CACHE BOOL "Disable spdlog install rules" FORCE)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
  		json
  	    GIT_REPOSITORY https://github.com/nlohmann/json.git
		GIT_TAG v3.12.0
		GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(json)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
	GIT_SHALLOW TRUE
)

set(INSTALL_GTEST OFF CACHE BOOL "Disable GoogleTest install rules" FORCE)
FetchContent_MakeAvailable(googletest)

set(CMAKE_SKIP_INSTALL_RULES OFF)

enable_testing()

add_executable(unit_tests tests/test.cpp)
target_link_libraries(unit_tests PRIVATE GTest::gtest_main math)
include(GoogleTest)
gtest_discover_tests(unit_tests)


set_target_properties(spdlog PROPERTIES CXX_CLANG_TIDY "")

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
list(FILTER CLANG_FORMAT_SRC EXCLUDE REGEX ".*/_deps/.*")

add_custom_target(clang-format-fix
	COMMAND
            ${CLANG_FORMAT_PATH} -style=file -i ${CLANG_FORMAT_SRC}
        WORKING_DIRECTORY
            ${CMAKE_SOURCE_DIR}
        COMMENT
	"Formatting ${PROJECT_NAME} with ${CLANG_FORMAT_PATH} ..."
    )

add_dependencies(${PROJECT_NAME} clang-format-fix)

find_package(PostgreSQL REQUIRED)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	find_program(CLANG_TIDY_PATH NAMES clang-tidy-21 clang-tidy-18 clang-tidy REQUIRED)
	message(STATUS "Found clang-tidy: ${CLANG_TIDY_PATH}")
	set(CLANG_TIDY_COMMAND ${CLANG_TIDY_PATH} "-config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
	set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
else()
	set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "")
endif()


target_link_libraries(${PROJECT_NAME} PRIVATE PostgreSQL::PostgreSQL)
target_link_libraries(${PROJECT_NAME} PRIVATE math)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::system)
target_link_libraries(unit_tests PRIVATE GTest::gtest_main math spdlog nlohmann_json::nlohmann_json)
target_link_libraries(unit_tests PRIVATE PostgreSQL::PostgreSQL)
target_link_libraries(unit_tests PRIVATE Boost::system)

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	COMPONENT runtime
)

install(FILES "${CMAKE_SOURCE_DIR}/calculation.service"
    DESTINATION "/etc/systemd/system"
	COMPONENT runtime
)

set(CPACK_PACKAGE_NAME "calculation")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_CONTACT "Ilia Rybakov <47.2region@gmail.com>")
set(CPACK_PACKAGE_VENDOR "Maybe")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ilia Rybakov <47.2region@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Calculation service")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_SET_DESTDIR ON)
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_COMPONENTS_ALL runtime)
set(CPACK_DEB_COMPONENT_INSTALL OFF)
configure_file(
	"${CMAKE_SOURCE_DIR}/cpack_cleanup.cmake.in"
	"${CMAKE_BINARY_DIR}/cpack_cleanup.cmake"
	@ONLY
)
set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_BINARY_DIR}/cpack_cleanup.cmake")

include(CPack)
