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

#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/utils/converter.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MFnAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>
#include <maya/MGlobal.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

#include <cmath>
#include <boost/algorithm/string.hpp>

using namespace MAYAUSD_NS;

PXR_NAMESPACE_OPEN_SCOPE

class Maya_GenericWriter : public UsdMayaShaderWriter {
public:
    Maya_GenericWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

    TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) override;

    static ContextSupport CanExport(const UsdMayaJobExportArgs&);
private:
    bool _IsConnectable(MPlug & plug);
};

// Custom registration since all dependency nodes with a drawdb/shader classification are supported
TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShaderWriterRegistry, Maya_GenericWriter)
{
    MString nodeTypesCmd("stringArrayToString(listNodeTypes(\"drawdb/shader\"), \" \");");
    std::string nodeTypes = MGlobal::executeCommandStringResult(nodeTypesCmd).asChar();
    std::vector<std::string> mayaTypeNames;
    boost::split(mayaTypeNames, nodeTypes, boost::is_space());
    mayaTypeNames.push_back("place3dTexture"); // drawdb/geometry
    for (const std::string& mayaTypeName: mayaTypeNames) {
        UsdMayaShaderWriterRegistry::Register(
            TfToken(mayaTypeName),
            &Maya_GenericWriter::CanExport,
            [](const MFnDependencyNode& depNodeFn,
                const SdfPath&           usdPath,
                UsdMayaWriteJobContext&  jobCtx) {
                return std::make_shared<Maya_GenericWriter>(depNodeFn, usdPath, jobCtx);
            });
    }
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    // Material conversion and render context:
    (maya)
    ((niceName, "Maya Shaders"))
    ((description, "Dumps the bound shader in a Maya UsdShade network that can only be used for "
                   "import. Will not render in the Maya viewport or usdView."))
);

REGISTER_SHADING_MODE_EXPORT_MATERIAL_CONVERSION(
    _tokens->maya,
    _tokens->maya,
    _tokens->niceName,
    _tokens->description);

UsdMayaShaderWriter::ContextSupport
Maya_GenericWriter::CanExport(const UsdMayaJobExportArgs& exportArgs)
{
    return exportArgs.convertMaterialsTo == _tokens->maya ? ContextSupport::Supported
                                                          : ContextSupport::Unsupported;
}

Maya_GenericWriter::Maya_GenericWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaShaderWriter(depNodeFn, usdPath, jobCtx)
{
    UsdShadeShader shaderSchema = UsdShadeShader::Define(GetUsdStage(), GetUsdPath());
    if (!TF_VERIFY(
            shaderSchema,
            "Could not define UsdShadeShader at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    UsdAttribute idAttr = shaderSchema.CreateIdAttr(VtValue(TfToken(depNodeFn.typeName().asChar())));

    _usdPrim = shaderSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdShadeShader at path '%s'\n",
            shaderSchema.GetPath().GetText())) {
        return;
    }
}

bool Maya_GenericWriter::_IsConnectable(MPlug & plug)
{
    MStatus    status;
    MPlugArray connections;
    if (plug.connectedTo(connections, true, true, &status) && status == MS::kSuccess) {
        auto connectionsLength = connections.length();
        for (uint32_t i = 0; i < connectionsLength; ++i)
        {
            MObject otherNode = connections[i].node();
            MFnDependencyNode depNodeFn(otherNode, &status);
            if (status != MS::kSuccess) {
                continue;
            }
            // See if it is connected to something exportable:
            UsdMayaShaderWriterRegistry::WriterFactoryFn primWriterFactory
                = UsdMayaShaderWriterRegistry::Find(
                    TfToken(depNodeFn.typeName().asChar()), _GetExportArgs());
            if (primWriterFactory) {
                return true;
            }
        }
    }

    return false;
}

/* virtual */
void Maya_GenericWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaShaderWriter::Write(usdTime);
    MStatus status;

    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    UsdShadeShader shaderSchema(_usdPrim);
    if (!TF_VERIFY(
            shaderSchema,
            "Could not get UsdShadeShader schema for UsdPrim at path '%s'\n",
            _usdPrim.GetPath().GetText())) {
        return;
    }

    const unsigned int attributeCount = depNodeFn.attributeCount(&status);
    if (status != MS::kSuccess) {
        return;
    }

    for (unsigned int attributeIndex = 0; attributeIndex < attributeCount; ++attributeIndex) {
        MObject attributeObject = depNodeFn.reorderedAttribute(attributeIndex, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        MFnAttribute attributeFn(attributeObject, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        MPlug shadingNodePlug = depNodeFn.findPlug(attributeObject, true, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        auto attributeTypeName = Converter::getUsdTypeName(shadingNodePlug);
        if (!attributeTypeName) {
            continue;
        }

        TfToken attributeName(attributeFn.name().asChar());

        UsdShadeInput shaderInput;
        if (_IsConnectable(shadingNodePlug)) {
            if (attributeFn.isWritable()) {
                shaderInput = shaderSchema.CreateInput(attributeName, attributeTypeName);
                // Might be an I/O attribute with a set value. Look for a value.
            } else {
                shaderSchema.CreateOutput(attributeName, attributeTypeName);
                continue; // Output only attribute. We are done.
            }
        }

        attributeFn.parent(&status);
        const bool isChild = (status != MS::kNotFound);
        if (isChild || attributeFn.isHidden() || !attributeFn.isWritable()) {
            continue;
        }

        if (!UsdMayaUtil::IsAuthored(shadingNodePlug)) {
            continue;
        }

        VtValue value = UsdMayaWriteUtil::GetVtValue(shadingNodePlug, attributeTypeName, false);
        if (value.IsEmpty()) {
            continue;
        }

        if (!shaderInput) {
            shaderInput = shaderSchema.CreateInput(attributeName, attributeTypeName);
        }
        shaderInput.Set(value, usdTime);
    }
}

/* virtual */
TfToken Maya_GenericWriter::GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName)
{
    MStatus                 status;
    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return TfToken();
    }

    MPlug shadingNodePlug
        = depNodeFn.findPlug(depNodeFn.attribute(mayaAttrName.GetText()), true, &status);
    if (status != MS::kSuccess) {
        return TfToken();
    }

    MFnAttribute attributeFn(shadingNodePlug.attribute());

    return UsdShadeUtils::GetFullName(
        mayaAttrName,
        attributeFn.isWritable() ? UsdShadeAttributeType::Input : UsdShadeAttributeType::Output);
}

PXR_NAMESPACE_CLOSE_SCOPE
