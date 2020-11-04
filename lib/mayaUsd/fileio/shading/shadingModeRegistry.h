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

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/shading/shadingModeExporter.h>
#include <mayaUsd/fileio/shading/shadingModeExporterContext.h>
#include <mayaUsd/fileio/shading/shadingModeImporter.h>

#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/singleton.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/pxr.h>

#include <maya/MObject.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

#define PXRUSDMAYA_SHADINGMODE_TOKENS (none)(displayColor)(useRegistry)

TF_DECLARE_PUBLIC_TOKENS(
    UsdMayaShadingModeTokens,
    MAYAUSD_CORE_PUBLIC,
    PXRUSDMAYA_SHADINGMODE_TOKENS);

#define PXRUSDMAYA_SHADINGCONVERSION_TOKENS \
    (none)(lambert)(standardSurface)(usdPreviewSurface)(blinn)(phong)

TF_DECLARE_PUBLIC_TOKENS(
    UsdMayaPreferredMaterialTokens,
    MAYAUSD_CORE_PUBLIC,
    PXRUSDMAYA_SHADINGCONVERSION_TOKENS);

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
    static UsdMayaShadingModeExporterCreator GetExporter(const TfToken& name)
    {
        return GetInstance()._GetExporter(name);
    }
    static UsdMayaShadingModeImporter GetImporter(const TfToken& name)
    {
        return GetInstance()._GetImporter(name);
    }
    static TfTokenVector ListExporters() { return GetInstance()._ListExporters(); }
    static TfTokenVector ListImporters() { return GetInstance()._ListImporters(); }

    /// Gets the nice name of an exporter. Used for the UI label of the export options
    static const std::string& GetExporterNiceName(const TfToken& name)
    {
        return GetInstance()._GetExporterNiceName(name);
    }

    /// Gets the nice name of an importer. Used for the UI label of the import options
    static const std::string& GetImporterNiceName(const TfToken& name)
    {
        return GetInstance()._GetImporterNiceName(name);
    }

    /// Gets the description of an exporter. Used for the popup help of the export options
    static const std::string& GetExporterDescription(const TfToken& name)
    {
        return GetInstance()._GetExporterDescription(name);
    }

    /// Gets the description of an importer. Used for the popup help of the import options
    static const std::string& GetImporterDescription(const TfToken& name)
    {
        return GetInstance()._GetImporterDescription(name);
    }

    MAYAUSD_CORE_PUBLIC
    static UsdMayaShadingModeRegistry& GetInstance();

    MAYAUSD_CORE_PUBLIC
    bool RegisterExporter(
        const std::string&                name,
        std::string                       niceName,
        std::string                       description,
        UsdMayaShadingModeExporterCreator fn);

    MAYAUSD_CORE_PUBLIC
    bool RegisterImporter(
        const std::string&         name,
        std::string                niceName,
        std::string                description,
        UsdMayaShadingModeImporter fn);

    /// The useRegistry exporters and importers can be specialized to support material conversions.
    /// The most well known is the default conversion to UsdPreviewSurface shaders. This registry
    /// allows introducing other material conversions as necessary to support other renderers. We
    /// also allow specifying that an import path is available for these renderers if support has
    /// been implemented.
    ///
    /// To register a material conversion on export, you need to use the
    /// REGISTER_SHADING_MODE_EXPORT_MATERIAL_CONVERSION macro for each material conversion
    /// supported by the library. Multiple registration is supported, so each plugin should declare
    /// once the material conversions it supports.
    ///
    /// To register a material conversion on import, you need to use the
    /// REGISTER_SHADING_MODE_IMPORT_MATERIAL_CONVERSION macro for each material conversion.

    /// Get all registered export conversions:
    static TfTokenVector ListMaterialConversions()
    {
        return GetInstance()._ListMaterialConversions();
    }

    /// All the information registered for a specific material conversion.
    struct ConversionInfo
    {
        TfToken renderContext;
        TfToken niceName;
        TfToken exportDescription;
        TfToken importDescription;
        bool    hasExporter = false;
        bool    hasImporter = false;
    };

    /// Gets the conversion information associated with \p materialConversion on export and import
    static const ConversionInfo& GetMaterialConversionInfo(const TfToken& materialConversion)
    {
        return GetInstance()._GetMaterialConversionInfo(materialConversion);
    }

    /// Registers an export material conversion, with render context, nice name and description.
    ///
    /// The \p materialConversion name will be used directly in the render option string as one of
    /// the valid values of the convertMaterialsTo export option.
    ///
    /// The \p renderContext will be used to specialize the binding point. See UsdShadeMaterial
    /// documentation for details. A value of "UsdShadeTokens->universalRenderContext" should be
    /// used if the resulting UsdShade nodes are written using an API shared by multiple renderers,
    /// like UsdPreviewSurface or MaterialX. For UsdShade nodes targetting a specific rendering
    /// engine, please define a custom render context understood by the renderer.
    ///
    /// The \p niceName is the name displayed in the render options dialog.
    ///
    /// The \p description is displayed as a tooltip in the render options dialog.
    MAYAUSD_CORE_PUBLIC
    void RegisterExportConversion(
        const TfToken& materialConversion,
        const TfToken& renderContext,
        const TfToken& niceName,
        const TfToken& description);

    /// Registers an import material conversion, with render context, nice name and description.
    /// This is the import counterpart of the RegisterExportConversion to be used if importers are
    /// available for a specific materialConversion. This covers only the "where to look in USD"
    /// part of material import. Extra conversion to a specified Maya surface node requires setting
    /// the optional preferredMaterial import option.
    ///
    /// The \p materialConversion name will be used directly in the import option string as one of
    /// the valid values of the second parameter to the shadingMode list to search on import.
    ///
    /// The \p renderContext will be used to locate the specialized binding point in the USD data.
    /// See UsdShadeMaterial documentation for details.
    ///
    /// The \p niceName is the name to be displayed in the import options dialog.
    ///
    /// The \p description is displayed as a tooltip in the import options dialog.
    MAYAUSD_CORE_PUBLIC
    void RegisterImportConversion(
        const TfToken& materialConversion,
        const TfToken& renderContext,
        const TfToken& niceName,
        const TfToken& description);

