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

#include <string>

#include <maya/MObject.h>

#include <pxr/pxr.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/singleton.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/weakBase.h>

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/shading/shadingModeExporter.h>
#include <mayaUsd/fileio/shading/shadingModeExporterContext.h>
#include <mayaUsd/fileio/shading/shadingModeImporter.h>

PXR_NAMESPACE_OPEN_SCOPE

#define PXRUSDMAYA_SHADINGMODE_TOKENS \
    (none) \
    (displayColor) \
    (useRegistry) \
    (preview)

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

    static UsdMayaShadingModeExporterCreator GetExporter(const TfToken& name) {
        return GetInstance()._GetExporter(name);
    }
    static UsdMayaShadingModeImporter GetImporter(const TfToken& name) {
        return GetInstance()._GetImporter(name);
    }
    static TfTokenVector ListExporters() {
        return GetInstance()._ListExporters();
    }
    static TfTokenVector ListImporters() {
        return GetInstance()._ListImporters();
    }

    /// Gets the nice name of an exporter. Used for the UI label of the export options
    static const std::string& GetExporterNiceName(const TfToken& name)
    {
        return GetInstance()._GetExporterNiceName(name);
    }

    /// Gets the description of an exporter. Used for the popup help of the export options
    static const std::string& GetExporterDescription(const TfToken& name)
    {
        return GetInstance()._GetExporterDescription(name);
    }

    MAYAUSD_CORE_PUBLIC
    static UsdMayaShadingModeRegistry& GetInstance();

    MAYAUSD_CORE_PUBLIC
    bool RegisterExporter(
            const std::string& name,
            std::string niceName,
            std::string description,
            UsdMayaShadingModeExporterCreator fn);

    MAYAUSD_CORE_PUBLIC
    bool RegisterImporter(
            const std::string& name,
            UsdMayaShadingModeImporter fn);

    /// The useRegistry importers and exporters can be specialized to support render contexts. The
    /// most well known is the default context where UsdPreviewSurface shaders are used. This
    /// registry allows introducing other render contexts as necessary to support other renderers
    /// and DCC. Look in "UsdShade Material Assignment", section "Refinement 2: Material Purpose"
    /// for more details.
    ///
    /// To register a render context, you need to use the REGISTER_SHADING_MODE_RENDER_CONTEXT macro
    /// for each render contexts supported by the library. Multiple registration is supported, so
    /// each plugin should declare once the render contexts it supports.
    ///
    static TfTokenVector ListRenderContexts() { return GetInstance()._ListRenderContexts(); }

    /// Gets the nice name of a render context. Used for the UI label of the export options
    static const std::string& GetRenderContextNiceName(const TfToken& renderContext)
    {
        return GetInstance()._GetRenderContextNiceName(renderContext);
    }

    /// Gets the description of a render context. Used for the popup help of the export options
    static const std::string& GetRenderContextDescription(const TfToken& renderContext)
    {
        return GetInstance()._GetRenderContextDescription(renderContext);
    }

    /// Registers a render context, along with the nice name and a description
    MAYAUSD_CORE_PUBLIC
    void RegisterRenderContext(
        const TfToken& renderContext,
        std::string    niceName,
        std::string    description);

private:
    MAYAUSD_CORE_PUBLIC
    UsdMayaShadingModeExporterCreator _GetExporter(const TfToken& name);
    MAYAUSD_CORE_PUBLIC const std::string& _GetExporterNiceName(const TfToken&);
    MAYAUSD_CORE_PUBLIC const std::string& _GetExporterDescription(const TfToken&);

    MAYAUSD_CORE_PUBLIC
    UsdMayaShadingModeImporter _GetImporter(const TfToken& name);

    MAYAUSD_CORE_PUBLIC TfTokenVector _ListExporters();
    MAYAUSD_CORE_PUBLIC TfTokenVector _ListImporters();

    MAYAUSD_CORE_PUBLIC TfTokenVector _ListRenderContexts();
    MAYAUSD_CORE_PUBLIC const std::string& _GetRenderContextNiceName(const TfToken&);
    MAYAUSD_CORE_PUBLIC const std::string& _GetRenderContextDescription(const TfToken&);

    UsdMayaShadingModeRegistry();
    ~UsdMayaShadingModeRegistry();
    friend class TfSingleton<UsdMayaShadingModeRegistry>;
};

#define DEFINE_SHADING_MODE_IMPORTER(name, contextName)                  \
    static MObject _ShadingModeImporter_##name(                          \
        UsdMayaShadingModeImportContext*, const UsdMayaJobImportArgs&);  \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShadingModeImportContext, name) \
    {                                                                    \
        UsdMayaShadingModeRegistry::GetInstance().RegisterImporter(      \
            #name, &_ShadingModeImporter_##name);                        \
    }                                                                    \
    MObject _ShadingModeImporter_##name(                                 \
        UsdMayaShadingModeImportContext* contextName, const UsdMayaJobImportArgs&)

#define DEFINE_SHADING_MODE_IMPORTER_WITH_JOB_ARGUMENTS(name, contextName, jobArgumentsName) \
    static MObject _ShadingModeImporter_##name(                                              \
        UsdMayaShadingModeImportContext*, const UsdMayaJobImportArgs&);                      \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShadingModeImportContext, name)                     \
    {                                                                                        \
        UsdMayaShadingModeRegistry::GetInstance().RegisterImporter(                          \
            #name, &_ShadingModeImporter_##name);                                            \
    }                                                                                        \
    MObject _ShadingModeImporter_##name(                                                     \
        UsdMayaShadingModeImportContext* contextName,                                        \
        const UsdMayaJobImportArgs&      jobArgumentsName)

#define REGISTER_SHADING_MODE_RENDER_CONTEXT(name, niceName, description) \
    TF_REGISTRY_FUNCTION(UsdMayaShadingModeExportContext)                 \
    {                                                                     \
        UsdMayaShadingModeRegistry::GetInstance().RegisterRenderContext(  \
            name, niceName, description);                                 \
    }

PXR_NAMESPACE_CLOSE_SCOPE


#endif
