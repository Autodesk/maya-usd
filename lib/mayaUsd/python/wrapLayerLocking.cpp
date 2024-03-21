//
// Copyright 2024 Autodesk
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
#include <mayaUsd/utils/layerLocking.h>

#include <pxr/base/tf/pyResultConversions.h>

#include <boost/python/def.hpp>

using namespace boost::python;

namespace {

void _lockLayer(const std::string& shapeName, PXR_NS::SdfLayer& layer)
{
    PXR_NS::SdfLayerRefPtr layerPtr(&layer);
    MayaUsd::lockLayer(shapeName, layerPtr, MayaUsd::LayerLock_Locked);
}

void _systemLockLayer(const std::string& shapeName, PXR_NS::SdfLayer& layer)
{
    PXR_NS::SdfLayerRefPtr layerPtr(&layer);
    MayaUsd::lockLayer(shapeName, layerPtr, MayaUsd::LayerLock_SystemLocked);
}

void _unlockLayer(const std::string& shapeName, PXR_NS::SdfLayer& layer)
{
    PXR_NS::SdfLayerRefPtr layerPtr(&layer);
    MayaUsd::lockLayer(shapeName, layerPtr, MayaUsd::LayerLock_Unlocked);
}

bool _isLayerLocked(PXR_NS::SdfLayer& layer)
{
    PXR_NS::SdfLayerRefPtr layerPtr(&layer);
    return MayaUsd::isLayerLocked(layerPtr);
}

bool _isLayerSystemLocked(PXR_NS::SdfLayer& layer)
{
    PXR_NS::SdfLayerRefPtr layerPtr(&layer);
    return MayaUsd::isLayerSystemLocked(layerPtr);
}

} // namespace

void wrapLayerLocking()
{
    def("lockLayer", _lockLayer);
    def("systemLockLayer", _systemLockLayer);
    def("unlockLayer", _unlockLayer);
    def("isLayerLocked", _isLayerLocked);
    def("isLayerSystemLocked", _isLayerSystemLocked);
}
