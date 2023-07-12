//
// Copyright 2019 Luma Pictures
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
// Copyright 2023 Autodesk, Inc. All rights reserved.
//
#include "delegateCtx.h"

#include <mayaHydraLib/utils.h>

#include <pxr/base/gf/frustum.h>
#include <pxr/base/gf/plane.h>
#include <pxr/base/gf/range1d.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hio/glslfx.h>

#include <maya/MFnLight.h>
#include <maya/MShaderManager.h>
#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>

#include <array>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

static const SdfPath lightedObjectsPath = SdfPath(std::string("Lighted"));

template<class T> SdfPath toSdfPath(const T& src);
template<> inline SdfPath toSdfPath<MDagPath>(const MDagPath& dag) {
    return MayaHydra::DagPathToSdfPath(dag, false, false);
}
template<> inline SdfPath toSdfPath<MRenderItem>(const MRenderItem& ri) {
    return MayaHydra::RenderItemToSdfPath(ri, false);
}

template<class T> SdfPath maybePrepend(const T& src, const SdfPath& inPath);
template<> inline SdfPath maybePrepend<MDagPath>(
    const MDagPath& , const SdfPath& inPath
) {
    return inPath;
}
template<> inline SdfPath maybePrepend<MRenderItem>(
    const MRenderItem& ri, const SdfPath& inPath
) {
    // Prepend Maya node name, for organisation and readability.
    std::string dependNodeNameString (MFnDependencyNode(ri.sourceDagPath().node()).name().asChar());
    MayaHydra::SanitizeNameForSdfPath(dependNodeNameString);
    return SdfPath(dependNodeNameString).AppendPath(inPath);
}

///Returns false if this object should not be lighted, true if it should be lighted
template<class T> bool shouldBeLighted(const T& src);
//Template specialization for MDagPath
template<> inline bool shouldBeLighted<MDagPath>(const MDagPath& dag) {
    return (MFnDependencyNode(dag.node()).typeName().asChar() == TfToken("mesh"));
}
//Template specialization for MRenderItem
template<> inline bool shouldBeLighted<MRenderItem>(const MRenderItem& ri) {
    
    //Special case to recognize the Arnold skydome light
    if (MayaHydraDelegateCtx::isRenderItem_aiSkyDomeLightTriangleShape(ri)){
        return false;//Don't light the sky dome light shape
    }
        
    return (MHWRender::MGeometry::Primitive::kLines != ri.primitive()
            && MHWRender::MGeometry::Primitive::kLineStrip != ri.primitive()
            && MHWRender::MGeometry::Primitive::kPoints != ri.primitive());
}

template<class T>
SdfPath GetMayaPrimPath(const T& src)
{
    SdfPath mayaPath = toSdfPath(src);
    if (mayaPath.IsEmpty() || mayaPath.IsAbsoluteRootPath())
        return {};

    // We cannot append an absolute path (I.e : starting with "/")
    if (mayaPath.IsAbsolutePath()) {
        mayaPath = mayaPath.MakeRelativePath(SdfPath::AbsoluteRootPath());
    }

    mayaPath = maybePrepend(src, mayaPath);

    if (shouldBeLighted(src)) {
        // Use a specific prefix when it's not an object that needs to interact with lights and shadows to be able
        // We filter the objects that don't have this prefix in lights HdLightTokens->shadowCollection parameter
        mayaPath = lightedObjectsPath.AppendPath(mayaPath);
    }

    return mayaPath;
}

SdfPath _GetRenderItemMayaPrimPath(const MRenderItem& ri)
{
    if (ri.InternalObjectId() == 0)
        return {};

    return GetMayaPrimPath(ri);
}

SdfPath _GetPrimPath(const SdfPath& base, const MDagPath& dg)
{
    return base.AppendPath(GetMayaPrimPath(dg));
}

SdfPath _GetRenderItemPrimPath(const SdfPath& base, const MRenderItem& ri)
{
    return base.AppendPath(_GetRenderItemMayaPrimPath(ri));
}

SdfPath _GetRenderItemShaderPrimPath(const SdfPath& base, const MRenderItem& ri)
{
    return _GetRenderItemPrimPath(base, ri);
}

