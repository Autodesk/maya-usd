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
#include "mtlxBaseWriter.h"
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

/// \class MtlxUsd_SymmetricShaderWriter
/// \brief Provides "literal" translation of Maya shading nodes to USD Shader
/// prims that are MaterialX-compatible.
///
/// This shader writer performs a "literal" translation of a Maya shading node
/// type to USD. Input and output attributes on the Maya node translate
/// directly to inputs and outputs with the same names on the exported
/// UsdShadeShader. With one major exception: color and alpha are kept together
/// to solve some temporary issues about multi-output management in MaterialX/USD.
///
/// A static "RegisterWriter()" function is provided to simplify the
/// registration of writers that use this class. Note however that it should be
/// called inside a "TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry)" block,
/// for example:
///
/// \code
/// TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry)
/// {
///     MtlxUsd_SymmetricShaderWriter::RegisterWriter(
///         TfToken("checker"),
///         TfToken("MayaND_checker_color3"));
/// };
/// \endcode
///
class MtlxUsd_SymmetricShaderWriter : public MtlxUsd_BaseWriter
{
public:
    /// Register a shader writer to translate \p mayaNodeTypeName Maya nodes to
    /// USD shaders with ID \p usdShaderId.
    ///
    /// Note that this function should generally only be called inside a
    /// TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry) block.
    static void RegisterWriter(
        const TfToken& mayaNodeTypeName,
        const TfToken& usdShaderId,
        bool           inNodeGraph = true,
        bool           fromPython = false);

    static ContextSupport CanExport(const UsdMayaJobExportArgs& exportArgs);

    MtlxUsd_SymmetricShaderWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx,
        const TfToken&           usdShaderId,
        bool                     inNodeGraph);

    void Write(const UsdTimeCode& usdTime) override;

    UsdAttribute GetShadingAttributeForMayaAttrName(
        const TfToken&          mayaAttrName,
        const SdfValueTypeName& typeName) override;

private:
    std::unordered_map<TfToken, MPlug, TfToken::HashFunctor> _inputNameAttrMap;
};

// Register symmetric writers:
TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry)
{
    // These nodes are not by default in a node graph. Might change as we start exporting layered
    // surfaced.
    MtlxUsd_SymmetricShaderWriter::RegisterWriter(
        TrMayaTokens->lambert, TrMtlxTokens->MayaND_lambert_surfaceshader, false);
    MtlxUsd_SymmetricShaderWriter::RegisterWriter(
        TrMayaTokens->phong, TrMtlxTokens->MayaND_phong_surfaceshader, false);
    MtlxUsd_SymmetricShaderWriter::RegisterWriter(
        TrMayaTokens->blinn, TrMtlxTokens->MayaND_blinn_surfaceshader, false);

    // These nodes are always in a NodeGraph:
    MtlxUsd_SymmetricShaderWriter::RegisterWriter(
        TrMayaTokens->floatCorrect, TrMtlxTokens->LdkND_FloatCorrect_float);
    MtlxUsd_SymmetricShaderWriter::RegisterWriter(
        TrMayaTokens->colorCorrect, TrMtlxTokens->LdkND_ColorCorrect_color4);
    MtlxUsd_SymmetricShaderWriter::RegisterWriter(
        TrMayaTokens->clamp, TrMtlxTokens->MayaND_clamp_vector3);
};

/* static */
void MtlxUsd_SymmetricShaderWriter::RegisterWriter(
    const TfToken& mayaNodeTypeName,
    const TfToken& usdShaderId,
    bool           inNodeGraph,
    bool           fromPython)
{
    UsdMayaShaderWriterRegistry::Register(
        mayaNodeTypeName,
        [](const UsdMayaJobExportArgs& exportArgs) {
            return MtlxUsd_SymmetricShaderWriter::CanExport(exportArgs);
        },
        [usdShaderId, inNodeGraph](
            const MFnDependencyNode& depNodeFn,
            const SdfPath&           usdPath,
            UsdMayaWriteJobContext&  jobCtx) {
            return std::make_shared<MtlxUsd_SymmetricShaderWriter>(
                depNodeFn, usdPath, jobCtx, usdShaderId, inNodeGraph);
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
    const TfToken&           usdShaderId,
    bool                     inNodeGraph)
    : MtlxUsd_BaseWriter(depNodeFn, usdPath, jobCtx)
{
    SdfPath nodePath = GetUsdPath();
    if (inNodeGraph) {
        // Utility nodes must be added in the material node graph:
        UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
        if (!TF_VERIFY(
                nodegraphSchema,
                "Could not get UsdShadeNodeGraph at path '%s'\n",
                GetUsdPath().GetText())) {
            return;
        }

        nodePath = nodegraphSchema.GetPath().AppendChild(
            TfToken(UsdMayaUtil::SanitizeName(depNodeFn.name().asChar())));
    }
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
        if (!UsdMayaUtil::IsAuthored(attrPlug) && !attrPlug.isConnected()
            && !attrPlug.numConnectedChildren()) {
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
        } else if (attrPlug.isConnected() || attrPlug.numConnectedChildren()) {
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
        if (!input || attrPlug.isConnected() || attrPlug.numConnectedChildren()) {
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

    UsdShadeInput input = shaderSchema.GetInput(mayaAttrName);
    if (input) {
        return PreserveNodegraphBoundaries(input);
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
                return AddConversion(typeName, mainOutput);
            }

            // Subcomponent requests:
            if (mayaAttrName == TrMayaTokens->outColorR) {
                return ExtractChannel(0, mainOutput);
            }

            if (mayaAttrName == TrMayaTokens->outColorG) {
                return ExtractChannel(1, mainOutput);
            }

            if (mayaAttrName == TrMayaTokens->outColorB) {
                return ExtractChannel(2, mainOutput);
            }

            if (mayaAttrName == TrMayaTokens->outAlpha) {
                return ExtractChannel(3, mainOutput);
            }
        }
    }

    UsdShadeOutput output = shaderSchema.GetOutput(mayaAttrName);
    if (output) {
        if (output.GetTypeName() == typeName) {
            return output;
        }

        // If types differ, then we need to handle all possible conversions and
        // channel swizzling.
        return AddConversion(typeName, output);
    }

    // We did not find the attribute directly, but we might be dealing with a subcomponent
    // connection on a compound attribute:
    MStatus                 status;
    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);

    MPlug childPlug = depNodeFn.findPlug(mayaAttrName.GetText(), &status);
    if (!status || childPlug.isNull() || !childPlug.isChild()) {
        return {};
    }

    MPlug              parentPlug = childPlug.parent();
    unsigned int       childIndex = 0;
    const unsigned int numChildren = parentPlug.numChildren();
    for (; childIndex < numChildren; ++childIndex) {
        if (childPlug.attribute() == parentPlug.child(childIndex).attribute()) {
            break;
        }
    }

    // We need the long name of the attribute:
    const TfToken parentAttrName(
        parentPlug.partialName(false, false, false, false, false, true).asChar());
    output = shaderSchema.GetOutput(parentAttrName);
    if (output) {
        return ExtractChannel(static_cast<size_t>(childIndex), output);
    }

    input = shaderSchema.GetInput(parentAttrName);
    if (input) {
        return AddConstructor(
            PreserveNodegraphBoundaries(input), static_cast<size_t>(childIndex), parentPlug);
    }

    return {};
}

PXR_NAMESPACE_CLOSE_SCOPE
