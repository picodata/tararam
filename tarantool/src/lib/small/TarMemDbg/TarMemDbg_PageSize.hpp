/**
 ** @file TarMemDbg_PageSize.hpp
 ** @author Astapov Konstantin
 ** @copyright 2021, Picodata. picodata.io - professional database services
 ** @remark "TaraRam" project - Memory allocation debugger for Tarantool DBMS
 ** \~english @brief
 ** \~english @details
 ** \~russian @brief Файл, хранящий класс, подсчитывающий размер страницы на данной системе
 ** \~russian @details
 **/

#if !defined(TARMEMDBG_ALLOW_INCLUDE)
#   error Do not include this file manually
#endif

#ifndef    TARMEMDBG_PAGESIZE_PROTECT_SIGNATURE_1DNXO51D909BJS

#   ifdef PAGESIZE
#      define TARMEMDBG_PAGESIZE PAGESIZE
#   else
#      define TARMEMDBG_PAGESIZE 4096
#   endif

namespace      TARMEMDBG_NAMESPACE {

class PageSize {
 public:
  static constexpr const Size kInitialPageSize = TARMEMDBG_PAGESIZE;
  Size operator()() { return GetPageSize(); }
  Size GetPageSize();

 private:
  struct Body {
    Size page_size = kInitialPageSize;
    static Body * Create();
  };
  Body * GetBody();
};

typename PageSize::Body * PageSize::Body::Create() {
  Body * ret = ::new (std::nothrow) Body;
# ifdef     _WIN32
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  ret->page_size = si.dwPageSize;
# else   // _WIN32
  ret->page_size = sysconf(_SC_PAGESIZE);
# endif  // _WIN32
  return ret;
}

typename PageSize::Body * PageSize::GetBody() {
  static std::unique_ptr<Body> body ( Body::Create() ); 
  return body.get();
}

Size PageSize::GetPageSize() {
  return GetBody()->page_size;
}

} // namespace TARMEMDBG_NAMESPACE

#define    TARMEMDBG_PAGESIZE_PROTECT_SIGNATURE_1DNXO51D909BJS
#endif  // TARMEMDBG_PAGESIZE_PROTECT_SIGNATURE_1DNXO51D909BJS