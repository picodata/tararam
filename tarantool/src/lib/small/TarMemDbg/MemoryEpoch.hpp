/**
 ** @file MemoryEpoch.hpp
 ** @author Astapov Konstantin
 ** @copyright 2021, Picodata. picodata.io - professional database services
 ** @remark "TaraRam" project - Memory allocation debugger for Tarantool DBMS
 ** \~english @brief
 ** \~english @details
 ** \~russian @brief Файл, описывающий эпоху памяти и частично функции её изоляции (т.е. закрытия доступа к ней)
 ** \~russian @details
 **/

#if !defined(TARMEMDBG_ALLOW_INCLUDE)
#   error Do not include this file manually
#endif

#ifndef    MEMORY_EPOCH_PROTECT_SIGNATURE_KFJ8TR4B03T5ZN
#define    MEMORY_EPOCH_PROTECT_SIGNATURE_KFJ8TR4B03T5ZN


namespace      TARMEMDBG_NAMESPACE {

/**
 ** @brief Структура, созданная для контроля больших блоков (хранения информации о них). 
 ** @details Small в некоторых случаях выделяет память malloc'ом, минуя lsregion и другие аллокаторы
 **        если этого не учитывать, то такая память "выпадет" из схемы изоляции эпох и изолирована
 **        не будет
 **/
struct LargeMemoryBlock {
  LargeMemoryBlock( void * address_val, Size alloc_size_val )
      :  address( address_val ),
         allocated_bytesize( alloc_size_val ) {             
  }

  inline void Protect( ProtectMemoryConstant protect_type );
  inline void Deallocate() noexcept;
  inline void Check( Size page_size ) noexcept;

  void * address = nullptr;
  Size allocated_bytesize = 0;
};
    
class MemoryEpochInterface {
 public:
  typedef std::vector< LargeMemoryBlock > LMBStorage;

  virtual ~MemoryEpochInterface() { signature_ = 0; }
  
  void operator delete( void * ptr ) noexcept;

  slab_arena * GetArena() noexcept { return GetArena_(); }
  lsregion * GetLsRegion() noexcept { return GetLsRegion_(); }
  void * AllocateLargeMemoryBlock( Size byte_size ) noexcept { return AllocateLargeMemoryBlock_( byte_size ); }
  void ProtectEpoch( ProtectMemoryConstant protect_type ) noexcept { ProtectEpoch_( protect_type ); }

  template <typename DerivedTn>  static MemoryEpochInterface * AllocateDerived() noexcept;
  //static slab_arena * GetArenaByHandle( void * handle ) noexcept;
  //static lsregion * GetLsRegionByHandle( void * handle ) noexcept;
  bool CheckIfThisIsReallyMemoryEpoch() { return signature_ == kSignature; }


 protected:
  virtual slab_arena * GetArena_() = 0;
  virtual lsregion * GetLsRegion_() = 0;
  virtual void * AllocateLargeMemoryBlock_( Size byte_size ) = 0;
  virtual void ProtectEpoch_( ProtectMemoryConstant protect_type ) = 0;

  //static MemoryEpochInterface * GetSelfByHandle( void * handle ) noexcept;
  //static void * GetHandle( MemoryEpochInterface * self ) noexcept;
  static void ProtectArena( 
      slab_arena * arena, 
      ProtectMemoryConstant protect_type );
  static void ProtectLsRegion(
      lsregion * lsallocator, 
      ProtectMemoryConstant protect_type );
      
  static void ProtectLsRegionCache(
      lsregion * lsallocator, 
      ProtectMemoryConstant protect_type );
  static void ProtectLsRegionSlabs(
      lsregion * lsallocator, 
      ProtectMemoryConstant protect_type );
  static void ProtectRlistOfLslabs(
      rlist * head, 
      Size page_size,
      Size lslab_size,
      ProtectMemoryConstant protect_type );
  static void ProtectLargeMemoryBlocks( 
      LMBStorage & large_blocks, 
      ProtectMemoryConstant protect_type );
  static void DeallocateLargeMemoryBlocks( LMBStorage & large_blocks ) noexcept;

 private:
  static constexpr const uint64_t kSignature = 0xDEADBEEFc0ffee72; // мёртвое мясо кофе 72
  volatile uint64_t signature_ = kSignature;
  //Size page_size_ = PageSize::kInitialPageSize;
};

class MemoryEpochLsRegion : public MemoryEpochInterface {
 public:
  static MemoryEpochLsRegion * Create();
  ~MemoryEpochLsRegion() noexcept;


 protected:
  MAKE_NEW_FRENDLY( MemoryEpochLsRegion )
  MemoryEpochLsRegion()
      :  arena_( nullptr ),
         lsregion_( nullptr ) {              
  }

  virtual slab_arena * GetArena_() override { return arena_; }
  virtual lsregion * GetLsRegion_() override { return lsregion_; }
  virtual void * AllocateLargeMemoryBlock_( Size byte_size ) override;
  void operator delete( void * ptr ) noexcept;
  virtual void ProtectEpoch_( ProtectMemoryConstant protect_type ) override;

  private:
   slab_arena * arena_;
   lsregion * lsregion_;
   LMBStorage large_blocks_;
};

} // namespace TARMEMDBG_NAMESPACE

#endif  // MEMORY_EPOCH_PROTECT_SIGNATURE_KFJ8TR4B03T5ZN