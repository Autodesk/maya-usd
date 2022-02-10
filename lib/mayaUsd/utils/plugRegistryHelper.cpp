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

#include "plugRegistryHelper.h"

#include <mayaUsd/base/debugCodes.h>

#include <pxr/base/js/json.h>
#include <pxr/base/js/value.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/getenv.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>

#include <fstream>
#include <string>

#if !defined(MAYAUSD_VERSION)
#error "MAYAUSD_VERSION is not defined"
#endif

#if !defined(MAYA_PY_VERSION)
#error "MAYA_PY_VERSION is not defined"
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x)  STRINGIFY(x)

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(_Tokens,
// Filename tokens
((MayaUsdPlugInfoName,  "mayaUsdPlugInfo.json"))

// Top level key
((IncludesKey,          "MayaUsdIncludes"))

// Include keys
((PlugPathKey,          "PlugPath"))
((VersionCheckKey,      "VersionCheck"))
                         
// VersionCheck keys
((CheckPythonKey,       "Python"))
((CheckUsdKey,          "USD"))
((CheckMayaUsdKey,      "MayaUsd"))
);
// clang-format on

/*! \brief  Read the mayaUsd plug info in pathname into result.
    \note   Heavily inspired by /pxr/base/plug/info.cpp
    \return true if the file could be opened.
 */
bool readPlugInfoObject(const std::string& pathname, JsObject& result)
{
    result.clear();

    // The file may not exist or be readable.
    std::ifstream ifs;
    ifs.open(pathname.c_str());
    if (!ifs.is_open()) {
        TF_WARN("Plugin info file %s couldn't be read", pathname.c_str());
        return false;
    }

    // The Js library doesn't allow comments, but we'd like to allow them.
    // Strip comments, retaining empty lines so line numbers reported in parse
    // errors match line numbers in the original file content.
    // NOTE: Joining a vector of strings and calling JsParseString()
    //       is *much* faster than writing to a stringstream and
    //       calling JsParseStream() as of this writing.
    std::string              line;
    std::vector<std::string> filtered;
    while (getline(ifs, line)) {
        if (line.find('#') < line.find_first_not_of(" \t#"))
            line.clear();
        filtered.push_back(line);
    }

    // Read JSON.
    JsParseError error;
    JsValue      plugInfo = JsParseString(TfStringJoin(filtered, "\n"), &error);

    // Validate.
    if (plugInfo.IsNull()) {
        TF_WARN(
            "Plugin info file %s couldn't be read "
            "(line %d, col %d): %s",
            pathname.c_str(),
            error.line,
            error.column,
            error.reason.c_str());
    } else if (!plugInfo.IsObject()) {
        // The contents didn't evaluate to a json object....
        TF_WARN("Plugin info file %s did not contain a JSON object", pathname.c_str());
    } else {
        result = plugInfo.GetJsObject();
    }
    return true;
}

/*! \brief  Perform version check for given MayaUsd plug info
    \return true if requested version are valid for current MayaUsd runtime configuration
*/
bool checkPluginVersions(
    const JsObject&    plugInfo,
    const std::string& pythonVersion,
    const std::string& usdVersion,
    const std::string& mayaUsdVersion,
    const std::string& debugLocation)
{
    JsObject::const_iterator checkIt = plugInfo.find(_Tokens->VersionCheckKey);
    if (checkIt == plugInfo.end()) {
        // Version check wasn't requested
        return true;
    }

    if (!checkIt->second.IsObject()) {
        TF_WARN(
            "Plugin info %s key '%s' doesn't hold an object",
            debugLocation.c_str(),
            checkIt->first.c_str());
        return false;
    }

    JsObject versionCheckObject = checkIt->second.GetJsObject();

    auto checkFn = [&versionCheckObject,
                    &debugLocation](const std::string& key, const std::string& versionValue) {
        JsObject::const_iterator checkIt = versionCheckObject.find(key);
        if (checkIt == versionCheckObject.end()) {
            // version check for this key was not requested
            TF_DEBUG(USDMAYA_PLUG_INFO_VERSION)
                .Msg(
                    "Plugin info %s version check '%s' not requested\n",
                    debugLocation.c_str(),
                    key.c_str());
            return true;
        }

        if (!checkIt->second.IsString()) {
            TF_WARN(
                "Plugin info %s key '%s' doesn't hold a string",
                debugLocation.c_str(),
                key.c_str());
            return false;
        }

        const std::string& requestedVersion = checkIt->second.GetString();
        if (versionValue == requestedVersion)
            return true;
        else {
            TF_DEBUG(USDMAYA_PLUG_INFO_VERSION)
                .Msg(
                    "Plugin info %s version check '%s' NOT match. "
                    "Requested '%s' but run under '%s'\n",
                    debugLocation.c_str(),
                    key.c_str(),
                    requestedVersion.c_str(),
                    versionValue.c_str());

            return false;
        }
    };

    if (!checkFn(_Tokens->CheckPythonKey, pythonVersion)) {
        return false;
    }

    if (!checkFn(_Tokens->CheckUsdKey, usdVersion)) {
        return false;
    }

    if (!checkFn(_Tokens->CheckMayaUsdKey, mayaUsdVersion)) {
        return false;
    }

    return true;
}

