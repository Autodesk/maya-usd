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

#include <maya/MDoubleArray.h>
#include <maya/MGlobal.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

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

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
