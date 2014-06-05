
add_executable(example1 test.C)
if (STATE_TRACKER_WITH_CALLPATH)
	target_link_libraries(example1 stracker callpath adept_utils ${MPI_C_LIBRARIES})
else()
	target_link_libraries(example1 stracker ${MPI_C_LIBRARIES})
endif()
