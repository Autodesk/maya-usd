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

#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaPrimUpdater
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaPrimUpdater(const MFnDependencyNode& depNodeFn, const SdfPath& usdPath);

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
        All = Push | Pull | Clear
    };

    MAYAUSD_CORE_PUBLIC
    virtual bool Push(UsdMayaPrimUpdaterContext* context);

    MAYAUSD_CORE_PUBLIC
    virtual bool Pull(UsdMayaPrimUpdaterContext* context);

    MAYAUSD_CORE_PUBLIC
    virtual void Clear(UsdMayaPrimUpdaterContext* context);

    /// The source Maya DAG path that we are consuming.
    ///
    /// If this prim updater is for a Maya DG node and not a DAG node, this will
    /// return an invalid MDagPath.
    MAYAUSD_CORE_PUBLIC
    const MDagPath& GetDagPath() const;

    /// The MObject for the Maya node being updated by this updater.
    MAYAUSD_CORE_PUBLIC
    const MObject& GetMayaObject() const;

    /// The path of the destination USD prim which we are updating.
    MAYAUSD_CORE_PUBLIC
    const SdfPath& GetUsdPath() const;

    /// The destination USD prim which we are updating.
    template <typename T> UsdPrim GetUsdPrim(UsdMayaPrimUpdaterContext& context) const
    {
        UsdPrim usdPrim;

        if (!TF_VERIFY(GetDagPath().isValid())) {
            return usdPrim;
        }

        T primSchema = T::Define(context.GetUsdStage(), GetUsdPath());
        if (!TF_VERIFY(
                primSchema,
                "Could not define given updater type at path '%s'\n",
                GetUsdPath().GetText())) {
            return usdPrim;
        }
        usdPrim = primSchema.GetPrim();
        if (!TF_VERIFY(
                usdPrim,
                "Could not get UsdPrim for given updater type at path '%s'\n",
                primSchema.GetPath().GetText())) {
            return usdPrim;
        }

        return usdPrim;
    }

private:
    /// The MDagPath for the Maya node being updated, valid only for DAG node
    /// prim updaters.
    const MDagPath _dagPath;

    /// The MObject for the Maya node being updated, valid for both DAG and DG
    /// node prim updaters.
    const MObject _mayaObject;

    const SdfPath                           _usdPath;
    const UsdMayaUtil::MDagPathMap<SdfPath> _baseDagToUsdPaths;
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
