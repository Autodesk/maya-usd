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
#ifndef HD_VP2_SUBTREE_SCOPING_SCENE_INDEX
#define HD_VP2_SUBTREE_SCOPING_SCENE_INDEX

#include <pxr/imaging/hd/filteringSceneIndex.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <map>
#include <set>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdVP2SubtreeScopingSceneIndex);

/*! \brief  Restricts what is drawn to one stage subtree, as
            UsdImagingDelegate::Populate(rootPrim) did, while keeping every prim
            at its stage path.

    Scopes a proxy shape to its primPath. Sits on stage paths, upstream of
    instancing resolution, because native and point instancing move prims out
    of any stage subtree.

    - Prims under the root pass through.
    - The root prim and its ancestors get an identity xform. UsdImagingDelegate
      defined the root prim as identity, and
      ProxyRenderDelegate::_UpdateSceneDelegate supplies its world transform
      through the root transform.
    - Ancestors lose their prim type and the ability to apply a draw mode, but
      keep everything else, so that visibility, purpose, material bindings and
      primvars still inherit, as they did with UsdImagingDelegate.
    - Materials pass through, for the unbound material pruning further down to
      remove unless something in scope binds them, and so do native-instancing
      prototypes (/__Prototype_*), which instances in scope need.
    - So do the subtrees that point instancers in scope name as prototypes,
      wherever they are, as UsdImagingDelegate followed the prototypes
      relationship out of the root. Prototypes of an instancer that is itself
      only in a prototype subtree are not followed. Like any prototype in
      scope, one outside the root also draws on its own wherever USD would draw it.
    - Everything else loses its type and keeps only what prims in scope read
      from it by path: collections, skeletal animation and blend shapes.

    Topology is left untouched, so this is a soft prune in the sense of
    HdsiPrimTypeAndPathPruningSceneIndex. UsdImagingRerootingSceneIndex(root,
    root) does not fit: it isolates the subtree, so ancestors stop contributing
    inherited data, and materials and prototypes outside it vanish.

    \class  HdVP2SubtreeScopingSceneIndex
*/
class HdVP2SubtreeScopingSceneIndex final : public HdSingleInputFilteringSceneIndexBase
{
public:
    static HdVP2SubtreeScopingSceneIndexRefPtr New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    //! \brief  Sets the subtree to draw; "/", the default, disables scoping.
    //!         Sends no notices, so call it before the input is populated.
    void SetRootPath(const SdfPath& rootPath) { _rootPath = rootPath; }

    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    SdfPathVector    GetChildPrimPaths(const SdfPath& primPath) const override;

protected:
    HdVP2SubtreeScopingSceneIndex(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    void _PrimsAdded(
        const HdSceneIndexBase&                       sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase&                         sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase&                         sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override;

private:
    enum class _Scope
    {
        PassThrough,
        RootPrim,
        Ancestor,
        Outside
    };

    _Scope _Classify(const SdfPath& primPath, const TfToken& primType) const;

    //! The type a prim is reported with: its own in and under the scope, none outside it.
    TfToken _ReportedType(const SdfPath& primPath, const TfToken& primType) const;

    bool _IsInScopeByPath(const SdfPath& primPath) const;
    bool _IsInPrototype(const SdfPath& primPath) const;

    //! Reads the prototypes of the in-scope instancer at the path, forgetting it if it has none.
    void _UpdateInstancer(const SdfPath& instancerPath);

    //! Rebuilds _prototypeRoots and returns the roots that were added or removed.
    SdfPathVector _RefreshPrototypeRoots();

    //! Sends PrimsAdded for the subtrees under the roots, with the type each prim has now.
    void _ResyncPrototypes(const SdfPathVector& roots);

    SdfPath _rootPath { SdfPath::AbsoluteRootPath() };

    //! The prototypes each instancer in scope draws, as it names them.
    std::map<SdfPath, SdfPathVector> _instancerPrototypes;

    //! The union of _instancerPrototypes, whose subtrees keep their prim type.
    std::set<SdfPath> _prototypeRoots;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
