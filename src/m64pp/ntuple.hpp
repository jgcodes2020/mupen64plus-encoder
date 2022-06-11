#ifndef _M64PP_NTUPLE_HPP_
#define _M64PP_NTUPLE_HPP_

#include <bits/utility.h>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "fixed_string.hpp"

namespace m64p {
  using fixstr::fixed_string;

  template <fixed_string K, class T>
  struct nt_leaf {
    static constexpr decltype(K) name = K;
    using type                        = T;
    
    T value;
  };

  template <fixed_string S>
  struct string_constant {
    static constexpr decltype(S) value = S;
  };

  template <fixed_string... Ss>
  struct string_sequence {
    static constexpr size_t size = sizeof...(Ss);
  };

  namespace details {
    // Concept to detect nt_leaf
    // =========================
    template <class T>
    struct is_any_nt_leaf : std::false_type {};

    template <fixed_string S, class T>
    struct is_any_nt_leaf<nt_leaf<S, T>> : std::true_type {};

    template <class T>
    concept any_nt_leaf = is_any_nt_leaf<T>::value;
  }  // namespace details

  template <details::any_nt_leaf... Ls>
  struct ntuple;

  namespace details {
    // Concept to detect ntuple
    template <class T>
    struct is_any_ntuple : std::false_type {};

    template <details::any_nt_leaf... Ls>
    struct is_any_ntuple<ntuple<Ls...>> : std::true_type {};

    template <class T>
    concept any_ntuple = is_any_ntuple<T>::value;

    // Setup to assert uniqueness of strings
    // =====================================

    template <fixed_string... Ss>
    struct string_set : string_constant<Ss>... {
      template <fixed_string S>
      constexpr auto operator+(string_constant<S>) {
        if constexpr (std::is_base_of_v<string_constant<S>, string_set>)
          return string_set {};
        else
          return string_set<Ss..., S> {};
      }

      constexpr size_t size() { return sizeof...(Ss); }
    };

    template <fixed_string... Ss>
    inline constexpr bool strings_unique =
      (string_set<> {} + ... + string_constant<Ss> {}).size() == sizeof...(Ss);

    // Setup for indexing an ntuple
    // ============================

    template <fixed_string K, class T>
    struct nt_empty_leaf {
      using type = T;
    };

    template <class... Ts>
    struct nt_empty_base;

    template <fixed_string... Ks, class... Ts>
    struct nt_empty_base<nt_empty_leaf<Ks, Ts>...> : nt_empty_leaf<Ks, Ts>... {
    };

    template <fixed_string K, class T>
    const nt_empty_leaf<K, T> empty_upcaster(const nt_empty_leaf<K, T>&);

  }  // namespace details

  template <fixed_string K, details::any_ntuple T>
  struct ntuple_element;

  template <fixed_string K, fixed_string... Ks, class... Ts>
  struct ntuple_element<K, ntuple<nt_leaf<Ks, Ts>...>> {
    static_assert(requires {
      typename decltype(details::empty_upcaster<K>(
        details::nt_empty_base<details::nt_empty_leaf<Ks, Ts>...> {}))::type;
    }, "No valid tuple element found");
    using type = typename decltype(details::empty_upcaster<K>(
      details::nt_empty_base<details::nt_empty_leaf<Ks, Ts>...> {}))::type;
  };

  template <fixed_string K, details::any_ntuple T>
  using ntuple_element_t = typename ntuple_element<K, T>::type;
  
  /**
   * Actual named tuple class.
   */
  template <fixed_string... Ks, class... Ts>
  class ntuple<nt_leaf<Ks, Ts>...> : private nt_leaf<Ks, Ts>... {
    static_assert(details::strings_unique<Ks...>, "Keys must be unique");

    
  public:
    using keys = string_sequence<Ks...>;
  
    ntuple() = default;
    ntuple(Ts&&... values) : nt_leaf<Ks, Ts> {std::forward<Ts>(values)}... {}
  
    template <fixed_string K, fixed_string... NKs, class... NTs>
    friend ntuple_element_t<K, ntuple<nt_leaf<NKs, NTs>...>>& get(ntuple<nt_leaf<NKs, NTs>...>& tuple);
    template <fixed_string K, fixed_string... NKs, class... NTs>
    friend const ntuple_element_t<K, ntuple<nt_leaf<NKs, NTs>...>>& get(const ntuple<nt_leaf<NKs, NTs>...>& tuple);
  };

  template <fixed_string K, fixed_string... Ks, class... Ts>
  ntuple_element_t<K, ntuple<nt_leaf<Ks, Ts>...>>& get(
    ntuple<nt_leaf<Ks, Ts>...>& tuple) {
    using leaf_type =
      nt_leaf<K, ntuple_element_t<K, ntuple<nt_leaf<Ks, Ts>...>>>;
    return static_cast<leaf_type&>(tuple).value;
  }
  template <fixed_string K, fixed_string... Ks, class... Ts>
  ntuple_element_t<K, ntuple<nt_leaf<Ks, Ts>...>>& get(
    const ntuple<nt_leaf<Ks, Ts>...>& tuple) {
    using leaf_type =
      nt_leaf<K, ntuple_element_t<K, ntuple<nt_leaf<Ks, Ts>...>>>;
    return static_cast<const leaf_type&>(tuple).value;
  }
}  // namespace m64p
#endif