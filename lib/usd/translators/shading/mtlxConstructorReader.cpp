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
#include "shadingTokens.h"

#include <mayaUsd/fileio/shaderReader.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/fileio/utils/shadingUtil.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

PXR_NAMESPACE_OPEN_SCOPE

// Reader for a Constructor input placed on a MaterialX node. Combined with
// MtlxUsd_BaseReader::TraverseUnconnectableInput this allows handling connections on subcomponents.
class MtlxUsd_ConstructorReader : public UsdMayaShaderReader
{
public:
    MtlxUsd_ConstructorReader(const UsdMayaPrimReaderArgs& readArgs)
        : UsdMayaShaderReader(readArgs) {};

    bool Read(UsdMayaPrimReaderContext& context) override
    {
        const auto&    prim = _GetArgs().GetUsdPrim();
        UsdShadeShader shaderSchema = UsdShadeShader(prim);
        if (!shaderSchema) {
            return false;
        }

        // The source node has registered us so we can get back the Maya node:
        MObject _mayaObject = context.GetMayaNode(prim.GetPath(), false);
        if (_mayaObject.isNull()) {
            return false;
        }

        if (prim.GetPath().GetName().rfind(TrMtlxTokens->ConstructorPrefix.GetString(), 0) != 0) {
            // Needs to follow the standard naming conventson, especially since the input name
            // is encoded in the name.
            return false;
        }

        std::string primName = prim.GetPath().GetName();
        size_t      lastUnderscorePos = primName.rfind('_');
        if (lastUnderscorePos == std::string::npos) {
            return false;
        }

        _parentAttributeName = primName.substr(lastUnderscorePos + 1);
        if (_parentAttributeName.empty()) {
            return false;
        }

        MStatus           status;
        MFnDependencyNode depFn(_mayaObject, &status);
        if (!status) {
            return false;
        }

        MPlug parentPlug = depFn.findPlug(_parentAttributeName.c_str(), true, &status);
        if (!status) {
            return false;
        }

        // Read back values that were unconnected in the CTOR:
        for (unsigned int i = 0; i < parentPlug.numChildren(); ++i) {
            TfToken inputName(TfStringPrintf("in%u", i + 1));

            UsdAttribute compAttr = shaderSchema.GetInput(inputName);
            VtValue      val;
            if (compAttr && compAttr.Get(&val) && val.IsHolding<float>()) {
                MPlug childPlug = parentPlug.child(i);
                childPlug.setFloat(val.UncheckedGet<float>());
            }
        }

        return true;
    }

    MPlug
    GetMayaPlugForUsdAttrName(const TfToken& usdAttrName, const MObject& _mayaObject) const override
    {
        TfToken               usdPortName;
        UsdShadeAttributeType attrType;
        std::tie(usdPortName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

        if (attrType == UsdShadeAttributeType::Output) {
            return {};
        }

        // We expect in1, in2, in3, and in4:
        const std::string& portName = usdPortName.GetString();
        if (portName.size() != 3 || !isdigit(portName.back())) {
            return {};
        }

        // Figure out which child plug to return:
        MStatus           status;
        MFnDependencyNode depFn(_mayaObject, &status);
        if (!status) {
            return {};
        }

        MPlug parentPlug = depFn.findPlug(_parentAttributeName.c_str(), true, &status);
        if (!status) {
            return {};
        }

        unsigned int childIndex = std::stoul(portName.substr(2));

        if (childIndex == 0 || childIndex > parentPlug.numChildren()) {
            return {};
        }

        return parentPlug.child(childIndex - 1);
    }

private:
    std::string _parentAttributeName;
    MObject     _mayaObject;
};

PXRUSDMAYA_REGISTER_SHADER_READER(ND_combine2_vector2, MtlxUsd_ConstructorReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_combine3_vector3, MtlxUsd_ConstructorReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_combine4_vector4, MtlxUsd_ConstructorReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_combine3_color3, MtlxUsd_ConstructorReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_combine4_color4, MtlxUsd_ConstructorReader);

PXR_NAMESPACE_CLOSE_SCOPE
