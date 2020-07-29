//
// Copyright 2017 Animal Logic
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
#pragma once

#include <AL/maya/utils/FileTranslatorBase.h>
#include <AL/maya/utils/PluginTranslatorOptions.h>
#include <AL/usdmaya/Api.h>
#include <AL/usdmaya/fileio/ImportParams.h>

namespace AL {
namespace usdmaya {
namespace fileio {

using AL::maya::utils::PluginTranslatorOptions;
using AL::maya::utils::PluginTranslatorOptionsContext;
using AL::maya::utils::PluginTranslatorOptionsContextManager;
using AL::maya::utils::PluginTranslatorOptionsInstance;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A USD importer into Maya (partially supporting Animal Logic specific things)
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_TRANSLATOR_BEGIN(
    ImportTranslator,
    "AL usdmaya import",
    true,
    false,
    "*.usda",
    "*.usdc;*.usda;*.usd;*.usdt");

/// \fn     void* creator()
/// \brief  creates an instance of the file translator
/// \return a new file translator instance

/// \var    const char* const kTranslatorName
/// \brief  the name of the file translator

/// \var    const char* const kClassName
/// \brief  the name of the C++ file translator class

// specify the option names (These will uniquely identify the exporter options)
static constexpr const char* const kParentPath = "Parent Path"; ///< the parent path option name
static constexpr const char* const kPrimPath = "Prim Path";     ///< the prim path option name
static constexpr const char* const kAnimations
    = "Import Animations"; ///< the import animation option name
static constexpr const char* const kDynamicAttributes
    = "Import Dynamic Attributes"; ///< the import dynamic attributes option name
static constexpr const char* const kStageUnload = "Load None"; ///< the import animation option name
static constexpr const char* const kReadDefaultValues
    = "Read Default Values"; ///< the import animation option name
static constexpr const char* const kActivateAllTranslators = "Activate all Plugin Translators";
static constexpr const char* const kActiveTranslatorList = "Active Translator List";
static constexpr const char* const kInactiveTranslatorList = "Inactive Translator List";

/// \brief  provide a method to specify the import options
/// \param  options a set of options that are constructed and later used for option parsing
/// \return MS::kSuccess if ok
static MStatus specifyOptions(AL::maya::utils::FileTranslatorOptions& options)
{
    if (!options.addFrame("AL USD Importer Options"))
        return MS::kFailure;
    if (!options.addString(kParentPath, ""))
        return MS::kFailure;
    if (!options.addString(kPrimPath, ""))
        return MS::kFailure;
    if (!options.addBool(kAnimations, true))
        return MS::kFailure;
    if (!options.addBool(kDynamicAttributes, true))
        return MS::kFailure;
    if (!options.addBool(kStageUnload, false))
        return MS::kFailure;
    if (!options.addBool(kReadDefaultValues, true))
        return MS::kFailure;
    if (!options.addBool(kActivateAllTranslators, true))
        return MS::kFailure;
    if (!options.addString(kActiveTranslatorList, ""))
        return MS::kFailure;
    if (!options.addString(kInactiveTranslatorList, ""))
        return MS::kFailure;

    // register the export translator context
    PluginTranslatorOptionsContextManager::registerContext("ImportTranslator", &m_pluginContext);
    return MS::kSuccess;
}

/// \brief clean up the options registered for this translator
/// \param  options a set of options that are constructed and later used for option parsing
/// \return MS::kSuccess if ok
static MStatus cleanupOptions(AL::maya::utils::FileTranslatorOptions& options)
{
    if (!options.removeFrame("AL USD Importer Options"))
        return MS::kFailure;

    // unregister the export translator context
    PluginTranslatorOptionsContextManager::unregisterContext("ImportTranslator");
    return MS::kSuccess;
}

void prepPluginOptions() override
{
    if (m_pluginContext.dirty()) {
        delete m_pluginInstance;
        m_pluginInstance = new PluginTranslatorOptionsInstance(m_pluginContext);
        setPluginOptionsContext(m_pluginInstance);
    }
}

static PluginTranslatorOptionsContext& pluginContext() { return m_pluginContext; }

private:
AL_USDMAYA_PUBLIC
static PluginTranslatorOptionsContext m_pluginContext;
AL_USDMAYA_PUBLIC
static PluginTranslatorOptions* m_compatPluginOptions;
AL_USDMAYA_PUBLIC
static PluginTranslatorOptionsInstance* m_pluginInstance;
AL_USDMAYA_PUBLIC
MStatus reader(
    const MFileObject&                    file,
    const AL::maya::utils::OptionsParser& options,
    FileAccessMode                        mode) override;
ImporterParams m_params;

AL_MAYA_TRANSLATOR_END();

//----------------------------------------------------------------------------------------------------------------------
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
