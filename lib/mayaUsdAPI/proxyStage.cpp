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

#include "proxyStage.h"

#include <mayaUsd/nodes/proxyStageProvider.h>

#include <maya/MFnDependencyNode.h>

namespace MAYAUSDAPI_NS_DEF {

struct ProxyStageImp
{
    ProxyStageImp(MPxNode* node)
        : _proxyStageProvider(dynamic_cast<PXR_NS::ProxyStageProvider*>(node))
    {
        PXR_NAMESPACE_USING_DIRECTIVE
        TF_VERIFY(
            _proxyStageProvider,
            "The node passed to the constructor of ProxyStage is not a MayaUsdProxyShapeBase "
            "subclass node while it should ! "
            "Its type is : %s",
            node->typeName().asChar());
        TF_AXIOM(_proxyStageProvider);
    }
    ProxyStageImp(PXR_NS::ProxyStageProvider* proxyStageProvider)
        : _proxyStageProvider(proxyStageProvider)
    {
    }

    PXR_NS::ProxyStageProvider* const _proxyStageProvider {};
};

// Use unique_ptr with custom deleter
using UniqueProxyStageImp = std::unique_ptr<MayaUsdAPI_v0::ProxyStageImp, ProxyStageImpDeleter>;

ProxyStage::ProxyStage(const MObject& obj)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    TF_AXIOM(!obj.isNull());
    MFnDependencyNode dep(obj);
    _imp = UniqueProxyStageImp(new ProxyStageImp(dep.userNode()));
}

ProxyStage::ProxyStage(const ProxyStage& other)
{
    _imp = UniqueProxyStageImp(new ProxyStageImp(other._imp->_proxyStageProvider));
}

ProxyStage::~ProxyStage() = default;

void ProxyStageImpDeleter::operator()(ProxyStageImp* ptr) const { delete ptr; }

bool ProxyStage::isValid() const { return (nullptr != _imp->_proxyStageProvider); }

PXR_NS::UsdTimeCode ProxyStage::getTime() const
{
    return (_imp->_proxyStageProvider) ? _imp->_proxyStageProvider->getTime()
                                       : PXR_NS::UsdTimeCode();
}

PXR_NS::UsdStageRefPtr ProxyStage::getUsdStage() const
{
    return (_imp->_proxyStageProvider) ? _imp->_proxyStageProvider->getUsdStage() : nullptr;
}

} // End of namespace MAYAUSDAPI_NS_DEF