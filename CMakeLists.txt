cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(bullseye VERSION 0.1.8 LANGUAGES CXX)

# Set a default build type if none was specified
set(default_build_type "Debug")
 
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

SET(CMAKE_C_COMPILER "clang")
SET(CMAKE_CPP_COMPILER "clang++")
SET(CMAKE_CXX_FLAGS "-Wall -Wextra")
SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O1 -s")
SET(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

file(GLOB src src/*.cpp)

add_executable(revvm ${src})
set_target_properties(revvm PROPERTIES CXX_STANDARD 17)
if (CMAKE_BUILD_TYPE MATCHES Debug)
	target_compile_definitions(revvm PRIVATE LOG DEBUG)
else ()
	target_compile_definitions(revvm PRIVATE LOG)
endif(CMAKE_BUILD_TYPE MATCHES Debug) #
#target_link_libraries(revvm tbb)
target_include_directories(revvm PUBLIC "/usr/include/libseccomp")
