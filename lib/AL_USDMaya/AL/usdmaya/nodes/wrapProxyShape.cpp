//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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
#include "AL/usdmaya/nodes/ProxyShape.h"

#include <boost/python/args.hpp>
#include <boost/python/def.hpp>
#include <boost/python.hpp>

#include "AL/usdmaya/Utils.h"

#include "maya/MBoundingBox.h"
#include "maya/MFnDependencyNode.h"
#include "maya/MSelectionList.h"

#include "pxr/base/tf/pyResultConversions.h"

#include <memory>

//using namespace std;
//using namespace boost::python;
//using namespace boost;


using AL::usdmaya::nodes::ProxyShape;
using boost::python::reference_existing_object;
using boost::python::object;

namespace {
  struct MBoundingBoxConverter
  {
    // to-python conversion of const PxrUsdMayaXformOpClassification.
    static PyObject* convert(const MBoundingBox& bbox)
    {
        TfPyLock lock;
        object OpenMaya = boost::python::import("maya.OpenMaya");
        // Considered grabbing the raw pointer to the MBoundingBox
        // from the swig wrapper, but this seems sketchy, so I'm
        // just re-constructing "in python"
        object PyMBoundingBox = OpenMaya.attr("MBoundingBox");
        object PyMPoint = OpenMaya.attr("MPoint");
        MPoint min = bbox.min();
        MPoint max = bbox.max();
        object pyMin = PyMPoint(min.x, min.y, min.z, min.w);
        object pyMax = PyMPoint(max.x, max.y, max.z, max.w);
        object pyBbox = PyMBoundingBox(pyMin, pyMax);
        return boost::python::incref(pyBbox.ptr());
    }
  };

  bool isProxyShape(MObject mobj)
  {
    MStatus status;
    MFnDependencyNode mfnDep(mobj, &status);
    if (!status)
    {
      return false;
    }
    return mfnDep.typeId() == ProxyShape::kTypeId;
  }

  ProxyShape* getProxyShapeByName(std::string name)
  {
    MStatus status;
    MSelectionList sel;
    status = sel.add(AL::usdmaya::convert(name));
    if (!status)
    {
      return nullptr;
    }
    MDagPath dag;
    status = sel.getDagPath(0, dag);
    if (!status)
    {
      return nullptr;
    }

    MObject proxyMObj = dag.node();
    if (!isProxyShape(proxyMObj))
    {
      // Try extending to shapes below...
      if (!dag.hasFn(MFn::kTransform))
      {
        return nullptr;
      }

      bool foundProxy = false;
      unsigned int numShapes;
      if (!dag.numberOfShapesDirectlyBelow(numShapes))
      {
        return nullptr;
      }
      for (int i = 0; i < numShapes; ++i)
      {
        dag.extendToShapeDirectlyBelow(i);
        if (isProxyShape(dag.node()))
        {
          foundProxy = true;
          proxyMObj = dag.node();
          break;
        }
        dag.pop();
      }
      if (!foundProxy)
      {
        return nullptr;
      }
    }

    MFnDependencyNode mfnDep(proxyMObj, &status);
    if (!status)
    {
      return nullptr;
    }
    auto proxyShapePtr = static_cast<ProxyShape*>(mfnDep.userNode(&status));
    if (!status)
    {
      return nullptr;
    }
    return proxyShapePtr;
  }
}

void wrapProxyShape()
{

  boost::python::scope s = boost::python::class_<ProxyShape, boost::noncopyable>(
      "ProxyShape", boost::python::no_init)
    .def("getByName", getProxyShapeByName,
        boost::python::return_value_policy<reference_existing_object>())
        .staticmethod("getByName")
    .def("getUsdStage", &ProxyShape::getUsdStage)
    .def("boundingBox", &ProxyShape::boundingBox)
    ;

    boost::python::to_python_converter<MBoundingBox, MBoundingBoxConverter>();
}
