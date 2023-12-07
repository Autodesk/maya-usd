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
#include "colorManagementPreferences.h"

#include <maya/MEventMessage.h>
#include <maya/MGlobal.h>
#include <maya/MSceneMessage.h>

#include <set>

namespace MAYAUSD_NS_DEF {

// We will cache the color management preferences since they are used in many loops.
ColorManagementPreferences::~ColorManagementPreferences() { RemoveSinks(); }

bool ColorManagementPreferences::Active() { return Get()._active; }

const MString& ColorManagementPreferences::RenderingSpaceName()
{
    return Get()._renderingSpaceName;
}

const MString& ColorManagementPreferences::sRGBName() { return Get()._sRGBName; }

std::string ColorManagementPreferences::getFileRule(const std::string& path)
{
    MString colorRuleCmd;
    colorRuleCmd.format("colorManagementFileRules -evaluate \"^1s\";", MString(path.c_str()));
    return MGlobal::executeCommandStringResult(colorRuleCmd).asChar();
}

void ColorManagementPreferences::SetDirty()
{
    auto& self = InternalGet();
    self._dirty = true;
}

void ColorManagementPreferences::MayaExit()
{
    auto& self = InternalGet();
    self.RemoveSinks();
}

ColorManagementPreferences::ColorManagementPreferences() = default;

const ColorManagementPreferences& ColorManagementPreferences::Get()
{
    auto& self = InternalGet();
    self.Refresh();
    return self;
}
ColorManagementPreferences& ColorManagementPreferences::InternalGet()
{
    static ColorManagementPreferences _self;
    return _self;
}

void ColorManagementPreferences::RemoveSinks()
{
    for (auto id : _mayaColorManagementCallbackIds) {
        MMessage::removeCallback(id);
    }
    _mayaColorManagementCallbackIds.clear();
}

void colorManagementRefreshCB(void*) { ColorManagementPreferences::SetDirty(); }

void mayaExitingCB(void*) { ColorManagementPreferences::MayaExit(); }

void ColorManagementPreferences::Refresh()
{
    if (_mayaColorManagementCallbackIds.empty()) {
        // Monitor color management prefs
        _mayaColorManagementCallbackIds.push_back(MEventMessage::addEventCallback(
            "colorMgtEnabledChanged", colorManagementRefreshCB, this));
        _mayaColorManagementCallbackIds.push_back(MEventMessage::addEventCallback(
            "colorMgtWorkingSpaceChanged", colorManagementRefreshCB, this));
        _mayaColorManagementCallbackIds.push_back(MEventMessage::addEventCallback(
            "colorMgtConfigChanged", colorManagementRefreshCB, this));
        _mayaColorManagementCallbackIds.push_back(MEventMessage::addEventCallback(
            "colorMgtConfigFilePathChanged", colorManagementRefreshCB, this));
        // The color management settings are quietly reset on file new:
        _mayaColorManagementCallbackIds.push_back(
            MSceneMessage::addCallback(MSceneMessage::kBeforeNew, colorManagementRefreshCB, this));
        _mayaColorManagementCallbackIds.push_back(
            MSceneMessage::addCallback(MSceneMessage::kBeforeOpen, colorManagementRefreshCB, this));

        // Cleanup on exit:
        _mayaColorManagementCallbackIds.push_back(
            MSceneMessage::addCallback(MSceneMessage::kMayaExiting, mayaExitingCB, this));
    }

    if (!_dirty) {
        return;
    }
    _dirty = false;

    int isActive = 0;
    MGlobal::executeCommand("colorManagementPrefs -q -cmEnabled", isActive, false, false);
    if (!isActive) {
        _active = false;
        return;
    }

    _active = true;

    _renderingSpaceName
        = MGlobal::executeCommandStringResult("colorManagementPrefs -q -renderingSpaceName");

    // Need some robustness around sRGB since not all OCIO configs declare it the same way:
    const auto sRGBAliases
        = std::set<std::string> { "sRGB",         "sRGB - Texture",
                                  "srgb_tx",      "Utility - sRGB - Texture",
                                  "srgb_texture", "Input - Generic - sRGB - Texture" };

    MStringArray allInputSpaces;
    MGlobal::executeCommand(
        "colorManagementPrefs -q -inputSpaceNames", allInputSpaces, false, false);

    for (auto&& spaceName : allInputSpaces) {
        if (sRGBAliases.count(spaceName.asChar())) {
            _sRGBName = spaceName;
            break;
        }
    }
}

} // namespace MAYAUSD_NS_DEF
