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

#include <pxr/base/tf/pyEnum.h>
#include <pxr/pxr.h>

#include <ufe/undoableCommandMgr.h>

#include <layerEditorWidgetManager.h>
#include <LayerEditorCommands.h>
#if PXR_VERSION < 2411
#include <boost/python.hpp>
using namespace boost::python;
using noncopyable = boost::noncopyable;
#else
#include <pxr/external/boost/python.hpp>
using namespace pxr::pxr_boost::python;
using noncopyable = pxr::pxr_boost::python::noncopyable;
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

static void selectLayers(std::vector<std::string> layerIds)
{
    LayerEditorWidgetManager::getInstance()->selectLayers(layerIds);
}

static std::vector<std::string> getSelectedLayers()
{
    return LayerEditorWidgetManager::getInstance()->getSelectedLayers();
}

} // namespace UsdLayerEditor

namespace {

std::shared_ptr<UsdLayerEditor::ClearLayerCmd>
ClearLayerCommandInit(const pxr::SdfLayerRefPtr& layer)
{
    return std::make_shared<UsdLayerEditor::ClearLayerCmd>(layer);
}

std::shared_ptr<UsdLayerEditor::DiscardEditCmd>
DiscardEditsCommandInit(const pxr::SdfLayerRefPtr& layer)
{
    return std::make_shared<UsdLayerEditor::DiscardEditCmd>(layer);
}

std::shared_ptr<UsdLayerEditor::SetEditTargetCmd>
SetEditTargetCommandInit(const pxr::UsdStagePtr& stage, const pxr::SdfLayerRefPtr& layer)
{
    return std::make_shared<UsdLayerEditor::SetEditTargetCmd>(stage, layer);
}

std::shared_ptr<UsdLayerEditor::MuteLayerCmd> MuteLayerCommandInit(
    const pxr::UsdStageRefPtr& stage,
    const pxr::SdfLayerRefPtr& layer,
    bool                       muteIt)
{
    return std::make_shared<UsdLayerEditor::MuteLayerCmd>(stage, layer, muteIt);
}

std::shared_ptr<UsdLayerEditor::LockLayerCmd> LockLayerCommandInit(
    const pxr::UsdStageRefPtr&    stage,
    const pxr::SdfLayerRefPtr&    layer,
    UsdLayerEditor::LayerLockType lockState,
    bool                          includeSubLayers,
    bool                          skipSystemLockedLayers)
{
    return std::make_shared<UsdLayerEditor::LockLayerCmd>(
        stage, layer, lockState, includeSubLayers, skipSystemLockedLayers);
}

std::shared_ptr<UsdLayerEditor::InsertSubPathCmd> InsertSubPathCommandInit(
    const pxr::UsdStageRefPtr& stage,
    const pxr::SdfLayerRefPtr& layer,
    const std::string&         subPath,
    int                        index)
{
    return std::make_shared<UsdLayerEditor::InsertSubPathCmd>(stage, layer, subPath, index);
}

std::shared_ptr<UsdLayerEditor::RemoveSubPathCmd> RemoveSubPathCommandFromIndexInit(
    const pxr::UsdStageRefPtr& stage,
    const pxr::SdfLayerRefPtr& layer,
    int                        index)
{
    return std::make_shared<UsdLayerEditor::RemoveSubPathCmd>(stage, layer, index);
}

std::shared_ptr<UsdLayerEditor::RemoveSubPathCmd> RemoveSubPathCommandFromIdInit(
    const pxr::UsdStageRefPtr& stage,
    const pxr::SdfLayerRefPtr& layer,
    const std::string&         subPath)
{
    return std::make_shared<UsdLayerEditor::RemoveSubPathCmd>(stage, layer, subPath);
}

std::shared_ptr<UsdLayerEditor::RefreshSystemLockLayerCmd> RefreshSystemLockLayerCommandInit(
    const pxr::UsdStageRefPtr& stage,
    const pxr::SdfLayerRefPtr& layer,
    bool                       refreshSublayers)
{
    return std::make_shared<UsdLayerEditor::RefreshSystemLockLayerCmd>(stage, layer, refreshSublayers);
}

std::shared_ptr<UsdLayerEditor::AddAnonSubLayerCmd> AddAnonSubLayerCommandInit(
    const pxr::UsdStageRefPtr& stage,
    const pxr::SdfLayerRefPtr& layer)
{
    return std::make_shared<UsdLayerEditor::AddAnonSubLayerCmd>(stage, layer);
}

std::shared_ptr<UsdLayerEditor::FlattenLayerCmd>
FlattenLayerCommandInit(const pxr::SdfLayerRefPtr& layer)
{
    return std::make_shared<UsdLayerEditor::FlattenLayerCmd>(layer);
}
} // namespace

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdLayerEditor::LayerLockType::LayerLock_Locked);
    TF_ADD_ENUM_NAME(UsdLayerEditor::LayerLockType::LayerLock_SystemLocked);
    TF_ADD_ENUM_NAME(UsdLayerEditor::LayerLockType::LayerLock_Unlocked);
}

