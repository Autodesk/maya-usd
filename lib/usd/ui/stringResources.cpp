//
// Copyright 2026 Autodesk
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
#include "stringResources.h"

#include <mayaUsd/base/stringResources.h>

#include <maya/MQtUtil.h>

namespace MayaUsdUI {
namespace StringResources {

// Define each resource once. The extern declarations in the header give these external linkage.
#define DEFINE_MAYAUSDUI_STR(key, value) const Resource key { "MayaUsdUI", #key, value };
MAYAUSDUI_STRING_LIST(DEFINE_MAYAUSDUI_STR)
#undef DEFINE_MAYAUSDUI_STR

QString getAsQString(const Resource& stringResourceID)
{
    return MQtUtil::toQString(MayaUsd::StringResources::getAsMString(stringResourceID));
}

bool registerStrings()
{
    return MayaUsd::StringResources::registerResources(
        STRING_RESOURCE_BASE_MAKE_MAP(MAYAUSDUI_STRING_LIST));
}

} // namespace StringResources
} // namespace MayaUsdUI
