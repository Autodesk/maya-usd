//
// Copyright 2023 Autodesk
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
#ifndef COLOR_MANAGEMENT_PREFERENCES
#define COLOR_MANAGEMENT_PREFERENCES

#include <mayaUsd/base/api.h>

#include <maya/MMessage.h>
#include <maya/MString.h>

#include <set>
#include <vector>

namespace MAYAUSD_NS_DEF {

/*! \brief  Cache of color management preferences and queries.

    Getting the information involves calling MEL scripts, so we cache the results for better
   performance.
*/
class MAYAUSD_CORE_PUBLIC ColorManagementPreferences
{
public:
    ~ColorManagementPreferences();

    /*! \brief  Is color management active.
     */
    static bool Active();

    /*! \brief  The current DCC rendering space name.
     */
    static const MString& RenderingSpaceName();

    /*! \brief  The current DCC color space name for plain sRGB

        Color management config files can rename or alias the sRGB color space name. We try a few
       common names and remember the first one that is found in the config.
     */
    static const MString& sRGBName();

    /*! \brief Prevent error spamming in the script editor by remembering failed requests
               for a color management fragment.
     */
    static bool isUnknownColorSpace(const std::string& colorSpace);
    static void addUnknownColorSpace(const std::string& colorSpace);

    /*! \brief  Returns the OCIO color space name according to config file rules.

        \param path The path of the file to be color managed.
     */
    static std::string getFileRule(const std::string& path);

    /*! \brief  Utility function to reset all cached data.
     */
    static void SetDirty();

    /*! \brief  Utility function to reset all message handlers on exit.
     */
    static void MayaExit();

private:
    ColorManagementPreferences();
    static const ColorManagementPreferences& Get();
    static ColorManagementPreferences&       InternalGet();

    bool                     _dirty = true;
    bool                     _active = false;
    MString                  _renderingSpaceName;
    MString                  _sRGBName;
    std::set<std::string>    _unknownColorSpaces;
    std::vector<MCallbackId> _mayaColorManagementCallbackIds;

    void Refresh();
    void RemoveSinks();
};

} // namespace MAYAUSD_NS_DEF

#endif
