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
#pragma once

#include <pxr/pxr.h>

#include <mayaUsd/base/api.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

#include "base/api.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

class ProxyShape : public MayaUsdProxyShapeBase
{
    public:
        typedef MayaUsdProxyShapeBase ParentClass;

        MAYAUSD_PLUGIN_PUBLIC
        static const MTypeId typeId;
        MAYAUSD_PLUGIN_PUBLIC
        static const MString typeName;

        MAYAUSD_PLUGIN_PUBLIC
        static void* creator();

        MAYAUSD_PLUGIN_PUBLIC
        static MStatus initialize();

        void postConstructor() override;

    protected:
		MAYAUSD_PLUGIN_PUBLIC
		SdfLayerRefPtr computeRootLayer(MDataBlock&, const std::string&) override;

        MAYAUSD_PLUGIN_PUBLIC
        SdfLayerRefPtr computeSessionLayer(MDataBlock&) override;

    private:
        ProxyShape();

        ProxyShape(const ProxyShape&);
        ~ProxyShape() override;
        ProxyShape& operator=(const ProxyShape&);
};

} // MayaUsd
