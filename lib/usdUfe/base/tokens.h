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

#ifndef USDUFE_TOKENS_H
#define USDUFE_TOKENS_H

#include "api.h"

#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>

namespace USDUFE_NS_DEF {

// Tokens for edit routing:
//
// clang-format off
#define USDUFE_EDIT_ROUTING_TOKENS                    \
    /* Tokens shared by multiple edit routers        */ \
                                                        \
    /* Prim received in the context                  */ \
    ((Prim, "prim"))                                    \
    /* Layer received in the context                 */ \
    ((Layer, "layer"))                                  \
    /* Routing operation                             */ \
    ((Operation, "operation"))                          \
    /* Stage received in the context of some router  */ \
    ((Stage, "stage"))                                  \
    ((EditTarget, "editTarget"))                        \
                                                        \
    /* Routing operations                            */ \
                                                        \
    ((RouteParent, "parent"))                           \
    ((RouteDuplicate, "duplicate"))                     \
    ((RouteVisibility, "visibility"))                   \
    ((RouteAttribute, "attribute"))                     \
    ((RouteDelete, "delete"))                           \
                                                        \
// clang-format on

PXR_NAMESPACE_USING_DIRECTIVE

TF_DECLARE_PUBLIC_TOKENS(EditRoutingTokens, USDUFE_PUBLIC, USDUFE_EDIT_ROUTING_TOKENS);

// Generic tokens:
//
// clang-format off
#define USDUFE_GENERIC_TOKENS                          \
    /* Metadata value to inherit the value from a     */ \
    /* parent prim. Used in selectability.            */ \
    ((Inherit, "inherit"))                               \
    /* Metadata value to turn on or off a feature.    */ \
    /* Used in selectability and lock, for example.   */ \
    ((On, "on"))                                         \
    ((Off, "off")) // clang-format on

TF_DECLARE_PUBLIC_TOKENS(GenericTokens, USDUFE_PUBLIC, USDUFE_GENERIC_TOKENS);

// Tokens that are used as metadata on prim and attributes:
//
// clang-format off
#define USDUFE_METADATA_TOKENS                         \
    /* Locking attribute metadata. A locked attribute */ \
    /* value cannot be changed.                       */ \
    /* TEMP (UsdUfe) - look at replacing mayaLock     */ \
    ((Lock, "mayaLock"))                                 \
    /* Metadata for UI queries                        */ \
    ((UIName, "uiname"))                                 \
    ((UIFolder, "uifolder"))                             \
    ((UIMin, "uimin"))                                   \
    ((UIMax, "uimax"))                                   \
    ((UISoftMin, "uisoftmin"))                           \
    ((UISoftMax, "uisoftmax")) // clang-format on

TF_DECLARE_PUBLIC_TOKENS(MetadataTokens, USDUFE_PUBLIC, USDUFE_METADATA_TOKENS);

} // namespace USDUFE_NS_DEF

#endif // USDUFE_TOKENS_H
