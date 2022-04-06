//
// Copyright 2021 Autodesk
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
#include "mtlxSymmetricShaderWriter.h"

#include "shadingTokens.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/utils/roundTripUtil.h>
#include <mayaUsd/fileio/utils/shadingUtil.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/converter.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>

#include <maya/MFnAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// Register symmetric writers:
TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry)
{
    // The MxVersion will export nodes to the MaterialX NodeGraph, as expected:
    MtlxUsd_SymmetricShaderWriter::RegisterWriter(
        TrMayaTokens->floatCorrect, TrMtlxTokens->LdkND_FloatCorrect_float);
    MtlxUsd_SymmetricShaderWriter::RegisterWriter(
        TrMayaTokens->colorCorrect, TrMtlxTokens->LdkND_ColorCorrect_color4);
};

/* static */
void MtlxUsd_SymmetricShaderWriter::RegisterWriter(
    const TfToken& mayaNodeTypeName,
    const TfToken& usdShaderId,
    bool           fromPython)
{
    UsdMayaShaderWriterRegistry::Register(
        mayaNodeTypeName,
        [](const UsdMayaJobExportArgs& exportArgs) {
            return MtlxUsd_SymmetricShaderWriter::CanExport(exportArgs);
        },
        [usdShaderId](
            const MFnDependencyNode& depNodeFn,
            const SdfPath&           usdPath,
            UsdMayaWriteJobContext&  jobCtx) {
            return std::make_shared<MtlxUsd_SymmetricShaderWriter>(
                depNodeFn, usdPath, jobCtx, usdShaderId);
        },
        fromPython);
}

/* static */
UsdMayaShaderWriter::ContextSupport
MtlxUsd_SymmetricShaderWriter::CanExport(const UsdMayaJobExportArgs& exportArgs)
{
    if (exportArgs.convertMaterialsTo == TrMtlxTokens->conversionName) {
        return ContextSupport::Supported;
    }

    return ContextSupport::Unsupported;
}

