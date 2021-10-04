/**
 ** @file MemoryEpochQueue.impl.hpp
 ** @author Astapov Konstantin
 ** @copyright 2021, Picodata. picodata.io - professional database services
 ** @remark "TaraRam" project - Memory allocation debugger for Tarantool DBMS
 ** \~english @brief
 ** \~english @details
 ** \~russian @brief Файл, содержащий определения функций для @ref "очередей эпох памяти" MemoryEpochQueue.hpp
 ** \~russian @details
 **/

#if !defined(TARMEMDBG_ALLOW_INCLUDE)
#   error Do not include this file manually
#endif

#ifndef    MEMORY_EPOCH_QUEUE_IMPL_PROTECT_SIGNATURE_YS0EVPHRVZFRTE

namespace      TARMEMDBG_NAMESPACE {

/*
MemoryEpochQueue::MemoryEpochQueue ( This && value ) {
  if ( &value == this ) return;
  if ( !epochs_ ) epochs_.reset();
  std::swap( *this, value );
}

This && MemoryEpochQueue::operator=( This && value ) {
  if ( &value == this ) return;
  if ( !epochs_ ) epochs_.reset();  
  std::swap( *this, value );
  return std::move(*this);
}
*/

MemoryEpochQueue * MemoryEpochQueue::Create( Size starting_index ) {
  typename Storage::OnDeleteFunPtr on_delete_value = std::make_shared< OnDeleteFunctor >();
  assert( (bool)on_delete_value );
  std::unique_ptr< MemoryEpochInterface > starting_epoch( MemoryEpochLsRegion::Create() );
  assert( (bool) starting_epoch );
  std::unique_ptr<Storage> sliding_window(   NewAligned< Storage >( 
      starting_index, 
      starting_epoch.release(),
      on_delete_value )   );
  assert( (bool)sliding_window );
  std::unique_ptr< MemoryEpochQueue > internal ( AllocateQueue() );
  assert( (bool)internal );
  internal->epochs_ = sliding_window.release();
  return internal.release();
}

ProtectMemoryConstant CalcProtectTypeFor2ndEpoch( 
    [[maybe_unused]] Size nepochs,
    Size buffer_size ) {
  assert( buffer_size != 1 );
  if ( buffer_size != 2 ) {
    // Вторая эпоха просто станет третьей, поэтому запрещаем к ней доступ
    return kProtectNone;
  }
  // При включённом флаге TARMEMDBG_REUSE_EPOCHS:
  // Вторая эпоха, которая только что была для чтения, сейчас станет главной
  // из-за флага повторного использования.
  // Поэтому разрешаем к ней полный доступ
  //
  // При выключенном флаге TARMEMDBG_REUSE_EPOCHS:
  // Вторая эпоха, так как буфер всего на две, при сдвиге станет третьей и будет удалена.
  // Чтобы её спокойно удалить, разрешаем к ней чтение и запись
  return kProtectReadWrite;
//# if       TARMEMDBG_REUSE_EPOCHS
//# else  // TARMEMDBG_REUSE_EPOCHS  
//# endif // TARMEMDBG_REUSE_EPOCHS
}

void MemoryEpochQueue::NextEpoch() noexcept {      
  if ( !epochs_->IsEmpty() ) {
    Size nepochs = epochs_->GetNumberEpochs();
    Size buffer_size = epochs_->buffer_size;
    assert( nepochs >= 2 );
    MemoryEpochInterface * last_epoch = GetCurrentEpoch(); 
    assert( last_epoch );
    last_epoch->ProtectEpoch( kProtectRead );
    if ( nepochs >= 2 ) {
      MemoryEpochInterface * previous_epoch = epochs_->GetPrevious( 2 );
      assert( previous_epoch );
      // если у нас НЕ ленивый вызов lsregion_gc, то вызываем его здесь (сдвиг на третью позицию), иначе - сразу перед удалением/переиспользованием самой дальней эпохи
#     if       ~TARMEMDBG_REUSE_LAZY_CLEAN
      lsregion_gc_orig( 
          previous_epoch->GetLsRegion(), 
          epochs_->GetPositionOrMaxId() - nepochs + 1 );
#     endif // ~TARMEMDBG_REUSE_LAZY_CLEAN
      previous_epoch->ProtectEpoch( CalcProtectTypeFor2ndEpoch(   nepochs, buffer_size )   );
      if ( epochs_->IsFull() && nepochs >= 3 ) {        
        MemoryEpochInterface * predelete_epoch = epochs_->GetPrevious( 2 );
        assert( predelete_epoch );
        predelete_epoch->ProtectEpoch( kProtectReadWrite );
        // если у нас ленивый вызов lsregion_gc, то вызываем его здесь, иначе - сразу после сдвига на третью позицию
#       if       TARMEMDBG_REUSE_LAZY_CLEAN
        lsregion_gc_orig( 
            predelete_epoch->GetLsRegion(), 
            epochs_->GetPositionOrMaxId() - nepochs + 1 );
#       endif // TARMEMDBG_REUSE_LAZY_CLEAN
      }
    }
  }
  // Если нужно поведение, когда старая эпоха полностью удаляется и заменяется новой, 
  // то нужно всегда делать Push
  // Если нужна политика переиспользования старых эпох, то при IsFull нужно делать SlideEpochs  
  if ( epochs_->IsFull() ) {
#   if       TARMEMDBG_REUSE_EPOCHS     
    epochs_->SlideEpochs();
#   else  // TARMEMDBG_REUSE_EPOCHS
    epochs_->Push( MemoryEpochLsRegion::Create() );
#   endif // TARMEMDBG_REUSE_EPOCHS
  } else {
    epochs_->Push( MemoryEpochLsRegion::Create() );
  }
}

MemoryEpochInterface * MemoryEpochQueue::GetCurrentEpoch() {
  assert( CheckIfThisIsReallyMemoryEpochQueue() );
  assert( (bool)epochs_ );
  MemoryEpochInterface * ret = epochs_->GetCurrent();
  assert( (bool)ret );
  assert( ret->CheckIfThisIsReallyMemoryEpoch() );
  return ret;
}

MemoryEpochLsRegion * MemoryEpochQueue::GetLsEpoch( MemoryEpochInterface * val_interface ) {
    assert( (bool)val_interface );
    assert(   dynamic_cast<MemoryEpochLsRegion *>( val_interface )   );
    MemoryEpochLsRegion * epoch = (MemoryEpochLsRegion *)val_interface;
    return epoch;
}

MemoryEpochQueue * 
MemoryEpochQueue::AllocateQueue() noexcept {
  //static_assert( std::is_base_of< MemoryEpochInterface, DerivedTn>::value, "Template argument must be inherited from MemoryEpochInterface" );
  Size allocated_bytes = 0;
  Size page_size = PageSize::kInitialPageSize; // AllocateWithForbiddenPageAtStart изменит это значение и установит его на PageSize()()
  auto ret = AllocateWithForbiddenPageAtStart<MemoryEpochQueue>( allocated_bytes, page_size );
  assert( (bool)ret );
  ret->offset_of_allocated_= -page_size;
  ret->allocated_byte_size_ = allocated_bytes;
  return ret;
}

void MemoryEpochQueue::operator delete( void * to_free ) noexcept {  
  if ( !to_free ) return;
  MemoryEpochQueue * object = (MemoryEpochQueue*)to_free;
  assert( object->CheckIfThisIsReallyMemoryEpochQueue() );
  DeallocateWithForbiddenPageAtStart( GetHandle( object ), object->allocated_byte_size_ );
}

MemoryEpochQueue * MemoryEpochQueue::GetSelfByHandleNoChecks( void * handle ) noexcept {
  return (MemoryEpochQueue *)( reinterpret_cast<Byte *>(handle) + PageSize()() );
}

MemoryEpochQueue * MemoryEpochQueue::GetSelfByHandle( void * handle ) noexcept {
  assert( (bool)handle );
  auto * ret = GetSelfByHandleNoChecks( handle );
  assert( ret->CheckIfThisIsReallyMemoryEpochQueue() );
  return ret;
}

void * MemoryEpochQueue::GetHandle( MemoryEpochQueue * self ) noexcept {
  assert( self->CheckIfThisIsReallyMemoryEpochQueue() );
  return (void *)( reinterpret_cast<Byte *>(self) + self->offset_of_allocated_ ); // offset_of_allocated_ отрицательный, поэтому прибавляем
}

Size MemoryEpochQueue::GetPositionOrMaxId() noexcept {
  return epochs_->GetPositionOrMaxId();
}

} // namespace TARMEMDBG_NAMESPACE

#define    MEMORY_EPOCH_QUEUE_IMPL_PROTECT_SIGNATURE_YS0EVPHRVZFRTE
#endif  // MEMORY_EPOCH_QUEUE_IMPL_PROTECT_SIGNATURE_YS0EVPHRVZFRTE