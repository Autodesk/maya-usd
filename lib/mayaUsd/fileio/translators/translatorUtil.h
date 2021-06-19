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
#ifndef PXRUSDMAYA_TRANSLATOR_UTIL_H
#define PXRUSDMAYA_TRANSLATOR_UTIL_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>

#include <maya/MObject.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE

enum class UsdDataType : uint32_t
{
    kBool,
    kUChar,
    kInt,
    kUInt,
    kInt64,
    kUInt64,
    kHalf,
    kFloat,
    kDouble,
    kString,
    kMatrix2d,
    kMatrix3d,
    kMatrix4d,
    kQuatd,
    kQuatf,
    kQuath,
    kVec2d,
    kVec2f,
    kVec2h,
    kVec2i,
    kVec3d,
    kVec3f,
    kVec3h,
    kVec3i,
    kVec4d,
    kVec4f,
    kVec4h,
    kVec4i,
    kToken,
    kAsset,
    kFrame4d,
    kColor3h,
    kColor3f,
    kColor3d,
    kUnknown
};


static const std::unordered_map<size_t, UsdDataType> usdTypeHashToEnum {
    { SdfValueTypeNames->Bool.GetHash(), UsdDataType::kBool },
    { SdfValueTypeNames->UChar.GetHash(), UsdDataType::kUChar },
    { SdfValueTypeNames->Int.GetHash(), UsdDataType::kInt },
    { SdfValueTypeNames->UInt.GetHash(), UsdDataType::kUInt },
    { SdfValueTypeNames->Int64.GetHash(), UsdDataType::kInt64 },
    { SdfValueTypeNames->UInt64.GetHash(), UsdDataType::kUInt64 },
    { SdfValueTypeNames->Half.GetHash(), UsdDataType::kHalf },
    { SdfValueTypeNames->Float.GetHash(), UsdDataType::kFloat },
    { SdfValueTypeNames->Double.GetHash(), UsdDataType::kDouble },
    { SdfValueTypeNames->String.GetHash(), UsdDataType::kString },
    { SdfValueTypeNames->Token.GetHash(), UsdDataType::kToken },
    { SdfValueTypeNames->Asset.GetHash(), UsdDataType::kAsset },
    { SdfValueTypeNames->Int2.GetHash(), UsdDataType::kVec2i },
    { SdfValueTypeNames->Int3.GetHash(), UsdDataType::kVec3i },
    { SdfValueTypeNames->Int4.GetHash(), UsdDataType::kVec4i },
    { SdfValueTypeNames->Half2.GetHash(), UsdDataType::kVec2h },
    { SdfValueTypeNames->Half3.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Half4.GetHash(), UsdDataType::kVec4h },
    { SdfValueTypeNames->Float2.GetHash(), UsdDataType::kVec2f },
    { SdfValueTypeNames->Float3.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Float4.GetHash(), UsdDataType::kVec4f },
    { SdfValueTypeNames->Double2.GetHash(), UsdDataType::kVec2d },
    { SdfValueTypeNames->Double3.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Double4.GetHash(), UsdDataType::kVec4d },
    { SdfValueTypeNames->Point3h.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Point3f.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Point3d.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Vector3h.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Vector3f.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Vector3d.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Normal3h.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Normal3f.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Normal3d.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Color3h.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Color3f.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Color3d.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Quath.GetHash(), UsdDataType::kQuath },
    { SdfValueTypeNames->Quatf.GetHash(), UsdDataType::kQuatf },
    { SdfValueTypeNames->Quatd.GetHash(), UsdDataType::kQuatd },
    { SdfValueTypeNames->Matrix2d.GetHash(), UsdDataType::kMatrix2d },
    { SdfValueTypeNames->Matrix3d.GetHash(), UsdDataType::kMatrix3d },
    { SdfValueTypeNames->Matrix4d.GetHash(), UsdDataType::kMatrix4d },
    { SdfValueTypeNames->Frame4d.GetHash(), UsdDataType::kFrame4d },
    { SdfValueTypeNames->BoolArray.GetHash(), UsdDataType::kBool },
    { SdfValueTypeNames->UCharArray.GetHash(), UsdDataType::kUChar },
    { SdfValueTypeNames->IntArray.GetHash(), UsdDataType::kInt },
    { SdfValueTypeNames->UIntArray.GetHash(), UsdDataType::kUInt },
    { SdfValueTypeNames->Int64Array.GetHash(), UsdDataType::kInt64 },
    { SdfValueTypeNames->UInt64Array.GetHash(), UsdDataType::kUInt64 },
    { SdfValueTypeNames->HalfArray.GetHash(), UsdDataType::kHalf },
    { SdfValueTypeNames->FloatArray.GetHash(), UsdDataType::kFloat },
    { SdfValueTypeNames->DoubleArray.GetHash(), UsdDataType::kDouble },
    { SdfValueTypeNames->StringArray.GetHash(), UsdDataType::kString },
    { SdfValueTypeNames->TokenArray.GetHash(), UsdDataType::kToken },
    { SdfValueTypeNames->AssetArray.GetHash(), UsdDataType::kAsset },
    { SdfValueTypeNames->Int2Array.GetHash(), UsdDataType::kVec2i },
    { SdfValueTypeNames->Int3Array.GetHash(), UsdDataType::kVec3i },
    { SdfValueTypeNames->Int4Array.GetHash(), UsdDataType::kVec4i },
    { SdfValueTypeNames->Half2Array.GetHash(), UsdDataType::kVec2h },
    { SdfValueTypeNames->Half3Array.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Half4Array.GetHash(), UsdDataType::kVec4h },
    { SdfValueTypeNames->Float2Array.GetHash(), UsdDataType::kVec2f },
    { SdfValueTypeNames->Float3Array.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Float4Array.GetHash(), UsdDataType::kVec4f },
    { SdfValueTypeNames->Double2Array.GetHash(), UsdDataType::kVec2d },
    { SdfValueTypeNames->Double3Array.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Double4Array.GetHash(), UsdDataType::kVec4d },
    { SdfValueTypeNames->Point3hArray.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Point3fArray.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Point3dArray.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Vector3hArray.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Vector3fArray.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Vector3dArray.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Normal3hArray.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Normal3fArray.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Normal3dArray.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Color3hArray.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Color3fArray.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Color3dArray.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->QuathArray.GetHash(), UsdDataType::kQuath },
    { SdfValueTypeNames->QuatfArray.GetHash(), UsdDataType::kQuatf },
    { SdfValueTypeNames->QuatdArray.GetHash(), UsdDataType::kQuatd },
    { SdfValueTypeNames->Matrix2dArray.GetHash(), UsdDataType::kMatrix2d },
    { SdfValueTypeNames->Matrix3dArray.GetHash(), UsdDataType::kMatrix3d },
    { SdfValueTypeNames->Matrix4dArray.GetHash(), UsdDataType::kMatrix4d },
    { SdfValueTypeNames->Frame4dArray.GetHash(), UsdDataType::kFrame4d }
};

