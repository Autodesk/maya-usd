//
// Copyright 2021 Autodesk
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
#ifndef MAYAUSD_UFE_USD_POINT_INSTANCE_MODIFIER_BASE_H
#define MAYAUSD_UFE_USD_POINT_INSTANCE_MODIFIER_BASE_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/types.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/pointInstancer.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Shared data for USD point instance read and write batching.
//
// This data structure can be used by UsdPointInstanceModifierBase when
// modifying the position, orientation, or scale of multiple point instances
// from a single point instancer.  These changes can be batched such that the
// first modifier to make a change will read from the point instancer attribute
// into the usdValues array, and the last modifier to make a change will write
// the usdValues array back to the point instancer attribute.  All modifiers in
// the batch will write to and read from the usdValues array, a much less
// expensive operation than writing to or reading from the point instancer
// attribute.
//
template <class UsdValueType> struct MAYAUSD_CORE_PUBLIC UsdPointInstanceBatch
{
    inline bool isReader() const { return (count % nbInstances) == 0; }
    inline bool isWriter() const { return ((count + 1) % nbInstances) == 0; }

    PXR_NS::VtArray<UsdValueType> usdValues;
    int                           nbInstances { 0 };
    int                           count { 0 };
};

/// Abstract utility class for accessing and modifying attributes of USD point
/// instances.
///
/// In USD, a PointInstancer prim generates point instances based on data
/// encoded as arrays in its attributes. The instance index of each point
/// instance is used to index into these arrays to access the point instance's
/// data.
///
/// This class provides an interface for getting and setting a point instance's
/// attribute values in either the native USD type or in its equivalent UFE
/// type. Derived classes of this base class must implement the functions to
/// directly get and create the USD attribute on the PointInstancer (typically
/// with a call to UsdGeomPointInstancer::Get...Attr() and
/// UsdGeomPointInstancer::Create...Attr(), respectively), functions to convert
/// values between USD and UFE types, and a function to retrieve the default
/// value in the USD type.
///
template <class UfeValueType, class UsdValueType>
class MAYAUSD_CORE_PUBLIC UsdPointInstanceModifierBase
{
public:
    typedef std::shared_ptr<UsdPointInstanceBatch<UsdValueType>> Batch;
    typedef std::unordered_map<Ufe::Path, Batch>                 Batches;

    UsdPointInstanceModifierBase()
        : _prim()
        , _instanceIndex(-1)
    {
    }

    virtual ~UsdPointInstanceModifierBase() = default;

    bool setSceneItem(const UsdSceneItem::Ptr& sceneItem)
    {
        _prim = PXR_NS::UsdPrim();
        _instanceIndex = -1;
        _path = Ufe::Path();

        auto prim = sceneItem->prim();
        auto instanceIndex = sceneItem->instanceIndex();

        if (!PXR_NS::UsdGeomPointInstancer(prim) || instanceIndex < 0) {
            return false;
        }

        _prim = prim;
        _instanceIndex = instanceIndex;
        _path = sceneItem->path();

        return true;
    }

    PXR_NS::UsdPrim prim() const { return _prim; };

    const Ufe::Path& path() const { return _path; };

    Ufe::Path pointInstancerPath() const { return _path.pop(); };

    virtual Batches& batches() = 0;

    PXR_NS::UsdGeomPointInstancer getPointInstancer() const
    {
        return PXR_NS::UsdGeomPointInstancer(_prim);
    };

    UfeValueType getUfeValue(PXR_NS::UsdTimeCode usdTime = PXR_NS::UsdTimeCode::Default()) const
    {
        const UsdValueType usdValue = getUsdValue(usdTime);
        return convertValueToUfe(usdValue);
    }

