//
// Copyright 2024 Autodesk
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
#ifndef MAYAUSD_XFORMOPUTILS_H
#define MAYAUSD_XFORMOPUTILS_H

#include <mayaUsd/base/api.h>

#include <ufe/types.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

/*! Decompose the argument matrix m into translation, rotation and scale
    components using the Maya API.

    \param m Input matrix.
    \param[out] t Output translation.  If null, will be ignored.
    \param[out] r Output rotation, in XYZ order.  If null, will be ignored.
    \param[out] s Output scale.  If null, will be ignored.
*/
MAYAUSD_CORE_PUBLIC
void extractTRS(const Ufe::Matrix4d& m, Ufe::Vector3d* t, Ufe::Vector3d* r, Ufe::Vector3d* s);

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_XFORMOPUTILS_H
