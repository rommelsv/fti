# Author: Rommel Sanchez, 2019
# Package to compile and link the  AML library

# Sets variables:
#    AML_FOUND						-> Automatic
#    AML_DEFINITIONS				-> Explicit
#    AML_INCLUDE_DIRS			-> Explicit
#    AML_LIBRARIES				-> Explicit

set(
    AML_DEFINITIONS 
	-D_USE_AML ${AML_CMAKE_DEFINITIONS})


#message(AUTHOR_WARNING "Env Var is : $ENV{AML_ROOT}")

find_path(
    AML_INCLUDE_DIR 
		NAMES aml.h 
		HINTS $ENV{AML_ROOT}/include ${AML_CMAKE_INCLUDE_DIRS} )

find_library(
    AML_LIBRARY
    NAMES libaml.so ${AML_CMAKE_LIBRARIES}
    HINTS $ENV{AML_ROOT}/lib ${AML_CMAKE_LIBRARY_DIRS})

find_library(
    AML_JEMALLOC_LIBRARY
    NAMES libjemalloc-aml.so ${AML_JEMALLOC_CMAKE_LIBRARIES}
    HINTS PATHS $ENV{AML_ROOT}/lib ${AML_JEMALLOC_CMAKE_LIBRARY_DIRS})

#message(AUTHOR_WARNING "Problem is: ${AML_INCLUDE_DIR} -> ${AML_LIBRARY} -> ${AML_JEMALLOC_LIBRARY} ") 

include( 
	FindPackageHandleStandardArgs )

find_package_handle_standard_args(
    AML DEFAULT_MSG
    AML_LIBRARY
    AML_INCLUDE_DIR)

mark_as_advanced(
    AML_INCLUDE_DIR 
    AML_LIBRARY)

set(
    AML_INCLUDE_DIRS ${AML_INCLUDE_DIR})
set(
    AML_LIBRARIES ${AML_LIBRARY})


