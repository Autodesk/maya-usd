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

#include <pxr/base/tf/refPtr.h>
#include <pxr/pxr.h>
#include <pxr_python.h>

#include <maya/MStringArray.h>

#include <memory>

using AL::usdmaya::nodes::LayerManager;
using PXR_BOOST_PYTHON_NAMESPACE::object;
using PXR_BOOST_PYTHON_NAMESPACE::reference_existing_object;

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
        typename PXR_BOOST_PYTHON_NAMESPACE::reference_existing_object::apply<LayerManager*>::type
                                             converter;
        PXR_BOOST_PYTHON_NAMESPACE::handle<> handle(converter(managerPtr));
        object                               managerPyObj(handle);
        if (returnWasCreated) {
            return PXR_BOOST_PYTHON_NAMESPACE::make_tuple(managerPyObj, wasCreated);
        }
        return managerPyObj;
    }

    static PXR_BOOST_PYTHON_NAMESPACE::list getLayerIdentifiers(LayerManager& manager)
    {
        MStringArray mstrings;
        manager.getLayerIdentifiers(mstrings);
        PXR_BOOST_PYTHON_NAMESPACE::list pyList;
        for (unsigned int i = 0; i < mstrings.length(); ++i) {
            pyList.append(mstrings[i].asChar());
        }
        return pyList;
    }
};
} // namespace

void wrapLayerManager()
{
    PXR_BOOST_PYTHON_NAMESPACE::class_<LayerManager, PXR_BOOST_PYTHON_NAMESPACE::noncopyable>(
        "LayerManager", PXR_BOOST_PYTHON_NAMESPACE::no_init)
        .def(
            "find",
            &LayerManager::findManager,
            PXR_BOOST_PYTHON_NAMESPACE::return_value_policy<reference_existing_object>())
        .staticmethod("find")
        .def(
            "findOrCreate",
            PyLayerManager::findOrCreate,
            (PXR_BOOST_PYTHON_NAMESPACE::arg("returnWasCreated") = false))
        .staticmethod("findOrCreate")
        .def(
            "addLayer",
            &LayerManager::addLayer,
            (PXR_BOOST_PYTHON_NAMESPACE::arg("layer"),
             PXR_BOOST_PYTHON_NAMESPACE::arg("identifier") = ""))
        .def("removeLayer", &LayerManager::removeLayer)
        .def("findLayer", &LayerManager::findLayer)
        .def("getLayerIdentifiers", PyLayerManager::getLayerIdentifiers);
}

// This macro was removed in USD 23.08. It was only needed to support
// Visual Studio 2015, which USD itself no longer supports.
#if PXR_VERSION < 2308
TF_REFPTR_CONST_VOLATILE_GET(LayerManager)
#endif
