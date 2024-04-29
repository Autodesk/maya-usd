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

// Tokens for edit routing in UsdUfe
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

} // namespace USDUFE_NS_DEF

#endif // USDUFE_TOKENS_H
