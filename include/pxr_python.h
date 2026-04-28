//
// Copyright 2024 Autodesk
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

#ifndef USDUFE_PYTHON_H
#define USDUFE_PYTHON_H

// clang-format off

//
// Helper header to deal with USD removing boost python and replacing
// it with their own version.
//
#include <pxr/pxr.h>

#ifdef PXR_USE_INTERNAL_BOOST_PYTHON // PXR_VERSION >= 2411

#include <pxr/external/boost/python.hpp>

#include <pxr/external/boost/python/suite/indexing/map_indexing_suite.hpp>
#include <pxr/external/boost/python/wrapper.hpp>

#else

#include <boost/python.hpp>

#include <boost/noncopyable.hpp>

#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/python/wrapper.hpp>

#define PXR_BOOST_NAMESPACE        boost
#define PXR_BOOST_PYTHON_NAMESPACE boost::python

namespace PXR_BOOST_NAMESPACE { namespace python {
    using boost::noncopyable;
}} // namespace PXR_BOOST_NAMESPACE::python

#define PXR_BOOST_PYTHON_FUNCTION_OVERLOADS BOOST_PYTHON_FUNCTION_OVERLOADS

#endif

// clang-format on

#endif // USDUFE_PYTHON_H
