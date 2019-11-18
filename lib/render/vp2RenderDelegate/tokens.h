//
// Copyright 2019 Autodesk
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

#ifndef HD_VP2_TOKENS_H
#define HD_VP2_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDVP2_REPR_TOKENS                          \
    (bbox)                                         \
    (selection)

TF_DECLARE_PUBLIC_TOKENS(HdVP2ReprTokens, , HDVP2_REPR_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_TOKENS_H
