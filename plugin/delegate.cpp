#include "delegate.h"

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/range3d.h>

#include <pxr/usd/usdGeom/tokens.h>

#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hd/camera.h>

#include <pxr/imaging/hdx/tokens.h>
#include <pxr/imaging/hdx/renderSetupTask.h>
#include <pxr/imaging/hdx/renderTask.h>

#include <usdMaya/util.h>

#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <maya/MString.h>

#include "utils.h"
#include "adapterRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

HdMayaDelegate::HdMayaDelegate(
    HdRenderIndex* renderIndex,
    const SdfPath& delegateID) :
    HdSceneDelegate(renderIndex, delegateID) {
    // Unique ID
}

HdMayaDelegate::~HdMayaDelegate() {
    for (auto callback: _callbacks) {
        MMessage::removeCallback(callback);
    }
}

HdMeshTopology
HdMayaDelegate::GetMeshTopology(const SdfPath& id) {
    HdMayaDagAdapterPtr adapter;
    if (!TfMapLookup(_pathToAdapterMap, id, &adapter) || adapter == nullptr) {
        return {};
    }
    return adapter->GetMeshTopology();
}

GfRange3d
HdMayaDelegate::GetExtent(const SdfPath& id) {
    HdMayaDagAdapterPtr adapter;
    if (!TfMapLookup(_pathToAdapterMap, id, &adapter) || adapter == nullptr) {
        return {};
    }
    return adapter->GetExtent();
}

GfMatrix4d
HdMayaDelegate::GetTransform(const SdfPath& id) {
    HdMayaDagAdapterPtr adapter;
    if (!TfMapLookup(_pathToAdapterMap, id, &adapter) || adapter == nullptr) {
        return GfMatrix4d(1.0);
    }
    return adapter->GetTransform();
}

bool
HdMayaDelegate::IsEnabled(const TfToken& option) const {
    // Maya scene can't be accessed on multiple threads,
    // so I don't think this is safe to enable.
    if (option == HdOptionTokens->parallelRprimSync) {
        return false;
    }

    std::cerr << "Checking for option: " << option << std::endl;

    return false;
}

VtValue
HdMayaDelegate::Get(SdfPath const& id, const TfToken& key) {
    HdMayaDagAdapterPtr adapter;
    if (!TfMapLookup(_pathToAdapterMap, id, &adapter) || adapter == nullptr) {
        return {};
    }
    return adapter->Get(key);
}

void
HdMayaDelegate::Populate() {
    auto& renderIndex = GetRenderIndex();
    auto& changeTracker = renderIndex.GetChangeTracker();
    for (MItDag dagIt(MItDag::kDepthFirst, MFn::kInvalid); !dagIt.isDone(); dagIt.next()) {
        MDagPath path;
        dagIt.getPath(path);

        // We don't care about transforms for now.
        if (path.hasFn(MFn::kTransform)) { continue; }

        auto adapterCreator = HdMayaAdapterRegistry::GetAdapterCreator(path);
        if (adapterCreator == nullptr) { continue; }
        const auto id = GetPrimPath(path);
        if (id.IsEmpty()) { continue; }
        if (TfMapLookupPtr(_pathToAdapterMap, id) != nullptr) { continue; }
        auto adapter = adapterCreator(id, this, path);
        if (adapter == nullptr) { continue; }
        adapter->Populate(renderIndex, this, id);
        adapter->CreateCallbacks(_callbacks);
        _pathToAdapterMap.insert({id, adapter});
        changeTracker.RprimInserted(id, HdChangeTracker::AllDirty);
    }
}

SdfPath
HdMayaDelegate::GetPrimPath(const MDagPath& dg) {
    const auto mayaPath = PxrUsdMayaUtil::MDagPathToUsdPath(dg, false, false);
    if (mayaPath.IsEmpty()) { return {}; }
    const auto* chr = mayaPath.GetText();
    if (chr == nullptr) { return {}; };
    std::string s(chr + 1);
    if (s.empty()) { return {}; }
    return GetDelegateID().AppendPath(SdfPath(s));
}

bool
HdMayaDelegate::GetVisible(const SdfPath& id) {
    return true;
}

bool
HdMayaDelegate::GetDoubleSided(const SdfPath& id) {
    std::cerr << "[HdSceneDelegate] Getting double sidedness of " << id << std::endl;
    return true;
}

HdCullStyle
HdMayaDelegate::GetCullStyle(const SdfPath& id) {
    std::cerr << "[HdSceneDelegate] Getting cull style of " << id << std::endl;
    return HdCullStyleDontCare;
}

VtValue
HdMayaDelegate::GetShadingStyle(const SdfPath& id) {
    std::cerr << "[HdSceneDelegate] Getting shading style of " << id << std::endl;
    return HdSceneDelegate::GetShadingStyle(id);
}

HdDisplayStyle
HdMayaDelegate::GetDisplayStyle(const SdfPath& id) {
    std::cerr << "[HdSceneDelegate] Getting display style of " << id << std::endl;
    HdDisplayStyle style;
    style.flatShadingEnabled = true;
    style.displacementEnabled = false;
    return style;
}

TfToken
HdMayaDelegate::GetReprName(const SdfPath& id) {
    std::cerr << "[HdSceneDelegate] Getting repr name of " << id << std::endl;
    return {};
}

SdfPath
HdMayaDelegate::GetMaterialId(const SdfPath& id) {
    std::cerr << "[HdSceneDelegate] Getting material id of " << id << std::endl;
    return {};
}

std::string
HdMayaDelegate::GetSurfaceShaderSource(const SdfPath& id) {
    std::cerr << "[HdSceneDelegate] Getting surface shader source of " << id << std::endl;
    return {};
}

std::string
HdMayaDelegate::GetDisplacementShaderSource(const SdfPath& id) {
    std::cerr << "[HdSceneDelegate] Getting displacement shader source of " << id << std::endl;
    return {};
}

VtValue
HdMayaDelegate::GetMaterialParamValue(const SdfPath& id, const TfToken& paramName) {
    std::cerr << "[HdSceneDelegate] Getting material param value of " << id << std::endl;
    return {};
}

HdMaterialParamVector
HdMayaDelegate::GetMaterialParams(const SdfPath& id) {
    std::cerr << "[HdSceneDelegate] Getting material params of " << id << std::endl;
    return {};
}

VtValue
HdMayaDelegate::GetMaterialResource(const SdfPath& id) {
    std::cerr << "[HdSceneDelegate] Getting material resource of " << id << std::endl;
    return {};
}

TfTokenVector
HdMayaDelegate::GetMaterialPrimvars(const SdfPath& id) {
    std::cerr << "[HdSceneDelegate] Getting material primvars of " << id << std::endl;
    return {};
}

PXR_NAMESPACE_CLOSE_SCOPE
