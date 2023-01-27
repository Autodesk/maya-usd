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
#include "utils.h"

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

MObject GetConnectedFileNode(const MObject& obj, const TfToken& paramName)
{
    MStatus           status;
    MFnDependencyNode node(obj, &status);
    if (ARCH_UNLIKELY(!status)) {
        return MObject::kNullObj;
    }
    return GetConnectedFileNode(node, paramName);
}

MObject GetConnectedFileNode(const MFnDependencyNode& node, const TfToken& paramName)
{
    MPlugArray conns;
    node.findPlug(paramName.GetText(), true).connectedTo(conns, true, false);
    if (conns.length() == 0) {
        return MObject::kNullObj;
    }
    const auto ret = conns[0].node();
    if (ret.apiType() == MFn::kFileTexture) {
        return ret;
    }
    return MObject::kNullObj;
}

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

PXR_NAMESPACE_CLOSE_SCOPE
