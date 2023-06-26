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
#include <mayaUsd/ufe/UsdUndoPayloadCommand.h>

#include <boost/python.hpp>
#include <boost/python/def.hpp>

using namespace boost::python;

namespace {

MayaUsd::ufe::UsdUndoLoadPayloadCommand*
LoadPayloadCommandInit(const PXR_NS::UsdPrim& prim, PXR_NS::UsdLoadPolicy policy)
{
    return new MayaUsd::ufe::UsdUndoLoadPayloadCommand(prim, policy);
}

MayaUsd::ufe::UsdUndoUnloadPayloadCommand* UnloadPayloadCommandInit(const PXR_NS::UsdPrim& prim)
{
    return new MayaUsd::ufe::UsdUndoUnloadPayloadCommand(prim);
}

} // namespace

void wrapCommands()
{
    {
        using This = MayaUsd::ufe::UsdUndoLoadPayloadCommand;
        class_<This, boost::noncopyable>("LoadPayloadCommand", no_init)
            .def("__init__", make_constructor(LoadPayloadCommandInit))
            .def("execute", &MayaUsd::ufe::UsdUndoLoadPayloadCommand::execute)
            .def("undo", &MayaUsd::ufe::UsdUndoLoadPayloadCommand::undo)
            .def("redo", &MayaUsd::ufe::UsdUndoLoadPayloadCommand::redo);
    }
    {
        using This = MayaUsd::ufe::UsdUndoUnloadPayloadCommand;
        class_<This, boost::noncopyable>("UnloadPayloadCommand", no_init)
            .def("__init__", make_constructor(UnloadPayloadCommandInit))
            .def("execute", &MayaUsd::ufe::UsdUndoUnloadPayloadCommand::execute)
            .def("undo", &MayaUsd::ufe::UsdUndoUnloadPayloadCommand::undo)
            .def("redo", &MayaUsd::ufe::UsdUndoUnloadPayloadCommand::redo);
    }
}
