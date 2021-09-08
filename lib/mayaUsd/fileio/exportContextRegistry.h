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
#ifndef PXRUSDMAYA_EXPORT_CONTEXT_REGISTRY_H
#define PXRUSDMAYA_EXPORT_CONTEXT_REGISTRY_H

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/declarePtrs.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/singleton.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/pxr.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(UsdMayaExportContextRegistry);

/// We understand it would be useful to have a unique entry point to enable all the export options
/// necessary for a specific task, either rendering or simulation. Therefore we provide a way to
/// register these broad categories and allow updating the export options to allow adding task
/// specific flags.
///
/// We provide macros that are entry points into the export context logic.
class UsdMayaExportContextRegistry : public TfWeakBase
{
public:
    /// An export context basically wraps a function that tweaks the set of export options. This
    /// context has a name and UI components, as well as an enabler function that allows modifying
    /// the options dictionnary.
    ///
    /// To register an export context, you need to use the REGISTER_EXPORT_CONTEXT macro for each
    /// export context supported by the library.
    ///
    /// In order for the core system to discover the plugin, you need a \c plugInfo.json that
    /// declares export contexts. \code
    /// {
    ///   "Plugins": [
    ///     {
    ///       "Info": {
    ///         "UsdMaya": {
    ///          "ExportContextPlugin": {
    ///            "mayaPlugin": "usdTestMayaPlugin" // (optional)
    ///          }
    ///         }
    ///       },
    ///       "Name": "myUsdPlugin",
    ///       "LibraryPath": "../myUsdPlugin.[dll|dylib|so]",
    ///       "Type": "library"
    ///     }
    ///   ]
    /// }
    /// \endcode
    ///
    /// If a mayaPlugin entry is provided, the plugin will be loaded via a call to loadPlugin inside
    /// Maya. Otherwise, the plugin at LibraryPath will be loaded via the regular USD plugin loading
    /// mechanism.

    /// Enabler function, returns a dictionary containing all the options for the context.
    typedef std::function<VtDictionary()> EnablerFn;

    /// Get all registered export conversions:
    static TfTokenVector ListExportContexts() { return GetInstance()._ListExportContexts(); }

    /// All the information registered for a specific export context.
    struct ContextInfo
    {
        std::string niceName;
        std::string description;
        EnablerFn   enablerCallback;

        ContextInfo() = default;

        ContextInfo(const std::string& nn, const std::string& dsc, EnablerFn ef)
            : niceName(nn)
            , description(dsc)
            , enablerCallback(ef)
        {
        }
    };

    /// Gets the conversion information associated with \p exportContext on export and import
    static const ContextInfo& GetExportContextInfo(const TfToken& exportContext)
    {
        return GetInstance()._GetExportContextInfo(exportContext);
    }

    /// Registers an export context, with nice name, description and enabler function.
    ///
    /// The \p exportContext name will be used directly in the render option string as one of
    /// the valid values of the convertMaterialsTo export option.
    ///
    /// The \p niceName is the name displayed in the render options dialog.
    ///
    /// The \p description is displayed as a tooltip in the render options dialog.
    ///
    /// The \p enablerFct will be called after option parsing to enable context specific options.
    MAYAUSD_CORE_PUBLIC
    void RegisterExportContext(
        const std::string& exportContext,
        const std::string& niceName,
        const std::string& description,
        EnablerFn          enablerFct);

    MAYAUSD_CORE_PUBLIC
    static UsdMayaExportContextRegistry& GetInstance();

private:
    MAYAUSD_CORE_PUBLIC TfTokenVector _ListExportContexts();
    MAYAUSD_CORE_PUBLIC const ContextInfo& _GetExportContextInfo(const TfToken&);

    UsdMayaExportContextRegistry();
    ~UsdMayaExportContextRegistry();
    friend class TfSingleton<UsdMayaExportContextRegistry>;
};

#define REGISTER_EXPORT_CONTEXT(name, niceName, description, enablerFct)   \
    TF_REGISTRY_FUNCTION(UsdMayaExportContextRegistry)                     \
    {                                                                      \
        UsdMayaExportContextRegistry::GetInstance().RegisterExportContext( \
            name, niceName, description, enablerFct);                      \
    }

#define REGISTER_EXPORT_CONTEXT_FCT(name, niceName, description)           \
    static VtDictionary _ExportContextEnabler_##name();                    \
    TF_REGISTRY_FUNCTION(UsdMayaExportContextRegistry)                     \
    {                                                                      \
        UsdMayaExportContextRegistry::GetInstance().RegisterExportContext( \
            #name, niceName, description, &_ExportContextEnabler_##name);  \
    }                                                                      \
    VtDictionary _ExportContextEnabler_##name()

PXR_NAMESPACE_CLOSE_SCOPE

#endif
