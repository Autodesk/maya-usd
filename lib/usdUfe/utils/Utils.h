//
// Copyright 2023 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef USDUFE_UTIL_H
#define USDUFE_UTIL_H

#include <usdUfe/base/api.h>

#include <string>

/// General utilities for working with the UsdUfe library.
namespace USDUFE_NS_DEF {

USDUFE_PUBLIC
std::string sanitizeName(const std::string& name);

/// Return a prettified name from camelCase or snake_case source.
///
/// Put a space in the name when preceded by a capital letter.
/// Exceptions: Number followed by capital
///            Multiple capital letters together
/// Replace underscore by space and capitalize next letter
/// Always capitalize first letter
USDUFE_PUBLIC
std::string prettifyName(const std::string& name);

} // namespace USDUFE_NS_DEF

#endif // USDUFE_UTIL_H
