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

typedef ::TARMEMDBG_NAMESPACE::Mutex TararamLock; // SpinLock подошёл бы лучше, но за неимением гербовой... К тому же, поначалу мьютекс почти спинлок
typedef ::TARMEMDBG_NAMESPACE::MemoryEpochQueue     MemoryEpochQueue;
typedef ::TARMEMDBG_NAMESPACE::MemoryEpochInterface MemoryEpoch     ;
typedef ::TARMEMDBG_NAMESPACE::LockGuard            LockGuard       ;

std::unique_ptr<::TARMEMDBG_NAMESPACE::MemoryEpochQueue> g_epochs;
TararamLock g_lock;

/**
 ** @brief 
 ** @param[in] handle Хэндл от очереди эпох, маскирующийся под lsregion * или slab_arena *.
 **                   Это указатель на очередь эпох, смещённый назад на одну страницу.
 **                   Эта страница запрещена к чтению и записи, и служит гарантией от
 **                   прямого обращения
 **/
static inline MemoryEpoch * GetEpochByHandleUnsafe( void * handle ) {
  assert( (bool)handle );
  assert( (bool)g_epochs );
  MemoryEpochQueue * que = MemoryEpochQueue::GetSelfByHandle( handle );
  // Мы сохраняем теоретическую возможность того, что очередей эпох будет несколько
  // Поэтому интерфейс у нас передаёт указатель на очередь эпох, хотя мог бы сразу взять
  // результат из *g_epochs.
  // Но мы-то знаем, что очередь эпох у нас одна, и это *g_epochs
  // поэтому полученный указатель мы проверяем на то, равен ли он *g_epochs
  assert( que == g_epochs.get() );
  return que->GetCurrentEpoch();
}
static inline MemoryEpoch * GetEpochByHandle( void * ptr ) {
  LockGuard lock( g_lock ); {
    return GetEpochByHandleUnsafe( ptr );
  }
}

static inline lsregion * GetLsRegionByHandleUnsafe( void * ptr ) {
  return GetEpochByHandleUnsafe( ptr )->GetLsRegion();
}

static inline slab_arena * GetArenaByHandleUnsafe( void * ptr ) {
  return GetEpochByHandleUnsafe( ptr )->GetArena();
}

static inline lsregion * GetLsRegionByHandle( void * ptr ) {
  LockGuard lock( g_lock ); {
    return GetLsRegionByHandleUnsafe( ptr );
  }
}

static inline slab_arena * GetArenaByHandle( void * ptr ) {
  LockGuard lock( g_lock ); {
    return GetArenaByHandleUnsafe( ptr );
  }
}

void AllocateEpochsIfNeed() {
  if ( !g_epochs ) {
    g_epochs.reset( MemoryEpochQueue::Create( 0 ) );
  }
  assert( (bool)g_epochs );
}

extern "C" {

int slab_arena_create(
    slab_arena **arena, 
    quota *quota, 
    size_t prealloc, 
    uint32_t slab_size, 
    int flags ) {
  slab_arena * current = nullptr;
  assert( (bool)arena );
  LockGuard lock( g_lock ); {
    AllocateEpochsIfNeed();
    *arena = (slab_arena*)MemoryEpochQueue::GetHandle( g_epochs.get() );
    current = g_epochs->GetCurrentEpoch()->GetArena();
  }
  return slab_arena_create_orig(
      current,
      quota,
      prealloc,
      slab_size,
      flags );  
}

void slab_arena_destroy( slab_arena **arena ) {    
  *arena = nullptr;
}

void * slab_map( slab_arena *arena ) {
  return slab_map_orig(   GetArenaByHandle( arena )   );
}

void slab_unmap( slab_arena *arena, void *ptr ) {
  return slab_unmap_orig(   GetArenaByHandle( arena ), ptr   );
}

void slab_arena_mprotect( slab_arena *arena ) {
  return slab_arena_mprotect_orig(   GetArenaByHandle( arena )   );
}

void   lsregion_create( 
    lsregion **lsregion_value, 
    slab_arena *arena ) {
  assert( (bool) lsregion_value );
  lsregion *   current_allocator = nullptr;
  slab_arena *current_arena = nullptr;
  LockGuard lock( g_lock ); {
    AllocateEpochsIfNeed();
    auto * epoch = GetEpochByHandleUnsafe( arena );
    assert( epoch->CheckIfThisIsReallyMemoryEpoch() );
    current_allocator = epoch->GetLsRegion();
    *lsregion_value = current_allocator;
    current_arena = epoch->GetArena();
  }
  assert( (bool)current_allocator );
  assert( (bool)current_arena );
  lsregion_create_orig( current_allocator, current_arena );
}

void * lsregion_aligned_reserve_slow(
    lsregion *lsregion_value, 
    size_t size, 
    size_t alignment, 
    void **unaligned ) {
  return lsregion_aligned_reserve_slow_orig(
      GetLsRegionByHandle( lsregion_value ),
      size,
      alignment,
      unaligned   );
}

void * lsregion_aligned_reserve(
    lsregion *lsregion_value, 
    size_t size, 
    size_t alignment, 
    void **unaligned ) {
  return lsregion_aligned_reserve_orig(
      GetLsRegionByHandle( lsregion_value ),
      size,
      alignment,
      unaligned   );
}

void * lsregion_reserve(
    lsregion *lsregion_value, 
    size_t size ) {
  return lsregion_reserve_orig(
      GetLsRegionByHandle( lsregion_value ),
      size   );
}

void * lsregion_alloc(
    lsregion *lsregion_value, 
    size_t size, 
    int64_t id ) {
  return lsregion_alloc_orig( 
      GetLsRegionByHandle( lsregion_value ),
      size,
      id   );
}

void * lsregion_aligned_alloc(
    lsregion *lsregion_value, 
    size_t size, 
    size_t alignment, 
    int64_t id ) {
  return lsregion_aligned_alloc_orig( 
      GetLsRegionByHandle( lsregion_value ),
      size,
      alignment,
      id   );
}

void   lsregion_gc(
    lsregion *lsregion_value, 
    int64_t min_id ) {
  LockGuard lock( g_lock ); {
    auto * que = MemoryEpochQueue::GetSelfByHandle( lsregion_value );
    assert( (uint64_t)min_id == (uint64_t)que->GetPositionOrMaxId() );  
    que->NextEpoch();
  }
}

void   lsregion_destroy( lsregion **lsregion_value ) {
    *lsregion_value = nullptr;
}

size_t lsregion_used( const lsregion *lsregion_value ) {
  return lsregion_used_orig(   GetLsRegionByHandle( (void *)lsregion_value )   );
}

size_t lsregion_total( const lsregion *lsregion_value ) {
  return lsregion_total_orig(   GetLsRegionByHandle( (void *)lsregion_value )   );
}

} // extern C