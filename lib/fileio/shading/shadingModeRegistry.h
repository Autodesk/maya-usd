//
// Copyright 2016 Pixar
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
#ifndef PXRUSDMAYA_SHADING_MODE_REGISTRY_H
#define PXRUSDMAYA_SHADING_MODE_REGISTRY_H

/// \file usdMaya/shadingModeRegistry.h

#include "../../base/api.h"
#include "shadingModeExporter.h"
#include "shadingModeExporterContext.h"
#include "shadingModeImporter.h"

#include "pxr/pxr.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/weakBase.h"

#include <maya/MObject.h>

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_SHADINGMODE_TOKENS \
    (none) \
    (displayColor)

TF_DECLARE_PUBLIC_TOKENS(UsdMayaShadingModeTokens,
    MAYAUSD_CORE_PUBLIC,
    PXRUSDMAYA_SHADINGMODE_TOKENS);


TF_DECLARE_WEAK_PTRS(UsdMayaShadingModeRegistry);

/// We understand that shading may want to be imported/exported in many ways
/// across studios.  Even within a studio, different workflows may call for
/// different shading modes.
///
/// We provide macros that are entry points into the shading import/export
/// logic.
class UsdMayaShadingModeRegistry : public TfWeakBase
{
public:

    MAYAUSD_CORE_PUBLIC
    static UsdMayaShadingModeExporterCreator GetExporter(const TfToken& name) {
        return GetInstance()._GetExporter(name);
    }
    MAYAUSD_CORE_PUBLIC
    static UsdMayaShadingModeImporter GetImporter(const TfToken& name) {
        return GetInstance()._GetImporter(name);
    }
    MAYAUSD_CORE_PUBLIC
    static TfTokenVector ListExporters() {
        return GetInstance()._ListExporters();
    }
    MAYAUSD_CORE_PUBLIC
    static TfTokenVector ListImporters() {
        return GetInstance()._ListImporters();
    }

    MAYAUSD_CORE_PUBLIC
    static UsdMayaShadingModeRegistry& GetInstance();

    MAYAUSD_CORE_PUBLIC
    bool RegisterExporter(
            const std::string& name,
            UsdMayaShadingModeExporterCreator fn);

    MAYAUSD_CORE_PUBLIC
    bool RegisterImporter(
            const std::string& name,
            UsdMayaShadingModeImporter fn);

private:
    MAYAUSD_CORE_PUBLIC
    UsdMayaShadingModeExporterCreator _GetExporter(const TfToken& name);
    MAYAUSD_CORE_PUBLIC
    UsdMayaShadingModeImporter _GetImporter(const TfToken& name);

    MAYAUSD_CORE_PUBLIC TfTokenVector _ListExporters();
    MAYAUSD_CORE_PUBLIC TfTokenVector _ListImporters();

    UsdMayaShadingModeRegistry();
    ~UsdMayaShadingModeRegistry();
    friend class TfSingleton<UsdMayaShadingModeRegistry>;
};

#define DEFINE_SHADING_MODE_IMPORTER(name, contextName) \
static MObject _ShadingModeImporter_##name(UsdMayaShadingModeImportContext*); \
TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShadingModeImportContext, name) {\
    UsdMayaShadingModeRegistry::GetInstance().RegisterImporter(#name, &_ShadingModeImporter_##name); \
}\
MObject _ShadingModeImporter_##name(UsdMayaShadingModeImportContext* contextName)


PXR_NAMESPACE_CLOSE_SCOPE


#endif
