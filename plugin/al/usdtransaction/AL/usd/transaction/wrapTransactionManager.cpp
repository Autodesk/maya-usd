//
// Copyright 2017 Animal Logic
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
#include "AL/usd/transaction/TransactionManager.h"

#include <pxr/base/tf/makePyConstructor.h>
#include <pxr/base/tf/pyPtrHelpers.h>
#include <pxr/base/tf/pyResultConversions.h>

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

typedef AL::usd::transaction::TransactionManager This;

static bool InProgressStage(const UsdStageWeakPtr& stage) { return This::InProgress(stage); }

static bool InProgressStageLayer(const UsdStageWeakPtr& stage, const SdfLayerHandle& layer)
{
    return This::InProgress(stage, layer);
}

static bool OpenStageLayer(const UsdStageWeakPtr& stage, const SdfLayerHandle& layer)
{
    return This::Open(stage, layer);
}

static bool CloseStageLayer(const UsdStageWeakPtr& stage, const SdfLayerHandle& layer)
{
    return This::Close(stage, layer);
}

void wrapTransactionManager()
{
    {
        class_<This>("TransactionManager", no_init)
            .def("InProgress", InProgressStage, (arg("stage")))
            .def("InProgress", InProgressStageLayer, (arg("stage"), arg("layer")))
            .staticmethod("InProgress")

            .def("Open", OpenStageLayer, (arg("stage"), arg("layer")))
            .staticmethod("Open")

            .def("Close", CloseStageLayer, (arg("stage"), arg("layer")))
            .staticmethod("Close");
    }
}
