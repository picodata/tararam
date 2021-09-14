/**
 ** @file   Declarations.hpp
 ** @author Astapov Konstantin
 ** @copyright 2021, Picodata. picodata.io - professional database services
 ** @remark "TaraRam" project - Memory allocation debugger for Tarantool DBMS
 ** \~english @brief
 ** \~english @details
 ** \~russian @brief Файл объявлений, деклараций, прототипов и алиасов
 ** \~russian @details В идеале включает в себя декларации всех функций и классов.
 **/

#if !defined(TARMEMDBG_ALLOW_INCLUDE)
#   error Do not include this file manually
#endif

#ifndef    DECLARATIONS_PROTECT_SIGNATURE_ZGOMVUOG3SF5M7
#define    DECLARATIONS_PROTECT_SIGNATURE_ZGOMVUOG3SF5M7

#   define DISALLOW_COPY_MOVE_AND_ASSIGN( Type )     \
        Type (                  Type && ) = delete;  \
        Type && operator=(      Type && ) = delete;  \
        Type(             const Type  & ) = delete;  \
        Type(                   Type  & ) = delete;  \
        Type & operator=( const Type  & ) = delete;  \
        Type & operator=(       Type  & ) = delete;
#   define MAKE_NEW_FRENDLY( ThisType ) \
        template <typename Type, typename... Args> friend Type * New              (Args &&...); \
        template <typename Type, typename... Args> friend Type * NewAligned       (Args &&...); \
        template <typename Type, typename... Args> friend Type * NewAlignedInPages(Args &&...); \
        template <typename Type, typename... Args> friend void  Construct( Type &, Args &&...); \
        friend void* ::operator new (std::size_t size, void* ptr) noexcept;

#   define TARMEMDBG_REUSE_EPOCHS 1 ///< Если 0, то старые эпохи удаляются, если 1 - то переиспользуются
#   define TARMEMDBG_REUSE_LAZY_CLEAN 1 ///< Если 0, то зачистка lsregion_gc проводится сразу после дампа, если 1 - то только перед удалением/переиспользованием эпохи

namespace      TARMEMDBG_NAMESPACE {

// MemTools - инструменты для работы с памятью

enum ProtectMemoryConstant : Size {
# if      _WIN32
  kProtectNone = PAGE_NOACCESS,
  kProtectRead = PAGE_READONLY,
  kProtectReadWrite = PAGE_READWRITE,  
# else // _WIN32
  kProtectNone = PROT_NONE,
  kProtectRead = PROT_READ,
  kProtectReadWrite = PROT_WRITE | PROT_READ,
# endif
};

static inline bool IsAlignedToMemoryPage( void * ptr, Size page_size ) noexcept;
static inline bool IsAlignedToMemoryPage( Size checked_size, Size page_size ) noexcept;

void ProtectMemoryOrDie( 
    void * aligned_start, 
    Size aligned_byte_size, 
    ProtectMemoryConstant protection );
template <typename Tn, typename ... Args> void Construct( Tn & to_construct, Args &&... args );
template <typename Tn> void Destruct( Tn & to_destruct );
static inline void * AllocateAligned( Size byte_size, Size alignment ) noexcept;
static inline void DeallocateAlignedUnsafe( void * to_free ) noexcept;
static inline void DeallocateAligned( void * to_free ) noexcept;
template <typename Tn, typename... Args> Tn * NewAligned( Args && ... args );
template <typename Tn> Tn * NewAlignedInPages();
template <typename Tn> void DeleteAligned( Tn * to_free );
template <typename Tn, typename... Args> Tn * New( Args && ... args );
template <typename Tn> void Delete( Tn * to_free );

// основной код

template <typename TypeTn, Size BufferSizeTn> class SlidingWindow;
struct LargeMemoryBlock;
class MemoryEpochInterface;
class MemoryEpochLsRegion;
template <typename Tn> inline Size CalculateSizeInPages( Size page_size ) noexcept;
template <typename Tn> Tn * AllocateWithForbiddenPageAtStart( 
    Size & out_allocated_size,
    Size & out_page_size ) noexcept;
void DeallocateWithForbiddenPageAtStart( void * to_free, Size allocated_size ) noexcept;

template <typename TypeTn> class SlidingWindowEpochOnDelete;
class Type;

} // namespace TARMEMDBG_NAMESPACE

#endif  // DECLARATIONS_PROTECT_SIGNATURE_ZGOMVUOG3SF5M7

/*
#if !defined(TARMEMDBG_ALLOW_INCLUDE)
#   error Do not include this file manually
#endif

#ifndef    _PROTECT_SIGNATURE_

namespace      TARMEMDBG_NAMESPACE {

} // namespace TARMEMDBG_NAMESPACE

#define    _PROTECT_SIGNATURE_
#endif  // _PROTECT_SIGNATURE_
*/