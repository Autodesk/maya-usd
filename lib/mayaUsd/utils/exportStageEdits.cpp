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
#include "exportStageEdits.h"

#include <mayaUsd/base/tokens.h>

#include <usdUfe/utils/fileSystem.h>

#include <pxr/base/tf/fileUtils.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/pxr.h>
#include <pxr/usd/pcp/expressionVariables.h>
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/pcp/node.h>
#include <pxr/usd/pcp/primIndex.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/changeBlock.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/layerTree.h>
#include <pxr/usd/sdf/layerUtils.h>
#include <pxr/usd/sdf/listOp.h>
#include <pxr/usd/sdf/payload.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/sdf/reference.h>
#include <pxr/usd/sdf/relationshipSpec.h>
#include <pxr/usd/sdf/schema.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdf/variantSetSpec.h>
#include <pxr/usd/sdf/variantSpec.h>
#if PXR_VERSION >= 2308
#include <pxr/usd/sdf/variableExpression.h>
#endif
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/object.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdUtils/authoring.h>
#include <pxr/usd/usdUtils/dependencies.h>
#include <pxr/usd/usdUtils/flattenLayerStack.h>

#include <algorithm>
#include <map>
#include <optional>
#include <set>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

namespace {

// Custom data key and session layer opinions written by Maya-USD on prims that are
// edited as Maya. See fileio/pullInformation.cpp. They must not be exported: the USD
// prims would be deactivated in the exported file.
const TfToken kPullPrimMetadataKey("Maya:Pull:DagPath");

// Attribute written by Maya-USD in the session layer on MayaReference prims.
const TfToken kMayaReferenceNodeName("MayaReferenceNodeName");

// Custom data keys written by Maya-USD in the session layer for the Outliner colors.
const char* const kOutlinerColorKeys[] = { "Autodesk:Use Text Color", "Autodesk:Text Color" };

const TfToken kSplineField("spline");

const SdfSchema& schema() { return SdfSchema::GetInstance(); }

// The fields holding the value of an attribute. The value of an attribute comes
// from the strongest layer that has any of them.
bool isValueField(const TfToken& field)
{
    return field == SdfFieldKeys->Default || field == SdfFieldKeys->TimeSamples
        || field == kSplineField;
}

bool hasTimeVaryingValue(const SdfLayerHandle& layer, const SdfPath& path)
{
    return layer->HasField(path, SdfFieldKeys->TimeSamples) || layer->HasField(path, kSplineField);
}

bool isRequiredField(SdfSpecType specType, const TfToken& field)
{
    const TfTokenVector& required = schema().GetRequiredFields(specType);
    return std::find(required.begin(), required.end(), field) != required.end();
}

// Prim specs are created as overs, so their required fields (the specifier) don't
// need to be copied. The other specs (e.g. attributes) need their required fields
// (e.g. the type name) to be valid.
bool mustCopyRequiredField(SdfSpecType specType, const TfToken& field)
{
    if (specType == SdfSpecTypePrim || specType == SdfSpecTypePseudoRoot
        || specType == SdfSpecTypeVariant)
        return false;
    return isRequiredField(specType, field);
}

bool isFileRelativePath(const std::string& assetPath)
{
    return TfStringStartsWith(assetPath, "./") || TfStringStartsWith(assetPath, "../")
        || TfStringStartsWith(assetPath, ".\\") || TfStringStartsWith(assetPath, "..\\");
}

bool isExpression(const std::string& assetPath)
{
#if PXR_VERSION >= 2308
    return SdfVariableExpression::IsExpression(assetPath);
#else
    return false;
#endif
}

// Returns the path of the target relative to the folder, starting with "./" so that
// USD anchors it to the layer, or the target unchanged if it can't be made relative.
std::string relativePathTo(const std::string& target, const std::string& folder)
{
    const auto relative = UsdUfe::FileSystem::makePathRelativeTo(target, folder);
    if (!relative.second)
        return target;

    std::string path = relative.first;
    if (!TfStringStartsWith(path, "./") && !TfStringStartsWith(path, "../"))
        path = "./" + path;
    return path;
}

std::string getLayerBaseName(const SdfLayerHandle& layer)
{
    std::string name;
    if (layer->IsAnonymous()) {
        name = layer->GetDisplayName();
    } else {
        name = TfStringGetBeforeSuffix(TfGetBaseName(layer->GetRealPath()));
    }
    if (name.empty())
        name = "layer";
    return TfMakeValidIdentifier(name);
}

bool layerHasContent(const SdfLayerHandle& layer)
{
    return layer && (!layer->GetRootPrims().empty() || layer->GetNumSubLayerPaths() > 0);
}

void clearSubLayers(const SdfLayerHandle& layer)
{
    layer->EraseField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->SubLayers);
    layer->EraseField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->SubLayerOffsets);
}

bool isMutedSubLayer(const UsdStage& stage, const SdfLayerHandle& layer, const std::string& path)
{
    if (stage.IsLayerMuted(path))
        return true;
    const std::string identifier = SdfComputeAssetPathRelativeToLayer(layer, path);
    return stage.IsLayerMuted(identifier);
}

bool hasMutedSubLayer(const UsdStage& stage, const SdfLayerHandle& layer)
{
    for (const std::string& path : layer->GetSubLayerPaths()) {
        if (isMutedSubLayer(stage, layer, path))
            return true;
    }
    return false;
}

// Removes the muted sublayers from the sublayers of the layer.
void removeMutedSubLayers(
    const UsdStage&       stage,
    const SdfLayerHandle& srcLayer,
    const SdfLayerHandle& layer)
{
    const std::vector<std::string> paths = layer->GetSubLayerPaths();
    const SdfLayerOffsetVector     offsets = layer->GetSubLayerOffsets();

    std::vector<std::string> keptPaths;
    SdfLayerOffsetVector     keptOffsets;
    for (size_t i = 0; i < paths.size(); ++i) {
        if (isMutedSubLayer(stage, srcLayer, paths[i]))
            continue;
        keptPaths.push_back(paths[i]);
        keptOffsets.push_back(i < offsets.size() ? offsets[i] : SdfLayerOffset());
    }

    if (keptPaths.size() == paths.size())
        return;

    clearSubLayers(layer);
    layer->SetSubLayerPaths(keptPaths);
    for (size_t i = 0; i < keptOffsets.size(); ++i)
        layer->SetSubLayerOffset(keptOffsets[i], static_cast<int>(i));
}

// Returns whether the spec holds data, ignoring its required fields and its children
// specs that don't hold data. Used to prune the empty specs created by SdfCopySpec.
bool specHasData(const SdfLayerHandle& layer, const SdfPath& path)
{
    const SdfSpecType specType = layer->GetSpecType(path);
    for (const TfToken& field : layer->ListFields(path)) {
        if (schema().HoldsChildren(field)) {
            // Relationship targets and connections are children specs: check them.
            // The other children (prims, properties, variants) are pruned first,
            // so any that remain hold data.
            const VtValue children = layer->GetField(path, field);
            if (children.IsHolding<SdfPathVector>()
                && (specType == SdfSpecTypeAttribute || specType == SdfSpecTypeRelationship)) {
                for (const SdfPath& target : children.UncheckedGet<SdfPathVector>()) {
                    if (specHasData(layer, path.AppendTarget(target)))
                        return true;
                }
                continue;
            }
            if (!children.IsEmpty())
                return true;
            continue;
        }

        if (field == SdfFieldKeys->Specifier) {
            if (layer->GetFieldAs<SdfSpecifier>(path, field, SdfSpecifierOver) != SdfSpecifierOver)
                return true;
            continue;
        }

        if (isRequiredField(specType, field))
            continue;

        return true;
    }
    return false;
}

