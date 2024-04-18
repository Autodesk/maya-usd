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
#include "MayaUsdUIInfoHandler.h"

#include <pxr/base/arch/fileSystem.h>
#include <pxr/base/tf/getenv.h>
#include <pxr/usd/ar/defaultResolverContext.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/ar/resolverContextBinder.h>

#include <maya/MDoubleArray.h>
#include <maya/MGlobal.h>

#ifdef UFE_V4_FEATURES_AVAILABLE
#include <maya/MSceneMessage.h>
#include <ufe/nodeDef.h>
#include <ufe/nodeDefHandler.h>
#include <ufe/runTimeMgr.h>
#endif

#include <algorithm>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdUIInfoHandler, MayaUsdUIInfoHandler);

MayaUsdUIInfoHandler::MayaUsdUIInfoHandler()
    : UsdUfe::UsdUIInfoHandler()
{
    // Register a callback to invalidate the invisible color.
    fColorChangedCallbackId = MEventMessage::addEventCallback(
        "DisplayRGBColorChanged", onColorChanged, reinterpret_cast<void*>(this));

    // Immediately update the invisible color to get a starting current value.
    updateInvisibleColor();
}

MayaUsdUIInfoHandler::~MayaUsdUIInfoHandler()
{
    // Unregister the callback used to invalidate the invisible color.
    if (fColorChangedCallbackId)
        MMessage::removeCallback(fColorChangedCallbackId);
}

/*static*/
MayaUsdUIInfoHandler::Ptr MayaUsdUIInfoHandler::create()
{
    return std::make_shared<MayaUsdUIInfoHandler>();
}

void MayaUsdUIInfoHandler::updateInvisibleColor()
{
    // Retrieve the invisible color of the Maya Outliner.
    //
    // We *cannot* intialize it in treeViewCellInfo() because
    // that function gets called in a paint event and calling
    // a command in a painting event can cause a recursive paint
    // event if commands echoing is on, which can corrupt the
    // Qt paint internal which lead to a crash. Typical symptom
    // is that the state variable of the Qt paint engine becomes
    // null midway through the repaint.

    MDoubleArray color;
    MGlobal::executeCommand("displayRGBColor -q \"outlinerInvisibleColor\"", color);

    if (color.length() == 3) {
        color.get(fInvisibleColor.data());
    }
}

/*static*/
void MayaUsdUIInfoHandler::onColorChanged(void* data)
{
    MayaUsdUIInfoHandler* infoHandler = reinterpret_cast<MayaUsdUIInfoHandler*>(data);
    if (!infoHandler)
        return;

    infoHandler->updateInvisibleColor();
}

UsdUfe::UsdUIInfoHandler::SupportedTypesMap MayaUsdUIInfoHandler::getSupportedIconTypes() const
{
    auto supportedTypes = Parent::getSupportedIconTypes();

    // We support these node types directly.
    static const UsdUfe::UsdUIInfoHandler::SupportedTypesMap mayaSupportedTypes {
        { "MayaReference", "out_USD_MayaReference.png" },
        { "ALMayaReference", "out_USD_MayaReference.png" }, // Same as mayaRef
    };
    supportedTypes.insert(mayaSupportedTypes.begin(), mayaSupportedTypes.end());
    return supportedTypes;
}

#ifdef UFE_V4_FEATURES_AVAILABLE
namespace {
class _MayaIconResolver
{
public:
    ~_MayaIconResolver() { Terminate(); }

    static _MayaIconResolver& Get()
    {
        static _MayaIconResolver sResolver;
        return sResolver;
    }

    bool FileExists(const std::string& iconName)
    {
        // Since this will be hitting the filesystem hard, mostly to find nothing, let's cache
        // search results. Note that "XBMLANGPATH" is Maya specific, which is why the code is here
        // and not in the base class.
        auto cachedHit = _searchCache.find(iconName);
        if (cachedHit != _searchCache.end()) {
            return cachedHit->second;
        }

        // Would be better using MQtUtil::createPixmap, but this requires linking against
        // QtCore.
        PXR_NS::ArResolverContextBinder binder(_iconContext);
        if (!PXR_NS::ArGetResolver().Resolve(iconName).empty()) {
            _searchCache.insert({ iconName, true });
            return true;
        }

        _searchCache.insert({ iconName, false });
        return false;
    }

