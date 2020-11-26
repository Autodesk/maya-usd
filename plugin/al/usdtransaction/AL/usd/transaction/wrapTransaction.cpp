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
#include "AL/usd/transaction/Transaction.h"

#include <pxr/base/tf/makePyConstructor.h>
#include <pxr/base/tf/pyPtrHelpers.h>
#include <pxr/base/tf/pyResultConversions.h>

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapTransaction()
{
    {
        typedef AL::usd::transaction::Transaction This;
        class_<This>("Transaction", no_init)
            .def(init<const UsdStageWeakPtr&, const SdfLayerHandle&>((arg("stage"), arg("layer"))))
            .def("Open", &This::Open)
            .def("Close", &This::Close)
            .def("InProgress", &This::InProgress);
    }
}
