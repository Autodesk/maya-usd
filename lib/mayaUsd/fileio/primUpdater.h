//
// Copyright 2018 Pixar
// Copyright 2019 Autodesk
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
#ifndef PXRUSDMAYA_MAYAPRIMUPDATER_H
#define PXRUSDMAYA_MAYAPRIMUPDATER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primUpdaterContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>

#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <ufe/path.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaPrimUpdater
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaPrimUpdater(const MFnDependencyNode& depNodeFn, const Ufe::Path& path);

    // clang errors if you use "= default" here, due to const SdfPath member
    //    see: http://open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#253
    // ...which it seems clang only implements if using newer version + cpp std
    UsdMayaPrimUpdater() { }

    virtual ~UsdMayaPrimUpdater() = default;

    enum class Supports
    {
        Invalid = 0,
        Push = 1 << 0,
        Pull = 1 << 1,
        Clear = 1 << 2,
        AutoPull = 1 << 3,
        All = Push | Pull | Clear
    };

    // Copy the pushed prim from the temporary srcLayer where it has been
    // exported by push into the destination dstLayer which is in the scene.
    MAYAUSD_CORE_PUBLIC
    virtual bool pushCopySpecs(
        SdfLayerRefPtr srcLayer,
        const SdfPath& srcSdfPath,
        SdfLayerRefPtr dstLayer,
        const SdfPath& dstSdfPath);

    /// Customize the pulled prim after pull import.  Default implementation in
    /// this class is a no-op.
    MAYAUSD_CORE_PUBLIC
    virtual bool pull(const UsdMayaPrimUpdaterContext& context);

    /// Discard edits done in Maya.  Implementation in this class removes the
    /// Maya node.
    MAYAUSD_CORE_PUBLIC
    virtual bool discardEdits(const UsdMayaPrimUpdaterContext& context);

    /// Clean up Maya data model at end of push.  Implementation in this class
    /// calls discardEdits().
    MAYAUSD_CORE_PUBLIC
    virtual bool pushEnd(const UsdMayaPrimUpdaterContext& context);

    /// The MObject for the Maya node being updated by this updater.
    MAYAUSD_CORE_PUBLIC
    const MObject& getMayaObject() const;

    /// The path of the destination USD prim which we are updating.
    MAYAUSD_CORE_PUBLIC
    const Ufe::Path& getUfePath() const;

    /// The destination USD prim which we are updating.
    MAYAUSD_CORE_PUBLIC
    UsdPrim getUsdPrim(const UsdMayaPrimUpdaterContext& context) const;

    MAYAUSD_CORE_PUBLIC
    static bool isAnimated(const MDagPath& path);

private:
    /// The MObject for the Maya node being updated, valid for both DAG and DG
    /// node prim updaters.
    const MObject _mayaObject;

    /// The proxy shape and destination sdf path if provided
    const Ufe::Path _path;
};

using UsdMayaPrimUpdaterSharedPtr = std::shared_ptr<UsdMayaPrimUpdater>;

inline UsdMayaPrimUpdater::Supports
operator|(UsdMayaPrimUpdater::Supports a, UsdMayaPrimUpdater::Supports b)
{
    return static_cast<UsdMayaPrimUpdater::Supports>(static_cast<int>(a) | static_cast<int>(b));
}

inline UsdMayaPrimUpdater::Supports
operator&(UsdMayaPrimUpdater::Supports a, UsdMayaPrimUpdater::Supports b)
{
    return static_cast<UsdMayaPrimUpdater::Supports>(static_cast<int>(a) & static_cast<int>(b));
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
