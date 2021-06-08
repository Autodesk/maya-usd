//
// Copyright 2016 Pixar
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
#include "translatorUtil.h"

#include <mayaUsd/fileio/translators/NodeHelper.h>
#include <mayaUsd/fileio/translators/DgNodeHelper.h>

#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/translators/translatorXformable.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/fileio/utils/xformStack.h>
#include <mayaUsd/utils/util.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/schema.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/xformable.h>

#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MFn.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE


UsdDataType getAttributeType(const UsdAttribute& usdAttr)
{
    if (!usdAttr.IsValid()) {
        return UsdDataType::kUnknown;
    }
    const SdfValueTypeName typeName = usdAttr.GetTypeName();
    const auto             it = usdTypeHashToEnum.find(typeName.GetHash());
    if (it == usdTypeHashToEnum.end()) {
        return UsdDataType::kUnknown;
    }
    return it->second;
}


bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}


//----------------------------------------------------------------------------------------------------------------------
MStatus UsdMayaTranslatorUtil::addDynamicAttribute(MObject node, const UsdAttribute& usdAttr)
{
    const SdfValueTypeName typeName = usdAttr.GetTypeName();
    const bool             isArray = typeName.IsArray();
    const UsdDataType      dataType = getAttributeType(usdAttr);
    MObject                attribute = MObject::kNullObj;
    const char*            attrName = usdAttr.GetName().GetString().c_str();


    if (isArray)
        cout << dataType << endl;

    // Fixing attr names to match the original maya names before writing them to USD Attrs
    // using -userattr flag that was added to usdExport in our implementation
    bool isShapeAttr = true;
    std::string tmpAttrName = attrName;
    if (tmpAttrName.rfind("xform:userProperties:", 0) == 0) {
        // This is transform userProperties
        isShapeAttr = false;
        replace(tmpAttrName, "xform:userProperties:", "");
    } else if (tmpAttrName.rfind("userProperties:", 0) == 0) {
        isShapeAttr = true;
        replace(tmpAttrName, "userProperties:", "");
    } else if (tmpAttrName.rfind("primvars:", 0) == 0) {
        return MS::kSuccess; // We will not create the primvars here (it's done somewhere else)
    }
    attrName = tmpAttrName.c_str();

    MDagPath dagPath = MDagPath::getAPathTo(node);
    // don't add/set the attribute when it's a shape attr and the node is transform and vise versa
    // in such case, assume kSuccess
    if ((isShapeAttr and node.apiType() ==  MFn::kTransform) || (!isShapeAttr and node.apiType() !=  MFn::kTransform))
        return  MS::kSuccess;

    // Some plugins like renderman creates custom attributes on time of object creation (before us here)
    // when these attributes are modified and exported out into the USD, we need to set them back when loading
    // the USD, So, we have to check if the custom attr exists, then we have to set the value rather than adding a new attr.
    MFnDependencyNode depNode(node);
    if (!depNode.hasAttribute(attrName)) {

        const uint32_t flags = (isArray ? NodeHelper::kArray : 0)
                               | NodeHelper::kReadable | NodeHelper::kWritable
                               | NodeHelper::kStorable | NodeHelper::kConnectable;
        switch (dataType) {
        case UsdDataType::kAsset: {
            return MS::kSuccess;
        } break;

        case UsdDataType::kBool: {
            NodeHelper::addBoolAttr(node, attrName, attrName, false, flags, &attribute);
        } break;

        case UsdDataType::kUChar: {
            NodeHelper::addInt8Attr(
                node, attrName, attrName, 0, flags, &attribute);
        } break;

        case UsdDataType::kInt:
        case UsdDataType::kUInt: {
            NodeHelper::addInt32Attr(
                node, attrName, attrName, 0, flags, &attribute);
        } break;

        case UsdDataType::kInt64:
        case UsdDataType::kUInt64: {
            NodeHelper::addInt64Attr(
                node, attrName, attrName, 0, flags, &attribute);
        } break;

        case UsdDataType::kHalf:
        case UsdDataType::kFloat: {
            NodeHelper::addFloatAttr(
                node, attrName, attrName, 0, flags, &attribute);
        } break;

        case UsdDataType::kDouble: {
            NodeHelper::addDoubleAttr(
                node, attrName, attrName, 0, flags, &attribute);
        } break;

        case UsdDataType::kString: {
            NodeHelper::addStringAttr(
                node, attrName, attrName, flags, true, &attribute);
        } break;

        case UsdDataType::kMatrix2d: {
            const float defValue[2][2] = { { 0, 0 }, { 0, 0 } };
            NodeHelper::addMatrix2x2Attr(
                node, attrName, attrName, defValue, flags, &attribute);
        } break;

        case UsdDataType::kMatrix3d: {
            const float defValue[3][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
            NodeHelper::addMatrix3x3Attr(
                node, attrName, attrName, defValue, flags, &attribute);
        } break;

        case UsdDataType::kMatrix4d: {
            NodeHelper::addMatrixAttr(
                node, attrName, attrName, MMatrix(), flags, &attribute);
        } break;

        case UsdDataType::kQuatd: {
            NodeHelper::addVec4dAttr(node, attrName, attrName, flags, &attribute);
        } break;

        case UsdDataType::kQuatf:
        case UsdDataType::kQuath: {
            NodeHelper::addVec4fAttr(node, attrName, attrName, flags, &attribute);
        } break;

        case UsdDataType::kVec2d: {
            NodeHelper::addVec2dAttr(node, attrName, attrName, flags, &attribute);
        } break;

        case UsdDataType::kVec2f:
        case UsdDataType::kVec2h: {
            NodeHelper::addVec2fAttr(node, attrName, attrName, flags, &attribute);
        } break;

        case UsdDataType::kVec2i: {
            NodeHelper::addVec2iAttr(node, attrName, attrName, flags, &attribute);
        } break;

        case UsdDataType::kVec3d: {
            NodeHelper::addVec3dAttr(node, attrName, attrName, flags, &attribute);
        } break;

        case UsdDataType::kVec3f:
        case UsdDataType::kVec3h: {
            NodeHelper::addVec3fAttr(node, attrName, attrName, flags, &attribute);
        } break;

        case UsdDataType::kVec3i: {
            NodeHelper::addVec3iAttr(node, attrName, attrName, flags, &attribute);
        } break;

        case UsdDataType::kVec4d: {
            NodeHelper::addVec4dAttr(node, attrName, attrName, flags, &attribute);
        } break;

        case UsdDataType::kVec4f:
        case UsdDataType::kVec4h: {
            NodeHelper::addVec4fAttr(node, attrName, attrName, flags, &attribute);
        } break;

        case UsdDataType::kVec4i: {
            NodeHelper::addVec4iAttr(node, attrName, attrName, flags, &attribute);
        } break;

        default:
            MGlobal::displayError(
                "DgNodeTranslator::addDynamicAttribute - unsupported USD data type");
            return MS::kFailure;
        }
    } else {
        // Get the attribute
        attribute = depNode.attribute(attrName);
    }

    if (isArray) {
        return DgNodeHelper::setArrayMayaValue(node, attribute, usdAttr, dataType);
    }
    return DgNodeHelper::setSingleMayaValue(node, attribute, usdAttr, dataType);

}


//----------------------------------------------------------------------------------------------------------------------
MStatus
UsdMayaTranslatorUtil::copyAttributes(const UsdPrim& from, MObject to)
{
    const std::vector<UsdAttribute> attributes = from.GetAttributes();
    for (size_t i = 0; i < attributes.size(); ++i) {
        if (attributes[i].IsAuthored() && attributes[i].HasValue()
            && attributes[i].IsCustom()) {
            if (!attributeHandled(attributes[i]))
                addDynamicAttribute(to, attributes[i]);
        }
    }
    return MS::kSuccess;
}


//----------------------------------------------------------------------------------------------------------------------
bool UsdMayaTranslatorUtil::attributeHandled(const UsdAttribute& usdAttr) { return false; }



const MString _DEFAULT_TRANSFORM_TYPE("transform");

/* static */
bool UsdMayaTranslatorUtil::CreateTransformNode(
    const UsdPrim&               usdPrim,
    MObject&                     parentNode,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext*    context,
    MStatus*                     status,
    MObject*                     mayaNodeObj)
{
    if (!usdPrim || !usdPrim.IsA<UsdGeomXformable>()) {
        return false;
    }

    if (!CreateNode(usdPrim, _DEFAULT_TRANSFORM_TYPE, parentNode, context, status, mayaNodeObj)) {
        return false;
    }

    // Read xformable attributes from the UsdPrim on to the transform node.
    UsdGeomXformable xformable(usdPrim);
    UsdMayaTranslatorXformable::Read(xformable, *mayaNodeObj, args, context);

    return true;
}

/* static */
bool UsdMayaTranslatorUtil::CreateDummyTransformNode(
    const UsdPrim&               usdPrim,
    MObject&                     parentNode,
    bool                         importTypeName,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext*    context,
    MStatus*                     status,
    MObject*                     mayaNodeObj)
{
    if (!usdPrim) {
        return false;
    }

    if (!CreateNode(usdPrim, _DEFAULT_TRANSFORM_TYPE, parentNode, context, status, mayaNodeObj)) {
        return false;
    }

    MFnDagNode dagNode(*mayaNodeObj);

    // Set the typeName on the adaptor.
    if (UsdMayaAdaptor adaptor = UsdMayaAdaptor(*mayaNodeObj)) {
        VtValue typeName;
        if (!usdPrim.HasAuthoredTypeName()) {
            // A regular typeless def.
            typeName = TfToken();
        } else if (importTypeName) {
            // Preserve type info for round-tripping.
            typeName = usdPrim.GetTypeName();
        } else {
            // Unknown type name; treat this as though it were a typeless def.
            typeName = TfToken();

            // If there is a typename that we're ignoring, leave a note so that
            // we know where it came from.
            const std::string notes = TfStringPrintf(
                "Imported from @%s@<%s> with type '%s'",
                usdPrim.GetStage()->GetRootLayer()->GetIdentifier().c_str(),
                usdPrim.GetPath().GetText(),
                usdPrim.GetTypeName().GetText());
            UsdMayaUtil::SetNotes(dagNode, notes);
        }
        adaptor.SetMetadata(SdfFieldKeys->TypeName, typeName);
    }

    // Lock all the transform attributes.
    for (const UsdMayaXformOpClassification& opClass : UsdMayaXformStack::MayaStack().GetOps()) {
        if (!opClass.IsInvertedTwin()) {
            MPlug plug = dagNode.findPlug(opClass.GetName().GetText(), true);
            if (!plug.isNull()) {
                if (plug.isCompound()) {
                    for (unsigned int i = 0; i < plug.numChildren(); ++i) {
                        MPlug child = plug.child(i);
                        child.setKeyable(false);
                        child.setLocked(true);
                        child.setChannelBox(false);
                    }
                } else {
                    plug.setKeyable(false);
                    plug.setLocked(true);
                    plug.setChannelBox(false);
                }
            }
        }
    }

    return true;
}

/* static */
bool UsdMayaTranslatorUtil::CreateNode(
    const UsdPrim&            usdPrim,
    const MString&            nodeTypeName,
    MObject&                  parentNode,
    UsdMayaPrimReaderContext* context,
    MStatus*                  status,
    MObject*                  mayaNodeObj)
{
    bool stat = CreateNode(usdPrim.GetPath(), nodeTypeName, parentNode, context, status, mayaNodeObj);

    // Copy userProperties to the created transform node
    copyAttributes(usdPrim, *mayaNodeObj);

    return stat;
}

/* static */
bool UsdMayaTranslatorUtil::CreateNode(
    const SdfPath&            path,
    const MString&            nodeTypeName,
    MObject&                  parentNode,
    UsdMayaPrimReaderContext* context,
    MStatus*                  status,
    MObject*                  mayaNodeObj)
{
    if (!CreateNode(
            MString(path.GetName().c_str(), path.GetName().size()),
            nodeTypeName,
            parentNode,
            status,
            mayaNodeObj)) {
        return false;
    }

    if (context) {
        context->RegisterNewMayaNode(path.GetString(), *mayaNodeObj);
    }

    return true;
}

/* static */
bool UsdMayaTranslatorUtil::CreateNode(
    const MString& nodeName,
    const MString& nodeTypeName,
    MObject&       parentNode,
    MStatus*       status,
    MObject*       mayaNodeObj)
{
    // XXX:
    // Using MFnDagNode::create() results in nodes that are not properly
    // registered with parent scene assemblies. For now, just massaging the
    // transform code accordingly so that child scene assemblies properly post
    // their edits to their parents-- if this is indeed the best pattern for
    // this, all Maya*Reader node creation needs to be adjusted accordingly (for
    // much less trivial cases like MFnMesh).
    MDagModifier dagMod;
    *mayaNodeObj = dagMod.createNode(nodeTypeName, parentNode, status);
    CHECK_MSTATUS_AND_RETURN(*status, false);
    *status = dagMod.renameNode(*mayaNodeObj, nodeName);
    CHECK_MSTATUS_AND_RETURN(*status, false);
    *status = dagMod.doIt();
    CHECK_MSTATUS_AND_RETURN(*status, false);

    return TF_VERIFY(!mayaNodeObj->isNull());
}

/* static */
bool UsdMayaTranslatorUtil::CreateShaderNode(
    const MString&               nodeName,
    const MString&               nodeTypeName,
    const UsdMayaShadingNodeType shadingNodeType,
    MStatus*                     status,
    MObject*                     shaderObj,
    const MObject                parentNode)
{
    MString typeFlag;
    switch (shadingNodeType) {
    case UsdMayaShadingNodeType::Light:
        typeFlag = "-al"; // -asLight
        break;
    case UsdMayaShadingNodeType::PostProcess:
        typeFlag = "-app"; // -asPostProcess
        break;
    case UsdMayaShadingNodeType::Rendering:
        typeFlag = "-ar"; // -asRendering
        break;
    case UsdMayaShadingNodeType::Shader:
        typeFlag = "-as"; // -asShader
        break;
    case UsdMayaShadingNodeType::Texture:
        typeFlag = "-icm -at"; // -isColorManaged -asTexture
        break;
    case UsdMayaShadingNodeType::Utility:
        typeFlag = "-au"; // -asUtility
        break;
    default: {
        MFnDependencyNode depNodeFn;
        depNodeFn.create(nodeTypeName, nodeName, status);
        CHECK_MSTATUS_AND_RETURN(*status, false);
        *shaderObj = depNodeFn.object(status);
        CHECK_MSTATUS_AND_RETURN(*status, false);
        return true;
    }
    }

    MString parentFlag;
    if (!parentNode.isNull()) {
        const MFnDagNode parentDag(parentNode, status);
        CHECK_MSTATUS_AND_RETURN(*status, false);
        *status = parentFlag.format(" -p \"^1s\"", parentDag.fullPathName());
        CHECK_MSTATUS_AND_RETURN(*status, false);
    }

    MString cmd;
    // ss = skipSelect
    *status = cmd.format(
        "shadingNode ^1s^2s -ss -n \"^3s\" \"^4s\"", typeFlag, parentFlag, nodeName, nodeTypeName);
    CHECK_MSTATUS_AND_RETURN(*status, false);

    const MString createdNode = MGlobal::executeCommandStringResult(cmd, false, false, status);
    CHECK_MSTATUS_AND_RETURN(*status, false);

    *status = UsdMayaUtil::GetMObjectByName(createdNode.asChar(), *shaderObj);
    CHECK_MSTATUS_AND_RETURN(*status, false);

    // Lights are unique in that they're the only DAG nodes we might create in
    // this function, so they also involve a transform node. The shadingNode
    // command unfortunately seems to return the transform node for the light
    // and not the light node itself, so we may need to manually find the light
    // so we can return that instead.
    if (shaderObj->hasFn(MFn::kDagNode)) {
        const MFnDagNode dagNodeFn(*shaderObj, status);
        CHECK_MSTATUS_AND_RETURN(*status, false);
        MDagPath dagPath;
        *status = dagNodeFn.getPath(dagPath);
        CHECK_MSTATUS_AND_RETURN(*status, false);
        unsigned int numShapes = 0u;
        *status = dagPath.numberOfShapesDirectlyBelow(numShapes);
        CHECK_MSTATUS_AND_RETURN(*status, false);
        if (numShapes == 1u) {
            *status = dagPath.extendToShape();
            CHECK_MSTATUS_AND_RETURN(*status, false);
            *shaderObj = dagPath.node(status);
            CHECK_MSTATUS_AND_RETURN(*status, false);
        }
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