MtlxUsd_SymmetricShaderWriter::MtlxUsd_SymmetricShaderWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx,
    const TfToken&           usdShaderId)
    : MtlxUsd_BaseWriter(depNodeFn, usdPath, jobCtx)
{
    // Everything must be added in the material node graph:
    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    if (!TF_VERIFY(
            nodegraphSchema,
            "Could not get UsdShadeNodeGraph at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    SdfPath nodegraphPath = nodegraphSchema.GetPath();
    SdfPath nodePath
        = nodegraphPath.AppendChild(TfToken(UsdMayaUtil::SanitizeName(depNodeFn.name().asChar())));

    UsdShadeShader shaderSchema = UsdShadeShader::Define(GetUsdStage(), nodePath);
    if (!TF_VERIFY(
            shaderSchema,
            "Could not define UsdShadeShader at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    _usdPrim = shaderSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdShadeShader at path '%s'\n",
            shaderSchema.GetPath().GetText())) {
        return;
    }

    shaderSchema.CreateIdAttr(VtValue(usdShaderId));

    for (unsigned int i = 0u; i < depNodeFn.attributeCount(); ++i) {
        const MObject      attrObj = depNodeFn.reorderedAttribute(i);
        MPlug              attrPlug = depNodeFn.findPlug(attrObj, true);
        const MFnAttribute attrFn(attrObj);

        if (attrPlug.isProcedural() || attrFn.isHidden()) {
            // The Maya docs say these should not be saved off.
            continue;
        }

        if (attrPlug.isChild()) {
            continue;
        }

        // For now, we only support arrays of length 1. If we encounter such an
        // array, we emit it's 0-th element.
        if (attrPlug.isArray()) {
            const unsigned int numElements = attrPlug.evaluateNumElements();
            if (numElements == 0u) {
                // Empty array, so skip this attribute.
                continue;
            }

            if (numElements > 1u) {
                TF_WARN(
                    "Array with multiple elements encountered for "
                    "attribute '%s' on node '%s'. Currently, only arrays "
                    "with a single element are supported.",
                    attrPlug.name().asChar(),
                    UsdMayaUtil::GetMayaNodeName(depNodeFn.object()).c_str());
            }

            attrPlug = attrPlug[0];
        }

        // Keep our authoring sparse by ignoring attributes with no values set
        // and no connections.
        if (!UsdMayaUtil::IsAuthored(attrPlug) && !attrPlug.isConnected()) {
            continue;
        }

        TfToken usdAttrName = TfToken(UsdMayaShadingUtil::GetStandardAttrName(attrPlug, false));
        if (usdAttrName.IsEmpty()) {
            continue;
        }

        SdfValueTypeName valueTypeName = MayaUsd::Converter::getUsdTypeName(attrPlug);
        if (!valueTypeName) {
            // Unsupported Maya attribute type (e.g. "message" attributes).
            continue;
        }

        // If the Maya attribute is writable, we assume it must be an input.
        // Inputs can still be connected as sources to inputs on other shaders.
        if (attrFn.isWritable()) {
            UsdShadeInput input = shaderSchema.CreateInput(usdAttrName, valueTypeName);
            if (!input) {
                continue;
            }

            if (attrPlug.isElement()) {
                UsdMayaRoundTripUtil::MarkAttributeAsArray(input.GetAttr(), 0u);
            }

            // Add this input to the name/attrPlug map. We'll iterate through
            // these entries during Write() to set their values.
            _inputNameAttrMap.insert(std::make_pair(usdAttrName, attrPlug));
        } else if (attrPlug.isConnected()) {
            // Only author outputs for non-writable attributes if they are
            // connected.
            if (usdAttrName == TrMayaTokens->outColor || usdAttrName == TrMayaTokens->outAlpha) {
                // If the Maya node has both attributes, then we know the underlying MaterialX node
                // definition did export only a single color4 output:
                if (!depNodeFn.attribute(TrMayaTokens->outColor.GetText()).isNull()
                    && !depNodeFn.attribute(TrMayaTokens->outAlpha.GetText()).isNull()) {
                    usdAttrName = TrMayaTokens->outColor;
                    valueTypeName = SdfValueTypeNames->Color4f;
                }
            }
            shaderSchema.CreateOutput(usdAttrName, valueTypeName);
        }
    }
}

/* override */
void MtlxUsd_SymmetricShaderWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaShaderWriter::Write(usdTime);

    UsdShadeShader shaderSchema(_usdPrim);
    if (!TF_VERIFY(
            shaderSchema,
            "Could not get UsdShadeShader schema for UsdPrim at path '%s'\n",
            _usdPrim.GetPath().GetText())) {
        return;
    }

    for (const auto& inputAttrPair : _inputNameAttrMap) {
        const TfToken& inputName = inputAttrPair.first;
        const MPlug&   attrPlug = inputAttrPair.second;

        UsdShadeInput input = shaderSchema.GetInput(inputName);
        if (!input) {
            continue;
        }

        UsdMayaWriteUtil::SetUsdAttr(attrPlug, input.GetAttr(), usdTime, _GetSparseValueWriter());
    }
}

/* override */
UsdAttribute MtlxUsd_SymmetricShaderWriter::GetShadingAttributeForMayaAttrName(
    const TfToken&          mayaAttrName,
    const SdfValueTypeName& typeName)
{
    UsdShadeShader shaderSchema(_usdPrim);
    if (!shaderSchema) {
        return {};
    }

    // Just check whether we created an input or an attribute with this name

    const UsdShadeInput input = shaderSchema.GetInput(mayaAttrName);
    if (input) {
        return input;
    }

    // color and alpha outputs might have been combined:
    if (mayaAttrName == TrMayaTokens->outColor || mayaAttrName == TrMayaTokens->outAlpha) {
        // If the Maya node has both attributes, then we know the underlying MaterialX node
        // definition did export only a single color4 output:
        MStatus                 status;
        const MFnDependencyNode depNodeFn(GetMayaObject(), &status);

        if (status == MS::kSuccess
            && !depNodeFn.attribute(TrMayaTokens->outColor.GetText()).isNull()
            && !depNodeFn.attribute(TrMayaTokens->outAlpha.GetText()).isNull()) {
            UsdShadeOutput mainOutput = shaderSchema.GetOutput(TrMayaTokens->outColor);

            if (mayaAttrName == TrMayaTokens->outColor) {
                if (mainOutput.GetTypeName() == typeName) {
                    return mainOutput;
                }

                // If types differ, then we need to handle all possible conversions and
                // channel swizzling.
                return AddSwizzleConversion(typeName, mainOutput);
            }

            // Subcomponent requests:
            if (mayaAttrName == TrMayaTokens->outColorR) {
                return AddSwizzle("r", 4, mainOutput);
            }

            if (mayaAttrName == TrMayaTokens->outColorG) {
                return AddSwizzle("g", 4, mainOutput);
            }

            if (mayaAttrName == TrMayaTokens->outColorB) {
                return AddSwizzle("b", 4, mainOutput);
            }

            if (mayaAttrName == TrMayaTokens->outAlpha) {
                return AddSwizzle("a", 4, mainOutput);
            }
        }
    }

    // TODO: Even in this fallback case we might be hitting subcomponent requests. Need to get the
    // right channel by finding if the attribute has a parent, getting the index in that parent, and
    // swizzling either rgba for color types, or xyzw for vector types.

    const UsdShadeOutput output = shaderSchema.GetOutput(mayaAttrName);
    if (output) {
        return output;
    }

    return {};
}

PXR_NAMESPACE_CLOSE_SCOPE
