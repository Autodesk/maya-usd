//
// Copyright 2021 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef MAYAUSD_UTILS_HASH_H
#define MAYAUSD_UTILS_HASH_H

#include <mayaUsd/base/api.h>

#include <functional>

namespace MAYAUSD_NS_DEF {

// hash combiner taken from:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0814r0.pdf

// boost::hash implementation also relies on the same algorithm:
// https://www.boost.org/doc/libs/1_64_0/boost/functional/hash/hash.hpp

template <typename T> inline void hash_combine(size_t& seed, const T& value)
{
    ::std::hash<T> hasher;
    seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UTILS_HASH_H
