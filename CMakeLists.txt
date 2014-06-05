project(statetracker)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE ON)

function(notify_package name)
  if (${name}_FOUND)
    message(STATUS "Found ${name} in ${${name}_DIR}.")
  else()
    message(STATUS "${name} package not found.")
  endif()
endfunction()

# Add our module directory to the include path.
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmakemodules;${CMAKE_MODULE_PATH}")
include(FullRpath)

find_package(Boost REQUIRED)
notify_package(Boost)

find_package(MPI REQUIRED)
notify_package(MPI)

include_directories(${MPI_C_INCLUDE_PATH})

# Required dependencies
find_package(PythonInterp REQUIRED)
if (PYTHON_VERSION_STRING VERSION_LESS "2.6")
	message(FATAL_ERROR "Automaded requires Python 2.6 or later.")
endif()

#option(HAVE_CONFIG_H "use callpath" ON)
#option(STATE_TRACKER_WITH_CALLPATH "use callpath" ON)
include(${PROJECT_SOURCE_DIR}/Options.cmake)

# Use wrap.py wrapper generator
set(WRAP ${PROJECT_SOURCE_DIR}/wrap/wrap.py)
include(${PROJECT_SOURCE_DIR}/wrap/WrapConfig.cmake)

# Configure a header file with all the variables we found.
configure_file(
  ${PROJECT_SOURCE_DIR}/config.h.in
  ${PROJECT_BINARY_DIR}/statetracker-config.h)

include_directories ("${PROJECT_BINARY_DIR}")
if (STATE_TRACKER_WITH_CALLPATH)
	message(STATUS "callpath defined")
	find_package(callpath REQUIRED)
	find_package(adept_utils REQUIRED)
	include_directories(${callpath_INCLUDE_DIR})
	include_directories(${adept_utils_INCLUDE_DIR})
endif()

#	      ADD_DEFINITIONS(-D_MYDEFINE=${_MYDEFINE})
#install(FILES ${PROJECT_BINARY_DIR}/config.h DESTINATION include)

# Make callpath findable
#configure_file(
#  ${PROJECT_SOURCE_DIR}/automaded-config.cmake.in
#  ${PROJECT_BINARY_DIR}/automaded-config.cmake
#  @ONLY)
#add_subdirectory(unit_tests)
#add_subdirectory(examples)
#install(FILES ${PROJECT_BINARY_DIR}/automaded-config.cmake DESTINATION
#share/cmake/automaded)
#install(EXPORT automaded-libs                              DESTINATION
#share/cmake/automaded)
#execute_process(COMMAND src/generate_mpi_wrappers.pl)
#add_executable(mylib try_src/test.C)
#install (TARGETS Tutorial DESTINATION ${CMAKE_INSTALL_PREFIX})
add_subdirectory(src)
#add_subdirectory(examples) # Examples should be built with -O0

