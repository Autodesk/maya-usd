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

#include "usdUtils.h"

#include <usdUfe/utils/Utils.h>

#include <pxr/pxr.h>
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/changeBlock.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#if PXR_VERSION >= 2311
#include <pxr/usd/sdf/pathExpression.h>
#endif
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/sdf/propertySpec.h>
#include <pxr/usd/sdf/relationshipSpec.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/inherits.h>
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usd/specializes.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/nodeGraph.h>

#include <set>
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
std::map<std::string, std::string> getDict(const UsdPrimCompositionQueryArc& arc)
{
    std::string arcType;
    switch (arc.GetArcType()) {
    case PcpArcTypeRoot: arcType = "PcpArcTypeRoot"; break;
    case PcpArcTypeReference: arcType = "PcpArcTypeReference"; break;
    case PcpArcTypePayload: arcType = "PcpArcTypePayload"; break;
    case PcpArcTypeInherit: arcType = "PcpArcTypeInherit"; break;
    case PcpArcTypeSpecialize: arcType = "PcpArcTypeSpecialize"; break;
    case PcpArcTypeVariant: arcType = "PcpArcTypeVariant"; break;
    default: break;
    }

    auto introducingLayer = arc.GetIntroducingLayer();
    auto introducingNode = arc.GetIntroducingNode();

    return {
        { "arcType", arcType },
        { "hasSpecs", arc.HasSpecs() ? "True" : "False" },
        { "introLayer", introducingLayer ? introducingLayer->GetRealPath() : "" },
        { "introLayerStack",
          introducingNode
              ? introducingNode.GetLayerStack()->GetIdentifier().rootLayer->GetRealPath()
              : "" },
        { "introPath", arc.GetIntroducingPrimPath().GetString() },
        { "isAncestral", arc.IsAncestral() ? "True" : "False" },
        { "isImplicit", arc.IsImplicit() ? "True" : "False" },
        { "isIntroRootLayer", arc.IsIntroducedInRootLayerStack() ? "True" : "False" },
        { "isIntroRootLayerPrim", arc.IsIntroducedInRootLayerPrimSpec() ? "True" : "False" },
        { "nodeLayerStack",
          arc.GetTargetNode().GetLayerStack()->GetIdentifier().rootLayer->GetRealPath() },
        { "nodePath", arc.GetTargetNode().GetPath().GetString() },
    };
}

void replaceInternalReferencePath(
    const UsdPrim&            oldPrim,
    const SdfPath&            newPath,
    const SdfReferencesProxy& referencesList,
    SdfListOpType             op)
{
    // set the listProxy based on the SdfListOpType
    SdfReferencesProxy::ListProxy listProxy = referencesList.GetAppendedItems();
    if (op == SdfListOpTypePrepended) {
        listProxy = referencesList.GetPrependedItems();
    } else if (op == SdfListOpTypeOrdered) {
        listProxy = referencesList.GetOrderedItems();
    } else if (op == SdfListOpTypeAdded) {
        listProxy = referencesList.GetAddedItems();
    } else if (op == SdfListOpTypeDeleted) {
        listProxy = referencesList.GetDeletedItems();
    }

    // fetching the existing SdfReference items and using
    // the Replace() method to replace them with updated SdfReference items.
    for (const SdfReference ref : listProxy) {
        if (UsdUfe::isInternalReference(ref)) {
            SdfPath finalPath;
            if (oldPrim.GetPath() == ref.GetPrimPath()) {
                finalPath = newPath;
            } else if (ref.GetPrimPath().HasPrefix(oldPrim.GetPath())) {
                finalPath = ref.GetPrimPath().ReplacePrefix(oldPrim.GetPath(), newPath);
            }

            if (finalPath.IsEmpty()) {
                continue;
            }

            // replace the old reference with new one
            SdfReference newRef;
            newRef.SetPrimPath(finalPath);
            listProxy.Replace(ref, newRef);
        }
    }
}

