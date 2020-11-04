//
// Copyright 2018 Animal Logic
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
#include "AL/usdmaya/nodes/LayerManager.h"

#include <boost/python/args.hpp>
#include <boost/python/def.hpp>

// On Windows, against certain versions of Maya and with strict compiler
// settings on, we are getting warning-as-error problems with a couple
// boost includes.  Disabling those warnings for the specific includes
// for now instead of disabling the strict settings at a higher level.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4275)
#endif
#include <boost/python.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <pxr/base/tf/refPtr.h>

#include <maya/MStringArray.h>

#include <memory>

using AL::usdmaya::nodes::LayerManager;
using boost::python::object;
using boost::python::reference_existing_object;

namespace {
struct PyLayerManager
{
    //------------------------------------------------------------------------------------------------------------------
    /// \brief  Python-wrappable version of LayerManager::findOrCreateManager
    /// \param  returnWasCreated boolean controlling whether the return result includes whether a
    /// new
    ///         LayerManager node was created or not
    /// \return if returnCreateCount is false, then will return the string name of the parent
    /// transform node for the
    ///         usdPrim; if returnCreateCount is true, the return will be a 2-tuple, the first item
    ///         of which is the name of the parent transform node, and the second is the number of
    ///         transforms created
    static object findOrCreate(bool returnWasCreated = false)
    {
        bool          wasCreated;
        LayerManager* managerPtr = LayerManager::findOrCreateManager(nullptr, &wasCreated);
        typename boost::python::reference_existing_object::apply<LayerManager*>::type converter;
        boost::python::handle<> handle(converter(managerPtr));
        object                  managerPyObj(handle);
        if (returnWasCreated) {
            return boost::python::make_tuple(managerPyObj, wasCreated);
        }
        return managerPyObj;
    }

    static boost::python::list getLayerIdentifiers(LayerManager& manager)
    {
        MStringArray mstrings;
        manager.getLayerIdentifiers(mstrings);
        boost::python::list pyList;
        for (unsigned int i = 0; i < mstrings.length(); ++i) {
            pyList.append(mstrings[i].asChar());
        }
        return pyList;
    }
};
} // namespace

void wrapLayerManager()
{
    boost::python::class_<LayerManager, boost::noncopyable>("LayerManager", boost::python::no_init)
        .def(
            "find",
            &LayerManager::findManager,
            boost::python::return_value_policy<reference_existing_object>())
        .staticmethod("find")
        .def(
            "findOrCreate",
            PyLayerManager::findOrCreate,
            (boost::python::arg("returnWasCreated") = false))
        .staticmethod("findOrCreate")
        .def(
            "addLayer",
            &LayerManager::addLayer,
            (boost::python::arg("layer"), boost::python::arg("identifier") = ""))
        .def("removeLayer", &LayerManager::removeLayer)
        .def("findLayer", &LayerManager::findLayer)
        .def("getLayerIdentifiers", PyLayerManager::getLayerIdentifiers);
}

TF_REFPTR_CONST_VOLATILE_GET(LayerManager)
