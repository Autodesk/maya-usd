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
#include "AL/usdmaya/fileio/Import.h"

#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/CodeTimings.h"
#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/fileio/ImportTranslator.h"
#include "AL/usdmaya/fileio/SchemaPrims.h"
#include "AL/usdmaya/fileio/TransformIterator.h"
#include "AL/usdmaya/utils/Utils.h"

#include <pxr/usd/usd/stage.h>

#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MFnTransform.h>
#include <maya/MSyntax.h>
#include <maya/MTime.h>

namespace AL {
namespace usdmaya {
namespace fileio {

using AL::maya::utils::PluginTranslatorOptions;
using AL::maya::utils::PluginTranslatorOptionsContext;
using AL::maya::utils::PluginTranslatorOptionsContextManager;
using AL::maya::utils::PluginTranslatorOptionsInstance;

AL_MAYA_DEFINE_COMMAND(ImportCommand, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
Import::Import(const ImporterParams& params)
    : m_params(params)
    , m_success(false)
{
    doImport();
}

//----------------------------------------------------------------------------------------------------------------------
Import::~Import() { }

//----------------------------------------------------------------------------------------------------------------------
MObject Import::createParentTransform(
    const UsdPrim&                     prim,
    TransformIterator&                 it,
    translators::TranslatorManufacture manufacture)
{
    NodeFactory&             factory = getNodeFactory();
    fileio::SchemaPrimsUtils utils(manufacture);

    MObject     parent = it.parent();
    const char* transformType = "transform";
    std::string ttype;
    prim.GetMetadata(Metadata::transformType, &ttype);
    if (!ttype.empty()) {
        transformType = ttype.c_str();
    }
    TF_DEBUG(ALUSDMAYA_COMMANDS)
        .Msg(
            "Import::doImport::createParentTransform prim=%s transformType=%s\n",
            prim.GetPath().GetText(),
            transformType);
    MObject obj = factory.createNode(prim, transformType, parent);

    // handle the special case of importing custom transform params
    {
        auto dataPlugins = manufacture.getExtraDataPlugins(obj);
        for (auto dataPlugin : dataPlugins) {
            // special case
            if (dataPlugin->getFnType() == MFn::kTransform) {
                dataPlugin->import(prim, obj);
            }
        }
    }
    it.append(obj);
    return obj;
};

//----------------------------------------------------------------------------------------------------------------------
void Import::doImport()
{
    AL::usdmaya::Profiler::clearAll();

    translators::TranslatorContextPtr  context = translators::TranslatorContext::create(nullptr);
    translators::TranslatorManufacture manufacture(context);
    if (m_params.m_activateAllTranslators) {
        manufacture.activateAll();
    } else {
        manufacture.deactivateAll();
    }

    if (!m_params.m_activePluginTranslators.empty()) {
        manufacture.activate(m_params.m_activePluginTranslators);
    }
    if (!m_params.m_inactivePluginTranslators.empty()) {
        manufacture.deactivate(m_params.m_inactivePluginTranslators);
    }

    UsdStageRefPtr stage;
    if (m_params.m_rootLayer) {
        // stage = UsdStage::Open(m_params.m_rootLayer, m_params.m_sessionLayer);
    } else {
        stage = UsdStage::Open(
            m_params.m_fileName.asChar(),
            m_params.m_stageUnloaded ? UsdStage::LoadNone : UsdStage::LoadAll);
    }

    if (stage != UsdStageRefPtr()) {
        // set timeline range if animation is enabled
        if (m_params.m_animations) {
            const char* const timeError = "ALUSDImport: error setting time range";
            const MTime       startTimeCode = stage->GetStartTimeCode();
            const MTime       endTimeCode = stage->GetEndTimeCode();
            AL_MAYA_CHECK_ERROR2(MAnimControl::setMinTime(startTimeCode), timeError);
            AL_MAYA_CHECK_ERROR2(MAnimControl::setMaxTime(endTimeCode), timeError);
        }

        NodeFactory& factory = getNodeFactory();
        factory.setImportParams(&m_params);

        MFnDependencyNode fn;

        fileio::SchemaPrimsUtils utils(manufacture);

        m_nonImportablePrims.clear();
        if (!m_params.getBool("Import Meshes")) {
            m_nonImportablePrims.insert(TfToken("Mesh"));
        }
        if (!m_params.getBool("Import Curves")) {
            m_nonImportablePrims.insert(TfToken("NurbsCurves"));
        }

        std::map<SdfPath, MObject> prototypeMap;
        TransformIterator          it(stage, m_params.m_parentPath);
        // start from the assigned prim
        const std::string importPrimPath = AL::maya::utils::convert(m_params.m_primPath);
        if (!importPrimPath.empty()) {
            UsdPrim importPrim = stage->GetPrimAtPath(SdfPath(importPrimPath));
            if (importPrim) {
                it = TransformIterator(importPrim, m_params.m_parentPath);
            }
        }

        for (; !it.done(); it.next()) {
            const UsdPrim& prim = it.prim();
            if (prim.IsInstance()) {
                UsdPrim prototypePrim =
#if PXR_VERSION < 2011
                    prim.GetMaster();
#else
                    prim.GetPrototype();
#endif
                auto iter = prototypeMap.find(prototypePrim.GetPath());
                if (iter == prototypeMap.end()) {
                    MObject    mayaObject = createParentTransform(prim, it, manufacture);
                    MFnDagNode fnInstance(mayaObject);
                    fnInstance.setInstanceable(true);
                    prototypeMap.emplace(prototypePrim.GetPath(), mayaObject);
                } else {
                    MStatus status;
                    MObject instanceParent = iter->second;
                    MObject mayaObject = createParentTransform(prim, it, manufacture);

                    MFnDagNode fnParent(mayaObject, &status);
                    MFnDagNode fnInstance(instanceParent, &status);
                    if (!status)
                        status.perror("Failed to access instance parent");

                    // add each child from the instance transform, to the new transform
                    for (int i = 0; unsigned(i) < fnInstance.childCount(); ++i) {
                        MObject child = fnInstance.child(i);
                        status = fnParent.addChild(child, MFnDagNode::kNextPos, true);
                        if (!status)
                            status.perror("Failed to parent instance");
                    }

                    it.prune();
                }
            } else {
                translators::TranslatorRefPtr schemaTranslator = utils.isSchemaPrim(prim);
                if (schemaTranslator != nullptr) {
                    if (m_nonImportablePrims.find(prim.GetTypeName())
                        == m_nonImportablePrims.end()) {
                        // check merge status on parent transform (we must use the parent from the
                        // iterator!)
                        bool    parentUnmerged = false;
                        TfToken val;
                        if (it.parentPrim().GetMetadata(
                                AL::usdmaya::Metadata::mergedTransform, &val)) {
                            parentUnmerged = (val == AL::usdmaya::Metadata::unmerged);
                        }

                        MObject obj;
                        if (!parentUnmerged) {
                            obj = createParentTransform(prim, it, manufacture);
                        } else {
                            obj = it.parent();
                        }
                        MObject shape
                            = createShape(schemaTranslator, manufacture, prim, obj, parentUnmerged);

                        MFnTransform fnP(obj);
                        fnP.addChild(shape, MFnTransform::kNextPos, true);
                    }
                } else {
                    createParentTransform(prim, it, manufacture);
                }
            }
        }
    }
    m_success = true;

    std::stringstream strstr;
    strstr << "Breakdown for file: " << m_params.m_fileName << std::endl;
    AL::usdmaya::Profiler::printReport(strstr);
    MGlobal::displayInfo(AL::maya::utils::convert(strstr.str()));
}

//----------------------------------------------------------------------------------------------------------------------
MObject Import::createShape(
    translators::TranslatorRefPtr       translator,
    translators::TranslatorManufacture& manufacture,
    const UsdPrim&                      prim,
    MObject                             parent,
    bool                                parentUnmerged)
{
    MObject shapeObj;
#if PXR_VERSION < 2011
    if (prim.IsInMaster()) {
#else
    if (prim.IsInPrototype()) {
#endif
        const SdfPath& primPath = prim.GetPrimPath();
        if (m_instanceObjects.find(primPath) != m_instanceObjects.end()) {
            shapeObj = m_instanceObjects[primPath];
        } else {
            translator->import(prim, parent, shapeObj);
            NodeFactory::setupNode(prim, shapeObj, parent, true);
            m_instanceObjects[primPath] = shapeObj;
        }
    } else {
        translator->import(prim, parent, shapeObj);
        NodeFactory::setupNode(prim, shapeObj, parent, parentUnmerged);
    }

    auto dataPlugins = manufacture.getExtraDataPlugins(shapeObj);
    for (auto dataPlugin : dataPlugins) {
        // special case
        dataPlugin->import(prim, parent);
    }

    return shapeObj;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ImportCommand::doIt(const MArgList& args)
{
    maya::utils::OptionsParser parser;
    ImportTranslator::options().initParser(parser);
    m_params.m_parser = &parser;
    PluginTranslatorOptionsInstance pluginInstance(ImportTranslator::pluginContext());
    parser.setPluginOptionsContext(&pluginInstance);

    MStatus      status;
    MArgDatabase argData(syntax(), args, &status);
    AL_MAYA_CHECK_ERROR(status, "ImportCommand: failed to match arguments");

    // fetch filename and ensure it's valid
    if (!argData.isFlagSet("-f", &status)) {
        MGlobal::displayError("ImportCommand: \"file\" argument must be set");
        return MS::kFailure;
    }

    AL_MAYA_CHECK_ERROR(
        argData.getFlagArgument("-f", 0, m_params.m_fileName),
        "ImportCommand: Unable to fetch \"file\" argument");

    // check for parent path flag. Convert to MDagPath if found.
    if (argData.isFlagSet("-p", &status)) {
        MString parent;
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("-p", 0, parent),
            "ImportCommand: Unable to fetch \"parent\" argument");

        MSelectionList sl, sl2;
        MGlobal::getActiveSelectionList(sl);
        MGlobal::selectByName(parent, MGlobal::kReplaceList);
        MGlobal::getActiveSelectionList(sl2);
        MGlobal::setActiveSelectionList(sl);
        if (sl2.length()) {
            sl2.getDagPath(0, m_params.m_parentPath);
        }
    }

    if (argData.isFlagSet("-pp", &status)) {
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("-pp", 0, m_params.m_primPath),
            "ImportCommand, Unable to fetch \"primPath\" argument");
    }

    if (argData.isFlagSet("-un", &status)) {
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("-un", 0, m_params.m_stageUnloaded),
            "ImportCommand: Unable to fetch \"unloaded\" argument")
    }

    if (argData.isFlagSet("-a", &status)) {
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("-a", 0, m_params.m_animations),
            "ImportCommand: Unable to fetch \"animation\" argument");
    }