void removeInternalReferencePath(
    const UsdPrim&            deletedPrim,
    const SdfReferencesProxy& referencesList,
    SdfListOpType             op)
{
    // set the listProxy based on the SdfListOpType
    SdfReferencesProxy::ListProxy listProxy = referencesList.GetAppendedItems();
    if (op == SdfListOpTypePrepended) {
        listProxy = referencesList.GetPrependedItems();
    } else if (op == SdfListOpTypeOrdered) {
        listProxy = referencesList.GetOrderedItems();
    } else if (op == SdfListOpTypeAdded) {
        listProxy = referencesList.GetAddedItems();
    } else if (op == SdfListOpTypeDeleted) {
        listProxy = referencesList.GetDeletedItems();
    }

    for (size_t idx = 0; idx < listProxy.size();) {
        const SdfReference ref = listProxy[idx];
        if (UsdUfe::isInternalReference(ref)) {
            if (deletedPrim.GetPath() == ref.GetPrimPath()
                || ref.GetPrimPath().HasPrefix(deletedPrim.GetPath())) {
                listProxy.Erase(idx);
                continue; // Not increasing idx
            }
        }
        ++idx;
    }
}

// This template method updates the SdfPath for inherited or specialized arcs
// when the path to the concrete prim they refer to has changed.
// HS January 13, 2021: Find a better generic way to consolidate this method with
// replaceReferenceItems
template <typename T>
void replacePath(const UsdPrim& oldPrim, const SdfPath& newPath, const T& proxy, SdfListOpType op)
{
    // set the listProxy based on the SdfListOpType
    typename T::ListProxy listProxy { proxy.GetAppendedItems() };
    if (op == SdfListOpTypePrepended) {
        listProxy = proxy.GetPrependedItems();
    } else if (op == SdfListOpTypeOrdered) {
        listProxy = proxy.GetOrderedItems();
    } else if (op == SdfListOpTypeAdded) {
        listProxy = proxy.GetAddedItems();
    } else if (op == SdfListOpTypeDeleted) {
        listProxy = proxy.GetDeletedItems();
    }

    for (const SdfPath path : listProxy) {
        SdfPath finalPath;
        if (oldPrim.GetPath() == path.GetPrimPath()) {
            finalPath = newPath;
        } else if (path.GetPrimPath().HasPrefix(oldPrim.GetPath())) {
            finalPath = path.GetPrimPath().ReplacePrefix(oldPrim.GetPath(), newPath);
        }

        if (finalPath.IsEmpty()) {
            continue;
        }

        // replace the old SdfPath with new one
        listProxy.Replace(path, finalPath);
    }
}

// This template method cleans the SdfPath for inherited or specialized arcs
// when the path to the concrete prim they refer to has becomed invalid.
// HS January 13, 2021: Find a better generic way to consolidate this method with
// removeReferenceItems
template <typename T> void removePath(const UsdPrim& deletedPrim, const T& proxy, SdfListOpType op)
{
    // set the listProxy based on the SdfListOpType
    typename T::ListProxy listProxy { proxy.GetAppendedItems() };
    if (op == SdfListOpTypePrepended) {
        listProxy = proxy.GetPrependedItems();
    } else if (op == SdfListOpTypeOrdered) {
        listProxy = proxy.GetOrderedItems();
    } else if (op == SdfListOpTypeAdded) {
        listProxy = proxy.GetAddedItems();
    } else if (op == SdfListOpTypeDeleted) {
        listProxy = proxy.GetDeletedItems();
    }

    for (size_t idx = 0; idx < listProxy.size();) {
        const SdfPath path = listProxy[idx];
        if (deletedPrim.GetPath() == path.GetPrimPath() || path.HasPrefix(deletedPrim.GetPath())) {
            listProxy.Erase(idx);
            continue; // Not increasing idx
        }
        ++idx;
    }
}

