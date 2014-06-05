noinst_PROGRAMS = serial_driver parallel_driver

# --------  Serial driver ----------------
serial_driver_SOURCES =  serial_driver.C assert_functions.h test_stateFactory.C test_stateFactory.h \
			test_MarkovModel.C test_MarkovModel.h test_RangeSet.C test_RangeSet.h

serial_driver_CPPFLAGS = -I$(top_srcdir)/src
serial_driver_LDADD = $(top_srcdir)/src/.libs/libstracker.a

# --------- Parallel driver ----------------
parallel_driver_SOURCES =  parallel_driver.C assert_functions.h test_stateReductions.C test_stateReductions.h \
test_DependencyMatrix.C test_DependencyMatrix.h test_RangeSetTable.C test_RangeSetTable.h \
test_DepMatrixMutator.C test_DepMatrixMutator.h test_AnalysisDriver.C test_AnalysisDriver.h

parallel_driver_CPPFLAGS = -I$(top_srcdir)/src
parallel_driver_LDADD = $(top_srcdir)/src/.libs/libstracker.a
