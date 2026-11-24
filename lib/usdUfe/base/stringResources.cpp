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

#include <usdUfe/ufe/Utils.h>

#ifdef WANT_QT_TRANSLATION
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#endif

namespace USDUFE_NS_DEF {
namespace StringResources {

bool registerResources(const ResourceMap& resources)
{
    return StringResourceBase::registerAll(resources, [](const Resource& resource) {
        return UsdUfe::registerString(resource._key, resource._value);
    });
}

bool registerStrings() { return registerResources(allResources()); }

// Define each resource once. The extern declarations in the header give these external linkage.
//

#ifdef WANT_QT_TRANSLATION
// QT_TRANSLATE_NOOP marks values for lupdate and expands to the bare literal, leaving the lookup
// to getAsQString().
#define DEFINE_USDUFE_STR(key, value) \
    const Resource key { "UsdUfe", #key, QT_TRANSLATE_NOOP("UsdUfe", value) };
#else
#define DEFINE_USDUFE_STR(key, value) const Resource key { "UsdUfe", #key, value };
#endif
USDUFE_STRING_LIST(DEFINE_USDUFE_STR)
#undef DEFINE_USDUFE_STR

const ResourceMap& allResources()
{
    // Built on first use from the same list, so no registration happens during static init.
    static const ResourceMap sResources = STRING_RESOURCE_BASE_MAKE_MAP(USDUFE_STRING_LIST);
    return sResources;
}

std::string getAsString(const Resource& stringResourceID)
{
    if (UsdUfe::haveTranslateStringFn())
        return UsdUfe::translateString(stringResourceID._key, stringResourceID._value);

#ifdef WANT_QT_TRANSLATION
    // Fallback to plain QT translation.
    return QCoreApplication::translate(
               stringResourceID._module.c_str(), stringResourceID._value.c_str())
        .toStdString();
#endif

    return stringResourceID._value;
}

} // namespace StringResources
} // namespace USDUFE_NS_DEF
