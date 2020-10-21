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
#ifndef MAYAUSD_PLUGREGISTRYHELPER_H
#define MAYAUSD_PLUGREGISTRYHELPER_H

#include <mayaUsd/base/api.h>

#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/getenv.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/stringUtils.h>

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF
{
    /*! \brief  Register USD plugins matching USD version distributed with MayaUSD.

        USD Plugins registered in the plug registry via PXR_PLUGINPATH_NAME have to link against
        the same version of USD as MayaUSD. Plugins distributed separately from MayaUSD can use
        MAYA_PXR_PLUGINPATH_NAME env variable to point to a folder with multiple binaries organized
        in folders per USD versions. MayaUSD will append to each path USD version and call plug registry
        to register the plugins.
     */
    inline void registerVersionedPlugins()
    {
        static std::once_flag once;
        std::call_once(once, [](){
            static std::string usd_version = std::to_string(PXR_VERSION);
            
            std::vector<std::string> pluginsToRegister;
            
            const std::string paths = TfGetenv("MAYA_PXR_PLUGINPATH_NAME");
            for (const auto& path: TfStringSplit(paths, ARCH_PATH_LIST_SEP)) {
                if (path.empty()) {
                    continue;
                }

                if(TfIsRelativePath(path)) {
                    TF_CODING_ERROR("Relative paths are unsupported for MAYA_PXR_PLUGINPATH_NAME: '%s'", path.c_str());
                    continue;
                }
                
                pluginsToRegister.push_back(TfStringCatPaths(path, usd_version));
            }
            
            PlugRegistry::GetInstance().RegisterPlugins(pluginsToRegister);
        });
    }
} // namespace MayaUsd

#endif
