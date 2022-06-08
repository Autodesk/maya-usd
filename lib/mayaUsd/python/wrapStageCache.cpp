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
#include <mayaUsd/utils/stageCache.h>

#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/pxr.h>

#include <boost/python.hpp>
#include <boost/python/args.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

using namespace std;
using namespace boost::python;
using namespace boost;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
UsdStageCache& _UsdMayaStageCacheGet(bool loadAll, bool shared)
{
    return UsdMayaStageCache::Get(
        loadAll ? UsdStage::InitialLoadSet::LoadAll : UsdStage::InitialLoadSet::LoadNone,
        shared ? UsdMayaStageCache::ShareMode::Shared : UsdMayaStageCache::ShareMode::Unshared);
}

} // namespace

void wrapStageCache()
{
    class_<UsdMayaStageCache>("StageCache")

        .def(
            "Get",
            _UsdMayaStageCacheGet,
            python::args("loadAll", "shared"),
            return_value_policy<reference_existing_object>())
        .staticmethod("Get")
        .def("Clear", &UsdMayaStageCache::Clear)
        .staticmethod("Clear");
}
