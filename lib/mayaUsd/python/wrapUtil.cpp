//
// Copyright 2022 Autodesk
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
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/pyConversions.h>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE;

namespace {
class UsdMayaUtilScope
{
};

VtDictionary getDictionaryFromEncodedOptions(const std::string& textOptions)
{
    VtDictionary dictOptions;
    auto         status = UsdMayaJobExportArgs::GetDictionaryFromEncodedOptions(
        MString(textOptions.c_str()), &dictOptions);
    if (status != MS::kSuccess)
        return VtDictionary();
    return dictOptions;
}

} // namespace

void wrapUtil()
{
    scope s(class_<UsdMayaUtilScope>("Util"));

    def("IsAuthored", UsdMayaUtil::IsAuthored);
    def("getDictionaryFromEncodedOptions", getDictionaryFromEncodedOptions);
}
