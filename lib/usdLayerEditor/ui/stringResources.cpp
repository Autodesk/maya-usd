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

#include "layerEditorDCCFunctions.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QString>

#include <type_traits>

namespace UsdLayerEditor {
namespace StringResources {

namespace {

// Function-local so it is initialized on first use: create() runs during static init of the
// constants in stringResources.h.
std::map<std::string, Resource>& registry()
{
    static std::map<std::string, Resource> sResources;
    return sResources;
}

} // namespace

Resource create(const char* module, const char* key, const char* value)
{
    Resource resource { module, key, value };
    registry().insert({ resource.key, resource });
    return resource;
}

const std::map<std::string, Resource>& allResources() { return registry(); }

namespace {

// Converts only the side that isn't already the requested type: the DCC returns std::string and
// the Qt fallback returns QString.
template <typename StringType> StringType translated(const Resource& stringResourceID)
{
    const auto& translate = layerEditorDCCFunctions().localization.translate;
    if (translate) {
        std::string result = translate(stringResourceID.key, stringResourceID.value);
        if constexpr (std::is_same_v<StringType, QString>)
            return QString::fromStdString(result);
        else
            return result;
    }

    // Fallback to plain QT translation.
    QString result = QCoreApplication::translate(
        stringResourceID.module.c_str(), stringResourceID.value.c_str());
    if constexpr (std::is_same_v<StringType, QString>)
        return result;
    else
        return result.toStdString();
}

} // namespace

std::string getAsString(const Resource& stringResourceID)
{
    return translated<std::string>(stringResourceID);
}

QString getAsQString(const Resource& stringResourceID)
{
    return translated<QString>(stringResourceID);
}

} // namespace StringResources
} // namespace UsdLayerEditor
