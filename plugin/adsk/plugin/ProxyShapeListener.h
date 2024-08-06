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
#ifndef ASDK_PLUGIN_PROXYSHAPELISTENER_H
#define ASDK_PLUGIN_PROXYSHAPELISTENER_H

#include "base/api.h"

#include <mayaUsd/base/api.h>
#include <mayaUsd/nodes/proxyShapeListenerBase.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

class ProxyShapeListener : public MayaUsdProxyShapeListenerBase
{
public:
    typedef MayaUsdProxyShapeListenerBase ParentClass;

    MAYAUSD_PLUGIN_PUBLIC
    static const MTypeId typeId;
    MAYAUSD_PLUGIN_PUBLIC
    static const MString typeName;

    MAYAUSD_PLUGIN_PUBLIC
    static void* creator();

    MAYAUSD_PLUGIN_PUBLIC
    static MStatus initialize();

private:
    ProxyShapeListener();

    ProxyShapeListener(const ProxyShapeListener&);
    ~ProxyShapeListener() override;
    ProxyShapeListener& operator=(const ProxyShapeListener&);
};

} // namespace MAYAUSD_NS_DEF

#endif // ASDK_PLUGIN_PROXYSHAPELISTENER_H