    UsdValueType getUsdValue(PXR_NS::UsdTimeCode usdTime = PXR_NS::UsdTimeCode::Default()) const
    {
        UsdValueType usdValue = this->getDefaultUsdValue();

        PXR_NS::UsdGeomPointInstancer pointInstancer = getPointInstancer();
        if (!pointInstancer) {
            return usdValue;
        }

        if (_instanceIndex < 0) {
            return usdValue;
        }

        const size_t instanceIndex = static_cast<size_t>(_instanceIndex);

        PXR_NS::UsdAttribute usdAttr = _getAttribute();
        if (!usdAttr) {
            return usdValue;
        }

        PXR_NS::VtArray<UsdValueType> usdValues;
        if (!usdAttr.Get(&usdValues, usdTime)) {
            return usdValue;
        }

        if (instanceIndex >= usdValues.size()) {
            return usdValue;
        }

        // For USD 21.02+, avoid triggering a copy-on-write by making sure that
        // we invoke operator[] on a const reference to the array.
#if PXR_VERSION >= 2102
        return usdValues.AsConst()[instanceIndex];
#else
        return usdValues[instanceIndex];
#endif
    }

    bool setValue(
        const UfeValueType& ufeValue,
        PXR_NS::UsdTimeCode usdTime = PXR_NS::UsdTimeCode::Default())
    {
        const UsdValueType usdValue = convertValueToUsd(ufeValue);
        return setValue(usdValue, usdTime);
    }

    bool setValue(
        const UsdValueType& usdValue,
        PXR_NS::UsdTimeCode usdTime = PXR_NS::UsdTimeCode::Default())
    {
        PXR_NS::UsdGeomPointInstancer pointInstancer = getPointInstancer();
        if (!pointInstancer) {
            return false;
        }

        if (_instanceIndex < 0) {
            return false;
        }

        const size_t instanceIndex = static_cast<size_t>(_instanceIndex);

        // Once setValue() is called, any point instance batch under
        // construction is closed.  If no batch was created, make a trivial,
        // unshared batch of a single point instance.
        _closeBatch();

        PXR_NS::UsdAttribute usdAttr;

        const bool reader = _batch->isReader();
        const bool writer = _batch->isWriter();
        _batch->count++;

        if (reader || writer) {
            usdAttr = _getOrCreateAttribute();
            if (!usdAttr) {
                return false;
            }
        }

        if (reader) {
            if (!usdAttr.Get(&_batch->usdValues, usdTime)) {
                return false;
            }
        }

        if (instanceIndex >= _batch->usdValues.size()) {
            return false;
        }

        _batch->usdValues[instanceIndex] = usdValue;

        return writer ? usdAttr.Set(_batch->usdValues, usdTime) : true;
    }

    void joinBatch()
    {
        // If we've already joined a point instance batch, nothing to do.
        if (_batch) {
            return;
        }

        const auto instancerPath = pointInstancerPath();
        auto&      b = batches();
        auto       found = b.find(instancerPath);
        // If we're the first to join the batch, create it.
        if (found == b.end()) {
            auto inserted = b.insert(typename Batches::value_type(
                instancerPath, std::make_shared<UsdPointInstanceBatch<UsdValueType>>()));
            found = inserted.first;
        }
        found->second->nbInstances++;

        _batch = found->second;
    }

    virtual UsdValueType convertValueToUsd(const UfeValueType& ufeValue) const = 0;

    virtual UfeValueType convertValueToUfe(const UsdValueType& usdValue) const = 0;

    virtual UsdValueType getDefaultUsdValue() const = 0;

protected:
    virtual PXR_NS::UsdAttribute _getAttribute() const = 0;

    virtual PXR_NS::UsdAttribute _createAttribute() = 0;

