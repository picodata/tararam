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

typedef std::unique_ptr<MemoryEpochQueue> MemoryEpochQueueUnique;
//std::unique_ptr<::TARMEMDBG_NAMESPACE::MemoryEpochQueue> g_epochs;
std::map<MemoryEpochQueue *, MemoryEpochQueueUnique> g_epochs;
TararamLock g_lock;

struct memory_epoch_queue;

/**
 ** @brief 
 ** @param[in] handle Хэндл от очереди эпох, маскирующийся под lsregion * или slab_arena *.
 **                   Это указатель на очередь эпох, смещённый назад на одну страницу.
 **                   Эта страница запрещена к чтению и записи, и служит гарантией от
 **                   прямого обращения
 **/
static inline MemoryEpoch * GetEpochByHandleUnsafe( memory_epoch_queue * handle ) {
  assert( (bool)handle );
  MemoryEpochQueue * que = MemoryEpochQueue::GetSelfByHandle( handle );
  // полученный указатель мы проверяем. Есть ли такой вообще?
  assert( g_epochs.count( que ) );
  return que->GetCurrentEpoch();
}
static inline MemoryEpoch * GetEpochByHandle( memory_epoch_queue * ptr ) {
  LockGuard lock( g_lock ); {
    return GetEpochByHandleUnsafe( ptr );
  }
}

static inline lsregion * GetLsRegionByHandleUnsafe( memory_epoch_queue * ptr ) {
  return GetEpochByHandleUnsafe( ptr )->GetLsRegion();
}

static inline slab_arena * GetArenaByHandleUnsafe( memory_epoch_queue * ptr ) {
  return GetEpochByHandleUnsafe( ptr )->GetArena();
}

static inline lsregion * GetLsRegionByHandle( memory_epoch_queue * ptr ) {
  LockGuard lock( g_lock ); {
    return GetLsRegionByHandleUnsafe( ptr );
  }
}

static inline slab_arena * GetArenaByHandle( memory_epoch_queue * ptr ) {
  LockGuard lock( g_lock ); {
    return GetArenaByHandleUnsafe( ptr );
  }
}

MemoryEpochQueue * AllocateEpochs() {
  MemoryEpochQueueUnique storage( MemoryEpochQueue::Create( 0 ) );
  assert( (bool)storage );
  MemoryEpochQueue * ret = storage.get();
  assert( (bool)ret );
  g_epochs[ret] = std::move( storage );
  return ret;
}

