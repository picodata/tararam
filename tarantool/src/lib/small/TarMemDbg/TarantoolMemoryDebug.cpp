/**
 ** @file TarantoolMemoryDebug.cpp
 ** @author Astapov Konstantin
 ** @copyright 2021, Picodata. picodata.io - professional database services
 ** @remark "TaraRam" project - Memory allocation debugger for Tarantool DBMS
 ** \~english @brief
 ** \~english @details
 ** \~russian @brief Единственная единица трансляции (единственный .cpp'шник) в библиотеке проекта TaraRam
 ** \~russian @details
 **/

#include "TarantoolMemoryDebug.hpp"

std::unique_ptr< TARMEMDBG_NAMESPACE::MemoryEpochQueue> g_epochs;