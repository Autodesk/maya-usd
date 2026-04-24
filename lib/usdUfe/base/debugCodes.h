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
#include <pxr/base/tf/diagnostic.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEBUG_CODES(
    USDUFE_UNDOSTACK,
    USDUFE_UNDOSTATEDELEGATE,
    USDUFE_UNDOCMD
);
// clang-format on

// Macro like TF_DEBUG_MSG, but for informational messages.
// IOW, it output its messages to the same place as TF_STATUS while being controlled
// by the same debug codes as TF_DEBUG_MSG. This is useful because the debug message
// are output to the standard error output while the status message are handled by
// the DCC diagnostic dellegate and are sent to a more appropriate place for the
// user to see. For example, in Maya, the status message are sent to the script editor
// which has nice feature like clearing, searching and identical-messages consolidation,
// while the debug message are output in a featureless console.
#define TF_DEBUG_INFO_MSG(enumVal, ...)           \
    if (!TfDebug::IsEnabled(enumVal)) /* empty */ \
        ;                                         \
    else                                          \
        TF_STATUS(__VA_ARGS__)

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDUFE_DEBUGCODES_H