extern "C" {

int slab_arena_create(
    memory_epoch_queue **arena, 
    quota *quota, 
    size_t prealloc, 
    uint32_t slab_size, 
    int flags ) {
  slab_arena * current = nullptr;
  assert( (bool)arena );
  LockGuard lock( g_lock ); {
    auto * allocated = AllocateEpochs();
    *arena = (memory_epoch_queue*)MemoryEpochQueue::GetHandle( allocated );
    current = allocated->GetCurrentEpoch()->GetArena();
  }
  return slab_arena_create_orig(
      current,
      quota,
      prealloc,
      slab_size,
      flags );  
}

void slab_arena_destroy( [[maybe_unused]] memory_epoch_queue * arena ) {    
  //*arena = nullptr;
}

void * slab_map( slab_arena *arena ) {
  return slab_map_orig(   GetArenaByHandle( (memory_epoch_queue *)arena )   );
}

void slab_unmap( slab_arena *arena, void *ptr ) {
  return slab_unmap_orig(   GetArenaByHandle( (memory_epoch_queue *)arena ), ptr   );
}

void slab_arena_mprotect( slab_arena *arena ) {
  return slab_arena_mprotect_orig(   GetArenaByHandle( (memory_epoch_queue *)arena )   );
}

size_t get_slab_arena_used(struct memory_epoch_queue **arena) {
  assert( (bool)arena );
  return GetArenaByHandle( *arena )->used;
}

struct quota * get_slab_arena_quota(struct memory_epoch_queue **arena) {
  assert( (bool)arena );
  //return GetArenaByHandle( *arena )->quota;
  return get_slab_arena_quota_orig(   GetArenaByHandle( *arena )   );
}

void   lsregion_create( 
    memory_epoch_queue **lsregion_value, 
    slab_arena *arena ) {
  assert( (bool) lsregion_value );
  lsregion *   current_allocator = nullptr;
  slab_arena *current_arena = nullptr;
  LockGuard lock( g_lock ); {
    //AllocateEpochsIfNeed();
    auto * epoch = GetEpochByHandleUnsafe( (memory_epoch_queue *)arena );
    assert( epoch->CheckIfThisIsReallyMemoryEpoch() );
    current_allocator = epoch->GetLsRegion();
    *lsregion_value = (memory_epoch_queue *)current_allocator;
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
      GetLsRegionByHandle( (memory_epoch_queue *)lsregion_value ),
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
      GetLsRegionByHandle( (memory_epoch_queue *)lsregion_value ),
      size,
      alignment,
      unaligned   );
}

void * lsregion_reserve(
    lsregion *lsregion_value, 
    size_t size ) {
  return lsregion_reserve_orig(
      GetLsRegionByHandle( (memory_epoch_queue *)lsregion_value ),
      size   );
}

void * lsregion_alloc(
    lsregion *lsregion_value, 
    size_t size, 
    int64_t id ) {
  return lsregion_alloc_orig( 
      GetLsRegionByHandle( (memory_epoch_queue *)lsregion_value ),
      size,
      id   );
}

void * lsregion_aligned_alloc(
    lsregion *lsregion_value, 
    size_t size, 
    size_t alignment, 
    int64_t id ) {
  return lsregion_aligned_alloc_orig( 
      GetLsRegionByHandle( (memory_epoch_queue *)lsregion_value ),
      size,
      alignment,
      id   );
}

void   lsregion_gc(
    lsregion *lsregion_value, 
    [[maybe_unused]] int64_t min_id ) {
  LockGuard lock( g_lock ); {
    auto * que = MemoryEpochQueue::GetSelfByHandle( (memory_epoch_queue *)lsregion_value );
    assert( (uint64_t)min_id == (uint64_t)que->GetPositionOrMaxId() );  
    que->NextEpoch();
  }
}

void   lsregion_destroy( memory_epoch_queue **lsregion_value ) {
    *lsregion_value = nullptr;
}

size_t lsregion_used_internal( memory_epoch_queue *lsregion_value ) {
  return lsregion_used_orig(   GetLsRegionByHandle( (memory_epoch_queue *)lsregion_value )   );
}

size_t lsregion_used_main(void * handle) {
  bool arg_type = 0;
  {
    LockGuard lock( g_lock ); {    
      MemoryEpochQueue * que = MemoryEpochQueue::GetSelfByHandleNoChecks( handle );
      // аргумент - lsregion * ?
      arg_type = (   g_epochs.count( que ) > 0   );
    }
  }
  return ( arg_type ) ? (   lsregion_used_internal(   (memory_epoch_queue *)handle  )    ) :
                        (   lsregion_used_internal( *((memory_epoch_queue**)handle) )   );
}

size_t lsregion_total( const lsregion **lsregion_value ) {
  return lsregion_total_orig(   GetLsRegionByHandle( (memory_epoch_queue *)lsregion_value )   );
}

struct lsregion * get_lsregion( memory_epoch_queue ** lsregion_value ) {
  //return (lsregion *)GetEpochByHandle( *lsregion_value );
  return (lsregion *)( *lsregion_value );
}
struct slab_arena * get_slab_arena( struct memory_epoch_queue ** arena ) {
  //return (slab_arena *)GetEpochByHandle( *arena );
  return (slab_arena *)( *arena );
}

void slab_cache_create(struct slab_cache *cache, struct memory_epoch_queue ** arena) {
  slab_cache_create_orig(   cache, (slab_arena *)GetEpochByHandle( *arena )  );
}

} // extern C