//
// Copyright 2021 Autodesk
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
#pragma once

#ifndef PXRUSDMAYA_PULLPUSHCOMMANDS_H
#define PXRUSDMAYA_PULLPUSHCOMMANDS_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/mayaUsd.h>
#include <mayaUsd/undo/OpUndoItemList.h>

#include <maya/MPxCommand.h>
#include <ufe/path.h>
#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// PullPushBaseCommand
//------------------------------------------------------------------------------

//! \brief Base undoable command holding the undo item list.

class PullPushBaseCommand : public MPxCommand
{
public:
    //! \brief MPxCommand API to undo the command.
    MAYAUSD_CORE_PUBLIC
    MStatus undoIt() override;

    //! \brief MPxCommand API to redo the command.
    MAYAUSD_CORE_PUBLIC
    MStatus redoIt() override;

    //! \brief MPxCommand API to specify the command is undoable.
    MAYAUSD_CORE_PUBLIC
    bool isUndoable() const override;

protected:
    OpUndoItemList fUndoItemList;
};

//------------------------------------------------------------------------------
// EditAsMayaCommand
//------------------------------------------------------------------------------

//! \brief Edit as maya undoable command.

class EditAsMayaCommand : public PullPushBaseCommand
{
public:
    //! \brief The edit as maya command name.
    MAYAUSD_CORE_PUBLIC
    static const char commandName[];

    //! \brief MPxCommand API to create the command object.
    MAYAUSD_CORE_PUBLIC
    static void* creator();

    //! \brief MPxCommand API to register the command syntax.
    MAYAUSD_CORE_PUBLIC
    static MSyntax createSyntax();

    //! \brief MPxCommand API to execute the command.
    MAYAUSD_CORE_PUBLIC
    MStatus doIt(const MArgList& argList) override;

private:
    // Make sure callers need to call creator().
    EditAsMayaCommand();

    Ufe::Path fPath;
};

//------------------------------------------------------------------------------
// MergeToUsdCommand
//------------------------------------------------------------------------------

//! \brief Merge to USD undoable command.

class MergeToUsdCommand : public PullPushBaseCommand
{
public:
    //! \brief The merge to USD command name.
    MAYAUSD_CORE_PUBLIC
    static const char commandName[];

    //! \brief MPxCommand API to create the command object.
    MAYAUSD_CORE_PUBLIC
    static void* creator();

    //! \brief MPxCommand API to register the command syntax.
    MAYAUSD_CORE_PUBLIC
    static MSyntax createSyntax();

    //! \brief MPxCommand API to execute the command.
    MAYAUSD_CORE_PUBLIC
    MStatus doIt(const MArgList& argList) override;

private:
    // Make sure callers need to call creator().
    MergeToUsdCommand();
};

//------------------------------------------------------------------------------
// DiscardEditsCommand
//------------------------------------------------------------------------------

//! \brief Discards edits undoable command.

class DiscardEditsCommand : public PullPushBaseCommand
{
public:
    //! \brief The edit as maya command name.
    MAYAUSD_CORE_PUBLIC
    static const char commandName[];

    //! \brief MPxCommand API to create the command object.
    MAYAUSD_CORE_PUBLIC
    static void* creator();

    //! \brief MPxCommand API to register the command syntax.
    MAYAUSD_CORE_PUBLIC
    static MSyntax createSyntax();

    //! \brief MPxCommand API to execute the command.
    MAYAUSD_CORE_PUBLIC
    MStatus doIt(const MArgList& argList) override;

private:
    // Make sure callers need to call creator().
    DiscardEditsCommand();
};

//------------------------------------------------------------------------------
// DuplicateCommand
//------------------------------------------------------------------------------

//! \brief Copy between Maya and USD undoable command.

class DuplicateCommand : public PullPushBaseCommand
{
public:
    //! \brief The copy between Maya and USD command name.
    MAYAUSD_CORE_PUBLIC
    static const char commandName[];

    //! \brief MPxCommand API to create the command object.
    MAYAUSD_CORE_PUBLIC
    static void* creator();

    //! \brief MPxCommand API to register the command syntax.
    MAYAUSD_CORE_PUBLIC
    static MSyntax createSyntax();

    //! \brief MPxCommand API to execute the command.
    MAYAUSD_CORE_PUBLIC
    MStatus doIt(const MArgList& argList) override;

private:
    // Make sure callers need to call creator().
    DuplicateCommand();

    Ufe::Path fSrcPath;
    Ufe::Path fDstPath;
};

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif /* PXRUSDMAYA_PULLPUSHCOMMANDS_H */