    if (argData.isFlagSet("-da", &status)) {
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("-da", 0, m_params.m_dynamicAttributes),
            "ImportCommand: Unable to fetch \"dynamicAttributes\" argument");
    }

    if (argData.isFlagSet("-m", &status)) {
        bool value = true;
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("-m", 0, value),
            "ImportCommand: Unable to fetch \"meshes\" argument");
        m_params.setBool("Import Meshes", value);
    }

    if (argData.isFlagSet("-nc", &status)) {
        bool value = true;
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("-nc", 0, value),
            "ImportCommand: Unable to fetch \"nurbs curves\" argument");
        m_params.setBool("Import Curves", value);
    }

    if (argData.isFlagSet("opt", &status)) {
        MString optionString;
        AL_MAYA_CHECK_ERROR(
            argData.getFlagArgument("opt", 0, optionString),
            "ImportCommand: Unable to fetch \"options\" argument");
        parser.parse(optionString);
    }

    if (argData.isFlagSet("-fd", &status)) {
        m_params.m_forceDefaultRead = true;
    }

    m_params.m_activateAllTranslators = true;
    bool eat = argData.isFlagSet("eat", &status);
    bool dat = argData.isFlagSet("dat", &status);
    if (eat && dat) {
        MGlobal::displayError("ImportCommand: cannot enable all translators, AND disable all "
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
MStatus ImportCommand::redoIt()
{
    Import importer(m_params);
    return importer ? MS::kSuccess : MS::kFailure;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ImportCommand::undoIt() { return MS::kSuccess; }

//----------------------------------------------------------------------------------------------------------------------
MSyntax ImportCommand::createSyntax()
{
    const char* const errorString = "ImportCommand: failed to create syntax";

    MSyntax syntax;
    AL_MAYA_CHECK_ERROR2(syntax.addFlag("-a", "-anim"), errorString);
    AL_MAYA_CHECK_ERROR2(syntax.addFlag("-f", "-file", MSyntax::kString), errorString);
    AL_MAYA_CHECK_ERROR2(syntax.addFlag("-un", "-unloaded", MSyntax::kBoolean), errorString);
    AL_MAYA_CHECK_ERROR2(syntax.addFlag("-p", "-parent", MSyntax::kString), errorString);
    AL_MAYA_CHECK_ERROR2(syntax.addFlag("-pp", "-primPath", MSyntax::kString), errorString);
    AL_MAYA_CHECK_ERROR2(
        syntax.addFlag("-da", "-dynamicAttribute", MSyntax::kBoolean), errorString);
    AL_MAYA_CHECK_ERROR2(syntax.addFlag("-m", "-meshes", MSyntax::kBoolean), errorString);
    AL_MAYA_CHECK_ERROR2(syntax.addFlag("-nc", "-nurbsCurves", MSyntax::kBoolean), errorString);
    AL_MAYA_CHECK_ERROR2(syntax.addFlag("-fd", "-forceDefaultRead", MSyntax::kNoArg), errorString);
    AL_MAYA_CHECK_ERROR2(
        syntax.addFlag("-eat", "-enableAllTranslators", MSyntax::kNoArg), errorString);
    AL_MAYA_CHECK_ERROR2(
        syntax.addFlag("-dat", "-disableAllTranslators", MSyntax::kNoArg), errorString);
    AL_MAYA_CHECK_ERROR2(
        syntax.addFlag("-ept", "-enablePluginTranslators", MSyntax::kString), errorString);
    AL_MAYA_CHECK_ERROR2(
        syntax.addFlag("-dpt", "-disablePluginTranslators", MSyntax::kString), errorString);
    syntax.makeFlagMultiUse("-arp");
    syntax.enableQuery(false);
    syntax.enableEdit(false);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