void replacePropertyPath(const UsdPrim& oldPrim, const SdfPath& newPath, UsdProperty& prop)
{
    if (prop.Is<UsdAttribute>()) {
        UsdAttribute  attr = prop.As<UsdAttribute>();
        SdfPathVector sources;
        attr.GetConnections(&sources);
        bool hasChanged = false;
        for (size_t i = 0; i < sources.size(); ++i) {
            const SdfPath& path = sources[i];
            SdfPath        finalPath = path.ReplacePrefix(oldPrim.GetPath(), newPath);
            if (path != finalPath) {
                sources[i] = finalPath;
                hasChanged = true;
            }
        }
        if (hasChanged) {
            attr.SetConnections(sources);
        }
    } else if (prop.Is<UsdRelationship>()) {
        UsdRelationship rel = prop.As<UsdRelationship>();
        SdfPathVector   targets;
        rel.GetTargets(&targets);
        bool hasChanged = false;
        for (size_t i = 0; i < targets.size(); ++i) {
            const SdfPath& path = targets[i];
            SdfPath        finalPath = path.ReplacePrefix(oldPrim.GetPath(), newPath);
            if (path != finalPath) {
                targets[i] = finalPath;
                hasChanged = true;
            }
        }
        if (hasChanged) {
            rel.SetTargets(targets);
        }
    }
}

void removePropertyPath(const UsdPrim& deletedPrim, UsdProperty& prop)
{
    if (prop.Is<UsdAttribute>()) {
        UsdAttribute  attr = prop.As<UsdAttribute>();
        SdfPathVector sources;
        attr.GetConnections(&sources);
        bool hasChanged = false;
        for (size_t i = 0; i < sources.size();) {
            const SdfPath& path = sources[i];
            if (deletedPrim.GetPath() == path.GetPrimPath()
                || path.HasPrefix(deletedPrim.GetPath())) {
                hasChanged = true;
                sources.erase(sources.cbegin() + i);
                continue;
            }
            ++i;
        }
        if (hasChanged) {
            if (sources.empty()) {
                attr.ClearConnections();
                if (!attr.HasValue()) {
                    prop.GetPrim().RemoveProperty(prop.GetName());
                }
            } else {
                attr.SetConnections(sources);
            }
        }
    } else if (prop.Is<UsdRelationship>()) {
        UsdRelationship rel = prop.As<UsdRelationship>();
        SdfPathVector   targets;
        rel.GetTargets(&targets);
        bool hasChanged = false;
        for (size_t i = 0; i < targets.size();) {
            const SdfPath& path = targets[i];
            if (deletedPrim.GetPath() == path.GetPrimPath()
                || path.HasPrefix(deletedPrim.GetPath())) {
                hasChanged = true;
                targets.erase(targets.cbegin() + i);
                continue;
            }
            ++i;
        }
        if (hasChanged) {
            if (targets.empty()) {
                rel.ClearTargets(true);
            } else {
                rel.SetTargets(targets);
            }
        }
    }
}

// Returns the root prim ancestor of an absolute prim or property path, or the empty
// path for the pseudo-root, the empty path and relative paths.
SdfPath getRootPrimPath(const SdfPath& path)
{
    if (!path.IsAbsolutePath() || path.IsAbsoluteRootPath())
        return {};

    return path.GetPrefixes().front();
}

// Authors the overrides of authorCrossRootOverrides(). Usd queries aren't safe while
// a change block is open, so the source stage is read first and the overrides are
// only authored once all of them are known.
class CrossRootOverridesAuthor
{
public:
    CrossRootOverridesAuthor(
        const std::vector<UsdPrim>& referencedRoots,
        const SdfLayerHandle&       destLayer,
        const SdfPath&              destPrimPath)
        : _destLayer(destLayer)
        , _destPrimPath(destPrimPath)
    {
        for (const UsdPrim& root : referencedRoots)
            _referencedRoots.insert(root.GetPath());
    }