SdfPath _GetMaterialPath(const SdfPath& base, const MObject& obj)
{
    MStatus           status;
    MFnDependencyNode node(obj, &status);
    if (!status) {
        return {};
    }
    const auto* chr = node.name().asChar();
    if (chr == nullptr || chr[0] == '\0') {
        return {};
    }

    std::string nodeName(chr);
    MAYAHYDRA_NS_DEF::SanitizeNameForSdfPath(nodeName);
    return base.AppendPath(SdfPath(nodeName));
}

} // namespace

// MayaHydraDelegateCtx is a set of common functions, and it is the aggregation of our
// MayaHydraDelegate base class and the hydra custom scene delegate class : HdSceneDelegate.
MayaHydraDelegateCtx::MayaHydraDelegateCtx(const InitData& initData)
    : HdSceneDelegate(initData.renderIndex, initData.delegateID)
    , MayaHydraDelegate(initData)
    , _rprimPath(initData.delegateID.AppendPath(SdfPath(std::string("rprims"))))
    , _sprimPath(initData.delegateID.AppendPath(SdfPath(std::string("sprims"))))
    , _materialPath(initData.delegateID.AppendPath(SdfPath(std::string("materials"))))
{
    GetChangeTracker().AddCollection(TfToken("visible"));
}

void MayaHydraDelegateCtx::InsertRprim(
    const TfToken& typeId,
    const SdfPath& id,
    const SdfPath& instancerId)
{
    if (!instancerId.IsEmpty()) {
        GetRenderIndex().InsertInstancer(this, instancerId);
    }
    GetRenderIndex().InsertRprim(typeId, this, id);
}

void MayaHydraDelegateCtx::InsertSprim(
    const TfToken& typeId,
    const SdfPath& id,
    HdDirtyBits    initialBits)
{
    GetRenderIndex().InsertSprim(typeId, this, id);
    GetChangeTracker().SprimInserted(id, initialBits);
}

void MayaHydraDelegateCtx::RemoveRprim(const SdfPath& id) { GetRenderIndex().RemoveRprim(id); }

void MayaHydraDelegateCtx::RemoveSprim(const TfToken& typeId, const SdfPath& id)
{
    GetRenderIndex().RemoveSprim(typeId, id);
}

void MayaHydraDelegateCtx::RemoveInstancer(const SdfPath& id)
{
    GetRenderIndex().RemoveInstancer(id);
}

SdfPath MayaHydraDelegateCtx::GetRprimPath() const { return _rprimPath; }

SdfPath MayaHydraDelegateCtx::GetPrimPath(const MDagPath& dg, bool isSprim)
{
    if (isSprim) {
        return _GetPrimPath(_sprimPath, dg);
    } else {
        return _GetPrimPath(_rprimPath, dg);
    }
}

SdfPath MayaHydraDelegateCtx::GetRenderItemPrimPath(const MRenderItem& ri)
{
    return _GetRenderItemPrimPath(_rprimPath, ri);
}

SdfPath MayaHydraDelegateCtx::GetRenderItemShaderPrimPath(const MRenderItem& ri)
{
    return _GetRenderItemShaderPrimPath(_rprimPath, ri);
}

SdfPath MayaHydraDelegateCtx::GetMaterialPath(const MObject& obj)
{
    return _GetMaterialPath(_materialPath, obj);
}

SdfPath MayaHydraDelegateCtx::GetLightedPrimsRootPath() const
{
    return _rprimPath.AppendPath(lightedObjectsPath);
}

bool MayaHydraDelegateCtx::isRenderItem_aiSkyDomeLightTriangleShape(const MRenderItem& renderItem)
{ 
    static const std::string _aiSkyDomeLight ("aiSkyDomeLight");

    const auto prim = renderItem.primitive();
    MDagPath dag    = renderItem.sourceDagPath();
    if( dag.isValid() && (MHWRender::MGeometry::Primitive::kTriangles == prim) && (MHWRender::MRenderItem::DecorationItem == renderItem.type()) ){
        std::string fpName = dag.fullPathName().asChar();
        if (fpName.find(_aiSkyDomeLight) != std::string::npos) {
            //This render item is a aiSkyDomeLight
            return true;
        }
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
