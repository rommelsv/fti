add_executable(check.exe check.c)
target_link_libraries(check.exe fti.static ${MPI_C_LIBRARIES} m)
set_property(TARGET check.exe APPEND PROPERTY COMPILE_FLAGS ${MPI_C_COMPILE_FLAGS})
set_property(TARGET check.exe APPEND PROPERTY LINK_FLAGS ${MPI_C_LINK_FLAGS})

if(ENABLE_SIONLIB)
    set(IO 3)
    set(IO_NAMES "POSIX MPIIO SIONLIB")
else()
    set(IO 2)
    set(IO_NAMES "POSIX MPIIO")
endif()

configure_file(run-checks.in ${CMAKE_CURRENT_SOURCE_DIR}/run-checks.sh @ONLY)

install(DIRECTORY DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Local)
install(DIRECTORY DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Global)
install(DIRECTORY DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Meta)
install(DIRECTORY DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/chk)

file(COPY Global Local Meta chk DESTINATION .)

file(INSTALL run-checks.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE)

file(REMOVE run-checks.sh)