private:
    MAYAUSD_CORE_PUBLIC
    UsdMayaShadingModeExporterCreator _GetExporter(const TfToken& name);
    MAYAUSD_CORE_PUBLIC const std::string& _GetExporterNiceName(const TfToken&);
    MAYAUSD_CORE_PUBLIC const std::string& _GetExporterDescription(const TfToken&);

    MAYAUSD_CORE_PUBLIC
    UsdMayaShadingModeImporter _GetImporter(const TfToken& name);
    MAYAUSD_CORE_PUBLIC const std::string& _GetImporterNiceName(const TfToken&);
    MAYAUSD_CORE_PUBLIC const std::string& _GetImporterDescription(const TfToken&);

    MAYAUSD_CORE_PUBLIC TfTokenVector _ListExporters();
    MAYAUSD_CORE_PUBLIC TfTokenVector _ListImporters();

    MAYAUSD_CORE_PUBLIC TfTokenVector _ListMaterialConversions();
    MAYAUSD_CORE_PUBLIC const ConversionInfo& _GetMaterialConversionInfo(const TfToken&);

    UsdMayaShadingModeRegistry();
    ~UsdMayaShadingModeRegistry();
    friend class TfSingleton<UsdMayaShadingModeRegistry>;
};

#define DEFINE_SHADING_MODE_IMPORTER(name, niceName, description, contextName) \
    static MObject _ShadingModeImporter_##name(                                \
        UsdMayaShadingModeImportContext*, const UsdMayaJobImportArgs&);        \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShadingModeImportContext, name)       \
    {                                                                          \
        UsdMayaShadingModeRegistry::GetInstance().RegisterImporter(            \
            #name, niceName, description, &_ShadingModeImporter_##name);       \
    }                                                                          \
    MObject _ShadingModeImporter_##name(                                       \
        UsdMayaShadingModeImportContext* contextName, const UsdMayaJobImportArgs&)

#define DEFINE_SHADING_MODE_IMPORTER_WITH_JOB_ARGUMENTS(                 \
    name, niceName, description, contextName, jobArgumentsName)          \
    static MObject _ShadingModeImporter_##name(                          \
        UsdMayaShadingModeImportContext*, const UsdMayaJobImportArgs&);  \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShadingModeImportContext, name) \
    {                                                                    \
        UsdMayaShadingModeRegistry::GetInstance().RegisterImporter(      \
            #name, niceName, description, &_ShadingModeImporter_##name); \
    }                                                                    \
    MObject _ShadingModeImporter_##name(                                 \
        UsdMayaShadingModeImportContext* contextName,                    \
        const UsdMayaJobImportArgs&      jobArgumentsName)

#define REGISTER_SHADING_MODE_EXPORT_MATERIAL_CONVERSION(                   \
    name, renderContext, niceName, description)                             \
    TF_REGISTRY_FUNCTION(UsdMayaShadingModeExportContext)                   \
    {                                                                       \
        UsdMayaShadingModeRegistry::GetInstance().RegisterExportConversion( \
            name, renderContext, niceName, description);                    \
    }

#define REGISTER_SHADING_MODE_IMPORT_MATERIAL_CONVERSION(                   \
    name, renderContext, niceName, description)                             \
    TF_REGISTRY_FUNCTION(UsdMayaShadingModeImportContext)                   \
    {                                                                       \
        UsdMayaShadingModeRegistry::GetInstance().RegisterImportConversion( \
            name, renderContext, niceName, description);                    \
    }

PXR_NAMESPACE_CLOSE_SCOPE

#endif
