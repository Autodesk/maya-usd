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
#include "AL/usdmaya/StageCache.h"

#include <pxr/base/tf/pyResultConversions.h>

#include <boost/python.hpp>
#include <boost/python/args.hpp>
#include <boost/python/def.hpp>

// using namespace std;
// using namespace boost::python;
// using namespace boost;

void wrapStageCache()
{
    boost::python::class_<AL::usdmaya::StageCache>("StageCache")

        .def(
            "Get",
            &AL::usdmaya::StageCache::Get,
            boost::python::return_value_policy<boost::python::reference_existing_object>())
        .staticmethod("Get")
        .def("Clear", &AL::usdmaya::StageCache::Clear)
        .staticmethod("Clear");
}
