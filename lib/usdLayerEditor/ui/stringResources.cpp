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

#include <usdUfe/base/stringResources.h>
#include <usdUfe/ufe/Utils.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QString>

namespace UsdLayerEditor {
namespace StringResources {

bool registerStrings()
{
    // Registers the layer editor strings with the DCC.
    return UsdUfe::StringResources::registerResources(allResources());
}

// Define each resource once. The extern declarations in the header give these external linkage.
//
// QT_TRANSLATE_NOOP marks values for lupdate and expands to the bare literal, leaving the lookup
// to getAsQString().
#define DEFINE_LE_STR(key, value) \
    const Resource key { "UsdLayerEditor", #key, QT_TRANSLATE_NOOP("UsdLayerEditor", value) };
USDLAYEREDITOR_STRING_LIST(DEFINE_LE_STR)
#undef DEFINE_LE_STR

const ResourceMap& allResources()
{
    // Built on first use from the same list, so no registration happens during static init.
    static const ResourceMap sResources = STRING_RESOURCE_BASE_MAKE_MAP(USDLAYEREDITOR_STRING_LIST);
    return sResources;
}

// Each getter converts only the side that isn't already its return type: the DCC returns
// std::string and the Qt fallback returns QString.
std::string getAsString(const Resource& stringResourceID)
{
    if (UsdUfe::haveTranslateStringFn())
        return UsdUfe::translateString(stringResourceID._key, stringResourceID._value);

    // Fallback to plain QT translation.
    return QCoreApplication::translate(
               stringResourceID._module.c_str(), stringResourceID._value.c_str())
        .toStdString();
}

QString getAsQString(const Resource& stringResourceID)
{
    if (UsdUfe::haveTranslateStringFn())
        return QString::fromStdString(
            UsdUfe::translateString(stringResourceID._key, stringResourceID._value));

    // Fallback to plain QT translation.
    return QCoreApplication::translate(
        stringResourceID._module.c_str(), stringResourceID._value.c_str());
}

} // namespace StringResources
} // namespace UsdLayerEditor
