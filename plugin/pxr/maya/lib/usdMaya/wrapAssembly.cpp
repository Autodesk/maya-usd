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
#include "usdMaya/referenceAssembly.h"

#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/pxr.h>

#include <maya/MFnAssembly.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>

#include <boost/python/def.hpp>

#include <map>
#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static std::map<std::string, std::string> _GetVariantSetSelections(const std::string& assemblyName)
{
    std::map<std::string, std::string> emptyResult;

    MObject assemblyObj;
    MStatus status = UsdMayaUtil::GetMObjectByName(assemblyName, assemblyObj);
    CHECK_MSTATUS_AND_RETURN(status, emptyResult);

    MFnAssembly assemblyFn(assemblyObj, &status);
    CHECK_MSTATUS_AND_RETURN(status, emptyResult);

    UsdMayaReferenceAssembly* assembly
        = dynamic_cast<UsdMayaReferenceAssembly*>(assemblyFn.userNode());
    if (!assembly) {
        return emptyResult;
    }

    return assembly->GetVariantSetSelections();
}

} // anonymous namespace

void wrapAssembly()
{
    def("GetVariantSetSelections", &_GetVariantSetSelections, arg("assemblyName"));
}
