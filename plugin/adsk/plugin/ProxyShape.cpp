//
// Copyright 2016 Pixar
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
#include "ProxyShape.h"

#include <mayaUsd/nodes/proxyShapePlugin.h>
#include <mayaUsd/nodes/hdImagingShape.h>

MAYAUSD_NS_DEF {

// ========================================================

const MTypeId MAYAUSD_PROXYSHAPE_ID (0x58000095);

const MTypeId ProxyShape::typeId(MayaUsd::MAYAUSD_PROXYSHAPE_ID);
const MString ProxyShape::typeName("mayaUsdProxyShape");

/* static */
void*
ProxyShape::creator()
{
    return new ProxyShape();
}

/* static */
MStatus
ProxyShape::initialize()
{
    MStatus retValue = inheritAttributesFrom(MayaUsdProxyShapeBase::typeName);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    return retValue;
}

ProxyShape::ProxyShape() : MayaUsdProxyShapeBase()
{
    TfRegistryManager::GetInstance().SubscribeTo<ProxyShape>();
}

/* virtual */
ProxyShape::~ProxyShape()
{
    //
    // empty
    //
}

void ProxyShape::postConstructor()
{
    ParentClass::postConstructor();

    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering())
    {
        // This shape uses Hydra for imaging, so make sure that the
        // pxrHdImagingShape is setup.
        PXR_NS::PxrMayaHdImagingShape::GetOrCreateInstance();
    }
}

} // MayaUsd
