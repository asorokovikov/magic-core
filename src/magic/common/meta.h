#pragma once

#include <type_traits>
#include <functional>

namespace magic {

//////////////////////////////////////////////////////////////////////

namespace detail {

template <typename TRet, typename... TArgs, typename TClass>
auto GetFunctionType(TRet (TClass::*)(TArgs...) const) -> std::function<TRet(TArgs...)>;

template <typename TRet, typename... TArgs, typename TClass>
auto GetFunctionType(TRet (TClass::*)(TArgs...)) -> std::function<TRet(TArgs...)>;

template <size_t argNum, size_t i, typename TArg, typename... TArgs>
struct ArgumentTraitHelper : public ArgumentTraitHelper<argNum, i + 1, TArgs...> {};

template <size_t argNum, typename TArg, typename... TArgs>
struct ArgumentTraitHelper<argNum, argNum, TArg, TArgs...> {
  using ArgumentType = TArg;
};

template <typename TRet, typename... TArgs>
struct StdFunctionTraits {
  using ReturnType = TRet;
  template <size_t i>
  struct ArgumentTraits : public ArgumentTraitHelper<i, 0, TArgs...> {};
};

template <typename TRet, typename... TArgs>
StdFunctionTraits<TRet, TArgs...> GetFunctionTraits(const std::function<TRet(TArgs...)>&);

template <typename TRet, typename... TArgs>
auto GetArgumentType(const std::function<TRet(TArgs...)>&) {
  using ArgumentType =
      typename StdFunctionTraits<TRet, TArgs...>::template ArgumentTraits<0>::ArgumentType;
  using ValueType = std::remove_reference_t<ArgumentType>;

  ValueType v{};
  ArgumentType t = v;
  return t;
}

template <typename TRet, typename... TArgs>
auto GetReturnType(const std::function<TRet(TArgs...)>&) -> TRet;

}  // namespace detail

//////////////////////////////////////////////////////////////////////

template <typename TFunc>
struct FunctorTraits {
  using FunctionType = decltype(detail::GetFunctionType(&TFunc::operator()));
  using ReturnType = decltype(detail::GetReturnType(FunctionType{}));
  using ArgumentType = decltype(detail::GetArgumentType(FunctionType{}));
};

}  // namespace magic