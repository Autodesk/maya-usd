//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "ProxyShape.h"

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

} // MayaUsd
