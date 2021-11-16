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
#ifndef PXRUSDMAYA_EXPORT_JOB_CONTEXT_REGISTRY_H
#define PXRUSDMAYA_EXPORT_JOB_CONTEXT_REGISTRY_H

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

TF_DECLARE_WEAK_PTRS(UsdMayaJobContextRegistry);

/// We understand it would be useful to have a unique entry point to enable all the job options
/// necessary for a specific task, either rendering or simulation. Therefore we provide a way to
/// register these broad categories and allow updating the import/export options to allow adding
/// task specific flags.
///
/// We provide macros that are entry points into the job context logic.
class UsdMayaJobContextRegistry : public TfWeakBase
{
public:
    /// An job context basically wraps a function that tweaks the set of import/export options. This
    /// job context has a name and UI components, as well as an enabler function that allows
    /// specifying the options dictionnary.
    ///
    /// To register an export job context, you need to use the REGISTER_EXPORT_JOB_CONTEXT macro for
    /// each export job context supported by the library.
    ///
    /// In order for the core system to discover the plugin, you need a \c plugInfo.json that
    /// declares job contexts. \code
    /// {
    ///   "Plugins": [
    ///     {
    ///       "Info": {
    ///         "UsdMaya": {
    ///          "JobContextPlugin": {
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

    /// Get all registered export job contexts:
    static TfTokenVector ListJobContexts() { return GetInstance()._ListJobContexts(); }

    /// All the information registered for a specific job context.
    struct ContextInfo
    {
        TfToken   jobContext;
        TfToken   niceName;
        TfToken   exportDescription;
        EnablerFn exportEnablerCallback;
        TfToken   importDescription;
        EnablerFn importEnablerCallback;

        ContextInfo() = default;

        ContextInfo(
            const TfToken& jc,
            const TfToken& nn,
            const TfToken& edsc,
            EnablerFn      eef,
            const TfToken& idsc,
            EnablerFn      ief)
            : jobContext(jc)
            , niceName(nn)
            , exportDescription(edsc)
            , exportEnablerCallback(eef)
            , importDescription(idsc)
            , importEnablerCallback(ief)
        {
        }
    };

    /// Gets the conversion information associated with \p jobContext on export and import
    static const ContextInfo& GetJobContextInfo(const TfToken& jobContext)
    {
        return GetInstance()._GetJobContextInfo(jobContext);
    }

    /// Registers an export job context, with nice name, description and enabler function.
    ///
    /// The \p jobContext name will be used directly in the render option string as one of
    /// the valid values of the job context option.
    ///
    /// The \p niceName is the name displayed in the options dialog.
    ///
    /// The \p description is displayed as a tooltip in the  options dialog.
    ///
    /// The \p enablerFct will be called after option parsing to enable context specific options.
    MAYAUSD_CORE_PUBLIC
    void RegisterExportJobContext(
        const std::string& jobContext,
        const std::string& niceName,
        const std::string& description,
        EnablerFn          enablerFct,
        bool               fromPython = false);

    /// Registers an import job context, with nice name, description and enabler function.
    ///
    /// The \p jobContext name will be used directly in the render option string as one of
    /// the valid values of the job context option.
    ///
    /// The \p niceName is the name displayed in the options dialog.
    ///
    /// The \p description is displayed as a tooltip in the  options dialog.
    ///
    /// The \p enablerFct will be called after option parsing to enable context specific options.
    MAYAUSD_CORE_PUBLIC
    void RegisterImportJobContext(
        const std::string& jobContext,
        const std::string& niceName,
        const std::string& description,
        EnablerFn          enablerFct,
        bool               fromPython = false);

    MAYAUSD_CORE_PUBLIC
    static UsdMayaJobContextRegistry& GetInstance();

private:
    MAYAUSD_CORE_PUBLIC TfTokenVector _ListJobContexts();
    MAYAUSD_CORE_PUBLIC const ContextInfo& _GetJobContextInfo(const TfToken&);

    UsdMayaJobContextRegistry();
    ~UsdMayaJobContextRegistry();
    friend class TfSingleton<UsdMayaJobContextRegistry>;
};

#define REGISTER_EXPORT_JOB_CONTEXT(name, niceName, description, enablerFct) \
    TF_REGISTRY_FUNCTION(UsdMayaJobContextRegistry)                          \
    {                                                                        \
        UsdMayaJobContextRegistry::GetInstance().RegisterExportJobContext(   \
            name, niceName, description, enablerFct);                        \
    }

#define REGISTER_EXPORT_JOB_CONTEXT_FCT(name, niceName, description)         \
    static VtDictionary _ExportJobContextEnabler_##name();                   \
    TF_REGISTRY_FUNCTION(UsdMayaJobContextRegistry)                          \
    {                                                                        \
        UsdMayaJobContextRegistry::GetInstance().RegisterExportJobContext(   \
            #name, niceName, description, &_ExportJobContextEnabler_##name); \
    }                                                                        \
    VtDictionary _ExportJobContextEnabler_##name()

#define REGISTER_IMPORT_JOB_CONTEXT(name, niceName, description, enablerFct) \
    TF_REGISTRY_FUNCTION(UsdMayaJobContextRegistry)                          \
    {                                                                        \
        UsdMayaJobContextRegistry::GetInstance().RegisterImportJobContext(   \
            name, niceName, description, enablerFct);                        \
    }

#define REGISTER_IMPORT_JOB_CONTEXT_FCT(name, niceName, description)         \
    static VtDictionary _ImportJobContextEnabler_##name();                   \
    TF_REGISTRY_FUNCTION(UsdMayaJobContextRegistry)                          \
    {                                                                        \
        UsdMayaJobContextRegistry::GetInstance().RegisterImportJobContext(   \
            #name, niceName, description, &_ImportJobContextEnabler_##name); \
    }                                                                        \
    VtDictionary _ImportJobContextEnabler_##name()

PXR_NAMESPACE_CLOSE_SCOPE

#endif
