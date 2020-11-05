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
#include "AL/usd/transaction/Notice.h"

#include <pxr/base/tf/pyModuleNotice.h>
#include <pxr/base/tf/pyNoticeWrapper.h>

#include <boost/python/copy_const_reference.hpp>

//#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

TF_INSTANTIATE_NOTICE_WRAPPER(AL::usd::transaction::OpenNotice, TfNotice);
TF_INSTANTIATE_NOTICE_WRAPPER(AL::usd::transaction::CloseNotice, TfNotice);

} // namespace

void wrapNotice()
{
    {
        typedef AL::usd::transaction::OpenNotice This;
        TfPyNoticeWrapper<This, TfNotice>::Wrap();
    }
    {
        typedef AL::usd::transaction::CloseNotice This;
        TfPyNoticeWrapper<This, TfNotice>::Wrap()
            .def(
                "GetChangedInfoOnlyPaths",
                &This::GetChangedInfoOnlyPaths,
                return_value_policy<copy_const_reference>())
            .def(
                "GetResyncedPaths",
                &This::GetResyncedPaths,
                return_value_policy<copy_const_reference>())
            .def("AnyChanges", &This::AnyChanges);
    }
}