// Removes the specs that hold no data under the prim spec. Returns true if the prim
// spec itself holds no data and can be removed.
bool pruneSpecsWithoutData(const SdfLayerHandle& layer, const SdfPrimSpecHandle& prim)
{
    const SdfPrimSpecHandleVector children
        = prim->GetNameChildren().values_as<SdfPrimSpecHandleVector>();
    for (const SdfPrimSpecHandle& child : children) {
        if (pruneSpecsWithoutData(layer, child))
            prim->RemoveNameChild(child);
    }

    std::vector<std::string> emptyVariantSets;
    for (const auto& variantSetEntry : prim->GetVariantSets()) {
        const SdfVariantSetSpecHandle variantSet = variantSetEntry.second;
        for (const SdfVariantSpecHandle& variant : variantSet->GetVariantList()) {
            if (pruneSpecsWithoutData(layer, variant->GetPrimSpec()))
                variantSet->RemoveVariant(variant);
        }
        if (variantSet->GetVariantList().empty())
            emptyVariantSets.push_back(variantSetEntry.first);
    }
    for (const std::string& name : emptyVariantSets)
        prim->RemoveVariantSet(name);

    const SdfPropertySpecHandleVector properties
        = prim->GetProperties().values_as<SdfPropertySpecHandleVector>();
    for (const SdfPropertySpecHandle& property : properties) {
        if (!specHasData(layer, property->GetPath()))
            prim->RemoveProperty(property);
    }

    return !specHasData(layer, prim->GetPath());
}

void pruneSpecsWithoutData(const SdfLayerHandle& layer)
{
    for (const SdfPrimSpecHandle& rootPrim :
         layer->GetRootPrims().values_as<SdfPrimSpecHandleVector>()) {
        if (pruneSpecsWithoutData(layer, rootPrim))
            layer->RemoveRootPrim(rootPrim);
    }
}

std::vector<SdfPath> getSpecPaths(const SdfLayerHandle& layer)
{
    std::vector<SdfPath> paths;
    layer->Traverse(SdfPath::AbsoluteRootPath(), [&paths](const SdfPath& path) {
        if (path != SdfPath::AbsoluteRootPath())
            paths.push_back(path);
    });
    std::sort(paths.begin(), paths.end());
    return paths;
}

// Replaces the asset paths of a value read from the composed stage by their
// resolved paths: the value is moved to another layer, so the authored paths,
// which are relative to the layer they come from, could not be resolved anymore.
void useResolvedAssetPaths(VtValue& value)
{
    if (value.IsHolding<SdfAssetPath>()) {
        const SdfAssetPath& assetPath = value.UncheckedGet<SdfAssetPath>();
        if (!assetPath.GetResolvedPath().empty())
            value = SdfAssetPath(assetPath.GetResolvedPath());
    } else if (value.IsHolding<VtArray<SdfAssetPath>>()) {
        VtArray<SdfAssetPath> assetPaths = value.UncheckedGet<VtArray<SdfAssetPath>>();
        for (SdfAssetPath& assetPath : assetPaths) {
            if (!assetPath.GetResolvedPath().empty())
                assetPath = SdfAssetPath(assetPath.GetResolvedPath());
        }
        value = assetPaths;
    }
}

template <class T> void appendListOpItems(const SdfListOp<T>& listOp, std::vector<T>& items)
{
    for (const auto* list : { &listOp.GetExplicitItems(),
                              &listOp.GetAddedItems(),
                              &listOp.GetPrependedItems(),
                              &listOp.GetAppendedItems() }) {
        for (const T& item : *list) {
            if (std::find(items.begin(), items.end(), item) == items.end())
                items.push_back(item);
        }
    }
}

// If the value is a list op, returns in result a list op deleting all its items.
template <class T> bool makeDeletingListOp(const VtValue& value, VtValue& result)
{
    if (!value.IsHolding<SdfListOp<T>>())
        return false;

    std::vector<T> items;
    appendListOpItems(value.UncheckedGet<SdfListOp<T>>(), items);
    SdfListOp<T> deleting;
    deleting.SetDeletedItems(items);
    result = VtValue(deleting);
    return true;
}

bool makeDeletingListOp(const VtValue& value, VtValue& result)
{
    return makeDeletingListOp<SdfPath>(value, result)
        || makeDeletingListOp<SdfReference>(value, result)
        || makeDeletingListOp<SdfPayload>(value, result)
        || makeDeletingListOp<TfToken>(value, result)
        || makeDeletingListOp<std::string>(value, result);
}

bool isListOp(const VtValue& value)
{
    return value.IsHolding<SdfPathListOp>() || value.IsHolding<SdfReferenceListOp>()
        || value.IsHolding<SdfPayloadListOp>() || value.IsHolding<SdfTokenListOp>()
        || value.IsHolding<SdfStringListOp>() || value.IsHolding<SdfIntListOp>()
        || value.IsHolding<SdfInt64ListOp>() || value.IsHolding<SdfUIntListOp>()
        || value.IsHolding<SdfUInt64ListOp>() || value.IsHolding<SdfUnregisteredValueListOp>();
}

// Returns whether the file on disk has fields at the path that the layer doesn't have.
bool hasDeletedFields(
    const SdfLayerHandle& layer,
    const SdfLayerHandle& savedLayer,
    const SdfPath&        path)
{
    if (!layer->HasSpec(path))
        return true;
    for (const TfToken& field : savedLayer->ListFields(path)) {
        if (!schema().HoldsChildren(field) && !layer->HasField(path, field))
            return true;
    }
    return false;
}

// Removes the value at the key path, and the dictionaries it leaves empty.
void removeValueAtPath(VtDictionary& dict, const std::string& keyPath)
{
    dict.EraseValueAtPath(keyPath);

    std::vector<std::string> keys = TfStringTokenize(keyPath, ":");
    while (keys.size() > 1) {
        keys.pop_back();
        const VtValue* parent = dict.GetValueAtPath(keys);
        if (!parent || !parent->IsHolding<VtDictionary>()
            || !parent->UncheckedGet<VtDictionary>().empty())
            break;
        dict.EraseValueAtPath(keys);
    }
}

// Removes the opinions Maya-USD authors in the session layer for its own needs, then
// the specs left empty. They only make sense in the Maya scene:
//   - on prims edited as Maya: the pull information, and the deactivation of the prim
//     (so that only the Maya data shows), see fileio/pullInformation.cpp,
//   - on MayaReference prims: the name of the Maya reference node, see
//     fileio/translators/translatorMayaReference.cpp,
//   - the text color of the prims in the Outliner, see displayCustomControl.py.
void removeMayaUsdBookkeeping(const SdfLayerHandle& layer)
{
    SdfChangeBlock changeBlock;

    for (const SdfPath& path : getSpecPaths(layer)) {
        if (layer->GetSpecType(path) != SdfSpecTypePrim)
            continue;

        const SdfPath nodeNamePath = path.AppendProperty(kMayaReferenceNodeName);
        if (layer->GetSpecType(nodeNamePath) == SdfSpecTypeAttribute) {
            if (SdfPrimSpecHandle prim = layer->GetPrimAtPath(path)) {
                if (SdfPropertySpecHandle property = layer->GetPropertyAtPath(nodeNamePath))
                    prim->RemoveProperty(property);
            }
        }

        VtValue customData;
        if (!layer->HasField(path, SdfFieldKeys->CustomData, &customData)
            || !customData.IsHolding<VtDictionary>())
            continue;

        VtDictionary dict = customData.UncheckedGet<VtDictionary>();
        bool         changed = false;

        if (dict.GetValueAtPath(kPullPrimMetadataKey.GetString())) {
            removeValueAtPath(dict, kPullPrimMetadataKey.GetString());
            layer->EraseField(path, SdfFieldKeys->Active);
            changed = true;
        }

        for (const char* key : kOutlinerColorKeys) {
            if (dict.GetValueAtPath(key)) {
                removeValueAtPath(dict, key);
                changed = true;
            }
        }

        if (!changed)
            continue;

        if (dict.empty())
            layer->EraseField(path, SdfFieldKeys->CustomData);
        else
            layer->SetField(path, SdfFieldKeys->CustomData, VtValue(dict));
    }

    pruneSpecsWithoutData(layer);
}

// Removes from the dictionary the keys that the stronger dictionary has, recursively:
// dictionaries are composed key by key.
void eraseHiddenKeys(VtDictionary& dict, const VtDictionary& stronger)
{
    for (const auto& entry : stronger) {
        auto found = dict.find(entry.first);
        if (found == dict.end())
            continue;
        if (found->second.IsHolding<VtDictionary>() && entry.second.IsHolding<VtDictionary>()) {
            VtDictionary sub = found->second.UncheckedGet<VtDictionary>();
            eraseHiddenKeys(sub, entry.second.UncheckedGet<VtDictionary>());
            if (sub.empty())
                dict.erase(found);
            else
                found->second = VtValue(sub);
        } else {
            dict.erase(found);
        }
    }
}