// Wrapper function to convert Python list to std::vector<std::string>
std::vector<std::string> pythonListToStdStringVector(object py_list)
{
    std::vector<std::string> vec;
    for (int i = 0; i < len(py_list); ++i) {
        vec.push_back(extract<std::string>(py_list[i]));
    }
    return vec;
}

std::shared_ptr<UsdLayerEditor::StitchLayersCmd> StitchLayersCommandInit(
    const pxr::UsdStageRefPtr& stage,
    object                     py_list)
{
    std::vector<std::string> identifiers = pythonListToStdStringVector(py_list);
    return std::make_shared<UsdLayerEditor::StitchLayersCmd>(stage, identifiers);
}

bool isPythonListOfStrings(object obj)
{
    // Check if the object is a list
    if (!PyList_Check(obj.ptr())) {
        return false;
    }

    // Iterate through the list and check each element
    for (ssize_t i = 0; i < len(obj); ++i) {
        object item = obj[i];
        if (!PyUnicode_Check(item.ptr())) {
            return false;
        }
    }

    return true;
}

void wrapUsdLayerEditor()
{
    TfPyWrapEnum<UsdLayerEditor::LayerLockType>("LayerLockType");

    {
        using This = Ufe::UndoableCommand;
        class_<This, noncopyable>("UndoableCommand", no_init);
    }

    {
        using This = UsdLayerEditor::ClearLayerCmd;
        class_<This, bases<Ufe::UndoableCommand>, noncopyable>("ClearLayerCommand", no_init)
            .def("__init__", make_constructor(ClearLayerCommandInit))
            .def("execute", &UsdLayerEditor::ClearLayerCmd::execute)
            .def("undo", &UsdLayerEditor::ClearLayerCmd::undo)
            .def("redo", &UsdLayerEditor::ClearLayerCmd::redo)
            .def("commandString", &UsdLayerEditor::ClearLayerCmd::commandString);
    }

    {
        using This = UsdLayerEditor::DiscardEditCmd;
        class_<This, bases<Ufe::UndoableCommand>, noncopyable>("DiscardEditsCommand", no_init)
            .def("__init__", make_constructor(DiscardEditsCommandInit))
            .def("execute", &UsdLayerEditor::DiscardEditCmd::execute)
            .def("undo", &UsdLayerEditor::DiscardEditCmd::undo)
            .def("redo", &UsdLayerEditor::DiscardEditCmd::redo)
            .def("commandString", &UsdLayerEditor::DiscardEditCmd::commandString);
    }

    {
        using This = UsdLayerEditor::SetEditTargetCmd;
        class_<This, bases<Ufe::UndoableCommand>, noncopyable>("SetEditTargetCommand", no_init)
            .def("__init__", make_constructor(SetEditTargetCommandInit))
            .def("execute", &UsdLayerEditor::SetEditTargetCmd::execute)
            .def("undo", &UsdLayerEditor::SetEditTargetCmd::undo)
            .def("redo", &UsdLayerEditor::SetEditTargetCmd::redo)
            .def("commandString", &UsdLayerEditor::SetEditTargetCmd::commandString);
    }

    {
        using This = UsdLayerEditor::MuteLayerCmd;
        class_<This, bases<Ufe::UndoableCommand>, noncopyable>("MuteLayerCommand", no_init)
            .def("__init__", make_constructor(MuteLayerCommandInit))
            .def("execute", &UsdLayerEditor::MuteLayerCmd::execute)
            .def("undo", &UsdLayerEditor::MuteLayerCmd::undo)
            .def("redo", &UsdLayerEditor::MuteLayerCmd::redo)
            .def("commandString", &UsdLayerEditor::MuteLayerCmd::commandString);
    }

    {
        using This = UsdLayerEditor::LockLayerCmd;
        class_<This, bases<Ufe::UndoableCommand>, noncopyable>("LockLayerCommand", no_init)
            .def(
                "__init__",
                make_constructor(
                    LockLayerCommandInit,
                    default_call_policies(),
                    (arg("includeSubLayers") = false, arg("skipSystemLockedLayers") = false)))
            .def("execute", &UsdLayerEditor::LockLayerCmd::execute)
            .def("undo", &UsdLayerEditor::LockLayerCmd::undo)
            .def("redo", &UsdLayerEditor::LockLayerCmd::redo)
            .def("commandString", &UsdLayerEditor::LockLayerCmd::commandString);
    }

    {
        using This = UsdLayerEditor::InsertSubPathCmd;
        class_<This, bases<Ufe::UndoableCommand>, noncopyable>("InsertSubPathCommand", no_init)
            .def("__init__", make_constructor(InsertSubPathCommandInit))
            .def("execute", &UsdLayerEditor::InsertSubPathCmd::execute)
            .def("undo", &UsdLayerEditor::InsertSubPathCmd::undo)
            .def("redo", &UsdLayerEditor::InsertSubPathCmd::redo)
            .def("commandString", &UsdLayerEditor::InsertSubPathCmd::commandString);
    }

    {
        using This = UsdLayerEditor::RemoveSubPathCmd;
        class_<This, bases<Ufe::UndoableCommand>, noncopyable>("RemoveSubPathCommand", no_init)
            .def("__init__", make_constructor(RemoveSubPathCommandFromIndexInit))
            .def("__init__", make_constructor(RemoveSubPathCommandFromIdInit))
            .def("execute", &UsdLayerEditor::RemoveSubPathCmd::execute)
            .def("undo", &UsdLayerEditor::RemoveSubPathCmd::undo)
            .def("redo", &UsdLayerEditor::RemoveSubPathCmd::redo)
            .def("commandString", &UsdLayerEditor::RemoveSubPathCmd::commandString);
    }

    {
        using This = UsdLayerEditor::RefreshSystemLockLayerCmd;
        class_<This, bases<Ufe::UndoableCommand>, noncopyable>(
            "RefreshSystemLockLayerCommand", no_init)
            .def("__init__", make_constructor(RefreshSystemLockLayerCommandInit))
            .def("execute", &UsdLayerEditor::RefreshSystemLockLayerCmd::execute)
            .def("undo", &UsdLayerEditor::RefreshSystemLockLayerCmd::undo)
            .def("redo", &UsdLayerEditor::RefreshSystemLockLayerCmd::redo)
            .def("commandString", &UsdLayerEditor::RefreshSystemLockLayerCmd::commandString);
    }

    {
        using This = UsdLayerEditor::AddAnonSubLayerCmd;
        class_<This, bases<Ufe::UndoableCommand>, noncopyable>(
            "AddAnonSubLayerCommand", no_init)
            .def("__init__", make_constructor(AddAnonSubLayerCommandInit))
            .def("execute", &UsdLayerEditor::AddAnonSubLayerCmd::execute)
            .def("undo", &UsdLayerEditor::AddAnonSubLayerCmd::undo)
            .def("redo", &UsdLayerEditor::AddAnonSubLayerCmd::redo)
            .def("commandString", &UsdLayerEditor::AddAnonSubLayerCmd::commandString)
            .def("addedLayer", &UsdLayerEditor::AddAnonSubLayerCmd::addedLayer);
    }

    {
        using This = UsdLayerEditor::StitchLayersCmd;
        class_<This, bases<Ufe::UndoableCommand>, noncopyable>(
            "StitchLayersCommand", no_init)
            .def("__init__", make_constructor(StitchLayersCommandInit))
            .def("execute", &UsdLayerEditor::StitchLayersCmd::execute)
            .def("undo", &UsdLayerEditor::StitchLayersCmd::undo)
            .def("redo", &UsdLayerEditor::StitchLayersCmd::redo)
            .def("commandString", &UsdLayerEditor::StitchLayersCmd::commandString);
    }
    
    {
        using This = UsdLayerEditor::FlattenLayerCmd;
        class_<This, bases<Ufe::UndoableCommand>, noncopyable>(
            "FlattenLayerCommand", no_init)
            .def("__init__", make_constructor(FlattenLayerCommandInit))
            .def("execute", &UsdLayerEditor::FlattenLayerCmd::execute)
            .def("undo", &UsdLayerEditor::FlattenLayerCmd::undo)
            .def("redo", &UsdLayerEditor::FlattenLayerCmd::redo)
            .def("commandString", &UsdLayerEditor::FlattenLayerCmd::commandString);
    }

    def("isLayerLocked",
        UsdLayerEditor::isLayerLocked,
        arg("layer"),
        "Checks if a layer is locked.");
    def("isLayerSystemLocked",
        UsdLayerEditor::isLayerSystemLocked,
        arg("layer"),
        "Checks if a layer is system locked.");

    // Note: the unary operator ("+") in front of the lambda functions is to force the compiler to convert the lambdas into function pointers.
    // It seems that Boost.Python has known issues with support of wrapping function objects, which causes compilation issues.
    // (see: https://stackoverflow.com/questions/16845547/using-c11-lambda-as-accessor-function-in-boostpythons-add-property-get-sig)
    def(
        "getSelectedLayers",
        +[]() {
            return UsdLayerEditor::getSelectedLayers();
        },
        return_value_policy<return_by_value>(),
        "Returns the selected layers in the Layer Editor.");

    def(
        "setSelectedLayers",
        +[](object py_list) {
            if (!isPythonListOfStrings(py_list)) {
                return;
            }

            UsdLayerEditor::selectLayers(pythonListToStdStringVector(py_list));
        },
        "Sets the selected layers in the Layer Editor, based on SdfLayer identifier.");
}