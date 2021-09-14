##
 # @file CMakeLists.txt
 # @author Astapov Konstantin
 # @copyright 2021, Picodata. picodata.io - professional database services
 # @remark "TaraRam" project - Memory allocation debugger for Tarantool DBMS
 # \~english @brief root CMakeLists.txt of project "TaraRam"
 # \~english @details
 # \~russian @brief 
 # \~russian @details
##

cmake_minimum_required(VERSION 3.16)   
set( TarDbgMODULE TarMemDbg )

PROJECT(${TarDbgMODULE} C CXX)
SET (TARMEMDBG_PROJECT_ROOT "${PROJECT_SOURCE_DIR}")
                           

add_library( ${TarDbgMODULE} STATIC ${TARMEMDBG_PROJECT_ROOT}/TarantoolMemoryDebug.cpp )


set_target_properties(${TarDbgMODULE} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS ON
)
target_compile_features(${TarDbgMODULE} INTERFACE cxx_std_20)

target_link_libraries(${TarDbgMODULE} stdc)

#target_include_directories(LibName
#        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
#        PRIVATE MyPath
#)

set( TarMemDbg_small_headers 
     ../small/rlist.h
     ../small/lf_lifo_struct.h
     ../small/quota_internal.h
     ../small/slab_arena_internal.h
     ../small/slab_cache_internal.h
     ../small/lsregion_internal.h
   )

set( TarMemDbg_headers 
         Declarations.hpp 
         StandardIncludes.hpp 
         TarMemDbg_Types.hpp
         TarMemDbg_MemTools.hpp
         TarMemDbg_PageSize.hpp
         SlidingWindow.hpp SlidingWindow.impl.hpp
         MemoryEpoch.hpp MemoryEpoch.impl.hpp
         MemoryEpochQueue.hpp  MemoryEpochQueue.impl.hpp
         TarantoolMemoryDebug.hpp 
   )
  # это хак, который позволяет выводить заголовочники в IDE
add_custom_target(TarMemDbg_HeadersFiles SOURCES ${TarMemDbg_headers})
add_custom_target(TarMemDbg_SmallHeaders SOURCES ${TarMemDbg_small_headers})