//
// Copyright 2019 Luma Pictures
// Copyright 2023 Autodesk, Inc. All rights reserved.
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

#include "mixedUtils.h"

#include <mayaHydraLib/adapters/mayaAttrs.h>
#include <mayaHydraLib/hydraUtils.h>

#include <maya/MDagPath.h>
#include <maya/MHWGeometry.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MRenderUtil.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAHYDRA_NS_DEF {

TfToken GetFileTexturePath(const MFnDependencyNode& fileNode)
{
    if (fileNode.findPlug(MayaAttrs::file::uvTilingMode, true).asShort() != 0) {
        const TfToken ret {
            fileNode.findPlug(MayaAttrs::file::fileTextureNamePattern, true).asString().asChar()
        };
        return ret.IsEmpty()
            ? TfToken { fileNode.findPlug(MayaAttrs::file::computedFileTextureNamePattern, true)
                            .asString()
                            .asChar() }
            : ret;
    } else {
        const TfToken ret { MRenderUtil::exactFileTextureName(fileNode.object()).asChar() };
        return ret.IsEmpty() ? TfToken { fileNode.findPlug(MayaAttrs::file::fileTextureName, true)
                                             .asString()
                                             .asChar() }
                             : ret;
    }
}

bool IsShape(const MDagPath& dagPath)
{
    if (dagPath.hasFn(MFn::kTransform)) {
        return false;
    }

    // go to the parent
    MDagPath parentDagPath = dagPath;
    parentDagPath.pop();
    if (!parentDagPath.hasFn(MFn::kTransform)) {
        return false;
    }

    unsigned int numberOfShapesDirectlyBelow = 0;
    parentDagPath.numberOfShapesDirectlyBelow(numberOfShapesDirectlyBelow);
    return (numberOfShapesDirectlyBelow == 1);
}

SdfPath DagPathToSdfPath(
    const MDagPath& dagPath,
    const bool      mergeTransformAndShape,
    const bool      stripNamespaces)
{
    std::string name = dagPath.fullPathName().asChar();
    SanitizeNameForSdfPath(name, stripNamespaces);
    SdfPath usdPath(name);

    if (mergeTransformAndShape && IsShape(dagPath)) {
        usdPath = usdPath.GetParentPath();
    }

    return usdPath;
}

SdfPath RenderItemToSdfPath(const MRenderItem& ri, const bool stripNamespaces)
{
    std::string internalObjectId(
        "_" + std::to_string(ri.InternalObjectId())); // preventively prepend item id by underscore
    std::string name(ri.name().asChar() + internalObjectId);
    // Try to sanitize maya path to be used as an sdf path.
    SanitizeNameForSdfPath(name, stripNamespaces);
    // Path names must start with a letter, not a number
    // If a number is found, prepend the path with an underscore
    char digit = name[0];
    if (std::isdigit(digit)) {
        name.insert(0, "_");
    }

    SdfPath sdfPath(name);
    if (!TF_VERIFY(
            !sdfPath.IsEmpty(),
            "Render item using invalid SdfPath '%s'. Using item's id instead.",
            name.c_str())) {
        // If failed to include render item's name as an SdfPath simply use the item id.
        return SdfPath(internalObjectId);
    }
    return sdfPath;
}

} // namespace MAYAHYDRA_NS_DEF
