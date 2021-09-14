/**
 ** @file SlidingWindow.impl.hpp
 ** @author Astapov Konstantin
 ** @copyright 2021, Picodata. picodata.io - professional database services
 ** @remark "TaraRam" project - Memory allocation debugger for Tarantool DBMS
 ** \~english @brief
 ** \~english @details
 ** \~russian @brief Файл, содержащий определения функций для @ref "скользящего окна" SlidingWindow.hpp
 ** \~russian @details
 **/

#if !defined(TARMEMDBG_ALLOW_INCLUDE)
#   error Do not include this file manually
#endif

#ifndef    SLIDING_WINDOW_IMPL_PROTECT_SIGNATURE_RBUYPDPXKNEBR4
#define    SLIDING_WINDOW_IMPL_PROTECT_SIGNATURE_RBUYPDPXKNEBR4


namespace      TARMEMDBG_NAMESPACE {

template<typename TypeTn, Size BufferSizeTn>
void SlidingWindow<TypeTn, BufferSizeTn>::Push( const Type & value ) noexcept {
  if ( !start_ ) start_ = 1;
  ++pos_;
  on_delete_->OnDelete( buffer_[GetLocalIndex(0)] );
  buffer_[GetLocalIndex(0)] = value;
}

template<typename TypeTn, Size BufferSizeTn>
void SlidingWindow<TypeTn, BufferSizeTn>::SlideEpochs() noexcept {
  ++start_;
  ++pos_;
}

template<typename TypeTn, Size BufferSizeTn>
Size SlidingWindow<TypeTn, BufferSizeTn>::GetLocalIndex( Size global_index ) noexcept {
  if ( !start_ || global_index >= pos_ || global_index < start_ ) return kNoPos;
  return global_index % buffer_size;
}

template<typename TypeTn, Size BufferSizeTn>
Size SlidingWindow<TypeTn, BufferSizeTn>::GetLocalIndexBackwards( Size backward_offset ) noexcept {
  if ( !start_ || backward_offset >= pos_ ) return kNoPos;
  return ( pos_ - backward_offset ) % buffer_size;
}

#   define TAR_MDBG_CHECK_OFFSET( backward_offset ) \
  assert( !IsEmpty() ); \
  assert( backward_offset < pos_ ); \
  assert( backward_offset < buffer_size );

template<typename TypeTn, Size BufferSizeTn>
typename SlidingWindow<TypeTn, BufferSizeTn>::Type const & 
SlidingWindow<TypeTn, BufferSizeTn>::GetPrevious( Size backward_offset ) const noexcept {
  TAR_MDBG_CHECK_OFFSET( backward_offset )
  return buffer_.at( GetLocalIndexBackwards(backward_offset) );
}
template<typename TypeTn, Size BufferSizeTn>
typename SlidingWindow<TypeTn, BufferSizeTn>::Type       & 
SlidingWindow<TypeTn, BufferSizeTn>::GetPrevious( Size backward_offset )       noexcept {
  TAR_MDBG_CHECK_OFFSET( backward_offset ) 
  return buffer_[GetLocalIndexBackwards(backward_offset)];
}
#   undef TAR_MDBG_CHECK_OFFSET


#   define TAR_MDBG_CHECK_INDEX( index ) \
  assert( !IsEmpty() ); \
  assert( index < pos_ ); \
  assert( index >= start_ );

template<typename TypeTn, Size BufferSizeTn>
typename SlidingWindow<TypeTn, BufferSizeTn>::Type const & 
SlidingWindow<TypeTn, BufferSizeTn>::GetById( Size index ) const noexcept {
  TAR_MDBG_CHECK_INDEX( index )
  return buffer_.at( GetLocalIndex(index) );
}

template<typename TypeTn, Size BufferSizeTn>
typename SlidingWindow<TypeTn, BufferSizeTn>::Type       & 
SlidingWindow<TypeTn, BufferSizeTn>::GetById( Size index )       noexcept { 
  TAR_MDBG_CHECK_INDEX( index )
  return buffer_[GetLocalIndex(index)];
}
#   undef TAR_MDBG_CHECK_INDEX

////////////////////////////// тесты //////////////////////////////

template<typename TypeTn, Size BufferSizeTn>
bool SlidingWindow<TypeTn, BufferSizeTn>::TestIndexes() noexcept {
  return 0;
}
    
} // namespace TARMEMDBG_NAMESPACE

#endif  // SLIDING_WINDOW_IMPL_PROTECT_SIGNATURE_RBUYPDPXKNEBR4