    UsdUfe::CrossRootOverridesResult author(const UsdStage& sourceStage)
    {
        if (_destLayer) {
            collectOverrides(sourceStage);
            applyOverrides();
        }
        return std::move(_result);
    }

private:
    // An override to author in the destination layer, for a property of the source
    // stage that has paths to another root prim.
    struct Override
    {
        enum class Kind
        {
            Targets,     // Relationship targets.
            Connections, // Attribute connections.
            Expression   // Path expression value.
        };

        Kind kind = Kind::Targets;

        // Path of the property in the source stage, to report it.
        SdfPath sourcePropPath;

        // Path of the prim spec to author the override on, in the destination layer.
        // It can contain variant selections.
        SdfPath overridePrimPath;

        // Property name, and what's needed to create the property spec.
        TfToken          propName;
        bool             custom = false;
        SdfValueTypeName typeName;
        SdfVariability   variability = SdfVariabilityVarying;

        // The targets or connections, remapped under the destination prim (for the
        // Targets and Connections kinds).
        SdfPathVector remappedPaths;

        // The path expression, remapped under the destination prim (for the
        // Expression kind).
        VtValue remappedExpression;
    };

    // Reads the source stage and lists the overrides to author. No layer is edited.
    void collectOverrides(const UsdStage& sourceStage)
    {
        // Instance proxies are traversed so that cross-root targets inside instances
        // are found. Prototypes are not traversed: they are not part of the namespace
        // of the stage and their content is reached through the instance proxies.
        // Unloaded prims are visited (their children are not populated) to report them.
        const auto predicate
            = UsdTraverseInstanceProxies(UsdPrimIsActive && UsdPrimIsDefined && !UsdPrimIsAbstract);
        for (const SdfPath& rootPath : _referencedRoots) {
            const UsdPrim rootPrim = sourceStage.GetPrimAtPath(rootPath);
            if (!rootPrim)
                continue;

            for (const UsdPrim& prim : UsdPrimRange(rootPrim, predicate)) {
                if (!prim.IsLoaded())
                    _result.unloadedPrims.push_back(prim.GetPath());

                collectRelationships(prim);
                collectAttributes(prim);
            }
        }
    }

    // Authors the collected overrides with the Sdf API only, in a single change block.
    void applyOverrides()
    {
        SdfChangeBlock changeBlock;

        for (const SdfPath& instancePath : _result.deinstancedPrims) {
            if (SdfPrimSpecHandle instanceSpec
                = SdfCreatePrimInLayer(_destLayer, toDestPath(instancePath)))
                instanceSpec->SetInstanceable(false);
        }

        for (const Override& over : _overrides) {
            if (applyOverride(over))
                ++_result.nbOverrides;
            else
                _result.failedOverrides.push_back(over.sourcePropPath);
        }
    }

    bool applyOverride(const Override& over)
    {
        const SdfPrimSpecHandle primSpec = SdfCreatePrimInLayer(_destLayer, over.overridePrimPath);
        if (!primSpec)
            return false;

        const SdfPath specPath = primSpec->GetPath().AppendProperty(over.propName);

        if (over.kind == Override::Kind::Targets) {
            SdfRelationshipSpecHandle relSpec = _destLayer->GetRelationshipAtPath(specPath);
            if (!relSpec)
                relSpec = SdfRelationshipSpec::New(primSpec, over.propName, over.custom);
            if (!relSpec)
                return false;

            // Explicit targets: they replace all the weaker, out-of-scope targets that
            // come through the reference.
            relSpec->GetTargetPathList().ClearEditsAndMakeExplicit();
            relSpec->GetTargetPathList().GetExplicitItems() = over.remappedPaths;
            return true;
        }

        SdfAttributeSpecHandle attrSpec = _destLayer->GetAttributeAtPath(specPath);
        if (!attrSpec) {
            attrSpec = SdfAttributeSpec::New(
                primSpec, over.propName, over.typeName, over.variability, over.custom);
        }
        if (!attrSpec)
            return false;

        if (over.kind == Override::Kind::Connections) {
            attrSpec->GetConnectionPathList().ClearEditsAndMakeExplicit();
            attrSpec->GetConnectionPathList().GetExplicitItems() = over.remappedPaths;
        } else {
            attrSpec->SetDefaultValue(over.remappedExpression);
        }
        return true;
    }