// Returns the items with their asset paths anchored to the layer, for the items of
// list ops (references and payloads) that are composed across layers.
template <class T> std::vector<T> anchorItems(const std::vector<T>& items, const SdfLayerHandle&)
{
    return items;
}

template <class ArcType>
std::vector<ArcType> anchorArcItems(const std::vector<ArcType>& items, const SdfLayerHandle& layer)
{
    std::vector<ArcType> anchored = items;
    for (ArcType& item : anchored) {
        if (!item.GetAssetPath().empty() && !isExpression(item.GetAssetPath()))
            item.SetAssetPath(SdfComputeAssetPathRelativeToLayer(layer, item.GetAssetPath()));
    }
    return anchored;
}

std::vector<SdfReference>
anchorItems(const std::vector<SdfReference>& items, const SdfLayerHandle& layer)
{
    return anchorArcItems(items, layer);
}

std::vector<SdfPayload>
anchorItems(const std::vector<SdfPayload>& items, const SdfLayerHandle& layer)
{
    return anchorArcItems(items, layer);
}

template <class T>
SdfListOp<T> anchorListOp(const SdfListOp<T>& listOp, const SdfLayerHandle& layer)
{
    if (listOp.IsExplicit())
        return SdfListOp<T>::CreateExplicit(anchorItems(listOp.GetExplicitItems(), layer));

    SdfListOp<T> anchored;
    anchored.SetAddedItems(anchorItems(listOp.GetAddedItems(), layer));
    anchored.SetPrependedItems(anchorItems(listOp.GetPrependedItems(), layer));
    anchored.SetAppendedItems(anchorItems(listOp.GetAppendedItems(), layer));
    anchored.SetDeletedItems(anchorItems(listOp.GetDeletedItems(), layer));
    anchored.SetOrderedItems(anchorItems(listOp.GetOrderedItems(), layer));
    return anchored;
}

// Composes the list op of a delta, which is now above the files of the layers that
// were stronger than its layer, with the list ops of these stronger layers, so that
// the result is the same as in the stage. Returns false if the value isn't a list op
// of type T. The value is emptied if the delta doesn't need the list op anymore.
template <class T>
bool composeListOpWithStronger(
    VtValue&                           value,
    const SdfPath&                     path,
    const TfToken&                     field,
    const SdfLayerHandle&              layer,
    const std::vector<SdfLayerHandle>& strongerLayers)
{
    if (!value.IsHolding<SdfListOp<T>>())
        return false;

    // The stronger list ops, from strongest to weakest, with their asset paths anchored
    // to their own layer, since they are composed in another layer.
    std::vector<SdfListOp<T>> strongerOps;
    for (const SdfLayerHandle& l : strongerLayers) {
        VtValue stronger;
        if (l->HasField(path, field, &stronger) && stronger.IsHolding<SdfListOp<T>>())
            strongerOps.push_back(anchorListOp(stronger.UncheckedGet<SdfListOp<T>>(), l));
    }
    if (strongerOps.empty())
        return true;

    const SdfListOp<T> listOp = anchorListOp(value.UncheckedGet<SdfListOp<T>>(), layer);

    if (listOp.IsExplicit()) {
        // The explicit list of the layer is what the stronger layers edited: author the
        // result, which replaces the opinions of all the layers below.
        std::vector<T> items = listOp.GetExplicitItems();
        for (auto stronger = strongerOps.rbegin(); stronger != strongerOps.rend(); ++stronger)
            stronger->ApplyOperations(&items);
        value = VtValue(SdfListOp<T>::CreateExplicit(items));
        return true;
    }

    // A stronger explicit list replaces the edits of the layer.
    const bool strongerExplicit
        = std::any_of(strongerOps.begin(), strongerOps.end(), [](const SdfListOp<T>& op) {
              return op.IsExplicit();
          });
    if (strongerExplicit) {
        value = VtValue();
        return true;
    }

    // Deleting an item that a stronger layer adds has no effect in the stage.
    std::vector<T> strongerItems;
    for (const SdfListOp<T>& stronger : strongerOps)
        appendListOpItems(stronger, strongerItems);

    std::vector<T> deleted;
    for (const T& item : listOp.GetDeletedItems()) {
        if (std::find(strongerItems.begin(), strongerItems.end(), item) == strongerItems.end())
            deleted.push_back(item);
    }

    SdfListOp<T> composed = listOp;
    composed.SetDeletedItems(deleted);
    if (composed.HasKeys())
        value = VtValue(composed);
    else
        value = VtValue();
    return true;
}

bool composeListOpWithStronger(
    VtValue&                           value,
    const SdfPath&                     path,
    const TfToken&                     field,
    const SdfLayerHandle&              layer,
    const std::vector<SdfLayerHandle>& strongerLayers)
{
    return composeListOpWithStronger<SdfPath>(value, path, field, layer, strongerLayers)
        || composeListOpWithStronger<SdfReference>(value, path, field, layer, strongerLayers)
        || composeListOpWithStronger<SdfPayload>(value, path, field, layer, strongerLayers)
        || composeListOpWithStronger<TfToken>(value, path, field, layer, strongerLayers)
        || composeListOpWithStronger<std::string>(value, path, field, layer, strongerLayers);
}

// A layer of the wrapper layer, from strongest to weakest.
struct WrapperEntry
{
    std::string    authoredPath; // the sublayer path to author in the wrapper
    SdfLayerOffset offset;       // the offset the layer must have in the layer stack
    double         timeCodesPerSecond;
};

// A layer that the wrapper brings in through its file on disk. Its arcs to layers
// that are copied need to be repointed.
struct LayerOnDisk
{
    SdfLayerHandle layer;      // the layer in memory
    SdfLayerHandle savedLayer; // its content on disk (the layer itself if not dirty)
    SdfLayerOffset offset;     // its offset in the layer stack
};

class StageEditsExporter
{
public:
    StageEditsExporter(
        const UsdStageRefPtr& stage,
        const std::string&    destFolder,
        const std::string&    fileFormat,
        bool                  relativeOriginalPaths,
        ExportedStageEdits&   result)
        : _stage(stage)
        , _destFolder(destFolder)
        , _fileFormat(fileFormat == "usda" ? "usda" : "usdc")
        , _relativeOriginalPaths(relativeOriginalPaths)
        , _result(result)
    {
    }

    void run();

private:
    void warn(const std::string& message) { _result.warnings.push_back(message); }

    std::string makeFilePath(const std::string& baseName, const std::string& extension);
    bool        writeLayer(const SdfLayerHandle& layer, const std::string& path);
    std::string authoredPathToOriginal(const SdfLayerHandle& layer) const;

    void remapAssetPaths(
        const SdfLayerHandle& layer,
        const SdfLayerHandle& srcLayer,
        bool                  rewriteDependencies);

    SdfLayerHandle getSavedLayer(const SdfLayerHandle& layer);
    bool           canReferenceFile(const SdfLayerTreeHandle& tree);
    void           emitTree(const SdfLayerTreeHandle& tree, std::vector<WrapperEntry>& entries);

    SdfLayerRefPtr computeDelta(
        const SdfLayerHandle&              layer,
        const SdfLayerHandle&              savedLayer,
        const SdfLayerOffset&              layerOffset,
        const std::vector<SdfLayerHandle>& strongerLayers);
    void handleDeletions(
        const SdfLayerHandle& layer,
        const SdfLayerHandle& savedLayer,
        const SdfLayerOffset& layerOffset,
        const SdfLayerHandle& delta);
    void handleValueDeletion(
        const SdfLayerHandle& layer,
        const SdfLayerHandle& savedLayer,
        const SdfPath&        path,
        const SdfLayerOffset& layerOffset,
        const SdfLayerHandle& delta);
    void handleFieldDeletion(
        const SdfLayerHandle& savedLayer,
        const SdfPath&        path,
        const TfToken&        field,
        const SdfLayerHandle& delta);
    bool
    ensureSpec(const SdfLayerHandle& delta, const SdfLayerHandle& savedLayer, const SdfPath& path);
    void filterShadowedOpinions(
        const SdfLayerHandle&              delta,
        const SdfLayerHandle&              layer,
        const std::vector<SdfLayerHandle>& strongerLayers);

