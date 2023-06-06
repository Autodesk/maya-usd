//
// Copyright 2023 Autodesk
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
#ifndef USDUFE_DEBUGCODES_H
#define USDUFE_DEBUGCODES_H

#include <pxr/base/tf/debug.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEBUG_CODES(
    USDUFE_UNDOSTACK,
    USDUFE_UNDOSTATEDELEGATE
);
// clang-format on

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDUFE_DEBUGCODES_H
