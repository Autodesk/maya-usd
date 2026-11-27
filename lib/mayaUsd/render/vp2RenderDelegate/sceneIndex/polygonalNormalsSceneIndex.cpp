//
// Copyright 2026 Autodesk
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
#include "polygonalNormalsSceneIndex.h"

#include <pxr/imaging/hd/dataSource.h>
#include <pxr/imaging/hd/dataSourceLocator.h>
#include <pxr/imaging/hd/meshSchema.h>
#include <pxr/imaging/hd/primvarsSchema.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <algorithm>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

/*! The primvars of a mesh, without its normals where it is subdivided. The scheme is read when
    the normals are asked for, so that no other primvar pays for it.
*/
class _PrimvarsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarsDataSource);

    TfTokenVector GetNames() override
    {
        TfTokenVector names = _primvars->GetNames();
        const auto normals = std::find(names.begin(), names.end(), HdPrimvarsSchemaTokens->normals);
        if (normals != names.end() && _IsSubdivided()) {
            names.erase(normals);
        }
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        if (name == HdPrimvarsSchemaTokens->normals && _IsSubdivided()) {
            return nullptr;
        }
        return _primvars->Get(name);
    }

private:
    _PrimvarsDataSource(
        const HdContainerDataSourceHandle& prim,
        const HdContainerDataSourceHandle& primvars)
        : _prim(prim)
        , _primvars(primvars)
    {
    }

    //! Read once, on whichever thread asks first: the names and the normals both ask, and
    //! reading the scheme takes an attribute query.
    bool _IsSubdivided()
    {
        std::call_once(_schemeRead, [this] { _subdivided = _ReadIsSubdivided(); });
        return _subdivided;
    }

    /*! A scheme other than none subdivides. The tokens are the same as PxOsdOpenSubdivTokens's.
        Where no scheme is published the mesh is polygonal, as the emulation delegate takes it.
    */
    bool _ReadIsSubdivided() const
    {
        const HdTokenDataSourceHandle scheme
            = HdMeshSchema::GetFromParent(_prim).GetSubdivisionScheme();
        return scheme && scheme->GetTypedValue(0.0f) != UsdGeomTokens->none;
    }

    const HdContainerDataSourceHandle _prim;
    const HdContainerDataSourceHandle _primvars;

    std::once_flag _schemeRead;
    bool           _subdivided = false;
};

//! A mesh, whose primvars are the filtered ones.
class _MeshDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MeshDataSource);

    TfTokenVector GetNames() override { return _input->GetNames(); }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        HdDataSourceBaseHandle result = _input->Get(name);
        if (name == HdPrimvarsSchema::GetSchemaToken()) {
            if (const HdContainerDataSourceHandle primvars = HdContainerDataSource::Cast(result)) {
                return _PrimvarsDataSource::New(_input, primvars);
            }
        }
        return result;
    }

private:
    _MeshDataSource(const HdContainerDataSourceHandle& input)
        : _input(input)
    {
    }

    const HdContainerDataSourceHandle _input;
};

} // namespace

HdVP2PolygonalNormalsSceneIndexRefPtr
HdVP2PolygonalNormalsSceneIndex::New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(new HdVP2PolygonalNormalsSceneIndex(inputSceneIndex));
}

HdVP2PolygonalNormalsSceneIndex::HdVP2PolygonalNormalsSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

HdSceneIndexPrim HdVP2PolygonalNormalsSceneIndex::GetPrim(const SdfPath& primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // Only a mesh has a scheme to go by.
    if (prim.primType == HdPrimTypeTokens->mesh && prim.dataSource) {
        prim.dataSource = _MeshDataSource::New(prim.dataSource);
    }
    return prim;
}

SdfPathVector HdVP2PolygonalNormalsSceneIndex::GetChildPrimPaths(const SdfPath& primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void HdVP2PolygonalNormalsSceneIndex::_PrimsAdded(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    _SendPrimsAdded(entries);
}

void HdVP2PolygonalNormalsSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    _SendPrimsRemoved(entries);
}

void HdVP2PolygonalNormalsSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    const HdDataSourceLocator& scheme = HdMeshSchema::GetSubdivisionSchemeLocator();

    const auto edited = [&scheme](const HdSceneIndexObserver::DirtiedPrimEntry& entry) {
        return entry.dirtyLocators.Intersects(scheme);
    };
    if (std::none_of(entries.begin(), entries.end(), edited)) {
        _SendPrimsDirtied(entries);
        return;
    }

    // A scheme edit puts normals in or takes them out, and the render index is told of a change
    // of topology only, which does not have the primvars asked for again.
    HdSceneIndexObserver::DirtiedPrimEntries withNormals = entries;
    for (HdSceneIndexObserver::DirtiedPrimEntry& entry : withNormals) {
        if (edited(entry)) {
            entry.dirtyLocators.insert(HdPrimvarsSchema::GetNormalsLocator());
        }
    }
    _SendPrimsDirtied(withNormals);
}

PXR_NAMESPACE_CLOSE_SCOPE