    SdfLayerRefPtr buildSessionLayer();

    void           findDependenciesToCopy();
    void           writeDependencyCopies();
    SdfLayerRefPtr buildRepointLayer();

    bool writeWrapper(const std::vector<WrapperEntry>& entries);

    UsdStageRefPtr      _stage;
    PcpLayerStackRefPtr _layerStack;
    double              _stackTcps = 24.0;
    std::string         _destFolder;
    std::string         _fileFormat;
    bool                _relativeOriginalPaths;
    ExportedStageEdits& _result;

    std::set<std::string> _usedFileNames;
    bool                  _folderCreated = false;
    bool                  _warnedAboutExpressions = false;
    bool                  _warnedAboutVariants = false;

    // The content on disk of the dirty layers.
    std::map<SdfLayerHandle, SdfLayerRefPtr> _savedLayers;

    // The layers outside the root layer stack that are copied, and their new path.
    std::map<SdfLayerHandle, std::string> _dependencyCopies;
    std::map<std::string, std::string>    _dependencyCopiesByIdentifier;

    // The layers of the root layer stack that the wrapper brings in from disk.
    std::vector<LayerOnDisk> _layersOnDisk;
};

std::string
StageEditsExporter::makeFilePath(const std::string& baseName, const std::string& extension)
{
    std::string name = baseName;
    for (int suffix = 1; !_usedFileNames.insert(TfStringToLower(name)).second; ++suffix)
        name = TfStringPrintf("%s_%d", baseName.c_str(), suffix);
    return TfStringCatPaths(_destFolder, name + "." + extension);
}

bool StageEditsExporter::writeLayer(const SdfLayerHandle& layer, const std::string& path)
{
    if (!_folderCreated) {
        if (!TfIsDir(_destFolder) && !TfMakeDirs(_destFolder, -1, true)) {
            warn(TfStringPrintf("Could not create the folder '%s'.", _destFolder.c_str()));
            return false;
        }
        _folderCreated = true;
    }

    if (!layer->Export(path)) {
        warn(TfStringPrintf("Could not write the file '%s'.", path.c_str()));
        return false;
    }

    // A previous export may have left this file open in memory: make sure the
    // exported stage sees the new content.
    if (SdfLayerHandle openLayer = SdfLayer::Find(path))
        openLayer->Reload(true);

    _result.writtenFiles.push_back(path);
    return true;
}

std::string StageEditsExporter::authoredPathToOriginal(const SdfLayerHandle& layer) const
{
    if (_relativeOriginalPaths && !layer->GetRealPath().empty())
        return relativePathTo(layer->GetRealPath(), _destFolder);
    return layer->GetIdentifier();
}

void StageEditsExporter::remapAssetPaths(
    const SdfLayerHandle& layer,
    const SdfLayerHandle& srcLayer,
    bool                  rewriteDependencies)
{
    UsdUtilsModifyAssetPaths(
        layer,
        [&](const std::string& assetPath) -> std::string {
            if (assetPath.empty())
                return assetPath;

            if (isExpression(assetPath)) {
                if (!_warnedAboutExpressions) {
                    _warnedAboutExpressions = true;
                    warn("Asset paths using expressions were written unchanged and may not "
                         "resolve from the exported files.");
                }
                return assetPath;
            }

            const std::string anchored = SdfComputeAssetPathRelativeToLayer(srcLayer, assetPath);

            if (rewriteDependencies) {
                const auto copy = _dependencyCopiesByIdentifier.find(anchored);
                if (copy != _dependencyCopiesByIdentifier.end())
                    return relativePathTo(copy->second, _destFolder);
            }

            // The layer is written to another folder: paths that USD anchors to the
            // original layer must point to where they resolved from it. This includes
            // search-path style paths (e.g. "textures/a.png"), which USD looks up next
            // to the layer first: their anchored path differs only if it exists. (Paths
            // in anonymous layers were never anchored, so they stay unchanged.)
            if (!srcLayer->IsAnonymous() && anchored != assetPath
                && (isFileRelativePath(assetPath) || TfIsRelativePath(assetPath))) {
                if (_relativeOriginalPaths)
                    return relativePathTo(anchored, _destFolder);
                return anchored;
            }

            return assetPath;
        },
        /*keepEmptyPathsInArrays*/ true);
}

SdfLayerHandle StageEditsExporter::getSavedLayer(const SdfLayerHandle& layer)
{
    if (layer->IsAnonymous())
        return {};
    if (!layer->IsDirty())
        return layer;

    auto saved = _savedLayers.find(layer);
    if (saved == _savedLayers.end()) {
        SdfLayerRefPtr    savedLayer;
        const std::string realPath = layer->GetRealPath();
        if (!realPath.empty() && TfIsFile(realPath))
            savedLayer = SdfLayer::OpenAsAnonymous(realPath);
        saved = _savedLayers.emplace(layer, savedLayer).first;
    }
    return saved->second;
}

// Returns whether the wrapper can bring in the layer through its file on disk, with
// deltas for the unsaved changes of the layers of its subtree. This is not possible
// when the file on disk doesn't have the same sublayers as the layer in memory.
bool StageEditsExporter::canReferenceFile(const SdfLayerTreeHandle& tree)
{
    const SdfLayerHandle& layer = tree->GetLayer();
    if (layer->IsAnonymous())
        return false;

    if (layer->IsDirty()) {
        const SdfLayerHandle savedLayer = getSavedLayer(layer);
        if (!savedLayer)
            return false;

        const SdfPath& root = SdfPath::AbsoluteRootPath();
        if (layer->GetField(root, SdfFieldKeys->SubLayers)
                != savedLayer->GetField(root, SdfFieldKeys->SubLayers)
            || layer->GetField(root, SdfFieldKeys->SubLayerOffsets)
                != savedLayer->GetField(root, SdfFieldKeys->SubLayerOffsets))
            return false;
    }

    if (hasMutedSubLayer(*_stage, layer))
        return false;

    for (const SdfLayerTreeHandle& child : tree->GetChildTrees()) {
        if (!canReferenceFile(child))
            return false;
    }

    return true;
}

void collectPreOrder(const SdfLayerTreeHandle& tree, std::vector<SdfLayerTreeHandle>& nodes)
{
    nodes.push_back(tree);
    for (const SdfLayerTreeHandle& child : tree->GetChildTrees())
        collectPreOrder(child, nodes);
}

void StageEditsExporter::emitTree(
    const SdfLayerTreeHandle&  tree,
    std::vector<WrapperEntry>& entries)
{
    const SdfLayerHandle& layer = tree->GetLayer();

    if (canReferenceFile(tree)) {
        // The file on disk brings in the whole subtree as it was saved. Add the unsaved
        // changes of the layers of the subtree as deltas above it, from strongest to
        // weakest. Each delta is now stronger than the layers of the subtree that were
        // stronger than its layer, so the opinions of those layers are removed from it.
        std::vector<SdfLayerTreeHandle> nodes;
        collectPreOrder(tree, nodes);

        std::vector<SdfLayerHandle> strongerLayers;
        for (const SdfLayerTreeHandle& node : nodes) {
            const SdfLayerHandle& nodeLayer = node->GetLayer();
            const SdfLayerHandle  savedLayer = getSavedLayer(nodeLayer);

            if (nodeLayer->IsDirty() && savedLayer) {
                SdfLayerRefPtr delta
                    = computeDelta(nodeLayer, savedLayer, node->GetOffset(), strongerLayers);
                if (delta) {
                    remapAssetPaths(delta, nodeLayer, /*rewriteDependencies*/ false);
                    const std::string path
                        = makeFilePath(getLayerBaseName(nodeLayer) + "_edits", _fileFormat);
                    if (writeLayer(delta, path)) {
                        entries.push_back({ relativePathTo(path, _destFolder),
                                            node->GetOffset(),
                                            delta->GetTimeCodesPerSecond() });
                    }
                }
            }

            strongerLayers.push_back(nodeLayer);
            _layersOnDisk.push_back({ nodeLayer, savedLayer, node->GetOffset() });
        }

        entries.push_back(
            { authoredPathToOriginal(layer), tree->GetOffset(), layer->GetTimeCodesPerSecond() });
        return;
    }

    // The file on disk can't be used: write the content of the layer, without its
    // sublayers, which are added to the wrapper separately.
    SdfLayerRefPtr copy = SdfLayer::CreateAnonymous(getLayerBaseName(layer));
    copy->TransferContent(layer);
    clearSubLayers(copy);

    if (!layer->IsAnonymous()) {
        _result.copiedLayers.push_back(layer->GetIdentifier());
        warn(TfStringPrintf(
            "The layer '%s' was written in full, because its sublayers were changed or muted.",
            layer->GetIdentifier().c_str()));
    }

    if (layerHasContent(copy)) {
        remapAssetPaths(copy, layer, /*rewriteDependencies*/ true);
        const std::string path = makeFilePath(getLayerBaseName(layer), _fileFormat);
        if (writeLayer(copy, path)) {
            entries.push_back({ relativePathTo(path, _destFolder),
                                tree->GetOffset(),
                                copy->GetTimeCodesPerSecond() });
        }
    }

    for (const SdfLayerTreeHandle& child : tree->GetChildTrees())
        emitTree(child, entries);
}

