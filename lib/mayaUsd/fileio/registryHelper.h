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
#ifndef PXRUSDMAYA_REGISTRYHELPER_H
#define PXRUSDMAYA_REGISTRYHELPER_H

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/pxr.h>

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

/// private helper so that both reader/writer registries can share the same
/// plugin discovery/load mechanism.
struct UsdMaya_RegistryHelper
{
    /// searches plugInfo's for \p value at the specified \p scope.
    ///
    /// The scope are the nested keys to search through in the plugInfo (for
    /// example, ["UsdMaya", "PrimReader"].
    ///
    /// {
    ///   'UsdMaya': {
    ///     'PrimReader': {
    ///       'providesTranslator': [ ... ],
    ///       'mayaPlugin': "px_..."
    ///     }
    ///   }
    /// }
    ///
    /// At that scope, it expects a dictionary that has two keys:
    /// "providesTranslator" and "mayaPlugin".  If \p value matches the
    /// something in the "providesTranslator" list, it will proceed to try to
    /// load the "mayaPlugin".
    static void FindAndLoadMayaPlug(const std::vector<TfToken>& scope, const std::string& value);

    /// Searches the plugInfos and looks for ShadingModePlugin.
    ///
    /// "UsdMaya" : {
    ///     "ShadingModePlugin" : {
    ///         "mayaPlugin" : "arnoldShaderExporter"
    ///     }
    /// }
    ///
    /// At that scope, it expects a dictionary with one key: "mayaPlugin".
    /// usdMaya will try to load the "mayaPlugin" when shading modes are first accessed.
    static void LoadShadingModePlugins();

    /// Searches the plugInfos and looks for ExportContextPlugin.
    ///
    /// "UsdMaya" : {
    ///     "ExportContextPlugin" : {
    ///         "mayaPlugin" : "arnoldExporterContext"
    ///     }
    /// }
    ///
    /// At that scope, it expects a dictionary with one key: "mayaPlugin".
    /// usdMaya will try to load the "mayaPlugin" when exporter contexts are first accessed.
    static void LoadExportContextPlugins();

    /// Searches the plugInfos for metadata dictionaries at the given \p scope,
    /// and composes them together.
    /// The scope are the nested keys to search through in the plugInfo (for
    /// example, ["UsdMaya", "UsdExport"]).
    /// The same key under the \p scope must not be defined in multiple
    /// plugInfo.json files. If this occurs, the key will not be defined in the
    /// composed result, and this function will raise a coding error indicating
    /// where the keys have been multiply-defined.
    /// XXX We might relax the restriction on multiply-defined keys later on
    /// if there is a need to define values at different scopes, e.g.
    /// site-specific, department-specific, show-specific values.
    static VtDictionary GetComposedInfoDictionary(const std::vector<TfToken>& scope);

    static void AddUnloader(const std::function<void()>& func);

    // 2021-09-01 Temporary flag to avoid errors when using registry to Python classes
    // AddUnloader is not supported for Python Bindings
    // Still need to implement a way of automatically unregister when a python module is unloaded.
    // Or provide UnRegister functions.
    MAYAUSD_CORE_PUBLIC
    static bool g_pythonRegistry;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
