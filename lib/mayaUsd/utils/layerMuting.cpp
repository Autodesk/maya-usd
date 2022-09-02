//
// Copyright 2022 Autodesk
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

#include "layerMuting.h"

#include "dynamicAttribute.h"

#include <maya/MCommandResult.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>

namespace MAYAUSD_NS_DEF {

namespace {

const char layerMutingAttrName[] = "usdStageLayerMuting";

// The USD layer identifier has an unspecified format that could change.
// In particular, we don't know what charaters it can use. To avoid problem
// when setting dynamic attributes, we convert the identifier into characters
// from A to P
// to ensure we don't get any issue and can select the separator used to split
// identifiers as we wish.

MString convertIdentifierToText(const std::string& identifier)
{
    // Note: do *not* use MString to concatenate as it treat single char as int!
    std::string text;

    for (const char c : identifier) {
        const char c1 = 'A' + (((unsigned int)c) / 16u);
        const char c2 = 'A' + (((unsigned int)c) % 16u);
        text += c1;
        text += c2;
    }

    return text.c_str();
}

std::string convertTextToIdentifier(const std::string& text)
{
    std::string identifier;

    for (size_t index = 0; index < text.length(); index += 2) {
        const char c1 = text[index];
        const char c2 = text[index + 1];
        const char c
            = (char)((((unsigned int)(c1 - 'A') * 16)) + (((unsigned int)(c2 - 'A')) % 16));
        identifier += c;
    }

    return identifier;
}

} // namespace

bool hasLayerMutingAttribute(const MObject& obj)
{
    return hasDynamicAttribute(MFnDependencyNode(obj), layerMutingAttrName);
}

MStatus copyLayerMutingToAttribute(const PXR_NS::UsdStage& stage, MObject& obj)
{
    MFnDependencyNode depNode(obj);
    if (!hasDynamicAttribute(depNode, layerMutingAttrName))
        createDynamicAttribute(depNode, layerMutingAttrName);

    MString layerMutingText = convertLayerMutingToText(stage);

    MStatus status = setDynamicAttribute(depNode, layerMutingAttrName, layerMutingText);

    return status;
}

MStatus copyLayerMutingFromAttribute(const MObject& obj, PXR_NS::UsdStage& stage)
{
    MFnDependencyNode depNode(obj);
    if (!hasDynamicAttribute(depNode, layerMutingAttrName))
        return MS::kNotFound;

    MString layerMutingText;
    MStatus status = getDynamicAttribute(depNode, layerMutingAttrName, layerMutingText);
    if (status == MS::kSuccess)
        setLayerMutingFromText(stage, layerMutingText);

    return status;
}

MString convertLayerMutingToText(const PXR_NS::UsdStage& stage)
{
    MString text;

    std::vector<std::string> muted = stage.GetMutedLayers();
    for (const std::string& item : muted) {
        if (text.length() > 0) {
            text += "/";
        }
        text += convertIdentifierToText(item);
    }

    return text;
}

void setLayerMutingFromText(PXR_NS::UsdStage& stage, const MString& text)
{
    MStringArray muted;
    text.split('/', muted);

    for (const MString& item : muted) {
        const std::string identifier = convertTextToIdentifier(item.asChar());
        stage.MuteLayer(identifier);
    }
}

} // namespace MAYAUSD_NS_DEF