SdfLayerRefPtr StageEditsExporter::computeDelta(
    const SdfLayerHandle&              layer,
    const SdfLayerHandle&              savedLayer,
    const SdfLayerOffset&              layerOffset,
    const std::vector<SdfLayerHandle>& strongerLayers)
{
    SdfLayerRefPtr delta = SdfLayer::CreateAnonymous(getLayerBaseName(layer) + "_edits");

    // The offsets of the references and payloads of a layer are scaled by its TCPS, so
    // the delta must have the same. (The wrapper compensates for it in the offset of
    // the delta.)
    if (layer->HasTimeCodesPerSecond())
        delta->SetTimeCodesPerSecond(layer->GetTimeCodesPerSecond());
    if (layer->HasFramesPerSecond())
        delta->SetFramesPerSecond(layer->GetFramesPerSecond());

    const auto valueDiffers = [&](const SdfPath& path) {
        for (const TfToken& field :
             { SdfFieldKeys->Default, SdfFieldKeys->TimeSamples, kSplineField }) {
            VtValue    value, savedValue;
            const bool hasValue = layer->HasField(path, field, &value);
            const bool hasSavedValue = savedLayer->HasField(path, field, &savedValue);
            if (hasValue != hasSavedValue || (hasValue && value != savedValue))
                return true;
        }
        return false;
    };

    // Copy the fields that differ from the file on disk. The value fields of an
    // attribute are copied together, since the strongest layer with any of them
    // provides the value.
    const SdfShouldCopyValueFn copyValue = [&](SdfSpecType           specType,
                                               const TfToken&        field,
                                               const SdfLayerHandle& srcLayer,
                                               const SdfPath&        srcPath,
                                               bool                  fieldInSrc,
                                               const SdfLayerHandle&,
                                               const SdfPath&,
                                               bool,
                                               std::optional<VtValue>*) {
        if (!fieldInSrc)
            return false;
        if (mustCopyRequiredField(specType, field))
            return true;
        if (isValueField(field))
            return valueDiffers(srcPath);

        VtValue savedValue;
        if (!savedLayer->HasField(srcPath, field, &savedValue))
            return true;
        return srcLayer->GetField(srcPath, field) != savedValue;
    };

    const SdfShouldCopyChildrenFn copyChildren = [](const TfToken&,
                                                    const SdfLayerHandle&,
                                                    const SdfPath&,
                                                    bool,
                                                    const SdfLayerHandle&,
                                                    const SdfPath&,
                                                    bool,
                                                    std::optional<VtValue>*,
                                                    std::optional<VtValue>*) { return true; };

    {
        SdfChangeBlock changeBlock;
        for (const SdfPrimSpecHandle& rootPrim : layer->GetRootPrims()) {
            SdfCopySpec(
                layer, rootPrim->GetPath(), delta, rootPrim->GetPath(), copyValue, copyChildren);
        }
        handleDeletions(layer, savedLayer, layerOffset, delta);
        filterShadowedOpinions(delta, layer, strongerLayers);
        pruneSpecsWithoutData(delta);
    }

    if (delta->GetRootPrims().empty())
        return {};

    return delta;
}

bool StageEditsExporter::ensureSpec(
    const SdfLayerHandle& delta,
    const SdfLayerHandle& savedLayer,
    const SdfPath&        path)
{
    if (delta->HasSpec(path))
        return true;

    const SdfSpecType specType = savedLayer->GetSpecType(path);
    if (specType == SdfSpecTypePrim)
        return bool(SdfCreatePrimInLayer(delta, path));

    if (specType == SdfSpecTypeAttribute) {
        const SdfAttributeSpecHandle savedAttr = savedLayer->GetAttributeAtPath(path);
        const SdfPrimSpecHandle      prim = SdfCreatePrimInLayer(delta, path.GetPrimPath());
        if (!savedAttr || !prim)
            return false;
        return bool(SdfAttributeSpec::New(
            prim,
            path.GetName(),
            savedAttr->GetTypeName(),
            savedAttr->GetVariability(),
            savedAttr->IsCustom()));
    }

    if (specType == SdfSpecTypeRelationship) {
        const SdfRelationshipSpecHandle savedRel = savedLayer->GetRelationshipAtPath(path);
        const SdfPrimSpecHandle         prim = SdfCreatePrimInLayer(delta, path.GetPrimPath());
        if (!savedRel || !prim)
            return false;
        return bool(SdfRelationshipSpec::New(
            prim, path.GetName(), savedRel->IsCustom(), savedRel->GetVariability()));
    }

    return false;
}

// Adds to the delta what's needed to remove the opinions of the file on disk that
// were deleted in memory. The delta is above the file on disk, so it can't delete
// them: it authors opinions giving the same result instead.
void StageEditsExporter::handleDeletions(
    const SdfLayerHandle& layer,
    const SdfLayerHandle& savedLayer,
    const SdfLayerOffset& layerOffset,
    const SdfLayerHandle& delta)
{
    std::vector<SdfPath> deactivatedPrims;

    for (const SdfPath& path : getSpecPaths(savedLayer)) {
        const bool underDeactivatedPrim = std::any_of(
            deactivatedPrims.begin(), deactivatedPrims.end(), [&path](const SdfPath& prim) {
                return path.HasPrefix(prim);
            });
        if (underDeactivatedPrim)
            continue;

        const SdfSpecType specType = savedLayer->GetSpecType(path);
        if (specType != SdfSpecTypePrim && specType != SdfSpecTypeAttribute
            && specType != SdfSpecTypeRelationship)
            continue;

        // The deleted opinions are replaced by values computed from the stage, which
        // can't be done in variants that may not be selected.
        if (path.ContainsPrimVariantSelection()) {
            if (hasDeletedFields(layer, savedLayer, path)) {
                if (!_warnedAboutVariants) {
                    _warnedAboutVariants = true;
                    warn("Some opinions deleted inside variants could not be exported.");
                }
            }
            continue;
        }

        if (specType == SdfSpecTypePrim) {
            // The prim was deleted, or this layer doesn't define it anymore. If the
            // stage doesn't have it anymore, or doesn't define it anymore while the
            // file on disk does, deactivate it: the file on disk can't be overridden
            // to not define it. Otherwise only the opinions of this layer were
            // removed: handle them below.
            const bool removedSpec = !layer->HasSpec(path);
            const bool savedDefines = savedLayer->GetFieldAs<SdfSpecifier>(
                                          path, SdfFieldKeys->Specifier, SdfSpecifierOver)
                != SdfSpecifierOver;
            const bool defines = !removedSpec
                && layer->GetFieldAs<SdfSpecifier>(path, SdfFieldKeys->Specifier, SdfSpecifierOver)
                    != SdfSpecifierOver;
            if (removedSpec || (savedDefines && !defines)) {
                const UsdPrim livePrim = _stage->GetPrimAtPath(path);
                if (!livePrim || (savedDefines && !livePrim.IsDefined())) {
                    if (SdfPrimSpecHandle prim = SdfCreatePrimInLayer(delta, path)) {
                        prim->SetActive(false);
                        deactivatedPrims.push_back(path);
                    }
                    continue;
                }
            }
        }

        if (specType == SdfSpecTypeAttribute)
            handleValueDeletion(layer, savedLayer, path, layerOffset, delta);

        for (const TfToken& field : savedLayer->ListFields(path)) {
            if (schema().HoldsChildren(field) || isValueField(field))
                continue;
            if (layer->HasField(path, field))
                continue;
            handleFieldDeletion(savedLayer, path, field, delta);
        }
    }
}

