//
// Copyright 2023 Autodesk
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
#include "ProxyShapeListener.h"

namespace MAYAUSD_NS_DEF {

// ========================================================

const MTypeId MAYAUSD_PROXYSHAPELISTENER_ID(0x5800009B);

const MTypeId ProxyShapeListener::typeId(MayaUsd::MAYAUSD_PROXYSHAPELISTENER_ID);
const MString ProxyShapeListener::typeName("mayaUsdProxyShapeListener");

/* static */
void* ProxyShapeListener::creator() { return new ProxyShapeListener(); }

/* static */
MStatus ProxyShapeListener::initialize()
{
    MStatus retValue = inheritAttributesFrom(MayaUsdProxyShapeListenerBase::typeName);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    return retValue;
}

ProxyShapeListener::ProxyShapeListener()
    : MayaUsdProxyShapeListenerBase()
{
}

/* virtual */
ProxyShapeListener::~ProxyShapeListener()
{
    //
    // empty
    //
}

} // namespace MAYAUSD_NS_DEF
