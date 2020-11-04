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
#include "AL/maya/utils/MayaHelperMacros.h"
#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"

#include <mayaUsd/nodes/stageData.h>

#include <pxr/base/tf/pyEnum.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/base/tf/refPtr.h>

#include <maya/MBoundingBox.h>
#include <maya/MDGModifier.h>
#include <maya/MDagModifier.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnPluginData.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>

#include <boost/python.hpp>
#include <boost/python/args.hpp>
#include <boost/python/def.hpp>

#include <memory>

using AL::usdmaya::nodes::ProxyShape;
using boost::python::object;
using boost::python::reference_existing_object;

namespace {
struct MBoundingBoxConverter
{
    // to-python conversion of const MBoundingBox.
    // Decided NOT to register this using boost::python::to_python_converter,
    // in case pxr registers one at some point
    static PyObject* convert(const MBoundingBox& bbox)
    {
        TfPyLock lock;
        object   OpenMaya = boost::python::import("maya.OpenMaya");
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

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Helper func to convert mobj for a dag or depend node into it's a python string for it's
/// name \param  mobj an MObject for a dependency graph node \param  description A string used in
/// error messages to describe what this node was \return a boost::python::object holding the string
/// name for the node
object MobjToName(const MObject& mobj, MString description)
{
    MStatus  status;
    TfPyLock lock;
    if (mobj.isNull()) {
        return object(); // None
    }

    if (mobj.hasFn(MFn::kDagNode)) {
        MFnDagNode requiredDagNode(mobj, &status);
        if (!status) {
            MGlobal::displayError(MString("Error converting MObject to dagNode: ") + description);
            return object(); // None
        }
        return boost::python::str(requiredDagNode.fullPathName().asChar());
    } else if (mobj.hasFn(MFn::kDependencyNode)) {
        MFnDependencyNode requiredDepNode(mobj, &status);
        if (!status) {
            MGlobal::displayError(
                MString("Error converting MObject to dependNode: ") + description);
            return object(); // None
        }
        return boost::python::str(requiredDepNode.name().asChar());
    } else {
        MGlobal::displayError(
            MString("MObject did not appear to be a dependency node: ") + description);
        return object(); // None
    }
}

struct PyProxyShape
{
    //------------------------------------------------------------------------------------------------------------------
    /// \brief Find Usd prim associated with Maya node.
    /// \param dagPath Dag path of AL_usdmaya_Transform node.
    /// \return The Usd prim if found, else an invalid prim.
    static UsdPrim getUsdPrimFromMayaPath(const std::string& dagPath)
    {
        MStatus        status;
        MSelectionList sel;
        status = sel.add(MString(dagPath.c_str()));
        if (status) {
            MObject node;
            status = sel.getDependNode(0, node);
            if (status) {
                MFnDependencyNode depNode(node);
                if (depNode.typeId() == AL::usdmaya::nodes::Transform::kTypeId
                    || depNode.typeId() == AL::usdmaya::nodes::Scope::kTypeId) {
                    // Get proxy shape
                    auto transform
                        = static_cast<AL::usdmaya::nodes::Transform*>(depNode.userNode());
                    MPlug stageDataPlug(
                        transform->getProxyShape(), AL::usdmaya::nodes::ProxyShape::outStageData());

                    // Get stage data
                    MObject stageObject;
                    status = stageDataPlug.getValue(stageObject);
                    MFnPluginData fnData(stageObject);
                    auto*         stageData = static_cast<MayaUsdStageData*>(fnData.data());

                    // Validate stage
                    if (stageData && stageData->stage) {

                        // Lookup prim path in stage
                        MPlug   primPathPlug(node, AL::usdmaya::nodes::Transform::primPath());
                        MString primPath;
                        status = primPathPlug.getValue(primPath);
                        if (status) {
                            return stageData->stage->GetPrimAtPath(SdfPath(primPath.asChar()));
                        }
                    }
                }
            }
        }
        return UsdPrim();
    }

    //------------------------------------------------------------------------------------------------------------------
    /// \brief Find Maya node associated with Usd prim.
    /// \param prim A valid Usd prim.
    /// \return The full dag path of the Maya node if found, else an empty string.
    static std::string getMayaPathFromUsdPrim(const ProxyShape& proxyShape, const UsdPrim& prim)
    {
        return prim.IsValid() ? AL::maya::utils::convert(proxyShape.getMayaPathFromUsdPrim(prim))
                              : std::string();
    }

    //------------------------------------------------------------------------------------------------------------------
    /// \brief  Returns a python bounding box for the given proxy shape
    ///
    /// Used because we opted NOT to register auto-conversion of c++ MBoundingBoxes to python
    /// MBoundingBoxes, with boost::python::to_python_converter, as if pixar ever does so in the
    /// future, the double registration would be a problem
    static PyObject* boundingBox(const ProxyShape& proxyShape)
    {
        return MBoundingBoxConverter::convert(proxyShape.boundingBox());
    }

    //------------------------------------------------------------------------------------------------------------------
    /// \brief  Python-wrappable version of ProxyShape::findRequiredPath
    static object findRequiredPath(const ProxyShape& proxyShape, const SdfPath& path)
    {
        MObject obj = proxyShape.findRequiredPath(path);
        MString desc = MString("from SdfPath '") + path.GetText() + "'";
        return MobjToName(obj, desc);
    }

    //------------------------------------------------------------------------------------------------------------------
    /// \brief  Utility method, for better readability, that returns whether given MObject is a
    /// ProxyShape
    static bool isProxyShape(MObject mobj)
    {
        MStatus           status;
        MFnDependencyNode mfnDep(mobj, &status);
        if (!status) {
            return false;
        }
        return mfnDep.typeId() == ProxyShape::kTypeId;
    }

    //------------------------------------------------------------------------------------------------------------------
    /// \brief  Given a name, returns a pointer to a ProxyShape with that name
    ///
    /// Used because we don't allow direct calling of the python ProxyShape's
    /// constructor, so we need function we can wrap to get an existing
    /// instance.
    /// The name can point to the proxyShape directly, or to it's parent
    /// transform. If no match is found, a nullptr is returned (and thus
    /// None will be returned in python).
    static ProxyShape* getProxyShapeByName(std::string name)
    {
        MStatus        status;
        MSelectionList sel;
        status = sel.add(AL::maya::utils::convert(name));
        if (!status) {
            return nullptr;
        }
        MDagPath dag;
        status = sel.getDagPath(0, dag);
        if (!status) {
            return nullptr;
        }

        MObject proxyMObj = dag.node();
        if (!isProxyShape(proxyMObj)) {
            // Try extending to shapes below...
            if (!dag.hasFn(MFn::kTransform)) {
                return nullptr;
            }

            bool         foundProxy = false;
            unsigned int numShapes;
            if (!dag.numberOfShapesDirectlyBelow(numShapes)) {
                return nullptr;
            }
            for (uint32_t i = 0; i < numShapes; ++i) {
                dag.extendToShapeDirectlyBelow(i);
                if (isProxyShape(dag.node())) {
                    foundProxy = true;
                    proxyMObj = dag.node();
                    break;
                }
                dag.pop();
            }
            if (!foundProxy) {
                return nullptr;
            }
        }

        MFnDependencyNode mfnDep(proxyMObj, &status);
        if (!status) {
            return nullptr;
        }
        auto proxyShapePtr = static_cast<ProxyShape*>(mfnDep.userNode(&status));
        if (!status) {
            return nullptr;
        }
        return proxyShapePtr;
    }

    //------------------------------------------------------------------------------------------------------------------
    /// \brief  Python-wrappable version of ProxyShape::makeUsdTransformChain
    /// \param  proxyShape the ProxyShape we're making transforms for
    /// \param  usdPrim  the leaf of the prim we wish to create (same as for
    /// ProxyShape::makeUsdTransformChain) \param  reason  the reason why this path is being
    /// generated. (same as for ProxyShape::makeUsdTransformChain) \param  pushToPrim boolean
    /// controlling whether or not to set pushToPrim to true; in the wrapped function, this
    ///         is essentially controlled by whether or not the modifier2 param is passed.
    /// \param  returnCreateCount boolean controlling whether the return result includes the
    /// transforms that were
    ///         created
    /// \return if returnCreateCount is false, then will return the string name of the parent
    /// transform node for the
    ///         usdPrim; if returnCreateCount is true, the return will be a 2-tuple, the first item
    ///         of which is the name of the parent transform node, and the second is the number of
    ///         transforms created
    static object makeUsdTransformChain(
        ProxyShape&                 proxyShape,
        const UsdPrim&              usdPrim,
        ProxyShape::TransformReason reason = ProxyShape::kRequested,
        bool                        pushToPrim = false,
        bool                        returnCreateCount = false)
    {
        // Note - this current doesn't support undo, but right now, neither
        // does the AL_usdmaya_ProxyShapeImportAllTransforms command
        MDagModifier modifier;
        MDGModifier  modifier2;
        uint32_t     createCount = 0;

        MDGModifier* mod2Ptr = nullptr;
        if (pushToPrim) {
            mod2Ptr = &modifier2;
        }
        MObject resultObj
            = proxyShape.makeUsdTransformChain(usdPrim, modifier, reason, mod2Ptr, &createCount);
        modifier.doIt();
        if (pushToPrim) {
            modifier2.doIt();
        }
        MString objDesc("maya transform chain root for '");
        objDesc += usdPrim.GetPath().GetText();
        objDesc += "'";
        object resultPyObj = MobjToName(resultObj, objDesc);
        if (returnCreateCount) {
            return boost::python::make_tuple(resultPyObj, createCount);
        }
        return resultPyObj;
    }

    //------------------------------------------------------------------------------------------------------------------
    /// \brief  Python-wrappable version of ProxyShape::makeUsdTransforms
    /// \param  proxyShape the ProxyShape we're making transforms for
    /// \param  usdPrim the root for the transforms to be created (same as for
    /// ProxyShape::makeUsdTransforms) \param  reason the reason for creating the transforms (use
    /// with selection, etc). (same as for
    ///         ProxyShape::makeUsdTransforms)
    /// \param  pushToPrim boolean controlling whether or not to set pushToPrim to true; in the
    /// wrapped function, this
    ///         is essentially controlled by whether or not the modifier2 param is passed.
    /// \return the string name of the parent transform node for the usdPrim
    static object makeUsdTransforms(
        ProxyShape&                 proxyShape,
        const UsdPrim&              usdPrim,
        ProxyShape::TransformReason reason = ProxyShape::kRequested,
        bool                        pushToPrim = false)
    {
        MDagModifier modifier;
        MDGModifier  modifier2;

        MDGModifier* mod2Ptr = nullptr;
        if (pushToPrim) {
            mod2Ptr = &modifier2;
        }

        MObject resultObj = proxyShape.makeUsdTransforms(usdPrim, modifier, reason, mod2Ptr);
        modifier.doIt();
        if (pushToPrim) {
            modifier2.doIt();
        }
        MString objDesc("maya transform for '");
        objDesc += usdPrim.GetPath().GetText();
        objDesc += "'";
        return MobjToName(resultObj, objDesc);
    }

    //------------------------------------------------------------------------------------------------------------------
    /// \brief  Python-wrappable version of ProxyShape::removeUsdTransformChain (that takes a
    /// UsdPrim object) \param  proxyShape the ProxyShape we're removing transforms from \param
    /// usdPrim the leaf node in the chain of transforms we wish to remove (same as for
    ///         ProxyShape::removeUsdTransformChain)
    /// \param  reason  the reason why this path is being removed. (same as for
    /// ProxyShape::removeUsdTransformChain)
    static void removeUsdTransformChain(
        ProxyShape&                 proxyShape,
        const UsdPrim&              usdPrim,
        ProxyShape::TransformReason reason = ProxyShape::kRequested)
    {
        MDagModifier modifier;
        proxyShape.removeUsdTransformChain(usdPrim, modifier, reason);
        modifier.doIt();
    }

    //------------------------------------------------------------------------------------------------------------------
    /// \brief  Python-wrappable version of ProxyShape::removeUsdTransformChain (that takes an
    /// SdfPath) \param  proxyShape the ProxyShape we're removing transforms from \param  path the
    /// leaf node in the chain of transforms we wish to remove (same as for
    ///         ProxyShape::removeUsdTransformChain)
    /// \param  reason  the reason why this path is being removed. (same as for
    /// ProxyShape::removeUsdTransformChain)
    static void removeUsdTransformChain(
        ProxyShape&                 proxyShape,
        const SdfPath&              path,
        ProxyShape::TransformReason reason = ProxyShape::kRequested)
    {
        MDagModifier modifier;
        proxyShape.removeUsdTransformChain(path, modifier, reason);
        modifier.doIt();
    }

    //------------------------------------------------------------------------------------------------------------------
    /// \brief  Python-wrappable version of ProxyShape::removeUsdTransforms
    /// \param  proxyShape the ProxyShape we're removing transforms from
    /// \param  path all AL_usdmaya_Transform nodes found underneath this path will be destroyed
    /// (unless those nodes are
    ///         required for another purpose). (same as for ProxyShape::removeUsdTransforms)
    /// \param  reason Are we deleting selected transforms, or those that are required, or
    /// requested? (same as for
    ///         ProxyShape::removeUsdTransforms)
    static void removeUsdTransforms(
        ProxyShape&                 proxyShape,
        const UsdPrim&              usdPrim,
        ProxyShape::TransformReason reason = ProxyShape::kRequested)
    {
        MDagModifier modifier;
        proxyShape.removeUsdTransforms(usdPrim, modifier, reason);
        modifier.doIt();
    }
};
} // namespace

void wrapProxyShape()
{
    void (*removeUsdTransformChain_prim)(
        ProxyShape & proxyShape, const UsdPrim& usdPrim, ProxyShape::TransformReason)
        = PyProxyShape::removeUsdTransformChain;

    void (*removeUsdTransformChain_path)(
        ProxyShape & proxyShape, const SdfPath& path, ProxyShape::TransformReason)
        = PyProxyShape::removeUsdTransformChain;

    boost::python::class_<ProxyShape, boost::noncopyable> proxyShapeCls(
        "ProxyShape", boost::python::no_init);

    boost::python::scope proxyShapeScope = proxyShapeCls;

    boost::python::enum_<ProxyShape::TransformReason>("TransformReason")
        .value("kSelection", ProxyShape::kSelection)
        .value("kRequested", ProxyShape::kRequested)
        .value("kRequired", ProxyShape::kRequired);

    proxyShapeCls
        .def(
            "getByName",
            PyProxyShape::getProxyShapeByName,
            boost::python::return_value_policy<reference_existing_object>())
        .staticmethod("getByName")
        .def("getUsdStage", &ProxyShape::getUsdStage)
        .def(
            "getUsdPrimFromMayaPath",
            PyProxyShape::getUsdPrimFromMayaPath,
            ("Find Usd prim associated with Maya node.\n"
             "Args:\n"
             "\tdagPath (str): Dag path of AL_usdmaya_Transform node.\n"
             "Returns:\n"
             "\tThe Usd prim if found, else an invalid prim.\n"),
            (boost::python::arg("dagPath")))
        .staticmethod("getUsdPrimFromMayaPath")
        .def(
            "getMayaPathFromUsdPrim",
            &PyProxyShape::getMayaPathFromUsdPrim,
            ("Find Maya node associated with Usd prim in Proxy's stage.\n"
             "Args:\n"
             "\tprim (pxr.Usd.Prim): A valid Usd prim.\n"
             "Returns:\n"
             "\tThe full dag path of the Maya node if found, else an empty string.\n"),
            (boost::python::arg("prim")))
        .def("resync", &ProxyShape::resync, (boost::python::arg("path")))
        .def("boundingBox", PyProxyShape::boundingBox)
        .def("isRequiredPath", &ProxyShape::isRequiredPath)
        .def("findRequiredPath", PyProxyShape::findRequiredPath)
        .def(
            "makeUsdTransformChain",
            PyProxyShape::makeUsdTransformChain,
            (boost::python::arg("usdPrim"),
             boost::python::arg("reason") = ProxyShape::kRequested,
             boost::python::arg("pushToPrim") = false,
             boost::python::arg("returnCreateCount") = false))
        .def(
            "makeUsdTransforms",
            PyProxyShape::makeUsdTransforms,
            (boost::python::arg("usdPrim"),
             boost::python::arg("reason") = ProxyShape::kRequested,
             boost::python::arg("pushToPrim") = false))
        .def(
            "removeUsdTransformChain",
            removeUsdTransformChain_prim,
            (boost::python::arg("usdPrim"), boost::python::arg("reason") = ProxyShape::kRequested))
        .def(
            "removeUsdTransformChain",
            removeUsdTransformChain_path,
            (boost::python::arg("path"), boost::python::arg("reason") = ProxyShape::kRequested))
        .def(
            "removeUsdTransforms",
            PyProxyShape::removeUsdTransforms,
            (boost::python::arg("usdPrim"), boost::python::arg("reason") = ProxyShape::kRequested))
        .def("destroyTransformReferences", &ProxyShape::destroyTransformReferences);

    // Decided NOT to register this using boost::python::to_python_converter,
    // in case pxr registers one at some point
    //    boost::python::to_python_converter<MBoundingBox, MBoundingBoxConverter>();
}

// This workaround for a VS compiler bug where we need to explicitly specify
// the conversion to pointer of your our class seems no longer needed for VS2017.
// What it means, is that compiler will properly generate this conversion
// implicitly and cause linker error LNK2005.
//
// In this particular case we got wrapTranslatorContext.obj generating error for
// already defined conversion to a pointer for ProxyShape in wrapProxyShape.obj
//
// The best place to put this fix would be in pxr/base/lib/tf/refPtr.h
// where TF_REFPTR_CONST_VOLATILE_GET is defined, but for now we are
// patching it locally.
#if defined(ARCH_COMPILER_MSVC) && ARCH_COMPILER_MSVC_VERSION <= 1910
TF_REFPTR_CONST_VOLATILE_GET(ProxyShape)
#endif
