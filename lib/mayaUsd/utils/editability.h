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
#ifndef MAYA_USD_EDITABILITY_H
#define MAYA_USD_EDITABILITY_H

#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/property.h>

namespace MAYAUSD_NS_DEF {

/*! \brief  Determine the editability status of a property.
 */

namespace Editability {
/*! \brief  Verify if a property is locked.
 */
bool isLocked(PXR_NS::UsdProperty property);
} // namespace Editability

} // namespace MAYAUSD_NS_DEF

#endif // MAYA_USD_EDITABILITY_H