    PXR_NS::UsdAttribute _getOrCreateAttribute()
    {
        // XXX: Needed for Tf error message macros.
        PXR_NAMESPACE_USING_DIRECTIVE

        // Get the size of the prototype indices array. If we need to create
        // the values attribute, we'll populate it with that number of elements
        // using the default value. If we are able to get an existing attribute
        // with a value authored, we ensure that its existing values array size
        // matches the prototype indices size.
        PXR_NS::UsdGeomPointInstancer pointInstancer = getPointInstancer();
        if (!pointInstancer) {
            TF_RUNTIME_ERROR("Cannot get PointInstancer");
            return PXR_NS::UsdAttribute();
        }

        const PXR_NS::UsdAttribute protoIndicesAttr = pointInstancer.GetProtoIndicesAttr();
        if (!protoIndicesAttr.HasAuthoredValue()) {
            TF_RUNTIME_ERROR(
                "Cannot create USD attribute for PointInstancer %s with "
                "unauthored prototype indices",
                pointInstancer.GetPath().GetText());
            return PXR_NS::UsdAttribute();
        }

        PXR_NS::VtIntArray protoIndices;
        if (!protoIndicesAttr.Get(&protoIndices)) {
            TF_RUNTIME_ERROR(
                "Cannot get prototype indices for PointInstancer %s",
                pointInstancer.GetPath().GetText());
            return PXR_NS::UsdAttribute();
        }

        if (protoIndices.empty()) {
            TF_RUNTIME_ERROR(
                "Cannot create USD attribute for PointInstancer %s which has "
                "zero instances",
                pointInstancer.GetPath().GetText());
            return PXR_NS::UsdAttribute();
        }

        const size_t numInstances = protoIndices.size();

        PXR_NS::UsdAttribute usdAttr = _getAttribute();
        if (!usdAttr || !usdAttr.HasAuthoredValue()) {
            usdAttr = _createAttribute();
            if (!usdAttr) {
                TF_RUNTIME_ERROR(
                    "Failed to create USD attribute for PointInstancer %s",
                    pointInstancer.GetPath().GetText());
                return PXR_NS::UsdAttribute();
            }

            const UsdValueType            defaultValue = this->getDefaultUsdValue();
            PXR_NS::VtArray<UsdValueType> attrValues(numInstances, defaultValue);
            if (!usdAttr.Set(attrValues)) {
                TF_RUNTIME_ERROR(
                    "Failed to fill USD attribute %s of PointInstancer %s "
                    "with %zu elements of the default value",
                    usdAttr.GetName().GetText(),
                    pointInstancer.GetPath().GetText(),
                    numInstances);
                return PXR_NS::UsdAttribute();
            }
        } else {
            PXR_NS::VtArray<UsdValueType> attrValues;
            if (!usdAttr.Get(&attrValues)) {
                TF_RUNTIME_ERROR(
                    "Failed to get values for USD attribute %s of "
                    "PointInstancer %s",
                    usdAttr.GetName().GetText(),
                    pointInstancer.GetPath().GetText());
                return PXR_NS::UsdAttribute();
            }

            const size_t numValues = attrValues.size();

            if (numValues != numInstances) {
                TF_RUNTIME_ERROR(
                    "PointInstancer %s has %zu instances, but its %s "
                    "attribute only contains %zu values",
                    pointInstancer.GetPath().GetText(),
                    numInstances,
                    usdAttr.GetName().GetText(),
                    numValues);
                return PXR_NS::UsdAttribute();
            }
        }

        return usdAttr;
    }

    void _closeBatch()
    {
        // To close the batch we simply remove it from the map of batches under
        // construction, and thus it can no longer be joined.  If a batch was
        // created, erase() will harmlessly fail for all modifiers except the
        // first one.  If no batch was created, make a trivial, unshared batch
        // just for this modifier.
        batches().erase(pointInstancerPath());
        if (!_batch) {
            _batch = std::make_shared<UsdPointInstanceBatch<UsdValueType>>();
            _batch->nbInstances = 1;
        }
    }

    // An alternative to storing these three data members would be to simply
    // store a UsdSceneItem.  PPT, 12-Apr-2021.
    PXR_NS::UsdPrim _prim;
    int             _instanceIndex;
    Ufe::Path       _path {};

    Batch _batch { nullptr };
};

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif
