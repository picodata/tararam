/**
 ** @file StandardIncludes.hpp
 ** @author Astapov Konstantin
 ** @copyright 2021, Picodata. picodata.io - professional database services
 ** @remark "TaraRam" project - Memory allocation debugger for Tarantool DBMS
 ** \~english @brief
 ** \~english @details
 ** \~russian @brief Файл, содержащий стандартные включения плюс включения файлов тарантула
 ** \~russian @details
 **/

#if !defined(TARMEMDBG_ALLOW_INCLUDE)
#   error Do not include this file manually
#endif


// общие включения
#include <cstdint>
#include <array>
#include <vector>
#include <memory>
#include <assert.h>
#include <type_traits>
#include <climits>

// платформозависимые включения
#ifdef     _WIN32
//#   include <sysinfoapi.h>
#   include <windows.h>
static constexpr const size_t MAP_PRIVATE = 0x400000;
typedef ptrdiff_t ssize_t;
#else   // _WIN32
#   include <stdio.h> // ssize_t for Apple
#   include <sys/types.h> // ssize_t
#   include <unistd.h>
#   include <malloc.h>
#   include <sys/mman.h>
#endif  // _WIN32

/// third-party включения - таратнул, small и прочее


#include "../small/rlist.h"
#include "../small/lf_lifo_struct.h"
#include "../small/quota_internal.h"
#include "../small/slab_arena_internal.h"
#include "../small/slab_cache_internal.h"
#include "../small/lsregion_internal.h"