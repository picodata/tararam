cmake_minimum_required(VERSION 3.16)
set( TMD_TEST_MODULE TarMemDbg_Test )
set( TarMemDbg_LibMODULE TarMemDbg )

PROJECT(${TMD_TEST_MODULE} C CXX)
SET (TARMEMDBG_TEST_PROJECT_ROOT "${PROJECT_SOURCE_DIR}")
SET (TARMEMDBG_PROJECT "${PROJECT_SOURCE_DIR}/..")

add_subdirectory(${TARMEMDBG_PROJECT} ${CMAKE_CURRENT_BINARY_DIR}/TMDLib)


#message( WARNING ${TARMEMDBG_PROJECT} )     

add_executable( ${TMD_TEST_MODULE} TarMemDbg_test.cpp)

set_target_properties(${TMD_TEST_MODULE} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS ON
)
target_compile_features(${TMD_TEST_MODULE} INTERFACE cxx_std_20)

find_library(
        libstdc++
        stdc )
#find_library( )

target_link_libraries( ${TMD_TEST_MODULE}
    #${stdc}
    TarMemDbg #${TarMemDbg_LibMODULE}
     )

target_include_directories(${TarMemDbg_LibMODULE}
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${TARMEMDBG_PROJECT}
)