/*! \brief  Extract plugin path from MayaUsd plug info
    \return Valid absolute path for properly configured json file. Empty string otherwise.
*/
std::string getPluginPath(
    const JsObject&    plugInfo,
    const std::string& mayaUsdPluginInfoPath,
    const std::string& debugLocation)
{
    std::string pluginIncludePath;

    JsObject::const_iterator pluginPathIt = plugInfo.find(_Tokens->PlugPathKey);
    if (pluginPathIt != plugInfo.end()) {
        if (!pluginPathIt->second.IsString()) {
            TF_WARN(
                "Plugin info %s key %s doesn't hold a string",
                debugLocation.c_str(),
                pluginPathIt->first.c_str());
        } else {
            const std::string& includePath = pluginPathIt->second.GetString();
            if (TfIsRelativePath(includePath)) {
                pluginIncludePath = TfStringCatPaths(mayaUsdPluginInfoPath, includePath);
            } else {
                pluginIncludePath = includePath;
            }
        }
    }

    return pluginIncludePath;
}

} // namespace

namespace MAYAUSD_NS_DEF {

void registerVersionedPlugins()
{
    static std::once_flag once;
    std::call_once(once, []() {
        const std::string pythonVersion = TOSTRING(MAYA_PY_VERSION);
        const std::string usdVersion = TOSTRING(MAYA_USD_VERSION);
        const std::string mayaUsdVersion = TOSTRING(MAYAUSD_VERSION);

        std::vector<std::string> pluginsToRegister;

        const std::string paths = TfGetenv("MAYA_PXR_PLUGINPATH_NAME");
        for (const auto& path : TfStringSplit(paths, ARCH_PATH_LIST_SEP)) {
            if (path.empty()) {
                continue;
            }

            if (TfIsRelativePath(path)) {
                TF_WARN(
                    "Relative paths are unsupported for MAYA_PXR_PLUGINPATH_NAME: '%s'",
                    path.c_str());
                continue;
            }

            // Append the maya usd plug info file name
            std::string plugInfoPath
                = TfStringCatPaths(path, _Tokens->MayaUsdPlugInfoName.GetString());

            JsObject plugInfoObject;
            if (!readPlugInfoObject(plugInfoPath, plugInfoObject)) {
                continue;
            }

            JsObject::const_iterator topIncludesIt = plugInfoObject.find(_Tokens->IncludesKey);
            if (topIncludesIt == plugInfoObject.end() || !topIncludesIt->second.IsArray()) {
                TF_WARN(
                    "Plugin info file %s key '%s' doesn't hold an object",
                    plugInfoPath.c_str(),
                    _Tokens->IncludesKey.GetString().c_str());
                continue;
            }

            const JsArray& pluginIncludes = topIncludesIt->second.GetJsArray();
            for (size_t i = 0, n = pluginIncludes.size(); i != n; ++i) {
                const std::string debugLocation = TfStringPrintf(
                    "file %s %s[%zd]", plugInfoPath.c_str(), topIncludesIt->first.c_str(), i);

                if (!pluginIncludes[i].IsObject()) {
                    TF_WARN(
                        "Plugin info %s key '%s' doesn't hold an object",
                        debugLocation.c_str(),
                        topIncludesIt->first.c_str());
                    continue;
                }

                const JsObject& topPluginObject = pluginIncludes[i].GetJsObject();
                if (!checkPluginVersions(
                        topPluginObject,
                        pythonVersion,
                        usdVersion,
                        mayaUsdVersion,
                        debugLocation)) {
                    // skipping plugin because it didn't pass version check
                    continue;
                }

                std::string pluginPath = getPluginPath(topPluginObject, path, debugLocation);
                if (!pluginPath.empty()) {
                    TF_DEBUG(USDMAYA_PLUG_INFO_VERSION)
                        .Msg(
                            "Plugin info %s. Will request registration for '%s'\n",
                            debugLocation.c_str(),
                            pluginPath.c_str());

                    pluginsToRegister.push_back(pluginPath);
                }
            }
        }

        PlugRegistry::GetInstance().RegisterPlugins(pluginsToRegister);
    });
}
} // namespace MAYAUSD_NS_DEF
