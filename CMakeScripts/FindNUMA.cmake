# Author: Rommel Sanchez, 2019
# Package to compile and link the  NUMA library

# Sets variables:
#    NUMA_FOUND						-> Automatic
#    NUMA_DEFINITIONS				-> Explicit
#    NUMA_INCLUDE_DIRS			-> Explicit
#    NUMA_LIBRARIES				-> Explicit

set(
    NUMA_DEFINITIONS 
	-D_USE_NUMA ${NUMA_CMAKE_DEFINITIONS})

find_path(
    NUMA_INCLUDE_DIR 
		NAMES numa.h 
		HINTS $ENV{NUMA_ROOT} ${NUMA_CMAKE_INCLUDE_DIRS})

find_library(
    NUMA_LIBRARY
    NAMES libnuma.so ${NUMA_CMAKE_LIBRARIES}
    HINTS $ENV{NUMA_ROOT} ${NUMA_CMAKE_LIBRARY_DIRS})

include( 
	FindPackageHandleStandardArgs )

find_package_handle_standard_args(
    NUMA DEFAULT_MSG
    NUMA_LIBRARY
    NUMA_INCLUDE_DIR)

mark_as_advanced(
    NUMA_INCLUDE_DIR 
    NUMA_LIBRARY)

set(
    NUMA_INCLUDE_DIRS ${NUMA_INCLUDE_DIR})
set(
    NUMA_LIBRARIES ${NUMA_LIBRARY})
