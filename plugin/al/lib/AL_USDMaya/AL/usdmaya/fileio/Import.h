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
#pragma once
#include "AL/maya/utils/MayaHelperMacros.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/NodeFactory.h"
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"

#include <pxr/pxr.h>

#include <maya/MPxCommand.h>
PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace fileio {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class that performs the import of data from USD into Maya.
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
class Import
{
public:
    /// \brief  the ctor runs the main import process. Simply pass in a set of parameters that will
    /// determine what maya
    ///         should import into the scene
    /// \param  params the import params
    Import(const ImporterParams& params);

    /// \brief  dtor
    ~Import();

    /// \brief  returns true if the import succeeded, false otherwise
    inline operator bool() const { return m_success; }

private:
    void    doImport();
    MObject createShape(
        translators::TranslatorRefPtr       translator,
        translators::TranslatorManufacture& manufacture,
        const UsdPrim&                      prim,
        MObject                             parent,
        bool                                parentUnmerged);
    MObject createParentTransform(
        const UsdPrim&                     prim,
        TransformIterator&                 it,
        translators::TranslatorManufacture manufacture);

    const ImporterParams&                      m_params;
    TfHashMap<SdfPath, MObject, SdfPath::Hash> m_instanceObjects;
    TfToken::HashSet                           m_nonImportablePrims;
    bool                                       m_success;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command to import a USD file into Maya (partially supporting Animal Logic specific
/// things) \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
class ImportCommand : public MPxCommand
{
public:
    AL_MAYA_DECLARE_COMMAND();

    /// ctor
    inline ImportCommand()
        : m_params()
    {
    }

    /// dtor
    inline ~ImportCommand() { }

private:
    MStatus        doIt(const MArgList& args);
    MStatus        redoIt();
    MStatus        undoIt();
    bool           isUndoable() const { return true; };
    ImporterParams m_params;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
