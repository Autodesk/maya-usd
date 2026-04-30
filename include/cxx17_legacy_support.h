//
// Copyright 2025 Autodesk
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

#ifndef MAYAUSD_CXX_LEGACY_SUPPORT_H
#define MAYAUSD_CXX_LEGACY_SUPPORT_H

//
// Helper header to compile with C++14 vs C++17.
//

#if (__cplusplus >= 201703L)

#include <optional>

namespace MayaUsdCxxLegacySupport {
using std::optional;
}

#else

#include <boost/optional.hpp>

namespace MayaUsdCxxLegacySupport {
using boost::optional;
}

#endif

#endif // MAYAUSD_CXX_LEGACY_SUPPORT_H
