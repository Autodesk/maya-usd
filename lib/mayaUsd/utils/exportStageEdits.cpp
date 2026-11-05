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
#include <pxr/usd/pcp/cache.h>
#include <pxr/usd/pcp/expressionVariables.h>
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/changeBlock.h>
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
#if PXR_VERSION >= 2308
#include <pxr/usd/sdf/variableExpression.h>
#endif
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/flattenUtils.h>
#include <pxr/usd/usd/object.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/resolveInfo.h>
#include <pxr/usd/usdUtils/authoring.h>

#include <algorithm>
#include <map>
#include <memory>
#include <set>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

namespace {

// Opinions that Maya-USD authors in the session layer for its own needs. They only
// make sense in the Maya scene, so they are not exported:
//   - on prims edited as Maya: the pull information, and the deactivation of the prim
//     (so that only the Maya data shows), see fileio/pullInformation.cpp,
//   - on MayaReference prims: the name of the Maya reference node, see
//     fileio/translators/translatorMayaReference.cpp,
//   - the text color of the prims in the Outliner, see displayCustomControl.py.
const char* const kPullPrimMetadataKey = "Maya:Pull:DagPath";
const TfToken     kMayaReferenceNodeName("MayaReferenceNodeName");
const char* const kOutlinerColorKeys[] = { "Autodesk:Use Text Color", "Autodesk:Text Color" };

const TfToken kSplineField("spline");

const SdfSchema& schema() { return SdfSchema::GetInstance(); }

// The fields holding the value of an attribute. The value of an attribute comes from
// the strongest layer that has any of them, so they are exported together.
bool isValueField(const TfToken& field)
{
    return field == SdfFieldKeys->Default || field == SdfFieldKeys->TimeSamples
        || field == kSplineField;
}

bool isRequiredField(SdfSpecType specType, const TfToken& field)
{
    const TfTokenVector& required = schema().GetRequiredFields(specType);
    return std::find(required.begin(), required.end(), field) != required.end();
}

bool isDefining(const SdfLayerHandle& layer, const SdfPath& path)
{
    return layer->GetFieldAs<SdfSpecifier>(path, SdfFieldKeys->Specifier, SdfSpecifierOver)
        != SdfSpecifierOver;
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

std::vector<SdfPath> getSpecPaths(const SdfLayerHandle& layer)
{
    std::vector<SdfPath> paths;
    layer->Traverse(SdfPath::AbsoluteRootPath(), [&paths](const SdfPath& path) {
        if (path != SdfPath::AbsoluteRootPath())
            paths.push_back(path);
    });
    return paths;
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

// Removes the Maya-USD bookkeeping (see above) from a copy of the session layer.
void removeMayaUsdBookkeeping(const SdfLayerHandle& layer)
{
    SdfChangeBlock changeBlock;

    for (const SdfPath& path : getSpecPaths(layer)) {
        if (layer->GetSpecType(path) != SdfSpecTypePrim)
            continue;

        const SdfPrimSpecHandle prim = layer->GetPrimAtPath(path);
        if (SdfPropertySpecHandle property
            = layer->GetPropertyAtPath(path.AppendProperty(kMayaReferenceNodeName)))
            prim->RemoveProperty(property);

        VtValue customData;
        if (!layer->HasField(path, SdfFieldKeys->CustomData, &customData)
            || !customData.IsHolding<VtDictionary>())
            continue;

        VtDictionary dict = customData.UncheckedGet<VtDictionary>();
        if (dict.GetValueAtPath(kPullPrimMetadataKey))
            layer->EraseField(path, SdfFieldKeys->Active);

        removeValueAtPath(dict, kPullPrimMetadataKey);
        for (const char* key : kOutlinerColorKeys)
            removeValueAtPath(dict, key);

        if (dict.empty())
            layer->EraseField(path, SdfFieldKeys->CustomData);
        else
            layer->SetField(path, SdfFieldKeys->CustomData, VtValue(dict));
    }
}

// Replaces the asset paths of a value read from the composed stage by their resolved
// paths: the authored paths are relative to the layer they come from.
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

template <class T> T anchorItem(const T& item, const SdfLayerHandle&) { return item; }

template <class ArcType> ArcType anchorArc(const ArcType& arc, const SdfLayerHandle& layer)
{
    ArcType anchored = arc;
    if (!arc.GetAssetPath().empty() && !isExpression(arc.GetAssetPath()))
        anchored.SetAssetPath(SdfComputeAssetPathRelativeToLayer(layer, arc.GetAssetPath()));
    return anchored;
}

SdfReference anchorItem(const SdfReference& item, const SdfLayerHandle& layer)
{
    return anchorArc(item, layer);
}

SdfPayload anchorItem(const SdfPayload& item, const SdfLayerHandle& layer)
{
    return anchorArc(item, layer);
}

// Adds to the deleted items of the list op in deleting the items that the list op in
// value adds, with their asset paths anchored to its layer: Pcp anchors the deleted
// items to the layer that authors the delete.
template <class T>
bool addDeletedItems(const VtValue& value, const SdfLayerHandle& layer, VtValue& deleting)
{
    if (!value.IsHolding<SdfListOp<T>>())
        return false;

    SdfListOp<T> result = deleting.IsHolding<SdfListOp<T>>() ? deleting.UncheckedGet<SdfListOp<T>>()
                                                             : SdfListOp<T>();
    std::vector<T> deleted = result.GetDeletedItems();

    const SdfListOp<T>& listOp = value.UncheckedGet<SdfListOp<T>>();
    for (const auto* items : { &listOp.GetExplicitItems(),
                               &listOp.GetAddedItems(),
                               &listOp.GetPrependedItems(),
                               &listOp.GetAppendedItems() }) {
        for (const T& item : *items) {
            const T anchored = anchorItem(item, layer);
            if (std::find(deleted.begin(), deleted.end(), anchored) == deleted.end())
                deleted.push_back(anchored);
        }
    }

    result.SetDeletedItems(deleted);
    deleting = VtValue(result);
    return true;
}

void addDeletedItems(const VtValue& value, const SdfLayerHandle& layer, VtValue& deleting)
{
    addDeletedItems<SdfPath>(value, layer, deleting)
        || addDeletedItems<SdfReference>(value, layer, deleting)
        || addDeletedItems<SdfPayload>(value, layer, deleting)
        || addDeletedItems<TfToken>(value, layer, deleting)
        || addDeletedItems<std::string>(value, layer, deleting);
}

// A layer that the exported files bring in from disk, with the content of its file.
struct LayerOnDisk
{
    SdfLayerHandle layer;
    SdfLayerHandle content;
    SdfLayerOffset offset;
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

    bool           computeLayerStack();
    SdfLayerHandle getFileContent(const SdfLayerHandle& layer);
    void           collectTopLayers(const SdfLayerTreeHandle& tree, std::vector<LayerOnDisk>& tops);
    void           collectLayersOnDisk(
                  const SdfLayerHandle&     layer,
                  const SdfLayerOffset&     offset,
                  std::set<SdfLayerHandle>& seen);
    void addAllFields(const SdfLayerHandle& layer);
    void addChangedFields(const SdfLayerHandle& layer, const SdfLayerHandle& saved);
    void findChangedFields(const std::vector<LayerOnDisk>& tops);
    void warnAboutOtherLayers();

    std::string    resolveAssetPath(const SdfLayerHandle& layer, const std::string& assetPath);
    SdfLayerRefPtr buildEdits(const SdfLayerHandle& flat);
    bool ensureSpec(const SdfLayerHandle& edits, const SdfLayerHandle& flat, const SdfPath& path);
    SdfSpecHandle findSpecOnDisk(const SdfPath& path) const;
    void          exportMissingValue(const SdfLayerHandle& edits, const SdfPath& path);
    void exportMissingField(const SdfLayerHandle& edits, const SdfPath& path, const TfToken& field);
    template <class ArcType>
    void fixArcTimeScale(VtValue& value, const SdfPath& path, const TfToken& field);
    void deactivateIfNotDefined(
        const SdfLayerHandle& edits,
        const SdfLayerHandle& flat,
        const SdfPath&        path);

    bool writeLayer(const SdfLayerHandle& layer, const std::string& path);
    bool writeWrapper(const std::string& editsPath, const std::vector<LayerOnDisk>& tops);

    UsdStageRefPtr      _stage;
    std::string         _destFolder;
    std::string         _fileFormat;
    bool                _relativeOriginalPaths;
    ExportedStageEdits& _result;

    // The layer stack of the stage, with a copy of the session layer without the
    // Maya-USD bookkeeping.
    SdfLayerRefPtr            _sessionCopy;
    std::unique_ptr<PcpCache> _cache;
    PcpLayerStackRefPtr       _layerStack;

    // The content on disk of the dirty layers.
    std::map<SdfLayerHandle, SdfLayerRefPtr> _savedLayers;

    // The layers that the original files bring in, from strongest to weakest.
    std::vector<LayerOnDisk>    _layersOnDisk;
    std::vector<SdfLayerRefPtr> _openedLayers;

    // The fields that have in-memory edits.
    std::map<SdfPath, std::set<TfToken>> _changedFields;

    bool _warnedAboutExpressions = false;
    bool _warnedAboutVariants = false;
};

bool StageEditsExporter::computeLayerStack()
{
    const SdfLayerHandle session = _stage->GetSessionLayer();
    if (session) {
        _sessionCopy = SdfLayer::CreateAnonymous("session");
        _sessionCopy->TransferContent(session);
        removeMayaUsdBookkeeping(_sessionCopy);
    }

    const PcpLayerStackIdentifier identifier(
        _stage->GetRootLayer(), _sessionCopy, _stage->GetPathResolverContext());
    _cache = std::make_unique<PcpCache>(identifier, std::string(), /*usd*/ true);

    std::vector<std::string> muted = _stage->GetMutedLayers();
    if (session && _stage->IsLayerMuted(session->GetIdentifier()))
        muted.push_back(_sessionCopy->GetIdentifier());
    _cache->RequestLayerMuting(muted, {});

    PcpErrorVector errors;
    _layerStack = _cache->ComputeLayerStack(identifier, &errors);
    return bool(_layerStack);
}

// Returns the content of the file of the layer: the layer itself if it has no unsaved
// changes, or null if it has no file.
SdfLayerHandle StageEditsExporter::getFileContent(const SdfLayerHandle& layer)
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

// The layers that the wrapper references from disk: the layers of the layer stack
// that have a file, without the layers below them, which their files bring in. The
// other layers (e.g. the session layer) are only in memory: their opinions are edits.
void StageEditsExporter::collectTopLayers(
    const SdfLayerTreeHandle& tree,
    std::vector<LayerOnDisk>& tops)
{
    if (!tree)
        return;

    const SdfLayerHandle& layer = tree->GetLayer();
    if (const SdfLayerHandle content = getFileContent(layer)) {
        tops.push_back({ layer, content, tree->GetOffset() });
        return;
    }

    for (const SdfLayerTreeHandle& child : tree->GetChildTrees())
        collectTopLayers(child, tops);
}

// Collects the layers that the file of a layer brings in, as they are on disk, with
// the offsets USD gives them. See PcpLayerStack: the offset of a sublayer is scaled by
// the ratio of the TCPS of its parent and its own.
void StageEditsExporter::collectLayersOnDisk(
    const SdfLayerHandle&     layer,
    const SdfLayerOffset&     offset,
    std::set<SdfLayerHandle>& seen)
{
    const SdfLayerHandle content = getFileContent(layer);
    if (!content || !seen.insert(layer).second)
        return;
    _layersOnDisk.push_back({ layer, content, offset });

    const std::vector<std::string> paths = content->GetSubLayerPaths();
    const SdfLayerOffsetVector     offsets = content->GetSubLayerOffsets();
    for (size_t i = 0; i < paths.size(); ++i) {
        const SdfLayerRefPtr sublayer
            = SdfLayer::FindOrOpen(SdfComputeAssetPathRelativeToLayer(layer, paths[i]));
        const SdfLayerHandle sublayerContent
            = sublayer ? getFileContent(sublayer) : SdfLayerHandle();
        if (!sublayerContent)
            continue;
        // Keep the layers that the stage doesn't use (e.g. muted) alive.
        _openedLayers.push_back(sublayer);

        const SdfLayerOffset authored = i < offsets.size() ? offsets[i] : SdfLayerOffset();
        const SdfLayerOffset scaled(
            authored.GetOffset(),
            authored.GetScale() * content->GetTimeCodesPerSecond()
                / sublayerContent->GetTimeCodesPerSecond());
        collectLayersOnDisk(sublayer, offset * scaled, seen);
    }
}

// Marks all the fields of the layer as changed.
void StageEditsExporter::addAllFields(const SdfLayerHandle& layer)
{
    for (const SdfPath& path : getSpecPaths(layer)) {
        for (const TfToken& field : layer->ListFields(path)) {
            if (schema().HoldsChildren(field))
                continue;
            // Overs don't change anything by themselves.
            if (field == SdfFieldKeys->Specifier && !isDefining(layer, path))
                continue;
            _changedFields[path].insert(field);
        }
    }
}

// Marks the fields that differ between the layer and its file on disk.
void StageEditsExporter::addChangedFields(const SdfLayerHandle& layer, const SdfLayerHandle& saved)
{
    const auto addDifferences = [this](const SdfLayerHandle& a, const SdfLayerHandle& b) {
        for (const SdfPath& path : getSpecPaths(a)) {
            for (const TfToken& field : a->ListFields(path)) {
                if (schema().HoldsChildren(field))
                    continue;
                VtValue other;
                if (b->HasField(path, field, &other) && other == a->GetField(path, field))
                    continue;
                _changedFields[path].insert(field);
            }
        }
    };
    addDifferences(layer, saved);
    addDifferences(saved, layer);
}

void StageEditsExporter::findChangedFields(const std::vector<LayerOnDisk>& tops)
{
    std::set<SdfLayerHandle> seen;
    for (const LayerOnDisk& top : tops)
        collectLayersOnDisk(top.layer, top.offset, seen);

    // The layers of the layer stack that have a file, in order, and their offsets.
    std::vector<SdfLayerHandle>              liveOrder;
    std::map<SdfLayerHandle, SdfLayerOffset> liveOffsets;
    const SdfLayerRefPtrVector&              layers = _layerStack->GetLayers();
    for (size_t i = 0; i < layers.size(); ++i) {
        const SdfLayerHandle layer = layers[i];
        const SdfLayerHandle content = getFileContent(layer);
        if (!content) {
            // Layers only in memory: all their opinions are edits.
            addAllFields(layer);
            continue;
        }
        liveOrder.push_back(layer);
        // (The offset is null when it is the identity.)
        const SdfLayerOffset* offset = _layerStack->GetLayerOffsetForLayer(i);
        liveOffsets[layer] = offset ? *offset : SdfLayerOffset();
        if (content != layer)
            addChangedFields(layer, content);
    }

    // Compare the layers on disk with the layers of the stage. All the opinions of a
    // layer that is only on disk (removed or muted) are removed by the edits. All the
    // opinions of a layer that was added, or whose offset or order changed, are
    // written with the edits, since its file can't be used where it is.
    std::vector<SdfLayerHandle> diskOrder;
    for (const LayerOnDisk& onDisk : _layersOnDisk) {
        const auto live = liveOffsets.find(onDisk.layer);
        if (live == liveOffsets.end()) {
            addAllFields(onDisk.content);
            continue;
        }
        diskOrder.push_back(onDisk.layer);
        if (!(live->second == onDisk.offset)) {
            addAllFields(onDisk.layer);
            warn(TfStringPrintf(
                "The content of the layer '%s' was written with the edits, because its time "
                "offset changed.",
                onDisk.layer->GetIdentifier().c_str()));
        }
    }

    std::vector<SdfLayerHandle> commonOrder;
    for (const SdfLayerHandle& layer : liveOrder) {
        if (std::find(diskOrder.begin(), diskOrder.end(), layer) != diskOrder.end()) {
            commonOrder.push_back(layer);
            continue;
        }
        addAllFields(layer);
        warn(TfStringPrintf(
            "The content of the layer '%s' was written with the edits, because it was added to "
            "the layers of the stage.",
            layer->GetIdentifier().c_str()));
    }
    for (size_t i = 0; i < diskOrder.size() && i < commonOrder.size(); ++i) {
        if (diskOrder[i] == commonOrder[i])
            continue;
        addAllFields(commonOrder[i]);
        warn(TfStringPrintf(
            "The content of the layer '%s' was written with the edits, because the order of the "
            "layers of the stage changed.",
            commonOrder[i]->GetIdentifier().c_str()));
    }

    // The layer metadata is exported through the wrapper.
    _changedFields.erase(SdfPath::AbsoluteRootPath());
}

// Warns about the unsaved edits outside the layer stack, which are not exported.
void StageEditsExporter::warnAboutOtherLayers()
{
    const SdfLayerRefPtrVector& stackLayers = _layerStack->GetLayers();
    for (const SdfLayerHandle& layer : _stage->GetUsedLayers(/*includeClipLayers*/ false)) {
        if (!layer || layer == _stage->GetSessionLayer()
            || (!layer->IsDirty() && !layer->IsAnonymous()))
            continue;
        if (std::find(stackLayers.begin(), stackLayers.end(), layer) != stackLayers.end())
            continue;
        warn(TfStringPrintf(
            "The unsaved changes of the layer '%s' are not exported, because it is not one of the "
            "layers of the stage (e.g. it is referenced). Save it to include them.",
            layer->GetIdentifier().c_str()));
    }
}

// The edits are written to another folder than the original layers: the paths that
// USD anchors to their layer must point to where they resolved from it. This includes
// search-path style paths (e.g. "textures/a.png"), which USD looks up next to the layer
// first: their anchored path only differs if it exists there. Paths in anonymous
// layers were never anchored, so they stay unchanged.
std::string
StageEditsExporter::resolveAssetPath(const SdfLayerHandle& layer, const std::string& assetPath)
{
    if (assetPath.empty() || layer->IsAnonymous())
        return assetPath;

    if (isExpression(assetPath)) {
        if (!_warnedAboutExpressions) {
            _warnedAboutExpressions = true;
            warn("Asset paths using expressions were written unchanged and may not resolve from "
                 "the exported files.");
        }
        return assetPath;
    }

    const std::string anchored = SdfComputeAssetPathRelativeToLayer(layer, assetPath);
    if (anchored == assetPath)
        return assetPath;
    return _relativeOriginalPaths ? relativePathTo(anchored, _destFolder) : anchored;
}

SdfSpecHandle StageEditsExporter::findSpecOnDisk(const SdfPath& path) const
{
    for (const LayerOnDisk& onDisk : _layersOnDisk) {
        if (SdfSpecHandle spec = onDisk.content->GetObjectAtPath(path))
            return spec;
    }
    return {};
}

bool StageEditsExporter::ensureSpec(
    const SdfLayerHandle& edits,
    const SdfLayerHandle& flat,
    const SdfPath&        path)
{
    if (edits->HasSpec(path))
        return true;

    if (path.IsPrimOrPrimVariantSelectionPath())
        return bool(SdfCreatePrimInLayer(edits, path));

    if (!path.IsPrimPropertyPath())
        return false;

    // Properties need their type: take it from the layers of the stage, or from the
    // files if the layers of the stage don't have the property anymore.
    SdfSpecHandle spec = flat->GetPropertyAtPath(path);
    if (!spec)
        spec = findSpecOnDisk(path);
    const SdfPrimSpecHandle prim
        = spec ? SdfCreatePrimInLayer(edits, path.GetPrimPath()) : SdfPrimSpecHandle();
    if (!prim)
        return false;

    if (const SdfAttributeSpecHandle attr = TfDynamic_cast<SdfAttributeSpecHandle>(spec)) {
        return bool(SdfAttributeSpec::New(
            prim, path.GetName(), attr->GetTypeName(), attr->GetVariability(), attr->IsCustom()));
    }
    if (const SdfRelationshipSpecHandle rel = TfDynamic_cast<SdfRelationshipSpecHandle>(spec)) {
        return bool(
            SdfRelationshipSpec::New(prim, path.GetName(), rel->IsCustom(), rel->GetVariability()));
    }
    return false;
}

// No layer of the stage has a value for the attribute anymore, while the files do:
// author the value of the stage (e.g. from a reference), or block the value.
void StageEditsExporter::exportMissingValue(const SdfLayerHandle& edits, const SdfPath& path)
{
    const UsdAttribute         attr = _stage->GetAttributeAtPath(path);
    const UsdResolveInfoSource source
        = attr ? attr.GetResolveInfo().GetSource() : UsdResolveInfoSourceNone;

    if (source == UsdResolveInfoSourceTimeSamples) {
        std::vector<double> times;
        attr.GetTimeSamples(&times);
        SdfTimeSampleMap samples;
        for (double time : times) {
            VtValue value;
            if (attr.Get(&value, time)) {
                useResolvedAssetPaths(value);
                samples[time] = value;
            }
        }
        edits->SetField(path, SdfFieldKeys->TimeSamples, VtValue(samples));
        return;
    }

    VtValue value;
    if (source != UsdResolveInfoSourceNone && source != UsdResolveInfoSourceFallback
        && attr.Get(&value, UsdTimeCode::Default())) {
        useResolvedAssetPaths(value);
    } else {
        value = SdfValueBlock();
    }
    edits->SetField(path, SdfFieldKeys->Default, value);
}

// No layer of the stage has the field anymore, while the files do.
void StageEditsExporter::exportMissingField(
    const SdfLayerHandle& edits,
    const SdfPath&        path,
    const TfToken&        field)
{
    // Without a specifier or type name, the ones of the files are used: prims that
    // are not defined anymore are handled by deactivateIfNotDefined().
    if (field == SdfFieldKeys->Specifier || field == SdfFieldKeys->TypeName)
        return;

    // List ops: delete the items of the files.
    VtValue deleting;
    for (const LayerOnDisk& onDisk : _layersOnDisk) {
        VtValue value;
        if (onDisk.content->HasField(path, field, &value))
            addDeletedItems(value, onDisk.layer, deleting);
    }
    if (!deleting.IsEmpty()) {
        edits->SetField(path, field, deleting);
        return;
    }

    // Other fields: author the value of the stage (e.g. from a reference), or the
    // fallback value.
    VtValue         value;
    const UsdObject object = _stage->GetObjectAtPath(path);
    if (!(object && object.GetMetadata(field, &value)))
        value = schema().GetFallback(field);

    if (value.IsEmpty()) {
        warn(TfStringPrintf(
            "The removal of '%s' on '%s' could not be exported.", field.GetText(), path.GetText()));
        return;
    }
    edits->SetField(path, field, value);
}

// A prim that the files define but the layers of the stage don't anymore (deleted,
// renamed, or only an over is left) can't be undefined from a stronger layer:
// deactivate it instead.
void StageEditsExporter::deactivateIfNotDefined(
    const SdfLayerHandle& edits,
    const SdfLayerHandle& flat,
    const SdfPath&        path)
{
    if (isDefining(flat, path))
        return;

    const bool definedOnDisk = std::any_of(
        _layersOnDisk.begin(), _layersOnDisk.end(), [&path](const LayerOnDisk& onDisk) {
            return isDefining(onDisk.content, path);
        });
    if (!definedOnDisk)
        return;

    if (path.ContainsPrimVariantSelection()) {
        if (!_warnedAboutVariants) {
            _warnedAboutVariants = true;
            warn("Some prims removed inside variants could not be exported.");
        }
        return;
    }

    // The prim may still be defined by other composition arcs. Under an unloaded or
    // inactive prim, the stage can't tell: leave it.
    if (const UsdPrim prim = _stage->GetPrimAtPath(path)) {
        if (prim.IsDefined())
            return;
    } else {
        for (SdfPath parent = path.GetParentPath(); !parent.IsAbsoluteRootPath();
             parent = parent.GetParentPath()) {
            if (const UsdPrim ancestor = _stage->GetPrimAtPath(parent)) {
                if (!ancestor.IsActive() || !ancestor.IsLoaded())
                    return;
                break;
            }
        }
    }

    if (SdfPrimSpecHandle spec = SdfCreatePrimInLayer(edits, path))
        spec->SetActive(false);
}

// USD scales the offset of a reference or payload by the ratio of the TCPS of the layer
// that authors it and of the referenced layer. The flattened layer stack applies the
// offsets of the layers to the arcs, but not this ratio: the arcs now come from the
// edits, which have the TCPS of the stage. Apply the ratio of the TCPS of their
// original layer and of the stage.
template <class ArcType>
void StageEditsExporter::fixArcTimeScale(VtValue& value, const SdfPath& path, const TfToken& field)
{
    if (!value.IsHolding<SdfListOp<ArcType>>())
        return;

    const double stackTcps = _layerStack->GetTimeCodesPerSecond();
    const auto   findScale = [&](const ArcType& arc) {
        for (const SdfLayerRefPtr& layer : _layerStack->GetLayers()) {
            const double tcps = layer->GetTimeCodesPerSecond();
            VtValue      authored;
            if (tcps == stackTcps || !layer->HasField(path, field, &authored)
                || !authored.IsHolding<SdfListOp<ArcType>>())
                continue;
            for (const ArcType& item :
                 authored.UncheckedGet<SdfListOp<ArcType>>().GetAppliedItems()) {
                if (item.GetPrimPath() == arc.GetPrimPath()
                    && resolveAssetPath(layer, item.GetAssetPath()) == arc.GetAssetPath())
                    return tcps / stackTcps;
            }
        }
        return 1.0;
    };
    const auto fixItems = [&](std::vector<ArcType> items) {
        for (ArcType& item : items) {
            const double scale = findScale(item);
            if (scale != 1.0) {
                const SdfLayerOffset& offset = item.GetLayerOffset();
                item.SetLayerOffset(SdfLayerOffset(offset.GetOffset(), offset.GetScale() * scale));
            }
        }
        return items;
    };

    const SdfListOp<ArcType>& listOp = value.UncheckedGet<SdfListOp<ArcType>>();
    SdfListOp<ArcType>        fixed;
    if (listOp.IsExplicit()) {
        fixed = SdfListOp<ArcType>::CreateExplicit(fixItems(listOp.GetExplicitItems()));
    } else {
        fixed = listOp;
        fixed.SetAddedItems(fixItems(listOp.GetAddedItems()));
        fixed.SetPrependedItems(fixItems(listOp.GetPrependedItems()));
        fixed.SetAppendedItems(fixItems(listOp.GetAppendedItems()));
    }
    value = VtValue(fixed);
}

// Writes, for each changed field, the value composed by the layers of the stage.
SdfLayerRefPtr StageEditsExporter::buildEdits(const SdfLayerHandle& flat)
{
    SdfLayerRefPtr edits = SdfLayer::CreateAnonymous("edits");
    // The flattened layer stack is in the time of the stage.
    edits->SetTimeCodesPerSecond(_layerStack->GetTimeCodesPerSecond());

    SdfChangeBlock changeBlock;
    for (const auto& entry : _changedFields) {
        const SdfPath& path = entry.first;
        if (!ensureSpec(edits, flat, path))
            continue;

        const SdfSpecType specType = edits->GetSpecType(path);
        bool              valueDone = false;
        for (const TfToken& field : entry.second) {
            // The required fields of properties (e.g. the type) are set by ensureSpec().
            if (specType != SdfSpecTypePrim && isRequiredField(specType, field))
                continue;

            if (isValueField(field)) {
                if (valueDone)
                    continue;
                valueDone = true;
                bool hasValue = false;
                for (const TfToken& valueField :
                     { SdfFieldKeys->Default, SdfFieldKeys->TimeSamples, kSplineField }) {
                    VtValue value;
                    if (flat->HasField(path, valueField, &value)) {
                        edits->SetField(path, valueField, value);
                        hasValue = true;
                    }
                }
                if (!hasValue && specType == SdfSpecTypeAttribute)
                    exportMissingValue(edits, path);
                continue;
            }

            VtValue value;
            if (flat->HasField(path, field, &value)) {
                fixArcTimeScale<SdfReference>(value, path, field);
                fixArcTimeScale<SdfPayload>(value, path, field);
                edits->SetField(path, field, value);
            } else {
                exportMissingField(edits, path, field);
            }
        }

        if (specType == SdfSpecTypePrim && entry.second.count(SdfFieldKeys->Specifier))
            deactivateIfNotDefined(edits, flat, path);
    }

    return edits;
}

bool StageEditsExporter::writeLayer(const SdfLayerHandle& layer, const std::string& path)
{
    // Never overwrite a layer in use, e.g. a stage opened from a previous export.
    if (SdfLayerHandle openLayer = SdfLayer::Find(path)) {
        const SdfLayerHandleVector used = _stage->GetUsedLayers();
        if (openLayer->IsDirty() || std::find(used.begin(), used.end(), openLayer) != used.end()) {
            warn(TfStringPrintf("Could not write the file '%s': it is in use.", path.c_str()));
            return false;
        }
    }

    if (!TfIsDir(_destFolder) && !TfMakeDirs(_destFolder, -1, true)) {
        warn(TfStringPrintf("Could not create the folder '%s'.", _destFolder.c_str()));
        return false;
    }

    if (!layer->Export(path)) {
        warn(TfStringPrintf("Could not write the file '%s'.", path.c_str()));
        return false;
    }

    // A previous export may have left this file open in memory: make sure the
    // exported stage sees the new content.
    if (SdfLayerHandle openLayer = SdfLayer::Find(path))
        openLayer->Reload(true);
    return true;
}

bool StageEditsExporter::writeWrapper(
    const std::string&              editsPath,
    const std::vector<LayerOnDisk>& tops)
{
    SdfLayerRefPtr wrapper = SdfLayer::CreateAnonymous("stage");

    // The metadata of the root layer is the metadata of the stage. The session layer
    // can override it.
    const SdfLayerHandle root = _stage->GetRootLayer();
    UsdUtilsCopyLayerMetadata(root, wrapper, /*skipSublayers*/ true);
    if (_sessionCopy)
        UsdUtilsCopyLayerMetadata(_sessionCopy, wrapper, /*skipSublayers*/ true);

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

    const double stackTcps = _layerStack->GetTimeCodesPerSecond();
    wrapper->SetTimeCodesPerSecond(stackTcps);
    if (_stage->HasAuthoredMetadata(SdfFieldKeys->FramesPerSecond))
        wrapper->SetFramesPerSecond(_stage->GetFramesPerSecond());

    const VtDictionary& variables = _layerStack->GetExpressionVariables().GetVariables();
    if (!variables.empty())
        wrapper->SetExpressionVariables(variables);

    // The edits are in the time of the stage. Give the files the offsets they have in
    // the layer stack. USD scales the offset of a sublayer by the ratio of the TCPS of
    // its parent and its own, so remove that ratio from the authored offset.
    std::vector<std::string>    paths;
    std::vector<SdfLayerOffset> offsets;
    if (!editsPath.empty()) {
        paths.push_back(relativePathTo(editsPath, _destFolder));
        offsets.push_back(SdfLayerOffset());
    }
    for (const LayerOnDisk& top : tops) {
        paths.push_back(
            _relativeOriginalPaths ? relativePathTo(top.layer->GetRealPath(), _destFolder)
                                   : top.layer->GetIdentifier());
        offsets.push_back(SdfLayerOffset(
            top.offset.GetOffset(),
            top.offset.GetScale() * top.content->GetTimeCodesPerSecond() / stackTcps));
    }
    if (paths.empty())
        return false;

    wrapper->SetSubLayerPaths(paths);
    for (size_t i = 0; i < offsets.size(); ++i)
        wrapper->SetSubLayerOffset(offsets[i], static_cast<int>(i));

    const std::string path = TfStringCatPaths(_destFolder, "stage.usda");
    if (!writeLayer(wrapper, path))
        return false;

    _result.referencedLayer = path;
    return true;
}

void StageEditsExporter::run()
{
    if (!computeLayerStack()) {
        warn("Could not access the layers of the stage.");
        return;
    }

    std::vector<LayerOnDisk> tops;
    collectTopLayers(_layerStack->GetSessionLayerTree(), tops);
    collectTopLayers(_layerStack->GetLayerTree(), tops);

    findChangedFields(tops);
    warnAboutOtherLayers();

    // Without edits, the stage is exported as before. (Unless its root layer is
    // anonymous: it can't be referenced then.)
    if (_changedFields.empty() && !_stage->GetRootLayer()->IsAnonymous())
        return;

    std::string editsPath;
    if (!_changedFields.empty()) {
        const SdfLayerRefPtr flat = UsdFlattenLayerStack(
            _layerStack,
            [this](const SdfLayerHandle& layer, const std::string& assetPath) {
                return resolveAssetPath(layer, assetPath);
            },
            "flattened");
        if (!flat) {
            warn("Could not compose the layers of the stage.");
            return;
        }

        const SdfLayerRefPtr edits = buildEdits(flat);
        if (!edits->GetRootPrims().empty()) {
            editsPath = TfStringCatPaths(_destFolder, "edits." + _fileFormat);
            if (!writeLayer(edits, editsPath))
                return;
        }
    }

    writeWrapper(editsPath, tops);
}

} // namespace

std::string getStageEditsFolder(const std::string& exportFilePath)
{
    const std::string baseName = TfStringGetBeforeSuffix(TfGetBaseName(exportFilePath));
    return TfStringCatPaths(TfGetPathName(exportFilePath), baseName + "_edits");
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
