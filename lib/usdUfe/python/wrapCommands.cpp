//
// Copyright 2023 Autodesk
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
#include <usdUfe/ufe/UsdUndoAddPayloadCommand.h>
#include <usdUfe/ufe/UsdUndoAddReferenceCommand.h>
#include <usdUfe/ufe/UsdUndoClearDefaultPrimCommand.h>
#include <usdUfe/ufe/UsdUndoClearPayloadsCommand.h>
#include <usdUfe/ufe/UsdUndoClearReferencesCommand.h>
#include <usdUfe/ufe/UsdUndoPayloadCommand.h>
#include <usdUfe/ufe/UsdUndoSetDefaultPrimCommand.h>
#include <usdUfe/ufe/UsdUndoToggleActiveCommand.h>
#include <usdUfe/ufe/UsdUndoToggleInstanceableCommand.h>

#include <boost/python.hpp>
#include <boost/python/def.hpp>

using namespace boost::python;

namespace {

UsdUfe::UsdUndoAddPayloadCommand*
AddPayloadCommandInit(const PXR_NS::UsdPrim& prim, const std::string& filePath, bool prepend)
{
    return new UsdUfe::UsdUndoAddPayloadCommand(prim, filePath, prepend);
}

UsdUfe::UsdUndoClearPayloadsCommand* ClearPayloadsCommandInit(const PXR_NS::UsdPrim& prim)
{
    return new UsdUfe::UsdUndoClearPayloadsCommand(prim);
}

UsdUfe::UsdUndoAddReferenceCommand*
AddReferenceCommandInit(const PXR_NS::UsdPrim& prim, const std::string& filePath, bool prepend)
{
    return new UsdUfe::UsdUndoAddReferenceCommand(prim, filePath, prepend);
}

UsdUfe::UsdUndoClearReferencesCommand* ClearReferencesCommandInit(const PXR_NS::UsdPrim& prim)
{
    return new UsdUfe::UsdUndoClearReferencesCommand(prim);
}

UsdUfe::UsdUndoToggleActiveCommand* ToggleActiveCommandInit(const PXR_NS::UsdPrim& prim)
{
    return new UsdUfe::UsdUndoToggleActiveCommand(prim);
}

UsdUfe::UsdUndoToggleInstanceableCommand* ToggleInstanceableCommandInit(const PXR_NS::UsdPrim& prim)
{
    return new UsdUfe::UsdUndoToggleInstanceableCommand(prim);
}

UsdUfe::UsdUndoLoadPayloadCommand*
LoadPayloadCommandInit(const PXR_NS::UsdPrim& prim, PXR_NS::UsdLoadPolicy policy)
{
    return new UsdUfe::UsdUndoLoadPayloadCommand(prim, policy);
}

UsdUfe::UsdUndoUnloadPayloadCommand* UnloadPayloadCommandInit(const PXR_NS::UsdPrim& prim)
{
    return new UsdUfe::UsdUndoUnloadPayloadCommand(prim);
}

UsdUfe::UsdUndoClearDefaultPrimCommand*
ClearDefaultPrimCommandInit(const PXR_NS::UsdStageRefPtr& stage)
{
    return new UsdUfe::UsdUndoClearDefaultPrimCommand(stage);
}

UsdUfe::UsdUndoSetDefaultPrimCommand* SetDefaultPrimCommandInit(const PXR_NS::UsdPrim& prim)
{
    return new UsdUfe::UsdUndoSetDefaultPrimCommand(prim);
}

} // namespace

