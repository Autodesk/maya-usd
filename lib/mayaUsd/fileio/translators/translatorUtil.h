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

enum class UsdMayaDummyTransformType
{
    UnlockedTransform,
    LockedTransform
};

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
        const UsdPrim&                  usdPrim,
        MObject&                        parentNode,
        bool                            importTypeName,
        const UsdMayaPrimReaderArgs&    args,
        UsdMayaPrimReaderContext*       context,
        MStatus*                        status,
        MObject*                        mayaNodeObj,
        const UsdMayaDummyTransformType dummyTransformType
        = UsdMayaDummyTransformType::LockedTransform);

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

    /// Find out the shading node type associated with a Maya node type name.
    MAYAUSD_CORE_PUBLIC static UsdMayaShadingNodeType
    ComputeShadingNodeTypeForMayaTypeName(const TfToken& mayaNodeTypeName);
    
    /// Write USD type information into the argument Maya object.  Returns true
    /// for success.
    MAYAUSD_CORE_PUBLIC
    static bool SetUsdTypeName(const MObject& mayaNodeObj, const TfToken& usdTypeName);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
