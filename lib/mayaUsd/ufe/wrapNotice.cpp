//
// Copyright 2020 Autodesk
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
#include <mayaUsd/listeners/proxyShapeNotice.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

#include <pxr/base/tf/pyNoticeWrapper.h>

#include <boost/python.hpp>

using namespace MayaUsd;
using namespace boost::python;

namespace {

TF_INSTANTIATE_NOTICE_WRAPPER(MayaUsdProxyStageSetNotice, TfNotice);
TF_INSTANTIATE_NOTICE_WRAPPER(MayaUsdProxyStageInvalidateNotice, TfNotice);

} // namespace

void wrapNotice()
{
    {
        typedef MayaUsdProxyStageSetNotice This;
        TfPyNoticeWrapper<This, TfNotice>::Wrap()
            .add_property("shapePath", &This::GetShapePath)
            .add_property("stage", &This::GetStage);
    }
    {
        typedef MayaUsdProxyStageInvalidateNotice This;
        TfPyNoticeWrapper<This, TfNotice>::Wrap()
            .add_property("shapePath", &This::GetShapePath)
            .add_property("stage", &This::GetStage);
    }
}
