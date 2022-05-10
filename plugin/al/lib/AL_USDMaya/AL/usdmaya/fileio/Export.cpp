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
#include "AL/usdmaya/fileio/Export.h"

#include "AL/maya/utils/MObjectMap.h"
#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/TransformOperation.h"
#include "AL/usdmaya/fileio/AnimationTranslator.h"
#include "AL/usdmaya/fileio/ExportTranslator.h"
#include "AL/usdmaya/fileio/NodeFactory.h"
#include "AL/usdmaya/fileio/translators/TransformTranslator.h"
#include "AL/usdmaya/utils/MeshUtils.h"
#include "AL/usdmaya/utils/Utils.h"

#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/nurbsCurves.h>
#include <pxr/usd/usdGeom/xform.h>

#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MFnMesh.h>
#include <maya/MFnTransform.h>
#include <maya/MItDag.h>
#include <maya/MNodeClass.h>
#include <maya/MPlugArray.h>
#include <maya/MSyntax.h>
#include <maya/MTypes.h> // For MAYA_APP_VERSION

namespace AL {
namespace usdmaya {
namespace fileio {

AL_MAYA_DEFINE_COMMAND(ExportCommand, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
static unsigned int mayaDagPathToSdfPath(char* dagPathBuffer, unsigned int dagPathSize)
{
    char* writer = dagPathBuffer;
    char* backTrack = writer;

    for (const char *reader = dagPathBuffer, *end = reader + dagPathSize; reader != end; ++reader) {
        const char c = *reader;
        switch (c) {
        case ':': writer = backTrack; break;

        case '|':
            *writer = '/';
            ++writer;
            backTrack = writer;
            break;

        default:
            if (reader != writer)
                *writer = c;

            ++writer;
            break;
        }
    }

    return writer - dagPathBuffer;
}

//----------------------------------------------------------------------------------------------------------------------
static inline SdfPath makeUsdPath(const MDagPath& rootPath, const MDagPath& path)
{
    // if the rootPath is empty, we can just use the entire path
    const uint32_t rootPathLength = rootPath.length();
    if (!rootPathLength) {
        std::string fpn = AL::maya::utils::convert(path.fullPathName());
        fpn.resize(mayaDagPathToSdfPath(&fpn[0], fpn.size()));
        return SdfPath(fpn);
    }

    // otherwise we need to do a little fiddling.
    MString rootPathString, pathString, newPathString;
    rootPathString = rootPath.fullPathName();
    pathString = path.fullPathName();

    // trim off the root path from the object we are exporting
    newPathString = pathString.substring(rootPathString.length(), pathString.length());

    std::string fpn = AL::maya::utils::convert(newPathString);
    fpn.resize(mayaDagPathToSdfPath(&fpn[0], fpn.size()));
    return SdfPath(fpn);
}

//----------------------------------------------------------------------------------------------------------------------
static inline MDagPath getParentPath(const MDagPath& dagPath)
{
    MFnDagNode dagNode(dagPath);
    MFnDagNode parentNode(dagNode.parent(0));
    MDagPath   parentPath;
    parentNode.getPath(parentPath);
    return parentPath;
}

//----------------------------------------------------------------------------------------------------------------------
static inline SdfPath makeMasterPath(UsdPrim instancesPrim, const MDagPath& dagPath)
{
    MString     fullPathName = dagPath.fullPathName();
    std::string fullPath(fullPathName.asChar() + 1, fullPathName.length() - 1);
    std::string usdPath = instancesPrim.GetPath().GetString() + '/' + fullPath;
    std::replace(usdPath.begin(), usdPath.end(), '|', '_');
    std::replace(usdPath.begin(), usdPath.end(), ':', '_');
    return SdfPath(usdPath);
}

//----------------------------------------------------------------------------------------------------------------------
/// Internal USD exporter implementation
//----------------------------------------------------------------------------------------------------------------------
struct Export::Impl
{
    inline bool contains(const MFnDependencyNode& fn)
    {
#if AL_UTILS_ENABLE_SIMD
        union
        {
            __m128i               sse;
            AL::maya::utils::guid uuid;
        };
        fn.uuid().get(uuid.uuid);
        bool contains = m_nodeMap.find(sse) != m_nodeMap.end();
        if (!contains)
            m_nodeMap.insert(std::make_pair(sse, fn.object()));
#else
        AL::maya::utils::guid uuid;
        fn.uuid().get(uuid.uuid);
        bool contains = m_nodeMap.find(uuid) != m_nodeMap.end();
        if (!contains)
            m_nodeMap.insert(std::make_pair(uuid, fn.object()));
#endif
        return contains;
    }

    inline bool contains(const MObject& obj)
    {
        MFnDependencyNode fn(obj);
        return contains(fn);
    }

    inline SdfPath getMasterPath(const MFnDagNode& fn)
    {
#if AL_UTILS_ENABLE_SIMD
        union
        {
            __m128i               sse;
            AL::maya::utils::guid uuid;
        };
        fn.uuid().get(uuid.uuid);
        auto iterInst = m_instanceMap.find(sse);
        if (iterInst == m_instanceMap.end()) {
            MDagPath dagPath;
            fn.getPath(dagPath);
            SdfPath instancePath = makeMasterPath(m_instancesPrim, dagPath);
            m_instanceMap.emplace(sse, instancePath);
            return instancePath;
        }
#else
        AL::maya::utils::guid uuid;
        fn.uuid().get(uuid.uuid);
        auto iterInst = m_instanceMap.find(uuid);
        if (iterInst == m_instanceMap.end()) {
            MDagPath dagPath;
            fn.getPath(dagPath);
            SdfPath instancePath = makeMasterPath(m_instancesPrim, dagPath);
            m_instanceMap.emplace(uuid, instancePath);
            return instancePath;
        }
#endif
        return iterInst->second;
    }

    inline bool setStage(UsdStageRefPtr ptr)
    {
        m_stage = ptr;
        return m_stage;
    }

    inline UsdStageRefPtr stage() { return m_stage; }

    void setAnimationFrame(double minFrame, double maxFrame)
    {
        m_stage->SetStartTimeCode(minFrame);
        m_stage->SetEndTimeCode(maxFrame);
    }

    void setDefaultPrimIfOnlyOneRoot(SdfPath defaultPrim)
    {
        UsdPrim psuedo = m_stage->GetPseudoRoot();
        auto    children = psuedo.GetChildren();
        auto    first = children.begin();
        if (first != children.end()) {
            auto next = first;
            ++next;
            if (next == children.end()) {
                // if we get here, there is only one prim at the root level.
                // set that prim as the default prim.
                m_stage->SetDefaultPrim(*first);
            }
        }
        if (!m_stage->HasDefaultPrim() && !defaultPrim.IsEmpty()) {
            m_stage->SetDefaultPrim(m_stage->GetPrimAtPath(defaultPrim));
        }
    }

    void filterSample()
    {
        std::vector<double> timeSamples;
        std::vector<double> dupSamples;
        for (auto prim : m_stage->Traverse()) {
            std::vector<UsdAttribute> attributes = prim.GetAuthoredAttributes();
            for (auto attr : attributes) {
                timeSamples.clear();
                dupSamples.clear();
                attr.GetTimeSamples(&timeSamples);
                VtValue prevSampleBlob;
                for (auto sample : timeSamples) {
                    VtValue currSampleBlob;
                    attr.Get(&currSampleBlob, sample);
                    if (prevSampleBlob == currSampleBlob) {
                        dupSamples.emplace_back(sample);
                    } else {
                        prevSampleBlob = currSampleBlob;
                        // only clear samples between constant segment
                        if (dupSamples.size() > 1) {
                            dupSamples.pop_back();
                            for (auto dup : dupSamples) {
                                attr.ClearAtTime(dup);
                            }
                        }
                        dupSamples.clear();
                    }
                }
                for (auto dup : dupSamples) {
                    attr.ClearAtTime(dup);
                }
            }
        }
    }

    void
    doExport(const char* const filename, bool toFilter = false, SdfPath defaultPrim = SdfPath())
    {
        setDefaultPrimIfOnlyOneRoot(defaultPrim);
        if (toFilter) {
            filterSample();
        }
        m_stage->GetRootLayer()->Save();
        m_nodeMap.clear();
    }

    inline UsdPrim instancesPrim() { return m_instancesPrim; }

    void createInstancesPrim()
    {
        m_instancesPrim = m_stage->OverridePrim(SdfPath("/MayaExportedInstanceSources"));
    }

    void processInstances()
    {
        if (!m_instancesPrim)
            return;
        if (!m_instancesPrim.GetAllChildren()) {
            m_stage->RemovePrim(m_instancesPrim.GetPrimPath());
        } else {
            m_instancesPrim.SetSpecifier(SdfSpecifierOver);
        }
    }

    void addSelection(MDagPath parentDagPath, const std::string& path)
    {
        const std::string parent(parentDagPath.fullPathName().asChar());
        m_selectionMap[parent].emplace(path);
        // Add all of the parent paths
        m_parentsMap.emplace(parent);
        while (parentDagPath.pop() == MStatus::kSuccess && parentDagPath.isValid()) {
            std::string pathStr(parentDagPath.fullPathName().asChar());
            if (m_parentsMap.find(pathStr) != m_parentsMap.end()) {
                // Parent path has been added
                return;
            }
            m_parentsMap.emplace(pathStr);
        }
    }

    bool isPathExcluded(MDagPath dagPath) const
    {
        // Excluded path is:
        // 1) path itself is not selected; AND
        // 2) none of any ascendants or descendants are selected

        const std::string pathStr(dagPath.fullPathName().asChar());
        if (dagPath.pop() != MStatus::kSuccess || !dagPath.isValid()) {
            // Path is likely the root level node
            return false;
        }
        // Check if the target path itself is selected or not
        auto it = m_selectionMap.find(dagPath.fullPathName().asChar());
        if (it != m_selectionMap.cend()) {
            // Found parent path in cache map, either target path or any of its siblings
            // is selected
            if (it->second.find(pathStr) == it->second.cend()) {
                // One of the target path's sibling is selected, do further checking
                // to see if any of the children is select or not.
                // If not found, this target path should be excluded.
                return m_parentsMap.find(pathStr) == m_parentsMap.cend();
            }
            // Reaching here means the target path itself is selected
        }
        return false;
    }

private:
#if AL_UTILS_ENABLE_SIMD
    std::map<i128, MObject, AL::maya::utils::guid_compare> m_nodeMap;
    std::map<i128, SdfPath, AL::maya::utils::guid_compare> m_instanceMap;
#else
    std::map<AL::maya::utils::guid, MObject, AL::maya::utils::guid_compare> m_nodeMap;
    std::map<AL::maya::utils::guid, SdfPath, AL::maya::utils::guid_compare> m_instanceMap;
#endif
    std::unordered_map<std::string, std::unordered_set<std::string>> m_selectionMap;
    std::unordered_set<std::string>                                  m_parentsMap;
    UsdStageRefPtr                                                   m_stage;
    UsdPrim                                                          m_instancesPrim;
};

static MObject g_transform_rotateAttr = MObject::kNullObj;
static MObject g_transform_translateAttr = MObject::kNullObj;
static MObject g_handle_startJointAttr = MObject::kNullObj;
static MObject g_effector_handleAttr = MObject::kNullObj;
static MObject g_geomConstraint_targetAttr = MObject::kNullObj;

//----------------------------------------------------------------------------------------------------------------------
Export::Export(const ExporterParams& params)
    : m_params(params)
    , m_impl(new Export::Impl)
    , m_translatorManufacture(nullptr)
{
    if (g_transform_rotateAttr == MObject::kNullObj) {
        MNodeClass nct("transform");
        MNodeClass nch("ikHandle");
        MNodeClass nce("ikEffector");
        MNodeClass ngc("geometryConstraint");
        g_transform_rotateAttr = nct.attribute("r");
        g_transform_translateAttr = nct.attribute("t");
        g_handle_startJointAttr = nch.attribute("hsj");
        g_effector_handleAttr = nce.attribute("hp");
        g_geomConstraint_targetAttr = ngc.attribute("tg");
    }

    if (params.m_activateAllTranslators) {
        m_translatorManufacture.activateAll();
    } else {
        m_translatorManufacture.deactivateAll();
    }

    if (!params.m_activePluginTranslators.empty()) {
        m_translatorManufacture.activate(params.m_activePluginTranslators);
    }
    if (!params.m_inactivePluginTranslators.empty()) {
        m_translatorManufacture.deactivate(params.m_inactivePluginTranslators);
    }

    if (m_impl->setStage(UsdStage::CreateNew(m_params.m_fileName.asChar()))) {
        doExport();
    }
}

//----------------------------------------------------------------------------------------------------------------------
Export::~Export() { delete m_impl; }

//----------------------------------------------------------------------------------------------------------------------
UsdPrim Export::exportMeshUV(MDagPath path, const SdfPath& usdPath)
{
    UsdPrim overPrim = m_impl->stage()->OverridePrim(usdPath);
    MStatus status;
    MFnMesh fnMesh(path, &status);
    AL_MAYA_CHECK_ERROR2(
        status, MString("unable to attach function set to mesh") + path.fullPathName());
    if (status == MStatus::kSuccess) {
        UsdGeomMesh                           mesh(overPrim);
        AL::usdmaya::utils::MeshExportContext context(path, mesh, UsdTimeCode::Default(), false);
        context.copyUvSetData();
    }
    return overPrim;
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim Export::exportAssembly(MDagPath path, const SdfPath& usdPath)
{
    UsdGeomXform mesh = UsdGeomXform::Define(m_impl->stage(), usdPath);
    if (!m_params.m_mergeTransforms)
        mesh.GetPrim().SetMetadata<TfToken>(
            AL::usdmaya::Metadata::mergedTransform, AL::usdmaya::Metadata::unmerged);
    return mesh.GetPrim();
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim Export::exportPluginLocatorNode(MDagPath path, const SdfPath& usdPath)
{
    UsdGeomXform mesh = UsdGeomXform::Define(m_impl->stage(), usdPath);
    if (!m_params.m_mergeTransforms)
        mesh.GetPrim().SetMetadata<TfToken>(
            AL::usdmaya::Metadata::mergedTransform, AL::usdmaya::Metadata::unmerged);
    return mesh.GetPrim();
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim Export::exportPluginShape(MDagPath path, const SdfPath& usdPath)
{
    UsdGeomXform mesh = UsdGeomXform::Define(m_impl->stage(), usdPath);
    if (!m_params.m_mergeTransforms)
        mesh.GetPrim().SetMetadata<TfToken>(
            AL::usdmaya::Metadata::mergedTransform, AL::usdmaya::Metadata::unmerged);
    return mesh.GetPrim();
}

//----------------------------------------------------------------------------------------------------------------------
void Export::exportGeometryConstraint(MDagPath constraintPath, const SdfPath& usdPath)
{
    auto animTranslator = m_params.m_animTranslator;
    if (!animTranslator) {
        return;
    }

    MPlug plug(constraintPath.node(), g_geomConstraint_targetAttr);
    for (uint32_t i = 0, n = plug.numElements(); i < n; ++i) {
        MPlug      geom = plug.elementByLogicalIndex(i).child(0);
        MPlugArray connected;
        geom.connectedTo(connected, true, true);
        if (connected.length()) {
            MPlug      inputGeom = connected[0];
            MFnDagNode fn(inputGeom.node());
            MDagPath   geomPath;
            fn.getPath(geomPath);
            if (AnimationTranslator::isAnimatedMesh(geomPath)) {
                auto stage = m_impl->stage();

                // move to the constrained node
                constraintPath.pop();

                SdfPath newPath = usdPath.GetParentPath();

                UsdPrim prim = stage->GetPrimAtPath(newPath);
                if (prim) {
                    UsdGeomXform                xform(prim);
                    bool                        reset;
                    std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&reset);
                    for (auto op : ops) {
                        const TransformOperation thisOp = xformOpToEnum(op.GetBaseName());
                        if (thisOp == kTranslate) {
                            animTranslator->forceAddPlug(
                                MPlug(constraintPath.node(), g_transform_translateAttr),
                                op.GetAttr());
                            break;
                        }
                    }
                    return;
                } else {
                    std::cout << "prim not valid " << newPath.GetText() << std::endl;
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void Export::exportIkChain(MDagPath effectorPath, const SdfPath& usdPath)
{
    auto animTranslator = m_params.m_animTranslator;
    if (!animTranslator) {
        return;
    }

    MPlug hp(effectorPath.node(), g_effector_handleAttr);
    hp = hp.elementByLogicalIndex(0);
    MPlugArray connected;
    hp.connectedTo(connected, true, true);
    if (connected.length()) {
        // grab the handle node
        MObject handleObj = connected[0].node();

        MPlug translatePlug(handleObj, g_transform_translateAttr);

        // if the translation values on the ikHandle is animated, then we can assume the rotation
        // values on the joint chain between the effector and the start joint will also be animated
        bool handleIsAnimated = AnimationTranslator::isAnimated(translatePlug, true);
        if (handleIsAnimated) {
            // locate the start joint in the chain
            MPlug      startJoint(handleObj, g_handle_startJointAttr);
            MPlugArray connected;
            startJoint.connectedTo(connected, true, true);
            if (connected.length()) {
                // this will be the top chain in the system
                MObject startNode = connected[0].node();

                auto    stage = m_impl->stage();
                SdfPath newPath = usdPath;

                UsdPrim prim;
                // now step up from the effector to the start joint and output the rotations
                do {
                    // no point handling the effector
                    effectorPath.pop();
                    newPath = newPath.GetParentPath();

                    prim = stage->GetPrimAtPath(newPath);
                    if (prim) {
                        UsdGeomXform xform(prim);
                        MPlug        rotatePlug(effectorPath.node(), g_transform_rotateAttr);
                        bool         reset;
                        std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&reset);
                        for (auto op : ops) {
                            const float radToDeg = 180.0f / 3.141592654f;
                            bool        added = false;
                            switch (op.GetOpType()) {
                            case UsdGeomXformOp::TypeRotateXYZ:
                            case UsdGeomXformOp::TypeRotateXZY:
                            case UsdGeomXformOp::TypeRotateYXZ:
                            case UsdGeomXformOp::TypeRotateYZX:
                            case UsdGeomXformOp::TypeRotateZXY:
                            case UsdGeomXformOp::TypeRotateZYX:
                                added = true;
                                animTranslator->forceAddPlug(rotatePlug, op.GetAttr(), radToDeg);
                                break;
                            default: break;
                            }
                            if (added)
                                break;
                        }
                    } else {
                        std::cout << "prim not valid" << std::endl;
                    }
                } while (effectorPath.node() != startNode);
            }
        }
    }
    return;
}

//----------------------------------------------------------------------------------------------------------------------
void Export::copyTransformParams(UsdPrim prim, MFnTransform& fnTransform, bool exportInWorldSpace)
{

    translators::TransformTranslator::copyAttributes(
        fnTransform.object(), prim, m_params, fnTransform.dagPath(), exportInWorldSpace);
    if (m_params.m_dynamicAttributes) {
        translators::DgNodeTranslator::copyDynamicAttributes(
            fnTransform.object(), prim, m_params.m_animTranslator);
    }

    // handle the special case of exporting
    {
        auto dataPlugins = m_translatorManufacture.getExtraDataPlugins(fnTransform.object());
        for (auto dataPlugin : dataPlugins) {
            if (dataPlugin->getFnType() == MFn::kTransform) {
                dataPlugin->exportObject(prim, fnTransform.object(), ExporterParams());
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
SdfPath Export::determineUsdPath(MDagPath path, const SdfPath& usdPath, ReferenceType refType)
{
    switch (refType) {
    case kNoReference: {
        return usdPath;
    } break;

    case kShapeReference: {
        return makeMasterPath(m_impl->instancesPrim(), path);
    } break;

    case kTransformReference: {
        UsdStageRefPtr stage = m_impl->stage();
        SdfPath masterTransformPath = makeMasterPath(m_impl->instancesPrim(), getParentPath(path));
        if (!stage->GetPrimAtPath(masterTransformPath)) {
            auto parentPrim = UsdGeomXform::Define(stage, masterTransformPath);
            if (!m_params.m_mergeTransforms)
                parentPrim.GetPrim().SetMetadata<TfToken>(
                    AL::usdmaya::Metadata::mergedTransform, AL::usdmaya::Metadata::unmerged);
        }
        TfToken shapeName(MFnDagNode(path).name().asChar());
        return masterTransformPath.AppendChild(shapeName);
    } break;
    }
    return SdfPath();
}

//----------------------------------------------------------------------------------------------------------------------
void Export::addReferences(
    MDagPath       shapePath,
    MFnTransform&  fnTransform,
    SdfPath&       usdPath,
    const SdfPath& instancePath,
    ReferenceType  refType,
    bool           exportInWorldSpace)
{
    UsdStageRefPtr stage = m_impl->stage();
    if (refType == kShapeReference) {
        if (shapePath.node().hasFn(MFn::kMesh)) {
            UsdGeomMesh::Define(stage, usdPath);
        } else if (shapePath.node().hasFn(MFn::kNurbsCurve)) {
            UsdGeomNurbsCurves::Define(stage, usdPath);
        }
    }
    UsdPrim usdPrim = stage->GetPrimAtPath(usdPath);
    if (usdPrim) {
        // usd only allows instanceable on transform prim
        switch (refType) {
        case kTransformReference: {
            usdPrim.SetInstanceable(true);
        } break;

        case kShapeReference: {
            copyTransformParams(usdPrim, fnTransform, exportInWorldSpace);
        } break;

        default: break;
        }
        usdPrim.GetReferences().AddReference(SdfReference("", instancePath));
    }
}

//----------------------------------------------------------------------------------------------------------------------
bool Export::isPrimDefined(SdfPath& usdPath)
{
    UsdPrim transformPrim = m_impl->stage()->GetPrimAtPath(usdPath);
    return (transformPrim && transformPrim.IsDefined());
}

//----------------------------------------------------------------------------------------------------------------------
void Export::exportShapesCommonProc(
    MDagPath            shapePath,
    MFnTransform&       fnTransform,
    SdfPath&            usdPath,
    const ReferenceType refType,
    bool                exportInWorldSpace)
{
    UsdPrim transformPrim;

    bool                                       copyTransform = true;
    MFnDagNode                                 dgNode(shapePath);
    translators::TranslatorManufacture::RefPtr translatorPtr
        = m_translatorManufacture.get(shapePath.node());
    if (translatorPtr) {
        SdfPath shapeUsdPath = determineUsdPath(shapePath, usdPath, refType);

        // If the prim is valid and has already been defined, we skip the leftover shapes.
        // Since that is probably the case a transform has multiple shapes and we chose to merge
        // transform. In that case, once a shape get exported we will skip the rest of shapes.
        if (isPrimDefined(shapeUsdPath)) {
            TF_WARN(
                "The usd prim: %s has already been defined, will skip exporting the shape: %s",
                shapeUsdPath.GetText(),
                shapePath.partialPathName().asChar());
            return;
        }

        transformPrim
            = translatorPtr->exportObject(m_impl->stage(), shapePath, shapeUsdPath, m_params);
        auto dataPlugins = m_translatorManufacture.getExtraDataPlugins(shapePath.node());
        for (auto dataPlugin : dataPlugins) {
            dataPlugin->exportObject(transformPrim, shapePath.node(), m_params);
        }

        copyTransform = (refType == kNoReference);
    } else // no translator register for this Maya type
    {
        // As same reason above, for multiple shapes transform, once there's a shape exported, we
        // ignore the leftovers:
        if (isPrimDefined(usdPath)) {
            TF_WARN(
                "The usd prim: %s has already been defined, will skip exporting the shape: %s",
                usdPath.GetText(),
                shapePath.partialPathName().asChar());
            return;
        }

        if (shapePath.node().hasFn(MFn::kAssembly)) {
            transformPrim = exportAssembly(shapePath, usdPath);
        } else if (shapePath.node().hasFn(MFn::kPluginLocatorNode)) {
            transformPrim = exportPluginLocatorNode(shapePath, usdPath);
        } else if (shapePath.node().hasFn(MFn::kPluginShape)) {
            transformPrim = exportPluginShape(shapePath, usdPath);
        }
    }

    // if we haven't created a transform for this shape (possible if we chose not to export it)
    // create a transform shape for the prim.
    if (!transformPrim) {
        UsdGeomXform xform = UsdGeomXform::Define(m_impl->stage(), usdPath);
        transformPrim = xform.GetPrim();
    }

    if (m_params.m_mergeTransforms && copyTransform) {
        copyTransformParams(transformPrim, fnTransform, exportInWorldSpace);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void Export::exportShapesOnlyUVProc(MDagPath shapePath, MFnTransform& fnTransform, SdfPath& usdPath)
{
    if (shapePath.node().hasFn(MFn::kMesh)) {
        exportMeshUV(shapePath, usdPath);
    } else {
        m_impl->stage()->OverridePrim(usdPath);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void Export::exportSceneHierarchy(MDagPath rootPath, SdfPath& defaultPrim)
{
    MDagPath parentPath = rootPath;
    parentPath.pop();
    static const TfToken xformToken("Xform");

    MItDag it(MItDag::kDepthFirst);
    it.reset(rootPath, MItDag::kDepthFirst, MFn::kTransform);

    std::function<void(MDagPath, MFnTransform&, SdfPath&, ReferenceType, bool)> exportShapeProc
        = [this](
              MDagPath      shapePath,
              MFnTransform& fnTransform,
              SdfPath&      usdPath,
              ReferenceType refType,
              bool          inWorldSpace) {
              this->exportShapesCommonProc(shapePath, fnTransform, usdPath, refType, inWorldSpace);
          };

    std::function<bool(MDagPath, MFnTransform&, SdfPath&, bool)> exportTransformFunc
        = [this](
              MDagPath      transformPath,
              MFnTransform& fnTransform,
              SdfPath&      usdPath,
              bool          inWorldSpace) {
              SdfPath path;
              if (inWorldSpace) {
                  const std::string& pathName = usdPath.GetString();
                  size_t             s = pathName.find_last_of('/');
                  path = SdfPath(pathName.data() + s);
              } else {
                  path = usdPath;
              }

              bool                                       exportKids = true;
              translators::TranslatorManufacture::RefPtr translatorPtr
                  = m_translatorManufacture.get(transformPath.node());
              if (translatorPtr) {
                  UsdPrim transformPrim
                      = translatorPtr->exportObject(m_impl->stage(), transformPath, path, m_params);
                  this->copyTransformParams(transformPrim, fnTransform, inWorldSpace);
                  exportKids = translatorPtr->exportDescendants();
              } else {

                  UsdGeomXform xform = UsdGeomXform::Define(m_impl->stage(), path);
                  UsdPrim      transformPrim = xform.GetPrim();
                  this->copyTransformParams(transformPrim, fnTransform, inWorldSpace);
              }
              return exportKids;
          };

    // choose right proc required by meshUV option;
    if (m_params.getBool("Mesh UV Only")) {
        exportShapeProc = [this](
                              MDagPath      shapePath,
                              MFnTransform& fnTransform,
                              SdfPath&      usdPath,
                              ReferenceType refType,
                              bool          inWorldSpace) {
            this->exportShapesOnlyUVProc(shapePath, fnTransform, usdPath);
        };
        exportTransformFunc = [this](
                                  MDagPath      transformPath,
                                  MFnTransform& fnTransform,
                                  SdfPath&      usdPath,
                                  bool          inWorldSpace) {
            const std::string& pathName = usdPath.GetString();
            size_t             s = pathName.find_last_of('/');
            SdfPath            path(pathName.data() + s);
            m_impl->stage()->OverridePrim(path);
            return true;
        };
    }

    MFnTransform fnTransform;
    bool         exportInWorldSpace = m_params.m_exportInWorldSpace;
    // loop through transforms only
    while (!it.isDone()) {
        // assign transform function set
        MDagPath transformPath;
        it.getPath(transformPath);

        // Check if any sibling is selected, exclude this node if not selected
        if (m_impl->isPathExcluded(transformPath)) {
            it.prune();
            it.next();
            continue;
        }

        if (exportInWorldSpace && !(transformPath == rootPath)) {
            exportInWorldSpace = false;
        }

        fnTransform.setObject(transformPath);

        // Make sure we haven't seen this transform before.
        bool transformHasBeenExported = m_impl->contains(fnTransform);
        if (transformHasBeenExported) {
            // "rootPath" is the user selected node but if it has been exported,
            // that means user selected a sub node under another selected node.
            if (transformPath == rootPath) {
                it.prune();
                it.next();
                continue;
            }
            // We have an instanced shape!
            std::cout << "encountered transform instance " << fnTransform.fullPathName().asChar()
                      << std::endl;
        }

        if (!transformHasBeenExported || m_params.m_duplicateInstances) {
            // generate a USD path from the current path
            SdfPath usdPath;

            // we should take a look to see whether the name was changed on import.
            // If it did change, make sure we save it using the original name, and not the new one.
            MStatus status;
            MPlug   originalNamePlug = fnTransform.findPlug("alusd_originalPath", &status);
            if (!status) {
                usdPath = makeUsdPath(parentPath, transformPath);
            }

            // for UV only exporting, record first prim as default
            if (m_params.getBool("Mesh UV Only") && defaultPrim.IsEmpty()) {
                defaultPrim = usdPath;
            }

            if (transformPath.node().hasFn(MFn::kIkEffector)) {
                exportIkChain(transformPath, usdPath);
            } else if (transformPath.node().hasFn(MFn::kGeometryConstraint)) {
                exportGeometryConstraint(transformPath, usdPath);
            }

            // how many shapes are directly under this transform path?
            uint32_t numShapes;
            transformPath.numberOfShapesDirectlyBelow(numShapes);

            if (!m_params.m_mergeTransforms && !exportInWorldSpace) {
                bool exportKids
                    = exportTransformFunc(transformPath, fnTransform, usdPath, exportInWorldSpace);
                UsdPrim prim = m_impl->stage()->GetPrimAtPath(usdPath);
                prim.SetMetadata<TfToken>(
                    AL::usdmaya::Metadata::mergedTransform, AL::usdmaya::Metadata::unmerged);
                if (!exportKids) {
                    it.prune();
                }
            }

            if (numShapes) {
                // This is a slight annoyance about the way that USD has no concept of
                // shapes (it merges shapes into transforms usually). This means if we have
                // 1 transform, with 4 shapes parented underneath, it means we'll end up with
                // the transform data duplicated four times.

                ReferenceType refType = kNoReference;

                for (uint32_t j = 0; j < numShapes; ++j) {
                    MDagPath shapePath = transformPath;
                    shapePath.extendToShapeDirectlyBelow(j);
                    MFnDagNode shapeDag(shapePath);
                    SdfPath    shapeUsdPath = usdPath;

                    if (!m_params.m_mergeTransforms) {
                        fnTransform.setObject(shapePath);
                        shapeUsdPath = makeUsdPath(parentPath, shapePath);
                    }

                    bool shapeNotYetExported = !m_impl->contains(shapePath.node());
                    bool shapeInstanced = shapePath.isInstanced();
                    if (shapeNotYetExported || m_params.m_duplicateInstances) {
                        // if the path has a child shape, process the shape now
                        if (!m_params.m_duplicateInstances && shapeInstanced) {
                            refType = m_params.m_mergeTransforms ? kShapeReference
                                                                 : kTransformReference;
                        }
                        exportShapeProc(
                            shapePath, fnTransform, shapeUsdPath, refType, exportInWorldSpace);
                    } else {
                        refType
                            = m_params.m_mergeTransforms ? kShapeReference : kTransformReference;
                    }

                    if (refType == kShapeReference) {
                        SdfPath instancePath = m_impl->getMasterPath(shapeDag);
                        addReferences(
                            shapePath,
                            fnTransform,
                            shapeUsdPath,
                            instancePath,
                            refType,
                            exportInWorldSpace);
                    } else if (refType == kTransformReference) {
                        SdfPath instancePath
                            = m_impl->getMasterPath(MFnDagNode(shapeDag.parent(0)));
                        addReferences(
                            shapePath,
                            fnTransform,
                            usdPath,
                            instancePath,
                            refType,
                            exportInWorldSpace);
                    }
                }
            } else {
                if (m_params.m_mergeTransforms) {
                    if (!exportTransformFunc(
                            transformPath, fnTransform, usdPath, exportInWorldSpace)) {
                        it.prune();
                    }
                }
            }
        } else {
            // We have an instanced transform
            // How do we reference that here?
        }

        it.next();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void Export::doExport()
{
    auto context = translators::TranslatorContext::create(nullptr);
    translators::TranslatorManufacture::preparePythonTranslators(context);

    // make sure the node factory has been initialised as least once prior to use
    getNodeFactory();

    const MTime oldCurTime = MAnimControl::currentTime();
    if (m_params.m_animTranslator) {
        // try to ensure that we have some sort of consistent output for each run by forcing the
        // export to the first frame
        MAnimControl::setCurrentTime(m_params.m_minFrame);
    }

    if (!m_params.m_duplicateInstances) {
        m_impl->createInstancesPrim();
    }

    const MSelectionList& sl = m_params.m_nodes;
    SdfPath               defaultPrim;

    // Use std::map to keep the same order regardless of the selection order from Maya
    // This makes sure we process the objects from top to bottom
    std::map<std::string, MDagPath> selectedPaths;
    for (uint32_t i = 0, n = sl.length(); i < n; ++i) {
        MDagPath path;
        if (sl.getDagPath(i, path)) {
            std::string pathStr;
            if (path.node().hasFn(MFn::kTransform)) {
                pathStr = path.fullPathName().asChar();
            } else if (path.node().hasFn(MFn::kShape)) {
                path.pop();
                pathStr = path.fullPathName().asChar();
            } else {
                continue;
            }
            selectedPaths.emplace(pathStr, path);
            // Store direct parent path
            if (path.pop() == MStatus::kSuccess && path.isValid()) {
                m_impl->addSelection(path, pathStr);
            }
        }
    }

    for (const auto& pathPair : selectedPaths) {
        exportSceneHierarchy(pathPair.second, defaultPrim);
    }

    if (m_params.m_animTranslator) {
        m_params.m_animTranslator->exportAnimation(m_params);
        m_impl->setAnimationFrame(m_params.m_minFrame, m_params.m_maxFrame);

        // return user to their original frame
        MAnimControl::setCurrentTime(oldCurTime);
    }

    m_impl->processInstances();
    m_impl->doExport(m_params.m_fileName.asChar(), m_params.m_filterSample, defaultPrim);
}

//----------------------------------------------------------------------------------------------------------------------
ExportCommand::ExportCommand()
    : m_params()
{
}

//----------------------------------------------------------------------------------------------------------------------
ExportCommand::~ExportCommand() { }

//----------------------------------------------------------------------------------------------------------------------
MStatus ExportCommand::doIt(const MArgList& args)
{
    maya::utils::OptionsParser parser;
    ExportTranslator::options().initParser(parser);
    m_params.m_parser = &parser;
    PluginTranslatorOptionsInstance pluginInstance(ExportTranslator::pluginContext());
    parser.setPluginOptionsContext(&pluginInstance);

    MStatus      status;
    MArgDatabase argData(syntax(), args, &status);
    AL_MAYA_CHECK_ERROR(status, "ALUSDExport: failed to match arguments");
    AL_MAYA_COMMAND_HELP(argData, g_helpText);

    // fetch filename and ensure it's valid
    if (!argData.isFlagSet("f", &status)) {
        MGlobal::displayError("ALUSDExport: \"file\" argument must be set");
        return MS::kFailure;
    }
    AL_MAYA_CHECK_ERROR(
        argData.getFlagArgument("f", 0, m_params.m_fileName),
        "ALUSDExport: Unable to fetch \"file\" argument");
    if (argData.isFlagSet("sl", &status)) {
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("sl", 0, m_params.m_selected),
            "ALUSDExport: Unable to fetch \"selected\" argument");
    }
    if (argData.isFlagSet("da", &status)) {
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("da", 0, m_params.m_dynamicAttributes),
            "ALUSDExport: Unable to fetch \"dynamic\" argument");
    }
    if (argData.isFlagSet("di", &status)) {
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("di", 0, m_params.m_duplicateInstances),
            "ALUSDExport: Unable to fetch \"duplicateInstances\" argument");
    }
    if (argData.isFlagSet("m", &status)) {
        bool option;
        argData.getFlagArgument("m", 0, option);
        m_params.setBool("Meshes", option);
    }
    if (argData.isFlagSet("uvs", &status)) {
        bool option;
        argData.getFlagArgument("uvs", 0, option);
        m_params.setBool("Mesh UVs", option);
    }
    if (argData.isFlagSet("uvo", &status)) {
        bool option;
        argData.getFlagArgument("uvo", 0, option);
        m_params.setBool("Mesh UV Only", option);
    }
    if (argData.isFlagSet("pr", &status)) {
        bool option;
        argData.getFlagArgument("pr", 0, option);
        m_params.setBool("Mesh Points as PRef", option);
    }

    if (argData.isFlagSet("luv", &status)) {
        MGlobal::displayWarning("-luv flag is deprecated in AL_usdmaya_ExportCommand\n");
    }
    if (argData.isFlagSet("mt", &status)) {
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("mt", 0, m_params.m_mergeTransforms),
            "ALUSDExport: Unable to fetch \"merge transforms\" argument");
    }
#if MAYA_APP_VERSION > 2019
    if (argData.isFlagSet("mom", &status)) {
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("mom", 0, m_params.m_mergeOffsetParentMatrix),
            "ALUSDExport: Unable to fetch \"merge offset parent matrix\" argument");
    }
#endif
    if (argData.isFlagSet("nc", &status)) {
        bool option;
        argData.getFlagArgument("nc", 0, option);
        m_params.setBool("Nurbs Curves", option);
    }

    if (argData.isFlagSet("fr", &status)) {
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("fr", 0, m_params.m_minFrame),
            "ALUSDExport: Unable to fetch \"frame range\" argument");
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("fr", 1, m_params.m_maxFrame),
            "ALUSDExport: Unable to fetch \"frame range\" argument");
        m_params.m_animation = true;
    } else if (argData.isFlagSet("ani", &status)) {
        m_params.m_animation = true;
        m_params.m_minFrame = MAnimControl::minTime().value();
        m_params.m_maxFrame = MAnimControl::maxTime().value();
    }

    if (argData.isFlagSet("ss", &status)) {
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("ss", 0, m_params.m_subSamples),
            "ALUSDExport: Unable to fetch \"sub samples\" argument");
    }

    if (argData.isFlagSet("fs", &status)) {
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("fs", 0, m_params.m_filterSample),
            "ALUSDExport: Unable to fetch \"filter sample\" argument");
    }
    if (argData.isFlagSet("eac", &status)) {
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("eac", 0, m_params.m_extensiveAnimationCheck),
            "ALUSDExport: Unable to fetch \"extensive animation check\" argument");
    }
    if (argData.isFlagSet("ws", &status)) {
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("ws", 0, m_params.m_exportInWorldSpace),
            "ALUSDExport: Unable to fetch \"world space\" argument");
    }
    if (argData.isFlagSet("opt", &status)) {
        MString optionString;
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("opt", 0, optionString),
            "ALUSDExport: Unable to fetch \"options\" argument");
        parser.parse(optionString);
    }
    if (m_params.m_animation) {
        m_params.m_animTranslator = new AnimationTranslator;
    }

    m_params.m_activateAllTranslators = true;
    bool eat = argData.isFlagSet("eat", &status);
    bool dat = argData.isFlagSet("dat", &status);
    if (eat && dat) {
        MGlobal::displayError("ALUSDExport: cannot enable all translators, AND disable all "
                              "translators, at the same time");
    } else if (dat) {
        m_params.m_activateAllTranslators = false;
    }

    if (argData.isFlagSet("ept", &status)) {
        MString arg;
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("ept", 0, arg),
            "ALUSDExport: Unable to fetch \"enablePluginTranslators\" argument");
        MStringArray strings;
        arg.split(',', strings);
        for (uint32_t i = 0, n = strings.length(); i < n; ++i) {
            m_params.m_activePluginTranslators.emplace_back(strings[i].asChar());
        }
    }

    if (argData.isFlagSet("dpt", &status)) {
        MString arg;
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("dpt", 0, arg),
            "ALUSDExport: Unable to fetch \"disablePluginTranslators\" argument");
        MStringArray strings;
        arg.split(',', strings);
        for (uint32_t i = 0, n = strings.length(); i < n; ++i) {
            m_params.m_inactivePluginTranslators.emplace_back(strings[i].asChar());
        }
    }

    return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ExportCommand::redoIt()
{
    static const std::unordered_set<std::string> ignoredNodes { "persp", "front", "top", "side" };

    if (m_params.m_selected) {
        MGlobal::getActiveSelectionList(m_params.m_nodes);
    } else {
        MDagPath path;
        MItDag   it(MItDag::kDepthFirst, MFn::kTransform);
        while (!it.isDone()) {
            it.getPath(path);
            const MString     name = path.partialPathName();
            const std::string s(name.asChar(), name.length());
            if (ignoredNodes.find(s) == ignoredNodes.end()) {
                m_params.m_nodes.add(path);
            }
            it.prune();
            it.next();
        }
    }

    Export exporter(m_params);
    delete m_params.m_animTranslator;

    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ExportCommand::undoIt() { return MS::kSuccess; }

//----------------------------------------------------------------------------------------------------------------------
MSyntax ExportCommand::createSyntax()
{
    const char* const errorString = "ALUSDExport: failed to create syntax";

    MStatus status;
    MSyntax syntax;
    status = syntax.addFlag("-f", "-file", MSyntax::kString);
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag("-sl", "-selected", MSyntax::kBoolean);
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag("-da", "-dynamic", MSyntax::kBoolean);
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag("-m", "-meshes", MSyntax::kBoolean);
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag(
        "-uvs",
        "-meshUVS",
        MSyntax::kBoolean); // If this is on, we export UV data beside normal data.
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag(
        "-uvo",
        "-meshUVOnly",
        MSyntax::kBoolean); // when this is on, only overs contains UV are exported.
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag(
        "-pr",
        "-meshPointsPref",
        MSyntax::kBoolean); // when this is on, only overs contains UV are exported.
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag("-luv", "-leftHandedUV", MSyntax::kBoolean);
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag("-nc", "-nurbsCurves", MSyntax::kBoolean);
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag("-di", "-duplicateInstances", MSyntax::kBoolean);
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag("-mt", "-mergeTransforms", MSyntax::kBoolean);
    AL_MAYA_CHECK_ERROR2(status, errorString);
#if MAYA_APP_VERSION > 2019
    status = syntax.addFlag("-mom", "-mergeOffsetParentMatrix", MSyntax::kBoolean);
    AL_MAYA_CHECK_ERROR2(status, errorString);
#endif
    status = syntax.addFlag("-ani", "-animation", MSyntax::kNoArg);
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag("-fr", "-frameRange", MSyntax::kDouble, MSyntax::kDouble);
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag("-fs", "-filterSample", MSyntax::kBoolean);
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag("-eac", "-extensiveAnimationCheck", MSyntax::kBoolean);
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag("-ss", "-subSamples", MSyntax::kUnsigned);
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag("-ws", "-worldSpace", MSyntax::kBoolean);
    AL_MAYA_CHECK_ERROR2(status, errorString);
    status = syntax.addFlag("-opt", "-options", MSyntax::kString);
    AL_MAYA_CHECK_ERROR2(status, errorString);
    AL_MAYA_CHECK_ERROR2(
        syntax.addFlag("-eat", "-enableAllTranslators", MSyntax::kNoArg), errorString);
    AL_MAYA_CHECK_ERROR2(
        syntax.addFlag("-dat", "-disableAllTranslators", MSyntax::kNoArg), errorString);
    AL_MAYA_CHECK_ERROR2(
        syntax.addFlag("-ept", "-enablePluginTranslators", MSyntax::kString), errorString);
    AL_MAYA_CHECK_ERROR2(
        syntax.addFlag("-dpt", "-disablePluginTranslators", MSyntax::kString), errorString);
    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
const char* const ExportCommand::g_helpText = R"(
ExportCommand Overview:

  This command will export your maya scene into the USD format. If you want the export to happen from
  a certain point in the hierarchy then select the node in maya and pass the parameter selected=True, otherwise
  it will export from the root of the scene.

  If you want to export keeping the time sampled data, you can do so by passing these flags
    1. AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -animation

  Exporting attributes that are dynamic attributes can be done by:
    1. AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -dynamic

  Exporting samples over a framerange can be done a few ways:
    1. AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -frameRange 0 24
    2. AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -ani

  Nurbs curves can be exported by passing the corresponding parameters:
    1. AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -nc

  The exporter can remove samples that contain the same data for adjacent samples
    1. AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -fs
)";

//----------------------------------------------------------------------------------------------------------------------
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
