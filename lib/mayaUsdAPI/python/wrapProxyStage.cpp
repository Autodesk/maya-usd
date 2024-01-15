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

#include <mayaUsd/utils/util.h>
#include <mayaUsdAPI/proxyStage.h>

#include <maya/MDagPath.h>

#include <boost/python/class.hpp>

namespace {

PXR_NS::UsdTimeCode ProxyStage_getTime(const std::string& nodeName)
{
    MDagPath dagPath = PXR_NS::UsdMayaUtil::nameToDagPath(nodeName);
    if (!dagPath.isValid()) {
        return PXR_NS::UsdTimeCode();
    }

    MayaUsdAPI::ProxyStage proxyStage(dagPath.node());
    if (!proxyStage.isValid()) {
        return PXR_NS::UsdTimeCode();
    }

    return proxyStage.getTime();
}

PXR_NS::UsdStageRefPtr ProxyStage_getUsdStage(const std::string& nodeName)
{
    MDagPath dagPath = PXR_NS::UsdMayaUtil::nameToDagPath(nodeName);
    if (!dagPath.isValid()) {
        return nullptr;
    }

    MayaUsdAPI::ProxyStage proxyStage(dagPath.node());
    if (!proxyStage.isValid()) {
        return nullptr;
    }

    return proxyStage.getUsdStage();
}

} // namespace

using namespace boost::python;

void wrapProxyStage()
{
    class_<MayaUsdAPI::ProxyStage, boost::noncopyable>("ProxyStage", no_init)
        .def("getTime", &ProxyStage_getTime)
        .staticmethod("getTime")
        .def("getUsdStage", &ProxyStage_getUsdStage)
        .staticmethod("getUsdStage");
}