void wrapCommands()
{
    {
        using This = UsdUfe::UsdUndoClearDefaultPrimCommand;
        class_<This, boost::noncopyable>("ClearDefaultPrimCommand", no_init)
            .def("__init__", make_constructor(ClearDefaultPrimCommandInit))
            .def("execute", &UsdUfe::UsdUndoClearDefaultPrimCommand::execute)
            .def("undo", &UsdUfe::UsdUndoClearDefaultPrimCommand::undo)
            .def("redo", &UsdUfe::UsdUndoClearDefaultPrimCommand::redo);
    }
    {
        using This = UsdUfe::UsdUndoSetDefaultPrimCommand;
        class_<This, boost::noncopyable>("SetDefaultPrimCommand", no_init)
            .def("__init__", make_constructor(SetDefaultPrimCommandInit))
            .def("execute", &UsdUfe::UsdUndoSetDefaultPrimCommand::execute)
            .def("undo", &UsdUfe::UsdUndoSetDefaultPrimCommand::undo)
            .def("redo", &UsdUfe::UsdUndoSetDefaultPrimCommand::redo);
    }
    {
        using This = UsdUfe::UsdUndoAddPayloadCommand;
        class_<This, boost::noncopyable>("AddPayloadCommand", no_init)
            .def("__init__", make_constructor(AddPayloadCommandInit))
            .def("execute", &UsdUfe::UsdUndoAddPayloadCommand::execute)
            .def("undo", &UsdUfe::UsdUndoAddPayloadCommand::undo)
            .def("redo", &UsdUfe::UsdUndoAddPayloadCommand::redo);
    }
    {
        using This = UsdUfe::UsdUndoClearPayloadsCommand;
        class_<This, boost::noncopyable>("ClearPayloadsCommand", no_init)
            .def("__init__", make_constructor(ClearPayloadsCommandInit))
            .def("execute", &UsdUfe::UsdUndoClearPayloadsCommand::execute)
            .def("undo", &UsdUfe::UsdUndoClearPayloadsCommand::undo)
            .def("redo", &UsdUfe::UsdUndoClearPayloadsCommand::redo);
    }
    {
        using This = UsdUfe::UsdUndoAddReferenceCommand;
        class_<This, boost::noncopyable>("AddReferenceCommand", no_init)
            .def("__init__", make_constructor(AddReferenceCommandInit))
            .def("execute", &UsdUfe::UsdUndoAddReferenceCommand::execute)
            .def("undo", &UsdUfe::UsdUndoAddReferenceCommand::undo)
            .def("redo", &UsdUfe::UsdUndoAddReferenceCommand::redo);
    }
    {
        using This = UsdUfe::UsdUndoClearReferencesCommand;
        class_<This, boost::noncopyable>("ClearReferencesCommand", no_init)
            .def("__init__", make_constructor(ClearReferencesCommandInit))
            .def("execute", &UsdUfe::UsdUndoClearReferencesCommand::execute)
            .def("undo", &UsdUfe::UsdUndoClearReferencesCommand::undo)
            .def("redo", &UsdUfe::UsdUndoClearReferencesCommand::redo);
    }
    {
        using This = UsdUfe::UsdUndoToggleActiveCommand;
        class_<This, boost::noncopyable>("ToggleActiveCommand", no_init)
            .def("__init__", make_constructor(ToggleActiveCommandInit))
            .def("execute", &UsdUfe::UsdUndoToggleActiveCommand::execute)
            .def("undo", &UsdUfe::UsdUndoToggleActiveCommand::undo)
            .def("redo", &UsdUfe::UsdUndoToggleActiveCommand::redo);
    }
    {
        using This = UsdUfe::UsdUndoToggleInstanceableCommand;
        class_<This, boost::noncopyable>("ToggleInstanceableCommand", no_init)
            .def("__init__", make_constructor(ToggleInstanceableCommandInit))
            .def("execute", &UsdUfe::UsdUndoToggleInstanceableCommand::execute)
            .def("undo", &UsdUfe::UsdUndoToggleInstanceableCommand::undo)
            .def("redo", &UsdUfe::UsdUndoToggleInstanceableCommand::redo);
    }
    {
        using This = UsdUfe::UsdUndoLoadPayloadCommand;
        class_<This, boost::noncopyable>("LoadPayloadCommand", no_init)
            .def("__init__", make_constructor(LoadPayloadCommandInit))
            .def("execute", &UsdUfe::UsdUndoLoadPayloadCommand::execute)
            .def("undo", &UsdUfe::UsdUndoLoadPayloadCommand::undo)
            .def("redo", &UsdUfe::UsdUndoLoadPayloadCommand::redo);
    }
    {
        using This = UsdUfe::UsdUndoUnloadPayloadCommand;
        class_<This, boost::noncopyable>("UnloadPayloadCommand", no_init)
            .def("__init__", make_constructor(UnloadPayloadCommandInit))
            .def("execute", &UsdUfe::UsdUndoUnloadPayloadCommand::execute)
            .def("undo", &UsdUfe::UsdUndoUnloadPayloadCommand::undo)
            .def("redo", &UsdUfe::UsdUndoUnloadPayloadCommand::redo);
    }
}
