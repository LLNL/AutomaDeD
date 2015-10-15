#
# Skip MPI C++ interface.
#
add_definitions(-DMPICH_SKIP_MPICXX -DOMPI_SKIP_MPICXX)

#
# Set up includes and linking for the stack trace library
# that we are using here.
#
add_wrapped_file(mpi_wrappers.h wrap_template.w)
include_directories(${Boost_INCLUDE_DIR})
include_directories(${MPI_INCLUDE_PATH} ${CMAKE_CURRENT_BINARY_DIR})

#
# Include directories from external libraries
#
#include_directories(
	# ${PROJECT_BINARY_DIR})
  #  ${MPI_INCLUDE_PATH})
  #include(${MPI_INCLUDE_PATH}/mpi.h)
#
# Headers -- these are installed into $prefix/include.
#
#set( CMAKE_CXX_FLAGS "-dynamic " )
set(AUTOMADED_HEADERS
    analysis_driver.h
	backtrace.h
	config.h
	debugging.h
	dependency_matrix.h
	dep_matrix_mutator.h
	documentation.h
	edge_reduction.h
	graph_algorithm.h
	graph_model.h
	helper_thread.h
	io_utilities.h
	markov_model.h
	markov_model_reduction.h
	model_components.h
	mpi_state.h
	mpi_wrappers.h
	range_set.h
	range_set_table.h
	reduction.h
	signal_handlers.h
	state_reduction.h
	statetracker-config.h
	timer.h
	transitions.h
	utilities.h)

#
# Library source files.
#
set(AUTOMADED_SOURCES
	analysis_driver.C
	backtrace.C
	config.C
	debugging.C
	dependency_matrix.C
	dep_matrix_mutator.C
	edge_reduction.C
	graph_algorithm.C
	graph_model.C
	helper_thread.C
	io_utilities.C
	main_controller.C
	model_components.C
	mpi_state.C
	range_set.C
	range_set_table.C
	signal_handlers.C
	state_reduction.C
	timer.C
	transitions.C
	utilities.C
	mpi_wrappers.h)

#for shared library
add_library(stracker SHARED ${AUTOMADED_SOURCES})
target_link_libraries(stracker ${MPI_C_LIBRARIES})

if (STATE_TRACKER_WITH_CALLPATH)
	target_link_libraries(stracker callpath)
#target_link_libraries(stracker adept_utils)
	set_target_properties(callpath  PROPERTIES  LINK_INTERFACE_LIBRARIES "" )
endif()
#set_target_properties(${MPI_C_LIBRARIES}  PROPERTIES  LINK_INTERFACE_LIBRARIES "" )
#set_target_properties(${MPI_CXX_LIBRARIES}  PROPERTIES  LINK_INTERFACE_LIBRARIES "" )

#for static library
add_library(stracker_static STATIC ${AUTOMADED_SOURCES})
target_link_libraries(stracker_static ${MPI_C_LIBRARIES})

if (STATE_TRACKER_WITH_CALLPATH)
	target_link_libraries(stracker_static callpath)
	target_link_libraries(stracker_static adept_utils)
endif()
#
# Things to install into the prefix.
#
install(TARGETS stracker DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
#install(TARGETS stracker_static DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
#install(FILES ${CALLPATH_HEADERS} DESTINATION include)