enum class UsdMayaShadingNodeType
{
    NonShading,
    Light,
    PostProcess,
    Rendering,
    Shader,
    Texture,
    Utility
};

UsdDataType getAttributeType(const UsdAttribute& usdAttr);

/// \brief Provides helper functions for other readers to use.
struct UsdMayaTranslatorUtil
{
    /// \brief Often when creating a prim, we want to first create a Transform
    /// node. This is a small helper to do this. If the \p args provided
    /// indicate that animation should be read, any transform animation from
    /// the prim is transferred onto the Maya transform node. If \p context is
    /// non-NULL, the new Maya node will be registered to the path of
    /// \p usdPrim.
    MAYAUSD_CORE_PUBLIC
    static bool CreateTransformNode(
        const UsdPrim&               usdPrim,
        MObject&                     parentNode,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext*    context,
        MStatus*                     status,
        MObject*                     mayaNodeObj);

    /// \brief Creates a "dummy" transform node for the given prim, where the
    /// dummy transform has all transform properties locked.
    /// A UsdMayaAdaptor-compatible attribute for the typeName metadata will
    /// be generated. If \p importTypeName is \c true, this attribute will
    /// contain the \c typeName metadata of \p usdPrim, so the \c typeName will
    /// be applied on export. Otherwise, this attribute will be set to the
    /// empty string, so a typeless def will be generated on export.
    MAYAUSD_CORE_PUBLIC
    static bool CreateDummyTransformNode(
        const UsdPrim&               usdPrim,
        MObject&                     parentNode,
        bool                         importTypeName,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext*    context,
        MStatus*                     status,
        MObject*                     mayaNodeObj);

