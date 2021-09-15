/**
 ** @file TarMemDbg_MemTools.hpp
 ** @author Astapov Konstantin
 ** @copyright 2021, Picodata. picodata.io - professional database services
 ** @remark "TaraRam" project - Memory allocation debugger for Tarantool DBMS
 ** \~english @brief
 ** \~english @details
 ** \~russian @brief Файл функций для работы с памятью
 ** \~russian @details
 **/

#if !defined(TARMEMDBG_ALLOW_INCLUDE)
#   error Do not include this file manually
#endif

#ifndef    TARMEMDBG_TOOLS_PROTECT_SIGNATURE_CIY7GVUUK3BJMJ

namespace      TARMEMDBG_NAMESPACE {

static inline bool IsAlignedToMemoryPage( void * ptr, Size page_size ) noexcept {
  return (intptr_t)ptr % page_size == 0;
}
static inline bool IsAlignedToMemoryPage( Size checked_size, Size page_size ) noexcept {
  return checked_size % page_size == 0;
}

/**
 ** @function MemoryProtect
 ** @brief позволяет установить/снять защиту региона памяти
 ** @param[in] aligned_start выровненное по странице начало защищаемого региона памяти (пропорциональная размеру страницы)
 ** @param[in] aligned_byte_size выровненная по странице длина защищаемого региона памяти (пропорциональная размеру страницы)
 ** @param[in] protection - константа, указывающая необходимый тип защиты памяти
 **/
void ProtectMemoryOrDie( 
    void * aligned_start, 
    Size aligned_byte_size, 
    ProtectMemoryConstant protection ) {
  if ( !aligned_byte_size ) return;
  [[maybe_unused]] bool no_error = 1;
  [[maybe_unused]] Size page_size = PageSize()();
  assert(   ( (intptr_t)aligned_start % page_size ) == 0 );
  assert(   ( aligned_byte_size % page_size ) == 0 );
# if      _WIN32
  no_error = (   TRUE == VirtualProtect( aligned_start, aligned_byte_size, protection, NULL )   );
# else // _WIN32
  no_error = mprotect( aligned_start, aligned_byte_size, protection ) >= 0;
# endif    
  assert( no_error );
}

template <typename Tn, typename ... Args> void Construct( Tn & to_construct, Args &&... args ) {
  ::new (&to_construct) Tn( std::forward<Args>(args)... );
}

template <typename Tn> void Destruct( Tn & to_destruct ) {
  to_destruct.~Tn();
}

static inline void * AllocateAligned( Size byte_size, Size alignment ) noexcept {    
# if      _WIN32
  return (void *) _aligned_malloc( byte_size, alignment );
# else // _WIN32
  return (void *) memalign( byte_size, alignment );
# endif
}

static inline void DeallocateAlignedUnsafe( void * to_free ) noexcept {
# if      _WIN32
  _aligned_free( to_free );
# else // _WIN32
  free( to_free );
# endif
}
static inline void DeallocateAligned( void * to_free ) noexcept {
  if ( to_free ) {
    DeallocateAlignedUnsafe( to_free );
  }
}

template <typename Tn, typename... Args> Tn * NewAligned( Args && ... args ) {
  Size page_size = PageSize()();
  Tn * ret = (Tn *)AllocateAligned( sizeof(Tn), page_size );
  if ( ret ) Construct(  *ret, std::forward<Args>( args )...   );
  return ret;
}

template <typename Tn> Tn * NewAlignedInPages() {
  Size page_size = PageSize()();
  Size allocate_size = CalculateSizeInPages<Tn>( page_size );
  Tn * ret = (Tn *)AllocateAligned( allocate_size, page_size );
  if ( ret ) Construct( *ret );
  return ret;
}

template <typename Tn> void DeleteAligned( Tn * to_free ) {
  if ( to_free ) {
    Destruct( to_free );
    DeallocateAlignedUnsafe( to_free );
  }
}


template <typename Tn, typename... Args> Tn * New( Args && ... args ) {  
  return ::new (std::nothrow) Tn(   std::forward<Args>( args )...   );
}

template <typename Tn> void Delete( Tn * to_free ) {
  ::delete( to_free );
}

} // namespace TARMEMDBG_NAMESPACE

#define    TARMEMDBG_TOOLS_PROTECT_SIGNATURE_CIY7GVUUK3BJMJ
#endif  // TARMEMDBG_TOOLS_PROTECT_SIGNATURE_CIY7GVUUK3BJMJ