void StageEditsExporter::handleValueDeletion(
    const SdfLayerHandle& layer,
    const SdfLayerHandle& savedLayer,
    const SdfPath&        path,
    const SdfLayerOffset& layerOffset,
    const SdfLayerHandle& delta)
{
    const bool hasDefault = layer->HasField(path, SdfFieldKeys->Default);
    const bool hasSamples = hasTimeVaryingValue(layer, path);
    const bool savedHasDefault = savedLayer->HasField(path, SdfFieldKeys->Default);
    const bool savedHasSamples = hasTimeVaryingValue(savedLayer, path);

    // The value fields of the layer are in the delta. What the delta can't express is
    // a value field of the file on disk that the layer doesn't have anymore, when the
    // layer doesn't provide another value that hides it.
    const bool defaultDeleted = savedHasDefault && !hasDefault;
    const bool samplesDeleted = savedHasSamples && !hasSamples && !hasDefault;
    if (!defaultDeleted && !samplesDeleted)
        return;

    if (!ensureSpec(delta, savedLayer, path))
        return;

    // Use the value of the stage, which doesn't have the deleted opinions anymore.
    const UsdAttribute attr = _stage->GetAttributeAtPath(path);

    if (samplesDeleted && attr && attr.GetNumTimeSamples() > 0) {
        std::vector<double> times;
        attr.GetTimeSamples(&times);
        const SdfLayerOffset toLayerTime = layerOffset.GetInverse();
        SdfTimeSampleMap     samples;
        for (double time : times) {
            VtValue value;
            if (attr.Get(&value, time)) {
                useResolvedAssetPaths(value);
                samples[toLayerTime * time] = value;
            }
        }
        delta->SetField(path, SdfFieldKeys->TimeSamples, VtValue(samples));
    }

    if (defaultDeleted || (samplesDeleted && !delta->HasField(path, SdfFieldKeys->TimeSamples))) {
        VtValue value;
        if (attr && attr.Get(&value, UsdTimeCode::Default())) {
            useResolvedAssetPaths(value);
        } else {
            value = SdfValueBlock();
        }
        delta->SetField(path, SdfFieldKeys->Default, value);
    }
}

void StageEditsExporter::handleFieldDeletion(
    const SdfLayerHandle& savedLayer,
    const SdfPath&        path,
    const TfToken&        field,
    const SdfLayerHandle& delta)
{
    const SdfSpecType specType = savedLayer->GetSpecType(path);

    // Without these fields, the spec is an over: the other layers provide them.
    if (isRequiredField(specType, field) || field == SdfFieldKeys->Specifier
        || field == SdfFieldKeys->TypeName)
        return;

    const VtValue savedValue = savedLayer->GetField(path, field);

    // List ops: delete the items the file on disk adds.
    VtValue deleting;
    if (makeDeletingListOp(savedValue, deleting)) {
        if (ensureSpec(delta, savedLayer, path))
            delta->SetField(path, field, deleting);
        return;
    }
    if (isListOp(savedValue)) {
        warn(TfStringPrintf(
            "The removal of '%s' on '%s' could not be exported.", field.GetText(), path.GetText()));
        return;
    }

    // Other fields: author the value of the stage, which doesn't have the deleted
    // opinion anymore, or the fallback value.
    VtValue         value;
    const UsdObject object = _stage->GetObjectAtPath(path);
    if (!(object && object.GetMetadata(field, &value)))
        value = schema().GetFallback(field);

    if (value.IsEmpty()) {
        warn(TfStringPrintf(
            "The removal of '%s' on '%s' could not be exported.", field.GetText(), path.GetText()));
        return;
    }

    if (value.IsHolding<VtDictionary>() && savedValue.IsHolding<VtDictionary>()) {
        const VtDictionary& composed = value.UncheckedGet<VtDictionary>();
        for (const auto& entry : savedValue.UncheckedGet<VtDictionary>()) {
            if (composed.find(entry.first) == composed.end()) {
                warn(TfStringPrintf(
                    "The removal of the key '%s' of '%s' on '%s' could not be exported.",
                    entry.first.c_str(),
                    field.GetText(),
                    path.GetText()));
            }
        }
    }

    if (ensureSpec(delta, savedLayer, path))
        delta->SetField(path, field, value);
}

// Removes from the delta the opinions that the stronger layers hide.
//
// The delta of a layer is above the files of all the layers of its subtree, including
// the layers that were stronger than it. So whatever these stronger layers hide must
// be removed from the delta, and the list ops must be composed with theirs.
void StageEditsExporter::filterShadowedOpinions(
    const SdfLayerHandle&              delta,
    const SdfLayerHandle&              layer,
    const std::vector<SdfLayerHandle>& strongerLayers)
{
    if (strongerLayers.empty())
        return;

    for (const SdfPath& path : getSpecPaths(delta)) {
        const SdfSpecType specType = delta->GetSpecType(path);

        for (const TfToken& field : delta->ListFields(path)) {
            if (schema().HoldsChildren(field) || mustCopyRequiredField(specType, field))
                continue;

            if (isValueField(field)) {
                // A stronger default hides all the values. Stronger time samples hide
                // the time samples, and the default at non-default times. (The default
                // is removed then: only its value at the default time is lost.)
                const bool hidden = std::any_of(
                    strongerLayers.begin(), strongerLayers.end(), [&](const SdfLayerHandle& l) {
                        return l->HasField(path, SdfFieldKeys->Default)
                            || hasTimeVaryingValue(l, path);
                    });
                if (hidden)
                    delta->EraseField(path, field);
                continue;
            }

            if (field == SdfFieldKeys->Specifier) {
                // The strongest defining specifier (def or class) wins: overs don't
                // hide a def.
                const auto isDefining = [&](const SdfLayerHandle& l) {
                    return l->GetFieldAs<SdfSpecifier>(path, field, SdfSpecifierOver)
                        != SdfSpecifierOver;
                };
                if (!isDefining(delta)
                    || std::any_of(strongerLayers.begin(), strongerLayers.end(), isDefining))
                    delta->EraseField(path, field);
                continue;
            }

            VtValue value = delta->GetField(path, field);

            if (value.IsHolding<VtDictionary>()) {
                // Dictionaries are composed key by key, recursively.
                VtDictionary dict = value.UncheckedGet<VtDictionary>();
                for (const SdfLayerHandle& l : strongerLayers) {
                    VtValue stronger;
                    if (l->HasField(path, field, &stronger) && stronger.IsHolding<VtDictionary>())
                        eraseHiddenKeys(dict, stronger.UncheckedGet<VtDictionary>());
                }
                if (dict.empty())
                    delta->EraseField(path, field);
                else
                    delta->SetField(path, field, VtValue(dict));
                continue;
            }

            if (value.IsHolding<SdfVariantSelectionMap>()) {
                // Variant selections are composed variant set by variant set.
                SdfVariantSelectionMap selections = value.UncheckedGet<SdfVariantSelectionMap>();
                for (const SdfLayerHandle& l : strongerLayers) {
                    VtValue stronger;
                    if (!l->HasField(path, field, &stronger)
                        || !stronger.IsHolding<SdfVariantSelectionMap>())
                        continue;
                    for (const auto& entry : stronger.UncheckedGet<SdfVariantSelectionMap>())
                        selections.erase(entry.first);
                }
                if (selections.empty())
                    delta->EraseField(path, field);
                else
                    delta->SetField(path, field, VtValue(selections));
                continue;
            }

            if (isListOp(value)) {
                if (composeListOpWithStronger(value, path, field, layer, strongerLayers)) {
                    if (value.IsEmpty())
                        delta->EraseField(path, field);
                    else
                        delta->SetField(path, field, value);
                }
                continue;
            }

            const bool hidden = std::any_of(
                strongerLayers.begin(), strongerLayers.end(), [&](const SdfLayerHandle& l) {
                    return l->HasField(path, field);
                });
            if (hidden)
                delta->EraseField(path, field);
        }
    }
}

