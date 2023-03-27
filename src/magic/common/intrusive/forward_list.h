#pragma once

#include <wheels/intrusive/forward_list.hpp>

namespace magic {

template <typename T>
using IntrusiveForwardListNode = wheels::IntrusiveForwardListNode<T>;

template <typename T>
using IntrusiveForwardList = wheels::IntrusiveForwardList<T>;

}  // namespace magic