/**
 ** @file SlidingWindow.hpp
 ** @author Astapov Konstantin
 ** @copyright 2021, Picodata. picodata.io - professional database services
 ** @remark "TaraRam" project - Memory allocation debugger for Tarantool DBMS
 ** \~english @brief
 ** \~english @details
 ** \~russian @brief Файл, содержащий класс скользящего окна объектов, способного работать и как циркулярный буфер
 ** \~russian @details
 **/

#if !defined(TARMEMDBG_ALLOW_INCLUDE)
#   error Do not include this file manually
#endif

#ifndef    SLIDING_WINDOW_PROTECT_SIGNATURE_OT0T280ONCVQ40
#define    SLIDING_WINDOW_PROTECT_SIGNATURE_OT0T280ONCVQ40


namespace      TARMEMDBG_NAMESPACE {

template <typename TypeTn>
class SlidingWindowOnDeleteInterface {
 public:
  typedef TypeTn Type;

  virtual ~SlidingWindowOnDeleteInterface() {}
  virtual void OnDelete( Type & before_delete_value ) = 0;
};

template <typename TypeTn>
class SlidingWindowDefaultOnDelete : public SlidingWindowOnDeleteInterface<TypeTn> {
 public:
  typedef TypeTn Type;
  SlidingWindowDefaultOnDelete() {}

  virtual void OnDelete( Type & before_delete_value ) {};
};

/**
 ** @tname SlidingWindow
 ** @tparam TypeTn Хранимый тип
 ** @tparam BufferSizeTn размер циркулярного буфера
 ** @brief SlidingWindow - это класс, который хранит @b BufferSizeTn последних значений типа TypeTn
 ** @details Внутри хранение осуществляется с помощью циркулярного буфера
 **/ 
template <typename TypeTn, Size BufferSizeTn> class SlidingWindow {
 public:
  static constexpr const Size buffer_size = BufferSizeTn;
  typedef TypeTn Type;  
  typedef SlidingWindowOnDeleteInterface<Type> OnDeleteFunction;
  typedef std::shared_ptr< SlidingWindowOnDeleteInterface<Type> > OnDeleteFunPtr;

  SlidingWindow( 
      Size starting_index, 
      const Type & initial_value, 
      OnDeleteFunPtr on_delete_value = std::make_shared< SlidingWindowDefaultOnDelete<Type> >() )
      :  start_( starting_index ), 
         pos_( starting_index ),
         on_delete_( on_delete_value ) {
    assert ( (bool)on_delete_ );
    buffer_[0] = initial_value;
  }
  SlidingWindow( OnDeleteFunPtr on_delete_value = std::make_shared< SlidingWindowDefaultOnDelete<Type> >() ) {}

  bool IsEmpty() const noexcept { return !start_; }
  bool IsFull() const noexcept { return GetNumberEpochs() == buffer_size;}
  Size GetNumberEpochs() const noexcept { return (bool)start_ ? pos_ - start_ + 1 : 0;}
  void Push( const Type & value ) noexcept;
  void SlideEpochs() noexcept;
  Type const & GetCurrent() const noexcept { return buffer_.at( GetLocalIndexBackwards(0) );}
  Type       & GetCurrent()       noexcept { return buffer_[GetLocalIndexBackwards(0)];}
  Type const & GetPrevious( Size backward_offset ) const noexcept;
  Type       & GetPrevious( Size backward_offset )       noexcept;
  Type const & GetById( Size index ) const  noexcept;
  Type       & GetById( Size index )        noexcept;
  Size GetPositionOrMaxId() const noexcept { return pos_; }
  static bool Test() noexcept;

 protected:
  typedef std::array<Type, buffer_size > Buffer;
  static constexpr const Size kNoPos = Size(-1);

  static bool TestIndexes() noexcept;

  Size GetLocalIndex( Size global_index ) noexcept;
  /**
   ** @brief ищет индекс в буфере считая от конца к началу
   **/
  Size GetLocalIndexBackwards( Size backward_offset ) noexcept;
  
 private:
  Size start_ = 0; ///< индексация на базе 1. 0 = неинициализированное значение
  Size pos_ = 0;   ///< индексация на базе 1. 0 = неинициализированное значение
  Buffer buffer_;
  OnDeleteFunPtr on_delete_; ///< Стратегия, вызываемая перед затиранием объекта в буфере. Может использоваться для дополнительной зачистки.
};


} // namespace TARMEMDBG_NAMESPACE

#endif  // SLIDING_WINDOW_PROTECT_SIGNATURE_OT0T280ONCVQ40