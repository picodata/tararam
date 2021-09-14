/**
 ** @file TarantoolMemoryDebug.hpp
 ** @author Astapov Konstantin
 ** @copyright 2021, Picodata. picodata.io - professional database services
 ** @remark "TaraRam" project - Memory allocation debugger for Tarantool DBMS
 ** \~english @brief Binds all other files
 ** \~english @details One Ring to rule them all, One Ring to find them,
 **                    One Ring to bring them all and in the darkness bind them.
 ** \~russian @brief Главный файл проекта "TaraRam", собирающий все файлы проекта воедино
 ** \~russian @details Чтобы всех отыскать, воедино собрать, и единою чёрною волей сковать. В Тарантуле, где с памятью вечная тьма.
 **/

#ifndef    TARANTOOL_MEMORY_DEBUG_PROTECT_SIGNATURE_M1F9QADHX986LU
#define    TARANTOOL_MEMORY_DEBUG_PROTECT_SIGNATURE_M1F9QADHX986LU
#   define TARMEMDBG_ALLOW_INCLUDE
#   define TARMEMDBG_NAMESPACE tarmemdbg
//#   define TARMEMDBG_DELETE_EPOCHS_OBJECTS ///< без этого флага наследники MemoryEpochInterface удалятся не будут, а выделенная под них память будет запрещена к доступу
//#   define TARMEMDBG_DELETE_ALL ///< При включении этого флага вся выделенная память будет удалятся
#   define TARMEMDBG_DELETE_MAIN_MEMORY ///< без этого флага выделенные блоки памяти удалятся не будут, а выделенная под них память будет запрещена к доступу

// Вспомогательные файлы
#   include "StandardIncludes.hpp"
#   include "TarMemDbg_Types.hpp"
#   include "Declarations.hpp"
#   include "TarMemDbg_PageSize.hpp"
#   include "TarMemDbg_MemTools.hpp"

// Основной код
#   include "SlidingWindow.hpp"
#   include "SlidingWindow.impl.hpp"
#   include "MemoryEpoch.hpp"
#   include "MemoryEpoch.impl.hpp"
#   include "MemoryEpochQueue.hpp"
#   include "MemoryEpochQueue.impl.hpp"


#   undef  TARMEMDBG_ALLOW_INCLUDE
#endif  // TARANTOOL_MEMORY_DEBUG_PROTECT_SIGNATURE_M1F9QADHX986LU