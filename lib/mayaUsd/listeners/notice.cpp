//
// Copyright 2018 Pixar
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
#include <mayaUsd/listeners/notice.h>

#include <maya/MFileIO.h>
#include <maya/MSceneMessage.h>

#include <pxr/base/tf/instantiateType.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

static
void
_OnMayaNewOrOpenSceneCallback(void* /*clientData*/)
{
    // kBeforeFileRead messages are emitted when importing/referencing files,
    // which we don't consider a "scene reset".
    if (MFileIO::isImportingFile() || MFileIO::isReferencingFile()) {
        return;
    }

    UsdMayaSceneResetNotice().Send();
}

} // anonymous namespace

TF_INSTANTIATE_TYPE(UsdMayaSceneResetNotice,
                    TfType::CONCRETE, TF_1_PARENT(TfNotice));

MCallbackId UsdMayaSceneResetNotice::_afterNewCallbackId = 0;
MCallbackId UsdMayaSceneResetNotice::_beforeFileReadCallbackId = 0;

UsdMayaSceneResetNotice::UsdMayaSceneResetNotice()
{
}

/* static */
void
UsdMayaSceneResetNotice::InstallListener()
{
    // Send scene reset notices when changing scenes (either by switching
    // to a new empty scene or by opening a different scene). We do not listen
    // for kSceneUpdate messages since those are also emitted after a SaveAs
    // operation, which we don't consider a "scene reset".
    // Note also that we listen for kBeforeFileRead messages because those fire
    // at the right time (after any existing scene has been closed but before
    // the new scene has been opened). However, they are also emitted when a
    // file is imported or referenced, so we check for that and do *not* send
    // a scene reset notice.
    if (_afterNewCallbackId == 0) {
        _afterNewCallbackId =
            MSceneMessage::addCallback(MSceneMessage::kAfterNew,
                                       _OnMayaNewOrOpenSceneCallback);
    }

    if (_beforeFileReadCallbackId == 0) {
        _beforeFileReadCallbackId =
            MSceneMessage::addCallback(MSceneMessage::kBeforeFileRead,
                                       _OnMayaNewOrOpenSceneCallback);
    }
}

/* static */
void
UsdMayaSceneResetNotice::RemoveListener()
{
    if (_afterNewCallbackId != 0) {
        MMessage::removeCallback(_afterNewCallbackId);
    }

    if (_beforeFileReadCallbackId == 0) {
        MMessage::removeCallback(_beforeFileReadCallbackId);
    }
}


UsdMaya_AssemblyInstancerNoticeBase::UsdMaya_AssemblyInstancerNoticeBase(
        const MObject& assembly,
        const MObject& instancer)
        : _assembly(assembly), _instancer(instancer)
{
}

MObject
UsdMaya_AssemblyInstancerNoticeBase::GetAssembly() const
{
    return _assembly;
}

MObject
UsdMaya_AssemblyInstancerNoticeBase::GetInstancer() const
{
    return _instancer;
}


TF_INSTANTIATE_TYPE(UsdMayaAssemblyConnectedToInstancerNotice,
                    TfType::CONCRETE, TF_1_PARENT(TfNotice));

UsdMayaAssemblyConnectedToInstancerNotice::UsdMayaAssemblyConnectedToInstancerNotice(
        const MObject& assembly,
        const MObject& instancer)
        : UsdMaya_AssemblyInstancerNoticeBase(assembly, instancer)
{
}


TF_INSTANTIATE_TYPE(UsdMayaAssemblyDisconnectedFromInstancerNotice,
                    TfType::CONCRETE, TF_1_PARENT(TfNotice));

UsdMayaAssemblyDisconnectedFromInstancerNotice::UsdMayaAssemblyDisconnectedFromInstancerNotice(
        const MObject& assembly,
        const MObject& instancer)
        : UsdMaya_AssemblyInstancerNoticeBase(assembly, instancer)
{
}


PXR_NAMESPACE_CLOSE_SCOPE
