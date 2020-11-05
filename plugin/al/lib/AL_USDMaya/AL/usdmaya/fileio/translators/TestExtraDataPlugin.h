//
// Copyright 2018 Animal Logic
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

#include <AL/maya/utils/Api.h>
#include <AL/usdmaya/Api.h>
#include <AL/usdmaya/fileio/ExportParams.h>
#include <AL/usdmaya/fileio/translators/ExtraDataPlugin.h>
#include <AL/usdmaya/fileio/translators/TranslatorContext.h>

#include <pxr/base/tf/refBase.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/type.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>

#include <maya/MDagPath.h>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

class TestExtraDataPlugin : public ExtraDataPluginBase
{
public:
    AL_USDMAYA_DECLARE_EXTRA_DATA_PLUGIN(TestExtraDataPlugin);

    /// \brief  dtor
    virtual ~TestExtraDataPlugin() = default;
    TestExtraDataPlugin() = default;

    /// \brief  Provides the base filter to remove Maya nodes to test for the applied schema
    ///         for this test, just output the distance dimension
    virtual MFn::Type getFnType() const { return MFn::kDistance; }

    /// \brief  Override this to do a one time initialization of your translator. Primarily this is
    /// to allow you to
    ///         extract some MObject attribute handles from an MNodeClass, to avoid the need for
    ///         calling findPlug at runtime (and the inherent cost of the strcmps/hash lookup that
    ///         entails)
    /// \return MS::kSuccess if all ok
    MStatus initialize()
    {
        initialiseCalled = true;
        return MS::kSuccess;
    }

    /// \brief  Override this method to import a prim into your scene.
    /// \param  prim the usd prim to be imported into maya
    /// \param  node the maya node to import the data onto
    /// \return MS::kSuccess if all ok
    MStatus import(const UsdPrim& prim, const MObject& node)
    {
        importCalled = true;
        UsdPrim(prim).CreateAttribute(TfToken("imported"), SdfValueTypeNames->Float);
        return MS::kSuccess;
    }

    /// \brief  Override this method to export a Maya object into USD
    /// \param  prim the USD prim to store the extra data attributes
    /// \param  node the maya node to read the data from
    /// \param  params the exporter params
    /// \return the prim created
    MStatus exportObject(UsdPrim& prim, const MObject& node, const ExporterParams& params)
    {
        exportObjectCalled = true;
        prim.CreateAttribute(TfToken("exported"), SdfValueTypeNames->Float);
        return MS::kSuccess;
    }

    /// \brief  If your node needs to set up any relationships after import (for example, adding the
    /// node to a set, or
    ///         making attribute connections), then all of that work should be performed here.
    /// \param  prim the prim we are importing.
    /// \return MS::kSuccess if all ok
    MStatus postImport(const UsdPrim& prim)
    {
        postImportCalled = true;
        return MS::kSuccess;
    }

    /// \brief  This method will be called prior to the tear down process taking place. This is the
    /// last chance you have
    ///         to do any serialisation whilst all of the existing nodes are available to query.
    /// \param  prim the prim that may be modified or deleted as a result of a variant switch
    /// \return MS::kSuccess if all ok
    MStatus preTearDown(UsdPrim& prim)
    {
        preTearDownCalled = true;
        return MS::kSuccess;
    }

    /// \brief  override this method and return true if the translator supports update
    /// \return true if your plugin supports update, false otherwise.
    bool supportsUpdate() const { return true; }

    /// \brief  Optionally override this method to copy the attribute values from the prim onto the
    /// Maya nodes you have
    ///         created.
    /// \param  prim  the prim
    /// \return MS::kSuccess if all ok
    MStatus update(const UsdPrim& prim)
    {
        updateCalled = true;
        return MS::kSuccess;
    }

    bool initialiseCalled = false;
    bool importCalled = false;
    bool exportObjectCalled = false;
    bool postImportCalled = false;
    bool preTearDownCalled = false;
    bool updateCalled = false;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