    void ResetCache()
    {
        auto pathVector
            = PXR_NS::TfStringSplit(PXR_NS::TfGetenv("XBMLANGPATH", ""), ARCH_PATH_LIST_SEP);
#ifdef LINUX
        // The paths on Linux end with /%B. Trim that:
        for (auto&& path : pathVector) {
            const auto pathLen = path.size();
            if (pathLen >= 3 && path.substr(pathLen - 3) == "/%B") {
                path = path.substr(0, pathLen - 3);
            }
        }
#endif
        _iconContext = PXR_NS::ArDefaultResolverContext(pathVector);
        _searchCache.clear();
    }

    static void OnPluginStateChange(const MStringArray& /*strs*/, void* /*clientData*/)
    {
        _MayaIconResolver::Get().ResetCache();
    }

    void Terminate()
    {
        if (_pluginLoadCB) {
            MMessage::removeCallback(_pluginLoadCB);
            _pluginLoadCB = 0;
        }
        if (_pluginUnloadCB) {
            MMessage::removeCallback(_pluginUnloadCB);
            _pluginLoadCB = 0;
        }
        if (_beforeExitCB) {
            MMessage::removeCallback(_beforeExitCB);
            _beforeExitCB = 0;
        }
    }

    static void OnTerminateCache(void* /*clientData*/) { _MayaIconResolver::Get().Terminate(); }

private:
    _MayaIconResolver()
    {
        ResetCache();

        // Set up callback to notify of plugin load and unload
        _pluginLoadCB = MSceneMessage::addStringArrayCallback(
            MSceneMessage::kAfterPluginLoad, OnPluginStateChange);
        _pluginUnloadCB = MSceneMessage::addStringArrayCallback(
            MSceneMessage::kAfterPluginUnload, OnPluginStateChange);
        _beforeExitCB = MSceneMessage::addCallback(MSceneMessage::kMayaExiting, OnTerminateCache);
    }

    PXR_NS::ArDefaultResolverContext      _iconContext;
    std::unordered_map<std::string, bool> _searchCache;
    MCallbackId                           _pluginLoadCB = 0;
    MCallbackId                           _pluginUnloadCB = 0;
    MCallbackId                           _beforeExitCB = 0;
};
} // namespace
#endif

Ufe::UIInfoHandler::Icon
MayaUsdUIInfoHandler::treeViewIcon(const Ufe::SceneItem::Ptr& mayaItem) const
{
    Ufe::UIInfoHandler::Icon icon = Parent::treeViewIcon(mayaItem);

#ifdef UFE_V4_FEATURES_AVAILABLE
    if (icon.baseIcon == "out_USD_Shader.png") {
        // Naming convention for third party shader outliner icons:
        //
        //  We take the info:id of the shader and make it safe by replacing : with _.
        //  Then we search the Maya icon paths for a PNG file with that name. If found we will use
        //  it. Please note that files with _150 and _200 can also be provided for high DPI
        //  displays.
        //
        //   For example an info:id of:
        //       MyRenderer:nifty_surface
        //   On a USD runtime item will have this code search the full Maya icon path for a file
        //   named:
        //       out_USD_MyRenderer_nifty_surface.png
        //   And will use it if found. At resolution 200%, the file:
        //       out_USD_MyRenderer_nifty_surface_200.png
        //   Will alternatively be used if found.
        //
        const auto nodeDefHandler
            = Ufe::RunTimeMgr::instance().nodeDefHandler(mayaItem->runTimeId());
        if (!nodeDefHandler) {
            return icon;
        }
        const auto nodeDef = nodeDefHandler->definition(mayaItem);
        if (!nodeDef || nodeDef->type().empty()) {
            return icon;
        }

        constexpr auto outlinerPrefix = "out_";
        const auto     runtimeName = Ufe::RunTimeMgr::instance().getName(mayaItem->runTimeId());
        std::string    iconName = outlinerPrefix + runtimeName + "_" + nodeDef->type();
        std::replace(iconName.begin(), iconName.end(), ':', '_');
        iconName += ".png";

        if (_MayaIconResolver::Get().FileExists(iconName)) {
            icon.baseIcon = iconName;
        }
    }
#endif

    return icon;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
