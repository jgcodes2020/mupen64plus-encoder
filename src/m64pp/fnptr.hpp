#ifndef _M64PP_FNPTR_HPP_
#define _M64PP_FNPTR_HPP_
#include <memory>
#include <type_traits>
#include <utility>
namespace m64p {
  namespace details {
    template <class T, class F>
    struct fnptr_dispatch;

    template <class T, class R, class... Args>
    struct fnptr_dispatch<T, R(Args...)> {
      static R dispatch(const void* ptr, Args... args) {
        std::add_lvalue_reference_t<std::add_const_t<T>> obj =
          *static_cast<std::add_pointer_t<std::add_const_t<T>>>(ptr);
        return obj(std::forward<Args>(args)...);
      }
    };
  }  // namespace details

  template <class T>
  class fnptr;

  // Acts like a reference to arbitrary callable object.
  template <class R, class... Args>
  class fnptr<R(Args...)> {
  public:
    template <class T>
    fnptr(const T& object) :
      obj(std::addressof(object)),
      dispatch(details::fnptr_dispatch<T, R(Args...)>::dispatch) {
      static_assert(
        std::is_invocable_r_v<R, T, Args...>,
        "Function must be callable using current signature");
    }

    R operator()(Args... args) {
      return dispatch(obj, std::forward<Args>(args)...);
    }

  private:
    const void* obj;
    R (*dispatch)(const void*, Args...);
  };
}  // namespace m64p
#endif