    void collectRelationships(const UsdPrim& prim)
    {
        for (const UsdRelationship& rel : prim.GetAuthoredRelationships()) {
            SdfPathVector targets;
            rel.GetTargets(&targets);
            Override over;
            if (!remapTargets(prim, targets, over.remappedPaths))
                continue;

            over.kind = Override::Kind::Targets;
            over.custom = rel.IsCustom();
            addOverride(prim, rel, std::move(over));
        }
    }

    void collectAttributes(const UsdPrim& prim)
    {
        for (const UsdAttribute& attr : prim.GetAuthoredAttributes()) {
            if (attr.HasAuthoredConnections()) {
                SdfPathVector sources;
                attr.GetConnections(&sources);
                Override over;
                if (remapTargets(prim, sources, over.remappedPaths)) {
                    over.kind = Override::Kind::Connections;
                    addAttributeOverride(prim, attr, std::move(over));
                }
            }

#if PXR_VERSION >= 2311
            if (attr.GetTypeName() == SdfValueTypeNames->PathExpression) {
                SdfPathExpression expression;
                if (!attr.Get(&expression))
                    continue;
                SdfPathExpression remapped;
                if (!remapExpression(prim, expression, remapped))
                    continue;
                Override over;
                over.kind = Override::Kind::Expression;
                over.remappedExpression = VtValue(remapped);
                addAttributeOverride(prim, attr, std::move(over));
            }
#endif
        }
    }

    void addAttributeOverride(const UsdPrim& prim, const UsdAttribute& attr, Override&& over)
    {
        over.custom = attr.IsCustom();
        over.typeName = attr.GetTypeName();
        over.variability = attr.GetVariability();
        addOverride(prim, attr, std::move(over));
    }

    // Completes and records an override of the given property. USD ignores opinions on
    // descendants of instances, so the enclosing instances are recorded to be made
    // non-instanceable. (An instance can carry overrides itself, only its descendants
    // can't.)
    void addOverride(const UsdPrim& prim, const UsdProperty& prop, Override&& over)
    {
        if (prim.IsInstanceProxy()) {
            for (UsdPrim parent = prim.GetParent(); parent && !parent.IsPseudoRoot();
                 parent = parent.GetParent()) {
                if (parent.IsInstance())
                    _result.deinstancedPrims.insert(parent.GetPath());
            }
        }

        over.sourcePropPath = prop.GetPath();
        over.overridePrimPath = getOverridePrimPath(prim, prop);
        over.propName = prop.GetName();
        _overrides.push_back(std::move(over));
    }

    // Returns where the given path of the source stage is in the destination layer.
    SdfPath toDestPath(const SdfPath& path) const
    {
        return path.ReplacePrefix(SdfPath::AbsoluteRootPath(), _destPrimPath);
    }