SdfLayerRefPtr StageEditsExporter::buildSessionLayer()
{
    const SdfLayerHandle session = _stage->GetSessionLayer();
    if (!session || !_layerStack->GetSessionLayerTree())
        return {};

    SdfLayerRefPtr copy;
    if (session->GetNumSubLayerPaths() == 0) {
        copy = SdfLayer::CreateAnonymous("session");
        copy->TransferContent(session);
    } else {
        // Flatten the session layer and its sublayers, as they are in the stage.
        UsdStageRefPtr sessionStage = UsdStage::Open(session, UsdStage::LoadNone);
        if (!sessionStage)
            return {};
        const std::set<std::string> muted(
            _stage->GetMutedLayers().begin(), _stage->GetMutedLayers().end());
        sessionStage->MuteAndUnmuteLayers(std::vector<std::string>(muted.begin(), muted.end()), {});
        copy = UsdUtilsFlattenLayerStack(sessionStage, "session");
    }

    if (!copy)
        return {};

    removeMayaUsdBookkeeping(copy);
    if (copy->GetRootPrims().empty())
        return {};

    remapAssetPaths(copy, session, /*rewriteDependencies*/ true);
    return copy;
}

// Finds the layers outside the root layer stack (e.g. reached through references)
// that have edits, and the layers that lead to them, which all need to be copied.
void StageEditsExporter::findDependenciesToCopy()
{
    std::set<SdfLayerHandle> stackLayers;
    for (const SdfLayerRefPtr& layer : _layerStack->GetLayers())
        stackLayers.insert(layer);

    std::vector<SdfLayerHandle>                           others;
    std::map<SdfLayerHandle, std::vector<SdfLayerHandle>> dependencies;
    std::set<SdfLayerHandle>                              toCopy;

    for (const SdfLayerHandle& layer : _stage->GetUsedLayers(/*includeClipLayers*/ false)) {
        if (!layer || stackLayers.count(layer))
            continue;
        others.push_back(layer);

        if (layer->IsDirty() || layer->IsAnonymous() || hasMutedSubLayer(*_stage, layer))
            toCopy.insert(layer);

        for (const std::string& assetPath : layer->GetCompositionAssetDependencies()) {
            if (isExpression(assetPath))
                continue;
            const std::string anchored = SdfComputeAssetPathRelativeToLayer(layer, assetPath);
            if (SdfLayerHandle dependency = SdfLayer::Find(anchored)) {
                if (!stackLayers.count(dependency))
                    dependencies[layer].push_back(dependency);
            }
        }
    }

    // A layer that has a copied dependency needs a copy too, pointing at the copy.
    for (bool changed = true; changed;) {
        changed = false;
        for (const SdfLayerHandle& layer : others) {
            if (toCopy.count(layer))
                continue;
            for (const SdfLayerHandle& dependency : dependencies[layer]) {
                if (toCopy.count(dependency)) {
                    toCopy.insert(layer);
                    changed = true;
                    break;
                }
            }
        }
    }

    for (const SdfLayerHandle& layer : others) {
        if (!toCopy.count(layer))
            continue;

        std::string extension = TfGetExtension(layer->GetRealPath());
        if (extension != "usd" && extension != "usda" && extension != "usdc")
            extension = _fileFormat;

        const std::string path = makeFilePath(getLayerBaseName(layer), extension);
        _dependencyCopies[layer] = path;
        _dependencyCopiesByIdentifier[layer->GetIdentifier()] = path;
        if (!layer->GetRealPath().empty())
            _dependencyCopiesByIdentifier[layer->GetRealPath()] = path;
    }
}

void StageEditsExporter::writeDependencyCopies()
{
    for (const auto& entry : _dependencyCopies) {
        const SdfLayerHandle& layer = entry.first;

        SdfLayerRefPtr copy = SdfLayer::CreateAnonymous(getLayerBaseName(layer));
        copy->TransferContent(layer);
        removeMutedSubLayers(*_stage, layer, copy);
        remapAssetPaths(copy, layer, /*rewriteDependencies*/ true);
        writeLayer(copy, entry.second);

        if (!layer->IsAnonymous()) {
            _result.copiedLayers.push_back(layer->GetIdentifier());
            warn(TfStringPrintf(
                "The layer '%s' was written in full, because it is not part of the layers of "
                "the stage itself (e.g. it is referenced) and it has unsaved changes, or leads "
                "to such a layer.",
                layer->GetIdentifier().c_str()));
        }
    }
}

template <class ArcType>
void repointArcs(
    const SdfLayerHandle&                     anchorLayer,
    const SdfListOp<ArcType>&                 listOp,
    const std::map<std::string, std::string>& copies,
    const std::string&                        destFolder,
    const SdfLayerOffset*                     newOffset, // null: only delete
    double                                    tcpsRatio,
    std::vector<ArcType>&                     deleted,
    std::vector<ArcType>&                     added)
{
    std::vector<ArcType> items;
    appendListOpItems(listOp, items);

    for (const ArcType& item : items) {
        if (item.GetAssetPath().empty())
            continue;
        const std::string anchored
            = SdfComputeAssetPathRelativeToLayer(anchorLayer, item.GetAssetPath());
        const auto copy = copies.find(anchored);
        if (copy == copies.end())
            continue;

        // Pcp anchors the items of a delete to the layer that authors it, like the
        // other items: delete the anchored path so that it matches the original arc.
        ArcType deletedItem = item;
        deletedItem.SetAssetPath(anchored);
        if (std::find(deleted.begin(), deleted.end(), deletedItem) == deleted.end())
            deleted.push_back(deletedItem);

        if (newOffset) {
            // The arc moves from its layer to the repointing layer: give it the
            // offset it had in the layer stack. See PcpPrimIndex: the offset of the
            // layer is applied first, then the TCPS of the layer that has the arc.
            const SdfLayerOffset composed = (*newOffset) * item.GetLayerOffset();
            ArcType              repointed = item;
            repointed.SetAssetPath(relativePathTo(copy->second, destFolder));
            repointed.SetLayerOffset(
                SdfLayerOffset(composed.GetOffset(), composed.GetScale() * tcpsRatio));
            if (std::find(added.begin(), added.end(), repointed) == added.end())
                added.push_back(repointed);
        }
    }
}

