/**
 ** @file MemoryEpoch.impl.hpp
 ** @author Astapov Konstantin
 ** @copyright 2021, Picodata. picodata.io - professional database services
 ** @remark "TaraRam" project - Memory allocation debugger for Tarantool DBMS
 ** \~english @brief
 ** \~english @details
 ** \~russian @brief Файл, содержащий определения функций для @ref "эпох памяти" MemoryEpoch.hpp
 ** \~russian @details
 **/

#if !defined(TARMEMDBG_ALLOW_INCLUDE)
#   error Do not include this file manually
#endif

#ifndef    MEMORY_EPOCH_IMPL_PROTECT_SIGNATURE_J2F9RLA9UAMUW7

namespace      TARMEMDBG_NAMESPACE {

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Вспомогательные функции                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

template <typename Tn> inline Size 
CalculateSizeInPages( Size page_size )  noexcept {
  return ( sizeof(Tn) - 1 + page_size ) & ( page_size - 1 );
}

/**
 ** @brief аллоцируем память под объект, оставляя перед ним страницу запрещённой памяти
 ** @param[out] out_allocated_size
 ** @param[out] out_page_size
 **/
template <typename Tn> Tn * AllocateWithForbiddenPageAtStart( 
    Size & out_allocated_size,
    Size & out_page_size ) noexcept {
  out_page_size = PageSize()();
  // выделяем память размером в байтах, кратно страницам. добавляем одну страница перед объектом
  out_allocated_size = (   CalculateSizeInPages<Tn>( out_page_size )   ) + out_page_size;
  Byte * start_block_address = (Byte *)AllocateAligned( out_allocated_size, out_page_size );
  if ( start_block_address == nullptr ) return nullptr;
  assert( (bool)out_page_size );
  assert( (bool)out_allocated_size );
  // запрещаем первую страницу блока памяти
  ProtectMemoryOrDie( start_block_address, out_page_size, kProtectNone );
  return (Tn *)(start_block_address + out_page_size);
}

void DeallocateWithForbiddenPageAtStart( void * to_free, Size allocated_size ) noexcept {
  Size page_size = PageSize()();
  Byte * address_of_block = (Byte *)to_free - page_size;
  // запрещаем весь блок памяти
  ProtectMemoryOrDie( address_of_block, allocated_size, kProtectNone );
  // если не заданы специальные макро-ключи, ничего не удаляем. Блок висит в памяти запрещённым
  // и отлавливает повторное обращение к нему
# if ( defined(TARMEMDBG_DELETE_EPOCHS_OBJECTS) || defined(TARMEMDBG_DELETE_ALL) )
  DeallocateAligned( address_of_block );  
# endif  
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// LargeMemoryBlock                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

void LargeMemoryBlock::Protect( ProtectMemoryConstant protect_type ) {
  Size page_size = PageSize()();
  Check( page_size );
  ProtectMemoryOrDie( address, allocated_bytesize, protect_type );
}
void LargeMemoryBlock::Deallocate() noexcept {
  Size page_size = PageSize()();
  Check( page_size );
  DeallocateAligned( address );
}
void LargeMemoryBlock::Check( [[maybe_unused]] Size page_size ) noexcept {
  assert(   ( (intptr_t)address % page_size ) == 0 );
  assert(   ( allocated_bytesize % page_size ) == 0 );
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// MemoryEpochInterface                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//slab_arena * MemoryEpochInterface::GetArenaByHandle( void * handle ) noexcept {
//  return GetSelfByHandle( handle )->GetArena_();
//}
//lsregion * MemoryEpochInterface::GetLsRegionByHandle( void * handle ) noexcept {    
//  return GetSelfByHandle( handle )->GetLsRegion_();
//}

template <typename DerivedTn> MemoryEpochInterface * 
MemoryEpochInterface::AllocateDerived() noexcept {
  static_assert( std::is_base_of< MemoryEpochInterface, DerivedTn>::value, "Template argument must be inherited from MemoryEpochInterface" );
  Size allocated_bytes = 0, page_size = PageSize::kInitialPageSize;
  auto ret = (DerivedTn *)AllocateAligned( allocated_bytes, page_size );
  assert( (bool)ret );
  return ret;
}

//MemoryEpochInterface * MemoryEpochInterface::GetSelfByHandle( void * handle ) noexcept {
//  return (MemoryEpochInterface *)( reinterpret_cast<Byte *>(handle) + PageSize()() );
//}
//void * MemoryEpochInterface::GetHandle( MemoryEpochInterface * self ) noexcept {
//  return (void *)( reinterpret_cast<Byte *>(self) - PageSize()() );
//}

void MemoryEpochInterface::operator delete( void * to_free ) noexcept {  
  if ( !to_free ) return;
  //MemoryEpochInterface * object = (MemoryEpochInterface*)to_free;
  DeallocateAligned( to_free );
  //DeallocateWithForbiddenPageAtStart( GetHandle( object ), object->allocated_byte_size_ );
}

void MemoryEpochInterface::ProtectArena( 
    slab_arena * arena, 
    ProtectMemoryConstant protect_type ) {
  Size page_size = PageSize()();
  assert( (bool)arena );
  assert(   IsAlignedToMemoryPage( arena, page_size )   );
  Size arena_size = CalculateSizeInPages<slab_arena>( page_size );
  // Страхуемся от конкурентного доступа + обеспечиваем права чтения, если их нет.
  // Для этого память под арену мы выделили целыми страницами
  ProtectMemoryOrDie( arena, arena_size, kProtectRead ); 
  //assert( (bool)arena->cache );
  assert( (bool)arena->slab_size );
  assert(   IsAlignedToMemoryPage( arena->slab_size, page_size )   );
  Size slab_size = arena->slab_size;

  lf_lifo * ptr = &arena->cache, *next;
  while ( !ptr ) {
    assert(   IsAlignedToMemoryPage( ptr, page_size )   );
    ProtectMemoryOrDie( ptr, slab_size, kProtectRead ); // даём себе права на чтение, если вдруг их нет
    next = (lf_lifo *) ptr->next;
    ProtectMemoryOrDie( ptr, slab_size, protect_type );
    ptr = next;
  }
  ProtectMemoryOrDie( arena, arena_size, protect_type );
}

void MemoryEpochInterface::ProtectLsRegion(
    lsregion * lsallocator, 
    ProtectMemoryConstant protect_type ) {
  assert( (bool)lsallocator );
  Size page_size = PageSize()();
  Size lspage_size = CalculateSizeInPages<lsregion>( page_size );
  assert(   IsAlignedToMemoryPage( lsallocator, page_size )   );
  // Страхуемся от конкурентного доступа + обеспечиваем права чтения, если их нет.
  // Для этого память под арену мы выделили целыми страницами
  ProtectMemoryOrDie( lsallocator, lspage_size, kProtectRead );
  ProtectLsRegionCache( lsallocator, protect_type );
  ProtectLsRegionSlabs( lsallocator, protect_type );
  ProtectMemoryOrDie( lsallocator, lspage_size, protect_type );
}

void MemoryEpochInterface::ProtectLsRegionCache(
    lsregion * lsallocator, 
    ProtectMemoryConstant protect_type ) {
  Size page_size = PageSize()();
  rlist * head = &lsallocator->cached->next_in_list;
  Size lslab_size = CalculateSizeInPages<lslab>( page_size );
  ProtectRlistOfLslabs( head, page_size, lslab_size, protect_type );
}

void MemoryEpochInterface::ProtectLsRegionSlabs(
    lsregion * lsallocator, 
    ProtectMemoryConstant protect_type ) {
  Size page_size = PageSize()();
  rlist * head = &lsallocator->slabs.slabs;
  Size lslab_size = CalculateSizeInPages<lslab>( page_size );
  ProtectRlistOfLslabs( head, page_size, lslab_size, protect_type );
}

void MemoryEpochInterface::ProtectRlistOfLslabs(
    rlist * head, 
    [[maybe_unused]] Size page_size,
    Size lslab_size,
    ProtectMemoryConstant protect_type ) {
  rlist * ptr = rlist_first( head );
  Size current_slab_size = 0;
  while ( ptr != head ) {
    assert( (bool)ptr );
    assert(   IsAlignedToMemoryPage( ptr, page_size )   );
    lslab * slab = rlist_entry( ptr, lslab, next_in_list );
    ProtectMemoryOrDie( slab, lslab_size, kProtectRead );    
    ptr = rlist_next( &slab->next_in_list ); // получаем следующее значение пока возможность чтения slab'а гарантирована
    current_slab_size = slab->slab_size;
    ProtectMemoryOrDie( slab, current_slab_size, protect_type );
  }
}
/*struct lslab *slab, *next;
rlist_foreach_entry_safe(slab, &lsregion->slabs.slabs, next_in_list, next);
rlist_foreach_entry_safe(item, head, member, tmp) {
	for (  (item) = rlist_first_entry((head), typeof(*item), member);	
	       &item->member != (head) && ((tmp) = rlist_next_entry((item), member));                
	       (item) = (tmp)
      ) {
    <...>
  }
}
rlist_entry(item, type, member) {				
	const typeof( ((type *)0)->member ) *__mptr = (item);		
	(type *)( (char *)__mptr - offsetof(type,member));		
}
rlist_next_entry(item, member)	{
	rlist_entry(rlist_next(&(item)->member), typeof(*item), member)
}
*/


//inline static struct rlist * rlist_first(struct rlist *head);
//inline static struct rlist * rlist_last(struct rlist *head);
//inline static struct rlist * rlist_next(struct rlist *item);

void MemoryEpochInterface::ProtectLargeMemoryBlocks( 
    LMBStorage & large_blocks_pool, 
    ProtectMemoryConstant protect_type ) {
  for ( auto & large_block : large_blocks_pool ) {
    large_block.Protect( protect_type );
  }
}

void MemoryEpochInterface::DeallocateLargeMemoryBlocks( LMBStorage & large_blocks_pool ) noexcept {
  for ( auto & large_block : large_blocks_pool ) {
    large_block.Deallocate();
  }
  large_blocks_pool.clear();
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// MemoryEpochLsRegion                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


void MemoryEpochLsRegion::operator delete( void * to_free ) noexcept {
  MemoryEpochInterface::operator delete( to_free );
}

MemoryEpochLsRegion * MemoryEpochLsRegion::Create() {
  MemoryEpochLsRegion * ret = (MemoryEpochLsRegion *)
                                  MemoryEpochInterface::AllocateDerived<MemoryEpochLsRegion>();
  Construct( *ret );
  // Размер структур не только выравнен по странице, но и аллоцируем размер кратный странице.
  // Это нужно для того, чтобы перед манипуляциями с этими структурами запретить их модификацию
  // одним mprotect'ом
  ret->arena_ = NewAlignedInPages<slab_arena>();
  ret->lsregion_ = NewAlignedInPages<lsregion>();
  
  static struct quota runtime_quota;
  static const constexpr size_t SLAB_SIZE = 4 * 1024 * 1024;
  quota_init(&runtime_quota, QUOTA_MAX);
  slab_arena_create_orig( ret->arena_, &runtime_quota, 0, SLAB_SIZE, MAP_PRIVATE );

  lsregion_create_orig( ret->lsregion_, ret->arena_ );

  return ret;
}

MemoryEpochLsRegion::~MemoryEpochLsRegion() noexcept {
  lsregion_destroy_orig( lsregion_ );
  slab_arena_destroy_orig( arena_ );
  DeleteAligned( arena_ );
  DeleteAligned( lsregion_ );
}

void MemoryEpochLsRegion::ProtectEpoch_( ProtectMemoryConstant protect_type ) {
  ProtectArena( arena_, protect_type );
  ProtectLsRegion( lsregion_, protect_type );
  ProtectLargeMemoryBlocks( large_blocks_, protect_type );
}

} // namespace TARMEMDBG_NAMESPACE

#define    MEMORY_EPOCH_IMPL_PROTECT_SIGNATURE_J2F9RLA9UAMUW7
#endif  // MEMORY_EPOCH_IMPL_PROTECT_SIGNATURE_J2F9RLA9UAMUW7