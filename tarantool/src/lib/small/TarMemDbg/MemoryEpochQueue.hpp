/**
 ** @file MemoryEpochQueue.hpp
 ** @author Astapov Konstantin
 ** @copyright 2021, Picodata. picodata.io - professional database services
 ** @remark "TaraRam" project - Memory allocation debugger for Tarantool DBMS
 ** \~english @brief
 ** \~english @details
 ** \~russian @brief Файл, описывающий очередь эпох, алгоритм сдвига эпох, их изоляции и запрета доступа к устаревшим эпохам
 ** \~russian @details (на самом деле, не очередь, а циркулярный буфер или скользящее окно в зависимости от флага TARMEMDBG_REUSE_EPOCHS)
 **/

#if !defined(TARMEMDBG_ALLOW_INCLUDE)
#   error Do not include this file manually
#endif

#ifndef    MEMORY_EPOCH_QUEUE_PROTECT_SIGNATURE_2P48TO9AM7DBPB

namespace      TARMEMDBG_NAMESPACE {

template <typename TypeTn>
class SlidingWindowEpochOnDelete : public SlidingWindowOnDeleteInterface<TypeTn> {
 public:
  typedef TypeTn Type;

  virtual void OnDelete( Type & before_delete_value );
};

//template <typename TypeTn, Size BufferSizeTn> class 

/**
 ** @warning при выделении памяти любой наследник должен выделить память через AllocateDerived,
             чтобы тот зарезервировал перед классом страницу памяти и запретил её к доступу. Это делается потому, 
             что указатель на MemoryEpochQueue прячется под видом lsregion * или под видом slab_arena *. 
             Прямой доступ к полям lsregion'а или slab_arena * вызовет катастрофу. 
             Поэтому при создании наследника MemoryEpochInterface выделяется выровненный блок
             размером на страницу больше, чем нужно, первая страница запрещается к записи,а в код тарантула
             отдаётся начало аллоцированного блока, "handle". И при попытке прямого доступа это вызовет segfault
             и проблема будет сразу замечена
 **/
class MemoryEpochQueue {
 public:
  static const constexpr Size kWinSize = 4;
  typedef SlidingWindowEpochOnDelete<MemoryEpochInterface *> OnDeleteFunctor;  
  typedef SlidingWindow< MemoryEpochInterface *, kWinSize > Storage;
  typedef MemoryEpochQueue This;

  static MemoryEpochQueue * Create( Size starting_index );
  ~MemoryEpochQueue() { DeleteAligned<Storage>( epochs_ ); }
  void operator delete( void * to_free ) noexcept;
  
  void NextEpoch();
  MemoryEpochInterface * GetCurrentEpoch();
  
  slab_arena * GetArena() noexcept { return GetCurrentEpoch()->GetArena(); }
  lsregion * GetLsRegion() noexcept { return GetCurrentEpoch()->GetLsRegion(); }

 protected:
  DISALLOW_COPY_MOVE_AND_ASSIGN( MemoryEpochQueue )
  MAKE_NEW_FRENDLY( MemoryEpochQueue )
  //friend void * ::operator new( size_t size, const std::nothrow_t & ) noexcept;
  MemoryEpochQueue() {}

  static MemoryEpochQueue * GetSelfByHandle( void * handle ) noexcept;
  static void * GetHandle( MemoryEpochQueue * self ) noexcept;
  static MemoryEpochLsRegion * GetLsEpoch( MemoryEpochInterface * value );
  static MemoryEpochQueue * AllocateQueue() noexcept;

 private:
  static constexpr const uint64_t kSignature = 0xDEADBEEFc0ffee27;
  const volatile uint64_t signature_ = kSignature;
  Storage * epochs_;
  PtrDiff offset_of_allocated_ = 0;
  Size allocated_byte_size_ = 0;
};
/*
  SlidingWindow( 
      Size starting_index, 
      const Type & initial_value, 
      OnDeleteFunPtr on_delete_value = std::make_shared< SlidingWindowDefaultOnDelete<Type> >() )
*/

} // namespace TARMEMDBG_NAMESPACE

#define    MEMORY_EPOCH_QUEUE_PROTECT_SIGNATURE_2P48TO9AM7DBPB
#endif  // MEMORY_EPOCH_QUEUE_PROTECT_SIGNATURE_2P48TO9AM7DBPB