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
#include <AL/usdmaya/fileio/translators/TranslatorContext.h>

#include <pxr/base/tf/refBase.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/type.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/usd/usd/prim.h>

#include <maya/MDagPath.h>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This base class defines an interface to allow you to import/export extra data to/from
/// USD prims. It works
///         alongside the core translator plugin concepts, and allows you to decorate the data of a
///         prim being imported/exported. It works by associating itself with a specific MFn::Type,
///         and if matched at export/import time, the api schema translator will be called to handle
///         its specific attributes.
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
class ExtraDataPluginAbstract
    : public TfRefBase
    , public TfWeakBase
{
public:
    typedef ExtraDataPluginAbstract This;   ///< this type
    typedef TfRefPtr<This>          RefPtr; ///< the type of a reference this type
    typedef TfWeakPtr<This>         Ptr;    ///< weak pointer to this type

    /// \brief  dtor
    virtual ~ExtraDataPluginAbstract() = default;
    ExtraDataPluginAbstract() = default;

    /// \brief  Provides the base filter to remove Maya nodes to test for. If the plugin is intended
    /// to apply to a
    ///         custom maya node, then the MFn::Type returned should be the relevant MFn::kPluginFoo
    ///         enum, and you will also need to specify the node typename by overloading the
    ///         getPluginTypeName method.
    virtual MFn::Type getFnType() const { return MFn::kInvalid; };

    /// \brief  If the plugin is to apply to a maya plugin node type, then you'll need to specify
    /// the typename
    ///         by overloading this method.
    /// \return the maya plugin typename
    virtual const char* getPluginTypeName() const { return ""; }

    /// \brief  Override this to do a one time initialization of your translator. Primarily this is
    /// to allow you to
    ///         extract some MObject attribute handles from an MNodeClass, to avoid the need for
    ///         calling findPlug at runtime (and the inherent cost of the strcmps/hash lookup that
    ///         entails)
    /// \return MS::kSuccess if all ok
    virtual MStatus initialize() { return MS::kSuccess; }

    /// \brief  Override this method to import a prim into your scene.
    /// \param  prim the usd prim to be imported into maya
    /// \param  node the node that has been imported, on which you wish to import additionl
    /// attributes \return MS::kSuccess if all ok
    virtual MStatus import(const UsdPrim& prim, const MObject& node) { return MS::kSuccess; }

    /// \brief  Override this method to export additional parameters on a node already handled by
    /// another translator. \param  prim the USD prim to export into \param  node the maya node
    /// being exported. \param  params the exporter params \return the prim created
    virtual MStatus exportObject(UsdPrim& prim, const MObject& node, const ExporterParams& params)
    {
        return MS::kSuccess;
    }

    /// \brief  If your node needs to set up any relationships after import (for example, adding the
    /// node to a set, or
    ///         making attribute connections), then all of that work should be performed here.
    /// \param  prim the prim we are importing.
    /// \return MS::kSuccess if all ok
    virtual MStatus postImport(const UsdPrim& prim) { return MS::kSuccess; }

    /// \brief  This method will be called prior to the tear down process taking place. This is the
    /// last chance you have
    ///         to do any serialisation whilst all of the existing nodes are available to query.
    /// \param  prim the prim that may be modified or deleted as a result of a variant switch
    /// \return MS::kSuccess if all ok
    virtual MStatus preTearDown(UsdPrim& prim) { return MS::kSuccess; }

    /// \brief  override this method and return true if the translator supports update
    /// \return true if your plugin supports update, false otherwise.
    virtual bool supportsUpdate() const { return true; }

    /// \brief  Optionally override this method to copy the attribute values from the prim onto the
    /// Maya nodes you have
    ///         created.
    /// \param  prim  the prim
    /// \return MS::kSuccess if all ok
    virtual MStatus update(const UsdPrim& prim) { return MS::kSuccess; }

    /// \brief  internal method - set the internal pointer to the translator context
    /// \param  ctx the context pointer
    inline void setContext(TranslatorContextPtr ctx) { m_context = ctx; }

    /// \brief  return a pointer to the translator context
    TranslatorContextPtr context() { return m_context; }

private:
    TranslatorContextPtr m_context;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  the base class for extra data plugins
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
class ExtraDataPluginBase : public ExtraDataPluginAbstract
{
public:
    typedef ExtraDataPluginBase This;
    typedef TfRefPtr<This>      RefPtr;

    /// \brief  dtor
    virtual ~ExtraDataPluginBase() { }
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  core factory type to create an extra data plug-in translator
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
class ExtraDataPluginFactoryBase : public TfType::FactoryBase
{
public:
    /// \brief  overridden by the TranslatorFactory to create a new translator for a given type
    /// \param  ctx the current translator context
    /// \return the plugin translator
    AL_USDMAYA_PUBLIC
    virtual TfRefPtr<ExtraDataPluginBase> create(TranslatorContextPtr ctx) const = 0;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  core factory type to create an extra data plug-in translator
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
template <typename T> class ExtraDataPluginTranslatorFactory : public ExtraDataPluginFactoryBase
{
public:
    /// \brief  creates a new translator for a given type T
    /// \param  ctx the current translator context
    /// \return the plugin translator associated with type T
    TfRefPtr<ExtraDataPluginBase> create(TranslatorContextPtr ctx) const override
    {
        return T::create(ctx);
    }
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  a macro to declare an extra data plug-in translator
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
#define AL_USDMAYA_DECLARE_EXTRA_DATA_PLUGIN(PlugClass) \
    typedef PlugClass       This;                       \
    typedef TfRefPtr<This>  RefPtr;                     \
    typedef TfWeakPtr<This> Ptr;                        \
    AL_MAYA_MACROS_PUBLIC                               \
    static RefPtr create(TranslatorContextPtr context);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  a macro to define an extra data plug-in translator
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
#define AL_USDMAYA_DEFINE_EXTRA_DATA_PLUGIN(PlugClass)                  \
    TfRefPtr<PlugClass> PlugClass::create(TranslatorContextPtr context) \
    {                                                                   \
        TfRefPtr<PlugClass> plugin = TfCreateRefPtr(new This());        \
        plugin->setContext(context);                                    \
        if (!plugin->initialize())                                      \
            return TfRefPtr<PlugClass>();                               \
        return plugin;                                                  \
    }                                                                   \
                                                                        \
    TF_REGISTRY_FUNCTION(TfType)                                        \
    {                                                                   \
        TfType::Define<PlugClass, TfType::Bases<ExtraDataPluginBase>>() \
            .SetFactory<ExtraDataPluginTranslatorFactory<PlugClass>>(); \
    }

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
