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

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
/*! \brief  Register USD plugins with USD / MayaUsd / Python version checks

    Plug registry plugins (either pure USD, like render delegates or MayaUsd ones like translators)
   should never be used with a mismatched version of shared libraries.

    When all components are compiled together, there is no chance for version mismatch and
   PXR_PLUGINPATH_NAME is the proper way to discover and register such plugins.

    Plugins distributed separately from MayaUSD should use MAYA_PXR_PLUGINPATH_NAME env variable to
   point to a folder with mayaUsdPlugInfo.json file. The JSON file is used to discover plugin paths
   to register after running requested version checks at runtime. Here is an example file:
    {
       "MayaUsdIncludes":[
          {
             "PlugPath":"testPlugModule1",
             "VersionCheck":{
                "Python":"3",
                "USD":"0.20.8"
             }
          },
          {
             "PlugPath":"testPlugModule2",
             "VersionCheck":{
                "MayaUsd":"0.6.0"
             }
          },
          {
             "PlugPath":"testPlugModule3"
          },
          {
             "PlugPath":"testPlugModule4",
             "VersionCheck":{
                "MayaUsd":"0.0.0"
             }
          },
          {
             "PlugPath":"testPlugModule5",
             "VersionCheck":{
                "Python":"1"
             }
          },
          {
             "PlugPath":"testPlugModule6",
             "VersionCheck":{
                "USD":"0.0.0"
             }
          }
       ]
    }

    The plugin must decide which validation checks are needed by listing them in "VersionCheck"
   object. Supported checks are:
        - "Python"
        - "USD"
        - "MayaUsd"

    Every plugin passing version check will get registered in plug registry via
   `PlugRegistry::GetInstance().RegisterPlugins` method.
 */
void registerVersionedPlugins();

} // namespace MAYAUSD_NS_DEF

#endif