// The layers that the wrapper brings in from disk may have references or payloads
// to layers that are copied. Since they can't be changed, a layer above them deletes
// those arcs and adds arcs to the copies.
SdfLayerRefPtr StageEditsExporter::buildRepointLayer()
{
    if (_dependencyCopies.empty())
        return {};

    struct Arcs
    {
        std::vector<SdfReference> deletedRefs, addedRefs;
        std::vector<SdfPayload>   deletedPayloads, addedPayloads;
    };
    std::map<SdfPath, Arcs> arcsByPath;

    for (const LayerOnDisk& onDisk : _layersOnDisk) {
        const double tcpsRatio = onDisk.layer->GetTimeCodesPerSecond() / _stackTcps;

        std::vector<SdfLayerHandle> versions { onDisk.layer };
        if (onDisk.savedLayer && onDisk.savedLayer != onDisk.layer)
            versions.push_back(onDisk.savedLayer);

        for (const SdfLayerHandle& version : versions) {
            // The arcs of the layer in memory are the ones to keep. The arcs of the
            // file on disk only need to be deleted.
            const bool            isLive = (version == onDisk.layer);
            const SdfLayerOffset* newOffset = isLive ? &onDisk.offset : nullptr;

            for (const SdfPath& path : getSpecPaths(version)) {
                if (version->GetSpecType(path) != SdfSpecTypePrim)
                    continue;

                VtValue value;
                if (version->HasField(path, SdfFieldKeys->References, &value)
                    && value.IsHolding<SdfReferenceListOp>()) {
                    Arcs& arcs = arcsByPath[path];
                    repointArcs(
                        onDisk.layer,
                        value.UncheckedGet<SdfReferenceListOp>(),
                        _dependencyCopiesByIdentifier,
                        _destFolder,
                        newOffset,
                        tcpsRatio,
                        arcs.deletedRefs,
                        arcs.addedRefs);
                }
                if (version->HasField(path, SdfFieldKeys->Payload, &value)
                    && value.IsHolding<SdfPayloadListOp>()) {
                    Arcs& arcs = arcsByPath[path];
                    repointArcs(
                        onDisk.layer,
                        value.UncheckedGet<SdfPayloadListOp>(),
                        _dependencyCopiesByIdentifier,
                        _destFolder,
                        newOffset,
                        tcpsRatio,
                        arcs.deletedPayloads,
                        arcs.addedPayloads);
                }
            }
        }
    }

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous("repointedArcs");
    layer->SetTimeCodesPerSecond(_stackTcps);

    for (const auto& entry : arcsByPath) {
        const SdfPath& path = entry.first;
        const Arcs&    arcs = entry.second;
        if (arcs.deletedRefs.empty() && arcs.deletedPayloads.empty())
            continue;

        if (path.ContainsPrimVariantSelection()) {
            warn(TfStringPrintf(
                "The references and payloads in the variant '%s' still point to the original "
                "files, without their unsaved changes.",
                path.GetText()));
            continue;
        }

        SdfPrimSpecHandle prim = SdfCreatePrimInLayer(layer, path);
        if (!prim)
            continue;

        if (!arcs.deletedRefs.empty()) {
            SdfReferenceListOp listOp;
            listOp.SetDeletedItems(arcs.deletedRefs);
            listOp.SetPrependedItems(arcs.addedRefs);
            layer->SetField(path, SdfFieldKeys->References, VtValue(listOp));
        }
        if (!arcs.deletedPayloads.empty()) {
            SdfPayloadListOp listOp;
            listOp.SetDeletedItems(arcs.deletedPayloads);
            listOp.SetPrependedItems(arcs.addedPayloads);
            layer->SetField(path, SdfFieldKeys->Payload, VtValue(listOp));
        }
    }

    if (layer->GetRootPrims().empty())
        return {};

    return layer;
}

bool StageEditsExporter::writeWrapper(const std::vector<WrapperEntry>& entries)
{
    SdfLayerRefPtr wrapper = SdfLayer::CreateAnonymous("stage");

    // The metadata of the root layer is the metadata of the stage. The session layer
    // can override it.
    const SdfLayerHandle root = _stage->GetRootLayer();
    UsdUtilsCopyLayerMetadata(root, wrapper, /*skipSublayers*/ true);
    if (const SdfLayerHandle session = _stage->GetSessionLayer())
        UsdUtilsCopyLayerMetadata(session, wrapper, /*skipSublayers*/ true);

    if (root->HasDefaultPrim())
        wrapper->SetDefaultPrim(root->GetDefaultPrim());
    else
        wrapper->ClearDefaultPrim();

    // Don't export the bookkeeping of the unshared stages of Maya-USD.
    VtDictionary customData = wrapper->GetCustomLayerData();
    if (customData.erase(MayaUsdMetadata->ReferencedLayers.GetString()) > 0) {
        if (customData.empty())
            wrapper->ClearCustomLayerData();
        else
            wrapper->SetCustomLayerData(customData);
    }

    wrapper->SetTimeCodesPerSecond(_stackTcps);
    if (_stage->HasAuthoredMetadata(SdfFieldKeys->FramesPerSecond))
        wrapper->SetFramesPerSecond(_stage->GetFramesPerSecond());

    const VtDictionary& variables = _layerStack->GetExpressionVariables().GetVariables();
    if (!variables.empty())
        wrapper->SetExpressionVariables(variables);

    // Give each layer the offset it has in the layer stack of the stage. USD scales
    // the offset of a sublayer by the ratio of the TCPS of its parent and its own,
    // so remove that ratio from the authored offset.
    std::vector<std::string> paths;
    for (const WrapperEntry& entry : entries)
        paths.push_back(entry.authoredPath);
    wrapper->SetSubLayerPaths(paths);

    for (size_t i = 0; i < entries.size(); ++i) {
        const WrapperEntry& entry = entries[i];
        wrapper->SetSubLayerOffset(
            SdfLayerOffset(
                entry.offset.GetOffset(),
                entry.offset.GetScale() * entry.timeCodesPerSecond / _stackTcps),
            static_cast<int>(i));
    }

    const std::string path = TfStringCatPaths(_destFolder, "stage.usda");
    if (!writeLayer(wrapper, path))
        return false;

    _result.referencedLayer = path;
    return true;
}

void StageEditsExporter::run()
{
    _layerStack = _stage->GetPseudoRoot().GetPrimIndex().GetRootNode().GetLayerStack();
    if (!_layerStack) {
        warn("Could not access the layers of the stage.");
        return;
    }
    _stackTcps = _layerStack->GetTimeCodesPerSecond();

    // Reserve the name of the wrapper.
    _usedFileNames.insert("stage");

    findDependenciesToCopy();

    std::vector<WrapperEntry> entries;

    if (SdfLayerRefPtr session = buildSessionLayer()) {
        const std::string path = makeFilePath("session", _fileFormat);
        if (writeLayer(session, path)) {
            entries.push_back({ relativePathTo(path, _destFolder),
                                _layerStack->GetSessionLayerTree()->GetOffset(),
                                session->GetTimeCodesPerSecond() });
        }
    }

    const size_t repointIndex = entries.size();

    if (const SdfLayerTreeHandle& tree = _layerStack->GetLayerTree())
        emitTree(tree, entries);

    if (SdfLayerRefPtr repoint = buildRepointLayer()) {
        const std::string path = makeFilePath("repointedArcs", _fileFormat);
        if (writeLayer(repoint, path)) {
            entries.insert(
                entries.begin() + repointIndex,
                { relativePathTo(path, _destFolder), SdfLayerOffset(), _stackTcps });
        }
    }

    writeDependencyCopies();

    // When nothing was written, the stage can be referenced as before.
    if (_result.writtenFiles.empty())
        return;

    writeWrapper(entries);
}

} // namespace

std::string getStageEditsFolder(const std::string& exportFilePath)
{
    const std::string baseName = TfStringGetBeforeSuffix(TfGetBaseName(exportFilePath));
    return TfStringCatPaths(TfGetPathName(exportFilePath), baseName + "_edits");
}

bool stageHasUnsavedEdits(const UsdStageRefPtr& stage)
{
    if (!stage)
        return false;

    const SdfLayerHandle root = stage->GetRootLayer();
    if (!root || root->IsAnonymous())
        return true;

    const SdfLayerHandle session = stage->GetSessionLayer();
    for (const SdfLayerHandle& layer : stage->GetUsedLayers(/*includeClipLayers*/ false)) {
        if (!layer || layer == session)
            continue;
        if (layer->IsAnonymous() ? layerHasContent(layer) : layer->IsDirty())
            return true;
        if (hasMutedSubLayer(*stage, layer))
            return true;
    }

    if (session && !session->GetRootPrims().empty()
        && !stage->IsLayerMuted(session->GetIdentifier())) {
        // Ignore the opinions Maya-USD authors on prims edited as Maya.
        SdfLayerRefPtr copy = SdfLayer::CreateAnonymous("session");
        copy->TransferContent(session);
        removeMayaUsdBookkeeping(copy);
        if (!copy->GetRootPrims().empty())
            return true;
    }

    return false;
}

ExportedStageEdits exportStageEdits(
    const UsdStageRefPtr& stage,
    const std::string&    destFolder,
    const std::string&    fileFormat,
    bool                  relativeOriginalPaths)
{
    ExportedStageEdits result;
    if (!stage)
        return result;

    StageEditsExporter exporter(stage, destFolder, fileFormat, relativeOriginalPaths, result);
    exporter.run();
    return result;
}

} // namespace MAYAUSD_NS_DEF