    // Remaps the targets of a relationship or connection of the given prim. Returns
    // false if no target points outside the root prim of the given prim: then the
    // reference remaps everything already and no override is needed.
    bool remapTargets(const UsdPrim& prim, const SdfPathVector& targets, SdfPathVector& remapped)
    {
        const SdfPath primRoot = getRootPrimPath(prim.GetPath());

        bool hasCrossRoot = false;
        for (const SdfPath& target : targets) {
            if (getRootPrimPath(target) != primRoot) {
                hasCrossRoot = true;
                break;
            }
        }

        if (!hasCrossRoot)
            return false;

        // All the targets must be written, even those within the same root prim,
        // since the explicit list replaces the targets coming from the reference.
        remapped.clear();
        remapped.reserve(targets.size());
        for (const SdfPath& target : targets) {
            if (_referencedRoots.count(getRootPrimPath(target)) == 0) {
                // No reference brings in the target, so there is nothing valid to
                // point to in the destination layer.
                _result.unresolvedTargets.insert(target);
                continue;
            }
            remapped.push_back(toDestPath(target));
        }

        return true;
    }

#if PXR_VERSION >= 2311
    // Remaps a path expression of the given prim. Returns false if all its paths stay
    // inside the root prim of the given prim.
    bool remapExpression(
        const UsdPrim&           prim,
        const SdfPathExpression& expression,
        SdfPathExpression&       remapped)
    {
        const SdfPath           primRoot = getRootPrimPath(prim.GetPath());
        const SdfPathExpression absolute = expression.MakeAbsolute(prim.GetPath());

        // TfFunctionRef only binds to lvalues, so the callbacks need names.
        bool hasCrossRoot = false;
        auto onLogic = [](SdfPathExpression::Op, int) {};
        auto onReference = [](const SdfPathExpression::ExpressionReference&) {};
        auto onPattern = [&](const SdfPathExpression::PathPattern& pattern) {
            const SdfPath patternRoot = getRootPrimPath(pattern.GetPrefix());
            if (patternRoot != primRoot)
                hasCrossRoot = true;
            if (!patternRoot.IsEmpty() && _referencedRoots.count(patternRoot) == 0)
                _result.unresolvedTargets.insert(pattern.GetPrefix());
        };
        absolute.Walk(onLogic, onReference, onPattern);

        if (!hasCrossRoot)
            return false;

        // Patterns under prims that aren't referenced are remapped too: they match
        // nothing in the destination layer, as they would through the reference.
        remapped = absolute.ReplacePrefix(SdfPath::AbsoluteRootPath(), _destPrimPath);
        return true;
    }
#endif

    // Returns the path of the prim spec to author the override of the given property on.
    //
    // When the strongest opinion of the property is inside variants, the override goes
    // inside the same variants of the destination prim: it only applies to the current
    // variant selections, and the other variants keep their own opinions from the
    // source file. Otherwise it goes on the destination prim itself.
    SdfPath getOverridePrimPath(const UsdPrim& prim, const UsdProperty& prop)
    {
        const SdfPath destPath = toDestPath(prim.GetPath());

        // Only the strongest opinion matters, to know which variants it comes from.
        // This is only read for the few properties that need an override.
        const SdfPropertySpecHandleVector stack = prop.GetPropertyStack(UsdTimeCode::Default());
        if (stack.empty())
            return destPath;

        const SdfPath specPrimPath = stack.front()->GetPath().GetParentPath();

        // An opinion outside variants is stronger than any variant opinion of the
        // same layer stack: the plain override is right for every variant selection.
        if (!specPrimPath.ContainsPrimVariantSelection())
            return destPath;

        // The spec path is the stage path with variant selections when the opinion
        // comes from the variants of the referenced layer stack itself, rather than
        // through another composition arc (e.g. the class of an instance).
        if (specPrimPath.StripAllVariantSelections() == prim.GetPath())
            return toDestPath(specPrimPath);

        // Fall back to the destination prim: the override then applies to all variants.
        _result.overridesOutsideVariants.push_back(prop.GetPath());
        return destPath;
    }

    // The layer holding the references, where the overrides are authored.
    const SdfLayerHandle _destLayer;
    // The prim holding the references, in the destination layer.
    const SdfPath                    _destPrimPath;
    std::set<SdfPath>                _referencedRoots;
    std::vector<Override>            _overrides;
    UsdUfe::CrossRootOverridesResult _result;
};

} // namespace