    /// \brief Helper to create a node for \p usdPrim of type \p
    /// nodeTypeName under \p parentNode. If \p context is non-NULL,
    /// the new Maya node will be registered to the path of \p usdPrim.
    MAYAUSD_CORE_PUBLIC
    static bool CreateNode(
        const UsdPrim&            usdPrim,
        const MString&            nodeTypeName,
        MObject&                  parentNode,
        UsdMayaPrimReaderContext* context,
        MStatus*                  status,
        MObject*                  mayaNodeObj);

    /// \brief Helper to create a node for \p usdPath of type \p
    /// nodeTypeName under \p parentNode. If \p context is non-NULL,
    /// the new Maya node will be registered to the path of \p usdPrim.
    MAYAUSD_CORE_PUBLIC
    static bool CreateNode(
        const SdfPath&            usdPath,
        const MString&            nodeTypeName,
        MObject&                  parentNode,
        UsdMayaPrimReaderContext* context,
        MStatus*                  status,
        MObject*                  mayaNodeObj);

    /// \brief Helper to create a node named \p nodeName of type \p
    /// nodeTypeName under \p parentNode. Note that this version does
    /// NOT take a context and cannot register the newly created Maya node
    /// since it does not know the SdfPath to an originating object.
    MAYAUSD_CORE_PUBLIC
    static bool CreateNode(
        const MString& nodeName,
        const MString& nodeTypeName,
        MObject&       parentNode,
        MStatus*       status,
        MObject*       mayaNodeObj);

    /// \brief Helper to create shadingNodes. Wrapper around mel "shadingNode".
    ///
    /// This does several things beyond just creating the node, including but
    /// not limited to:
    ///     - hook up the node to appropriate default groups (ie,
    ///       defaultShadingList1 for shaders, defaultLightSet for lights)
    ///     - handle basic color management setup for textures
    ///     - make sure nodes show up in the hypershade
    ///
    /// TODO: add a ShadingNodeType::Unspecified, which will make this function
    /// determine the type of node automatically using it's classification
    /// string
    MAYAUSD_CORE_PUBLIC
    static bool CreateShaderNode(
        const MString&               nodeName,
        const MString&               nodeTypeName,
        const UsdMayaShadingNodeType shadingNodeType,
        MStatus*                     status,
        MObject*                     shaderObj,
        const MObject                parentNode = MObject::kNullObj);

    /// Gets an API schema of the requested type for the given \p usdPrim.
    ///
    /// This ensures that the USD prim has the API schema applied to it if it
    /// does not already.
    template <typename APISchemaType>
    static APISchemaType GetAPISchemaForAuthoring(const UsdPrim& usdPrim)
    {
        if (!usdPrim.HasAPI<APISchemaType>()) {
            return APISchemaType::Apply(usdPrim);
        }

        return APISchemaType(usdPrim);
    }

    MAYAUSD_CORE_PUBLIC
    static MStatus addDynamicAttribute(MObject node, const UsdAttribute& usdAttr);

    /// \brief  helper method to copy attributes from the UsdPrim to the Maya node
    /// \param  from the UsdPrim to copy the data from
    /// \param  to the maya node to copy the data to
    /// \param  params the importer params to determine what to import
    /// \return MS::kSuccess if ok
    MAYAUSD_CORE_PUBLIC
    static MStatus copyAttributes(const UsdPrim& from, MObject to);

    /// \brief  A temporary solution. Given a custom attribute, if a translator handles it somehow
    /// (i.e. lazy approach to
    ///         not creating a schema), then overload this method and return true on the attribute
    ///         you are handling. This will prevent the attribute from being imported/exported as a
    ///         dynamic attribute.
    /// \param  usdAttr the attribute to test
    /// \return true if your translator is handling this attr
    MAYAUSD_CORE_PUBLIC
    static bool attributeHandled(const UsdAttribute& usdAttr);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
