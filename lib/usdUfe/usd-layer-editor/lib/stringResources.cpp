//
// Copyright 2020 Autodesk
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

#include <QtCore/QString>

// TODO LE-EXTRACT : String resources - Is this enough for maya-usd? Does the maya registration
// matter?

namespace UsdLayerEditor {
namespace StringResources {

Resource create(const char* key, const char* value)
{
    Resource stringResourceID { "usdLayerEditor", key, value };
    return stringResourceID;
}

QString getAsQString(const Resource& stringResourceID)
{
    return QString::fromStdString(stringResourceID.value);
}

} // namespace StringResources
} // namespace UsdLayerEditor