namespace USDUFE_NS_DEF {

SdfPrimSpecHandle getPrimSpecAtEditTarget(const UsdPrim& prim)
{
    auto stage = prim.GetStage();
    return stage->GetEditTarget().GetPrimSpecForScenePath(prim.GetPath());
}

void printCompositionQuery(const UsdPrim& prim, std::ostream& os)
{
    UsdPrimCompositionQuery query(prim);

    os << "[\n";

    // the composition arcs are always returned in order from strongest
    // to weakest regardless of the filter.
    for (const auto& arc : query.GetCompositionArcs()) {
        const auto& arcDic = getDict(arc);
        os << "{\n";
        std::for_each(arcDic.begin(), arcDic.end(), [&](const auto& it) {
            os << it.first << ": " << it.second << '\n';
        });
        os << "}\n";
    }

    os << "]\n\n";
}

bool updateReferencedPath(const UsdPrim& oldPrim, const SdfPath& newPath)
{
    SdfChangeBlock changeBlock;

    for (const auto& p : oldPrim.GetStage()->Traverse()) {

        auto primSpec = getPrimSpecAtEditTarget(p);
        // check different composition arcs
        if (p.HasAuthoredReferences()) {
            if (primSpec) {

                SdfReferencesProxy referencesList = primSpec->GetReferenceList();

                // update append/prepend lists individually
                replaceInternalReferencePath(
                    oldPrim, newPath, referencesList, SdfListOpTypeAppended);
                replaceInternalReferencePath(
                    oldPrim, newPath, referencesList, SdfListOpTypePrepended);
            }
        } else if (p.HasAuthoredInherits()) {
            if (primSpec) {

                SdfInheritsProxy inheritsList = primSpec->GetInheritPathList();

                // update append/prepend lists individually
                replacePath<SdfInheritsProxy>(
                    oldPrim, newPath, inheritsList, SdfListOpTypeAppended);
                replacePath<SdfInheritsProxy>(
                    oldPrim, newPath, inheritsList, SdfListOpTypePrepended);
            }
        } else if (p.HasAuthoredSpecializes()) {
            if (primSpec) {
                SdfSpecializesProxy specializesList = primSpec->GetSpecializesList();

                // update append/prepend lists individually
                replacePath<SdfSpecializesProxy>(
                    oldPrim, newPath, specializesList, SdfListOpTypeAppended);
                replacePath<SdfSpecializesProxy>(
                    oldPrim, newPath, specializesList, SdfListOpTypePrepended);
            }
        }

        // Need to repath connections and relationships:
        for (auto& prop : p.GetProperties()) {
            replacePropertyPath(oldPrim, newPath, prop);
        }
    }

    return true;
}

bool cleanReferencedPath(const UsdPrim& deletedPrim)
{
    SdfChangeBlock changeBlock;

    for (const auto& p : deletedPrim.GetStage()->Traverse()) {

        auto primSpec = getPrimSpecAtEditTarget(p);
        // check different composition arcs
        if (p.HasAuthoredReferences()) {
            if (primSpec) {

                SdfReferencesProxy referencesList = primSpec->GetReferenceList();

                // update append/prepend lists individually
                removeInternalReferencePath(deletedPrim, referencesList, SdfListOpTypeAppended);
                removeInternalReferencePath(deletedPrim, referencesList, SdfListOpTypePrepended);
            }
        } else if (p.HasAuthoredInherits()) {
            if (primSpec) {

                SdfInheritsProxy inheritsList = primSpec->GetInheritPathList();

                // update append/prepend lists individually
                removePath<SdfInheritsProxy>(deletedPrim, inheritsList, SdfListOpTypeAppended);
                removePath<SdfInheritsProxy>(deletedPrim, inheritsList, SdfListOpTypePrepended);
            }
        } else if (p.HasAuthoredSpecializes()) {
            if (primSpec) {
                SdfSpecializesProxy specializesList = primSpec->GetSpecializesList();

                // update append/prepend lists individually
                removePath<SdfSpecializesProxy>(
                    deletedPrim, specializesList, SdfListOpTypeAppended);
                removePath<SdfSpecializesProxy>(
                    deletedPrim, specializesList, SdfListOpTypePrepended);
            }
        }

        // Need to repath connections and relationships:
        for (auto& prop : p.GetProperties()) {
            removePropertyPath(deletedPrim, prop);
        }
    }

    return true;
}

bool isInternalReference(const SdfReference& ref) { return ref.IsInternal(); }

bool isConnected(const PXR_NS::UsdAttribute& srcUsdAttr, const PXR_NS::UsdAttribute& dstUsdAttr)
{
    PXR_NS::SdfPathVector connectedAttrs;
    dstUsdAttr.GetConnections(&connectedAttrs);

    for (PXR_NS::SdfPath path : connectedAttrs) {
        if (path == srcUsdAttr.GetPath()) {
            return true;
        }
    }

    return false;
}

bool canRemoveSrcProperty(const PXR_NS::UsdAttribute& srcAttr)
{
    // Do not remove if it has a value.
    if (srcAttr.HasValue()) {
        return false;
    }

    PXR_NS::SdfPathVector connectedAttrs;
    srcAttr.GetConnections(&connectedAttrs);

    // Do not remove if it has connections.
    if (!connectedAttrs.empty()) {
        return false;
    }

    const auto prim = srcAttr.GetPrim();
    if (!prim) {
        return false;
    }

    PXR_NS::UsdShadeNodeGraph ngPrim(prim);
    if (!ngPrim) {
        const auto primParent = prim.GetParent();

        if (!primParent) {
            return false;
        }

        // Do not remove if there is a connection with a prim.
        for (const auto& childPrim : primParent.GetChildren()) {
            if (childPrim != prim) {
                for (const auto& attribute : childPrim.GetAttributes()) {
                    const PXR_NS::UsdAttribute dstUsdAttr = attribute.As<PXR_NS::UsdAttribute>();
                    if (isConnected(srcAttr, dstUsdAttr)) {
                        return false;
                    }
                }
            }
        }

        // Do not remove if there is a connection with the parent prim.
        for (const auto& attribute : primParent.GetAttributes()) {
            const PXR_NS::UsdAttribute dstUsdAttr = attribute.As<PXR_NS::UsdAttribute>();
            if (isConnected(srcAttr, dstUsdAttr)) {
                return false;
            }
        }

        return true;
    }

    // Do not remove boundary properties even if there are connections.
    return false;
}

bool canRemoveDstProperty(const PXR_NS::UsdAttribute& dstAttr)
{
    // Do not remove if it has a value.
    if (dstAttr.HasValue()) {
        return false;
    }

    PXR_NS::SdfPathVector connectedAttrs;
    dstAttr.GetConnections(&connectedAttrs);

    // Do not remove if it has connections.
    if (!connectedAttrs.empty()) {
        return false;
    }

    const auto prim = dstAttr.GetPrim();
    if (!prim) {
        return false;
    }

    PXR_NS::UsdShadeNodeGraph ngPrim(prim);
    if (!ngPrim) {
        return true;
    }

    UsdShadeMaterial asMaterial(prim);
    if (asMaterial) {
        const TfToken baseName = dstAttr.GetBaseName();
        // Remove Material intrinsic outputs since they are re-created automatically.
        if (baseName == UsdShadeTokens->surface || baseName == UsdShadeTokens->volume
            || baseName == UsdShadeTokens->displacement) {
            return true;
        }
    }

    // Do not remove boundary properties even if there are connections.
    return false;
}

std::vector<UsdPrim> getRootPrimsToReference(const UsdStage& stage)
{
    std::vector<UsdPrim> rootPrims;

    for (const UsdPrim& prim : stage.GetPseudoRoot().GetAllChildren()) {
        if (prim.IsDefined() && !prim.IsAbstract())
            rootPrims.push_back(prim);
    }

    return rootPrims;
}

CrossRootOverridesResult authorCrossRootOverrides(
    const UsdStage&             sourceStage,
    const std::vector<UsdPrim>& referencedRoots,
    const SdfLayerHandle&       destLayer,
    const SdfPath&              destPrimPath)
{
    return CrossRootOverridesAuthor(referencedRoots, destLayer, destPrimPath).author(sourceStage);
}

} // namespace USDUFE_NS_DEF
